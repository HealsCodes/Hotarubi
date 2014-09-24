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

#include <stdint.h>
#include <bitmask.h>

/* shortcut to access PhysPageFlagSet */
#define __VPF( x ) memory::virtmm::kVPFlag ##x

namespace memory
{
namespace virtmm
{
	enum VirtPageFlagSet : uint64_t
	{
		kVPFlagNone         = 0,
		kVPFlagPresent      = 1 << 0,
		kVPFlagWritable     = 1 << 1,
		kVPFlagUser         = 1 << 2,
		kVPFlagWriteThrough = 1 << 3,
		kVPFlagCacheDisable = 1 << 4,
		kVPFlagAccessed     = 1 << 5,
		kVPFlagDirty        = 1 << 6,
		kVPFlagSizeExtend   = 1 << 7,
		kVPFlagGlobal       = 1 << 8,

		kVPFlagCopyOnWrite  = 1 << 9,
		kVPFlagSparse       = 1 << 10,
		kVPFlagSwappedOut   = 1 << 11,

		kVPFlagNoExecute    = 0x8000000000000000UL,
		kVPFlagMask         = 0xffe0000000000fffUL,
		kVPFlagMask2M       = 0xffe00000000fffffUL,
	};
	BITMASK( VirtPageFlagSet );

	extern const uint64_t map_invalid;

	bool map_address( uint64_t vaddr, VirtPageFlagSet flags );
	bool map_fixed( uint64_t vaddr, uint64_t paddr, VirtPageFlagSet flags );
	bool map_address_range( uint64_t vaddr, size_t npages, VirtPageFlagSet flags );

	void unmap_address( uint64_t vaddr );
	void unmap_fixed( uint64_t vaddr );
	void unmap_address_range( uint64_t vaddr, size_t npages );

	bool lookup_mapping( uint64_t vaddr, uint64_t &pml4e, uint64_t &pdpte,
	                                     uint64_t &pdte, uint64_t &pte );

	void init_ap( void );
	void init( void );
};
};

#endif
