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

#ifndef __MEMORY_H
#define __MEMORY_H 1

#include <bitmask.h>
#include <hotarubi/boot/multiboot.h>

namespace memory
{
namespace physmm
{
	void init( const multiboot_info_t *boot_info );

	void set_physical_base_offset( const uint64_t offset );
	uint64_t physical_base_offset( void );

	uint32_t free_page_count( void );

	void *alloc_page( void );
	void *alloc_page_range( unsigned count );

	void free_page( const void *page );
	void free_page_range( const void *page, unsigned count );
};

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
		kPageFlagNoExecute    = 0x8000000000000000,
	};
	BITMASK( PageFlagSystemSet );

	void init( void );
};

namespace cache
{
	#define SLAB_SIZE PAGE_SIZE
	#define SLAB_MAX_FRAGMENT_SIZE SLAB_SIZE / 8

	typedef void  (*cache_obj_setup)( void *ptr, size_t obj_size );
	typedef void  (*cache_obj_erase)( void *ptr );

	typedef void* (*backend_alloc)( size_t n );
	typedef void  (*backend_free)( void *ptr, size_t n );

	typedef struct mem_cache *mem_cache_t;

	struct mem_cache_stats
	{
		size_t slabs;
		size_t buffers;
		size_t allocation;

		size_t cache_hits;
		size_t cache_misses;

		size_t overflows;
	};
	typedef struct mem_cache_stats mem_cache_stats_t;

	mem_cache_t create( const char *name, size_t size, size_t align = 1,
		                cache_obj_setup setup = nullptr,
		                cache_obj_erase erase = nullptr,
		                backend_alloc back_alloc = nullptr, 
		                backend_free  back_free  = nullptr );

	bool reap( mem_cache_t cache );

	bool release( mem_cache_t cache, bool force = false );

	void stats( mem_cache_t cache, mem_cache_stats_t &statbuf );

	void *get_object( mem_cache_t cache );

	void put_object( mem_cache_t cache, void *ptr );

	void init( void );
};
};

#endif
