/*******************************************************************************

    Copyright (C) 2014  René 'Shirk' Köcher
 
    This file is part of Hotarubi.

    Hotarubi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
    Hotarubi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <hotarubi/processor.h>

#include <hotarubi/memory/physmm.h>
#include <hotarubi/memory/virtmm.h>
#include <hotarubi/memory/const.h>

#include <hotarubi/log/log.h>
#include <hotarubi/boot/multiboot.h>

#define MMU_PML4_INDEX( x ) ( ( ( x ) >> 39 ) & 0x1ff )
#define MMU_PDPT_INDEX( x ) ( ( ( x ) >> 30 ) & 0x1ff )
#define MMU_PDT_INDEX( x )  ( ( ( x ) >> 21 ) & 0x1ff )
#define MMU_PT_INDEX( x )   ( ( ( x ) >> 12 ) & 0x1ff )

#define MMU_PDPT_ADDR( pml4, vaddr ) ( pml4[MMU_PML4_INDEX( vaddr )] & ~kPageFlagMask )
#define MMU_PDT_ADDR( pdpt, vaddr )  ( pdpt[MMU_PDPT_INDEX( vaddr )] & ~kPageFlagMask )
#define MMU_PT_ADDR( pdt , vaddr )   (  pdt[MMU_PDT_INDEX ( vaddr )] & ~kPageFlagMask )

#define MMU_PHYS_ADDR_4K( pt  , vaddr ) (   pt[MMU_PT_INDEX ( vaddr )] & ~kPageFlagMask )
#define MMU_PHYS_ADDR_2M( pdt , vaddr ) (  pdt[MMU_PDT_INDEX( vaddr )] & ~kPageFlagMask2M )

#define PHYS_ADDR( addr ) ( ( ( addr ) == 0 ) ? 0 : ( ( addr ) - physmm::physical_base_offset() ) )
#define VIRT_ADDR( addr ) ( ( ( addr ) == 0 ) ? 0 : ( ( addr ) + physmm::physical_base_offset() ) )

#ifdef KERNEL
extern "C"
{
	/* constants from link.ld */
	extern unsigned char __bootstrap[], __ebootstrap[];
	extern unsigned char __text[], __data[], __bss[], __end[];
}
#endif

