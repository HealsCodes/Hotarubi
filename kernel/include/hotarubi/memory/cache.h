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

#ifndef __MEMORY_CACHE_H
#define __MEMORY_CACHE_H 1

#include <stddef.h>

#include <hotarubi/memory/const.h>

namespace memory
{
namespace cache
{
	#define SLAB_SIZE PAGE_SIZE
	#define SLAB_MAX_FRAGMENT_SIZE( x ) ( x ) / 8

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
		                bool check_overflow = true,
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
