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

#include <hotarubi/io.h>

#include <hotarubi/memory.h>
#include <hotarubi/log/log.h>
#include <hotarubi/boot/bootmem.h>
#include <hotarubi/boot/multiboot.h>

#define MMU_PML4_INDEX( x ) ( ( ( x ) >> 39 ) & 0x1ff )
#define MMU_PDPT_INDEX( x ) ( ( ( x ) >> 30 ) & 0x1ff )
#define MMU_PDT_INDEX( x )  ( ( ( x ) >> 21 ) & 0x1ff )
#define MMU_PT_INDEX( x )   ( ( ( x ) >> 12 ) & 0x1ff )

#define MMU_PDPT_ADDR( pml4, vaddr ) ( pml4[MMU_PML4_INDEX( vaddr )] & ~0xfff )
#define MMU_PDT_ADDR( pdpt, vaddr )  ( pdpt[MMU_PDPT_INDEX( vaddr )] & ~0xfff )
#define MMU_PT_ADDR( pdt , vaddr )   (  pdt[MMU_PDT_INDEX ( vaddr )] & ~0xfff )

#define MMU_PHYS_ADDR_4K( pt  , vaddr ) (   pt[MMU_PT_INDEX ( vaddr )] & ~0xfff )
#define MMU_PHYS_ADDR_2M( pdt , vaddr ) (  pdt[MMU_PDT_INDEX( vaddr )] & ~0xfffff )

#define PHYS_ADDR( addr ) ( ( addr ) - physmm::physical_base_offset() )

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
static uint64_t *system_pml4 = nullptr;

static bool
_map_region( uint64_t *pml4, uint64_t vaddr, uint64_t paddr, size_t len,
             PageFlagSystemSet flags=kPageFlagNone )
{
	uint64_t *pdpt = nullptr,
	         *pdt  = nullptr,
	         *pt   = nullptr;

	if( len % 0x1000 != 0 )
	{
		len += 0x1000;
	}
	len /= 0x1000;

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
			memset( pdpt, 0, 0x1000 );
			pml4[MMU_PML4_INDEX( vaddr )] = PHYS_ADDR( ( uint64_t )pdpt ) | kPageFlagPresent;
		}

		pdt = ( uint64_t* )MMU_PDT_ADDR( pdpt, vaddr );
		if( pdt == nullptr )
		{
			if( ( pdt = ( uint64_t* )physmm::alloc_page() ) == nullptr )
			{
				return false;
			}
			memset( pdt, 0, 0x1000 );
			pdpt[MMU_PDPT_INDEX( vaddr )] = PHYS_ADDR( ( uint64_t )pdt ) | kPageFlagPresent;
		}

		pt = ( uint64_t* )MMU_PT_ADDR( pdt, vaddr );
		if( pt == nullptr )
		{
			if( ( pt = ( uint64_t* )physmm::alloc_page() ) == nullptr )
			{
				return false;
			}
			memset( pt, 0, 0x1000 );
			pdt[MMU_PDT_INDEX( vaddr )] = PHYS_ADDR( ( uint64_t )pt ) | kPageFlagPresent;
		}

		pt[MMU_PT_INDEX( vaddr )] = paddr | kPageFlagPresent | flags;

		vaddr += 0x1000;
		paddr += 0x1000;
	} while( --len );

	return true;
}

static
void _create_system_vm( void )
{
	log::printk( "Initializing kernel virtual address space\n" );

	if( ( system_pml4 = ( uint64_t* )physmm::alloc_page() ) == nullptr )
	{
		goto error_out;
	}
	memset( system_pml4, 0, 0x1000 );

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
	                  ( uint64_t )__text, ( uint64_t )__text - KERNEL_VMA,
	                  ( uint64_t )__data - ( uint64_t )__text ) )
	{
		goto error_out;
	}

	/* map kernel .data and .bss (rw) */
	if( !_map_region( system_pml4, 
	                  ( uint64_t )__data, ( uint64_t )__data -  KERNEL_VMA,
	                  ( uint64_t )__end - ( uint64_t )__data,
	                  kPageFlagWritable ) )
	{
		goto error_out;
	}

	/* map the whole set of physical memory */
	if( !_map_region( system_pml4,
	                  0xffff880000000000, 0,
	                  physmm::memory_upper_bound,
	                  kPageFlagWritable ) )
	{
		goto error_out;
	}

	/* be bold and activate the new PML4 for testing */
	io::write_cr3( ( uint64_t )system_pml4 );

	/* relocate the memory bitmap */
	physmm::set_physical_base_offset( 0xffff880000000000 );
	return;

error_out:
	log::printk( "\nPANIC: out of memory while creating the system VM!\n" );
	do {} while( 1 );
}

void
init( void )
{
	_create_system_vm();
}

};
};
