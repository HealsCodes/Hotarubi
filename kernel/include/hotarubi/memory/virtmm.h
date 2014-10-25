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

#ifndef __MEMORY_VIRTMM_H
#define __MEMORY_VIRTMM_H 1

#include <bitmask.h>
#include <hotarubi/types.h>

/* shortcut to access PhysPageFlagSet */
#define __VPF( x ) memory::virtmm::Flags::k ##x

namespace memory
{
namespace virtmm
{
	enum class Flags : uint64_t
	{
		kNone         = 0,
		kPresent      = 1 << 0,
		kWritable     = 1 << 1,
		kUser         = 1 << 2,
		kWriteThrough = 1 << 3,
		kCacheDisable = 1 << 4,
		kAccessed     = 1 << 5,
		kDirty        = 1 << 6,
		kSizeExtend   = 1 << 7,
		kGlobal       = 1 << 8,

		kCopyOnWrite  = 1 << 9,
		kSparse       = 1 << 10,
		kSwappedOut   = 1 << 11,

		kNoExecute    = 0x8000000000000000UL,
		kMask         = 0xffe0000000000fffUL,
		kMask2M       = 0xffe00000000fffffUL,

		is_bitmask
	};

	extern const virt_addr_t map_invalid;

	bool map_address( virt_addr_t vaddr, Flags flags );
	bool map_fixed( virt_addr_t vaddr, phys_addr_t paddr, Flags flags );
	bool map_address_range( virt_addr_t vaddr, size_t npages, Flags flags );

	void unmap_address( virt_addr_t vaddr );
	void unmap_fixed( virt_addr_t vaddr );
	void unmap_address_range( virt_addr_t vaddr, size_t npages );

	bool lookup_mapping( virt_addr_t vaddr, uint64_t &pml4e, uint64_t &pdpte,
	                                     uint64_t &pdte, uint64_t &pte );

	void init_ap( void );
	void init( void );
};
};

#endif
