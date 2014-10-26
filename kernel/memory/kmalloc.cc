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

/* kmalloc / kfree / krealloc */

#include <string.h>
#include <hotarubi/types.h>

#include <hotarubi/memory/cache.h>
#include <hotarubi/log/log.h>

struct kmalloc_cache
{
	size_t size;
	memory::cache::mem_cache_t cache;
};

static kmalloc_cache _kmalloc_cache_table[] = {
	{    32, nullptr },
	{    64, nullptr },
	{   128, nullptr },
	{   256, nullptr },
	{   512, nullptr },
	{  1024, nullptr },
	{  2048, nullptr },
	{  3072, nullptr },
	{  4096, nullptr },
	{  8192, nullptr },
	{ 12288, nullptr },
	{ 16384, nullptr },
	/* bigger stuff should be allocated in other ways ( for now ) */
	{     0, nullptr }
};

static const char *_kmalloc_cache_names[] = {
	"kmalloc::size-32",
	"kmalloc::size-64",
	"kmalloc::size-128",
	"kmalloc::size-256",
	"kmalloc::size-512",
	"kmalloc::size-1024",
	"kmalloc::size-2048",
	"kmalloc::size-3072",
	"kmalloc::size-4096",
	"kmalloc::size-8192",
	"kmalloc::size-12288",
	"kmalloc::size-16384",
	"\0",
};


static inline kmalloc_cache*
_lookup_cache_for_ptr( void *ptr )
{
	auto cache = memory::cache::get_cache( ptr );
	if( cache != nullptr )
	{
		for( size_t i = 0; _kmalloc_cache_table[i].size; ++i )
		{
			if( cache == _kmalloc_cache_table[i].cache )
			{
				return &_kmalloc_cache_table[i];
			}
		}
	}
	return nullptr;
}

static inline kmalloc_cache*
_lookup_cache_for_size( size_t n )
{
	for( size_t i = 0; _kmalloc_cache_table[i].size; ++i )
	{
		if( n <= _kmalloc_cache_table[i].size )
		{
			return &_kmalloc_cache_table[i];
		}
	}
	return nullptr;
}

void*
kmalloc( size_t n )
{
	auto cache = _lookup_cache_for_size( n );
	if( cache != nullptr )
	{
		auto res = memory::cache::get_object( cache->cache );
		if( res != nullptr )
		{
			return ( void* )res;
		}
	}
	log::printk( "kmalloc: can't serve request for %zd bytes!\n", n );
	return nullptr;
}

void
kfree( void *ptr )
{
	if( ptr == nullptr )
	{
		panic( "kfree: attempting to free a nullptr!" );
		return;
	}

	auto cache = memory::cache::get_cache( ptr );
	if( cache != nullptr )
	{
		memory::cache::put_object( cache, ptr );
		return;
	}
	panic( "kfree: attempting to free %p which is not managed by kmalloc!",
	       ptr );
}

void*
krealloc( void *ptr, size_t n )
{
	if( ptr == nullptr )
	{
		return kmalloc( n );
	}

	auto cache = _lookup_cache_for_ptr( ptr );
	if( cache != nullptr )
	{
		void *res = kmalloc( n );
		if( res != nullptr )
		{
			memcpy( res, ptr, ( n > cache->size ) ? cache->size : n );
			memory::cache::put_object( cache->cache, cache );
			return res;
		}
	}
	log::printk( "krealloc: failed to resize %p ( not managed by kmalloc? )\n",
	             ptr );
	return ptr;
}

namespace memory
{
namespace kmalloc
{

void
init( void )
{
	for( size_t i = 0; _kmalloc_cache_table[i].size; ++i )
	{
		bool overflow_check = ( _kmalloc_cache_table[i].size < 1024 );

		auto cache = memory::cache::create( _kmalloc_cache_names[i],
		                                    _kmalloc_cache_table[i].size,
		                                    16, overflow_check );
		if( cache != nullptr )
		{
			_kmalloc_cache_table[i].cache = cache;
		}
	}
}

};
};
