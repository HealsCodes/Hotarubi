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

#ifndef __MEMORY_PHYSMM_H
#define __MEMORY_PHYSMM_H 1

#include <hotarubi/boot/multiboot.h>

#include <list.h>
#include <bitmask.h>

/* BEWARE: this macro only works for addresses in kernel space! */
#define __VMBASE memory::virtmm::kVMRangePhysMemBase
#define __VMKERN memory::virtmm::kVMRangeKernelBase

#define __PA( x )  ( ( ( uintptr_t )( x ) >= __VMKERN ) ? ( ( uintptr_t )( x ) - __VMKERN ) \
                                                        : ( ( uintptr_t )( x ) & ~__VMBASE ) )

/* BEWARE: this macro only works for addresses in kernel space! */
#define __VA( x )  ( ( ( uintptr_t )( x ) + __VMBASE ) )

/* shortcut to access PhysPageFlagSet */
#define __PPF( x ) memory::physmm::kPPFlag ##x

namespace memory
{
namespace physmm
{
	enum PhysPageFlagSet : uint16_t
	{
		kPPFlagUnused       = ( 1 << 0 ),
		kPPFlagActive       = ( 1 << 1 ),
		kPPFlagLocked       = ( 1 << 2 ),
		kPPFlagReserved     = ( 1 << 3 ),
		kPPFlagSlab         = ( 1 << 4 ),
	};
	BITMASK( PhysPageFlagSet );

	struct page_map
	{
		struct list_head link;
		PhysPageFlagSet flags;
	};
	typedef struct page_map page_map_t;

	void init( const multiboot_info_t *boot_info );

	void set_physical_base_offset( const uint64_t offset );
	uint64_t physical_base_offset( void );

	uint32_t free_page_count( void );

	void *alloc_page( PhysPageFlagSet flags );
	void *alloc_page_range( unsigned count, PhysPageFlagSet flags );

	void free_page( const void *page );
	void free_page_range( const void *page, unsigned count );

	page_map_t *get_page_map( uint64_t paddr );
};
};

#endif
