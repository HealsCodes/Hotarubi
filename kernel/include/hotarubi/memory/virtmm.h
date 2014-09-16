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

namespace memory
{
namespace virtmm
{
	enum PageFlagSystemSet
	{
		kPageFlagNone         = 0,
		kPageFlagPresent      = 1 << 0,
		kPageFlagWritable     = 1 << 1,
		kPageFlagUser         = 1 << 2,
		kPageFlagWriteThrough = 1 << 3,
		kPageFlagCacheDisable = 1 << 4,
		kPageFlagAccessed     = 1 << 5,
		kPageFlagDirty        = 1 << 6,
		kPageFlagSizeExtend   = 1 << 7,
		kPageFlagGlobal       = 1 << 8,

		kPageFlagCopyOnWrite  = 1 << 9,
		kPageFlagSparse       = 1 << 10,
		kPageFlagSwappedOut   = 1 << 11,

		kPageFlagNoExecute    = 0x8000000000000000,
	};
	BITMASK( PageFlagSystemSet );

	bool map_address( uint64_t vaddr, PageFlagSystemSet flags );
	bool map_fixed( uint64_t vaddr, uint64_t paddr, PageFlagSystemSet flags );
	bool map_address_range( uint64_t vaddr, size_t npages, PageFlagSystemSet flags );

	void unmap_address( uint64_t vaddr );
	void unmap_fixed( uint64_t vaddr );
	void unmap_address_range( uint64_t vaddr, size_t npages );

	void init_ap( void );
	void init( void );
};
};

#endif