namespace memory
{
namespace physmm
{
extern uint64_t memory_upper_bound;
};

namespace virtmm
{
const uint64_t map_invalid = 0xffffffffffffffff;
static uint64_t *system_pml4 = nullptr;

static bool
_map_region( uint64_t *pml4, uintptr_t vaddr, uintptr_t paddr, size_t len,
             PageFlagSystemSet flags=kPageFlagNone )
{
	uint64_t *pdpt = nullptr,
	         *pdt  = nullptr,
	         *pt   = nullptr;

	if( len % PAGE_SIZE != 0 )
	{
		len += PAGE_SIZE;
	}
	len /= PAGE_SIZE;

	/* map region expects pml4 to be accessible without any offset calculation */
	if( pml4 == nullptr )
	{
		return false;
	}

	do
	{
		pdpt = ( uint64_t* )MMU_PDPT_ADDR( pml4, vaddr );
		if( pdpt == nullptr )
		{
			if( ( pdpt = ( uint64_t* )physmm::alloc_page() ) == nullptr )
			{
				return false;
			}
			memset( pdpt, 0, PAGE_SIZE );
			pml4[MMU_PML4_INDEX( vaddr )] = PHYS_ADDR( ( uintptr_t )pdpt ) | kPageFlagPresent;
		}

		pdt = ( uint64_t* )MMU_PDT_ADDR( pdpt, vaddr );
		if( pdt == nullptr )
		{
			if( ( pdt = ( uint64_t* )physmm::alloc_page() ) == nullptr )
			{
				return false;
			}
			memset( pdt, 0, PAGE_SIZE );
			pdpt[MMU_PDPT_INDEX( vaddr )] = PHYS_ADDR( ( uintptr_t )pdt ) | kPageFlagPresent;
		}

		pt = ( uint64_t* )MMU_PT_ADDR( pdt, vaddr );
		if( pt == nullptr )
		{
			if( ( pt = ( uint64_t* )physmm::alloc_page() ) == nullptr )
			{
				return false;
			}
			memset( pt, 0, PAGE_SIZE );
			pdt[MMU_PDT_INDEX( vaddr )] = PHYS_ADDR( ( uintptr_t )pt ) | kPageFlagPresent;
		}

		pt[MMU_PT_INDEX( vaddr )] = paddr | kPageFlagPresent | flags;

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	} while( --len );

	return true;
}

static bool
_unmap_region( uint64_t *pml4, uint64_t vaddr, size_t len, bool free_page )
{
	uint64_t *pdpt  = nullptr,
	         *pdt   = nullptr,
	         *pt    = nullptr,
	          paddr = 0;

	if( len % PAGE_SIZE != 0 )
	{
		len += PAGE_SIZE;
	}
	len /= PAGE_SIZE;

	/* unmap region expects pml4 to be accessible without any offset calculation */
	if( pml4 == nullptr )
	{
		return false;
	}

	do
	{
		pdpt = ( uint64_t* )VIRT_ADDR( MMU_PDPT_ADDR( pml4, vaddr ) );
		if( pdpt == nullptr )
		{
			continue;
		}

		pdt = ( uint64_t* )VIRT_ADDR( MMU_PDT_ADDR( pdpt, vaddr ) );
		if( pdt == nullptr )
		{
			continue;
		}

		pt = ( uint64_t* )VIRT_ADDR( MMU_PT_ADDR( pdt, vaddr ) );
		if( pt == nullptr )
		{
			continue;
		}

		paddr = MMU_PHYS_ADDR_4K( pt, vaddr );
		if( paddr != 0 )
		{
			if( free_page )
			{
				physmm::free_page( ( void* )VIRT_ADDR( paddr ) );
			}
			pt[MMU_PT_INDEX( vaddr )] = 0;
		}
		/* FIXME: other cores need to know that too */
		__asm__ __volatile__( "invlpg %0" :: "m"( vaddr ) );

		vaddr += PAGE_SIZE;
	} while( --len );
	return true;
}

static
void _create_system_vm( void )
{
	log::printk( "Initializing kernel virtual address space..\n" );

	if( ( system_pml4 = ( uint64_t* )physmm::alloc_page() ) == nullptr )
	{
		goto error_out;
	}
	memset( system_pml4, 0, PAGE_SIZE );

	/* map the video ram (bootstrap needs it) */
	if( !_map_region( system_pml4, 0xa0000, 0xa0000, 80 * 25 * 2, kPageFlagWritable ) )
	{
		goto error_out;
	}
	if( !_map_region( system_pml4, 0xb8000, 0xb8000, 80 * 25 * 2, kPageFlagWritable ) )
	{
		goto error_out;
	}

	/* map kernel .text (ro) */
	if( !_map_region( system_pml4, 
	                  ( uintptr_t )__text, ( uintptr_t )__text - kVMRangeKernelBase,
	                  ( uintptr_t )__data - ( uintptr_t )__text ) )
	{
		goto error_out;
	}

	/* map kernel .data and .bss (rw) */
	if( !_map_region( system_pml4, 
	                  ( uintptr_t )__data, ( uintptr_t )__data -  kVMRangeKernelBase,
	                  ( uintptr_t )__end - ( uintptr_t )__data,
	                  kPageFlagWritable ) )
	{
		goto error_out;
	}

	/* map the whole set of physical memory */
	if( !_map_region( system_pml4,
	                  kVMRangePhysMemBase, 0,
	                  physmm::memory_upper_bound,
	                  kPageFlagWritable ) )
	{
		goto error_out;
	}
	return;

error_out:
	log::printk( "\nPANIC: out of memory while creating the system VM!\n" );
	do {} while( 1 );
}

bool
map_address( uint64_t vaddr, PageFlagSystemSet flags )
{
	uint64_t *pml4 = ( uint64_t* )VIRT_ADDR( processor::regs::read_cr3() );
	uintptr_t addr = ( uintptr_t )physmm::alloc_page();

	return _map_region( pml4, vaddr, addr, PAGE_SIZE, flags );
}

bool
map_fixed( uint64_t vaddr, uint64_t paddr, PageFlagSystemSet flags )
{
	uint64_t *pml4 = ( uint64_t* )VIRT_ADDR( processor::regs::read_cr3() );
	return _map_region( pml4, vaddr, paddr, PAGE_SIZE, flags );
}

bool
map_address_range( uint64_t vaddr, size_t npages, PageFlagSystemSet flags )
{
	for( size_t n = 0; n < npages; ++n )
	{
		if( map_address( vaddr, flags ) == false )
		{
			/* backtrack */
			vaddr -= PAGE_SIZE * n;
			unmap_address_range( vaddr, n - 1 );
			return false;
		}
		vaddr += PAGE_SIZE;
	}
	return true;
}

void
unmap_address( uint64_t vaddr )
{
	uint64_t *pml4 = ( uint64_t* )VIRT_ADDR( processor::regs::read_cr3() );
	( void )_unmap_region( pml4, vaddr, PAGE_SIZE, true );
}

void
unmap_fixed( uint64_t vaddr )
{
	uint64_t *pml4 = ( uint64_t* )VIRT_ADDR( processor::regs::read_cr3() );
	( void )_unmap_region( pml4, vaddr, PAGE_SIZE, false );
}

void
unmap_address_range( uint64_t vaddr, size_t npages )
{
	uint64_t *pml4 = ( uint64_t* )VIRT_ADDR( processor::regs::read_cr3() );
	( void )_unmap_region( pml4, vaddr, PAGE_SIZE * npages, true );
}

bool
lookup_mapping( uint64_t vaddr, uint64_t &pml4e, uint64_t &pdpte,
                                uint64_t &pdte, uint64_t &pte )
{
	uint64_t *pml4 = ( uint64_t* )VIRT_ADDR( processor::regs::read_cr3() ),
	         *ptr  = nullptr;

	if( pml4 == nullptr )
	{
		goto err_pml4;
	}

	pml4e = pml4[MMU_PML4_INDEX( vaddr )];
	ptr   = ( uint64_t* )VIRT_ADDR( MMU_PDPT_ADDR( pml4, vaddr ) );
	if( ptr == nullptr )
	{
		goto err_pml4;
	}

	pdpte = ptr[MMU_PDPT_INDEX( vaddr )];
	ptr   = ( uint64_t* )VIRT_ADDR( MMU_PDT_ADDR( ptr, vaddr ) );
	if( ptr == nullptr )
	{
		goto err_pdpt;
	}
	pdte = ptr[MMU_PDT_INDEX( vaddr )];
	ptr  = ( uint64_t* )VIRT_ADDR( MMU_PT_ADDR( ptr, vaddr ) );
	if( ptr == nullptr )
	{
		goto err_pdt;
	}
	pte = ptr[MMU_PT_INDEX( vaddr )];
	ptr = ( uint64_t* )VIRT_ADDR( MMU_PHYS_ADDR_4K( ptr, vaddr ) );
	if( ptr == nullptr )
	{
		goto err_pt;
	}
	return true;

err_pml4:
	pml4e = map_invalid;
err_pdpt:
	pdpte = map_invalid;
err_pdt:
	pdte = map_invalid;
err_pt:
	pte = map_invalid;
	return false;
}

void
init_ap( void )
{
	processor::regs::write_cr3( ( uintptr_t )system_pml4 );
}

void
init( void )
{
	_create_system_vm();

	/* be bold and activate the new PML4 */
	log::printk( "Switching to kernel virtual address space..\n");
	processor::regs::write_cr3( ( uintptr_t )system_pml4 );

	/* relocate the memory bitmap */
	physmm::set_physical_base_offset( kVMRangePhysMemBase );
}

};
};
