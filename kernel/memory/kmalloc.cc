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
#include <stdint.h>
#include <stddef.h>

#include <hotarubi/memory/cache.h>
#include <hotarubi/log/log.h>

struct kmalloc_cache
{
	size_t size;
	memory::cache::mem_cache_t cache;
};

static struct kmalloc_cache kmalloc_cache_table[] = {
	{    32, nullptr },
	{    64, nullptr },
	{   128, nullptr },
	{   256, nullptr },
	{   512, nullptr },
	{  1024, nullptr },
	{  2048, nullptr },
	{  3072, nullptr },
	{  4092, nullptr },
	/* bigger stuff should be allocated in other ways ( for now ) */
	{     0, nullptr }
};

static const char *kmalloc_cache_names[] = {
	"kmalloc::size-32",
	"kmalloc::size-64",
	"kmalloc::size-128",
	"kmalloc::size-256",
	"kmalloc::size-512",
	"kmalloc::size-1024",
	"kmalloc::size-2048",
	"kmalloc::size-3072",
	"kmalloc::size-4092",
	""
};

static inline struct kmalloc_cache*
_lookup_cache_for_ptr( void *ptr )
{
	uintptr_t *obj = ( uintptr_t* )( ( uintptr_t )ptr - sizeof( uintptr_t ) );
	for( size_t i = 0; kmalloc_cache_table[i].size; ++i )
	{
		if( ( uintptr_t )kmalloc_cache_table[i].cache == *obj )
		{
			return &kmalloc_cache_table[i];
		}
	}
	return nullptr;
}

static inline struct kmalloc_cache*
_lookup_cache_for_size( size_t n )
{
	for( size_t i = 0; kmalloc_cache_table[i].size; ++i )
	{
		if( n + sizeof( uintptr_t ) <= kmalloc_cache_table[i].size )
		{
			return &kmalloc_cache_table[i];
		}
	}
	return nullptr;
}

void*
kmalloc( size_t n )
{
	struct kmalloc_cache *cache = _lookup_cache_for_size( n );
	if( cache != nullptr )
	{
		uintptr_t *obj = ( uintptr_t* )memory::cache::get_object( cache->cache );
		if( obj != nullptr )
		{
			*obj = ( uintptr_t )cache->cache;
			++obj;
		}
		return ( void* )obj;
	}
	log::printk( "kmalloc: can't serve request for %zd bytes!\n", n );
	return nullptr;
}

void
kfree( void *ptr )
{
	if( ptr == nullptr )
	{
		log::printk( "kfree: attempting to free a nullptr!\n" );
		return;
	}

	struct kmalloc_cache *cache = _lookup_cache_for_ptr( ptr );
	if( cache != nullptr )
	{
		void *obj = ( void* )( ( uintptr_t )ptr - sizeof( uintptr_t ) );
		memory::cache::put_object( cache->cache, obj );
		return;
	}
	log::printk( "kfree: attempting to free %p which is not managed by kmalloc!\n",
	             ptr );
}

void*
krealloc( void *ptr, size_t n )
{
	if( ptr == nullptr )
	{
		return kmalloc( n );
	}

	struct kmalloc_cache *cache = _lookup_cache_for_ptr( ptr );
	if( cache != nullptr )
	{
		void *res = kmalloc( n );

		if( res != nullptr )
		{
			void *obj = ( void* )( ( uintptr_t )ptr - sizeof( uintptr_t ) );

			memcpy( res, ptr, ( n > cache->size ) ? cache->size : n );
			memory::cache::put_object( cache->cache, obj );
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
	memory::cache::mem_cache_t cache = nullptr;
	for( size_t i = 0; kmalloc_cache_table[i].size; ++i )
	{
		cache = memory::cache::create( kmalloc_cache_names[i],
		                               kmalloc_cache_table[i].size + sizeof( uintptr_t ) );
		if( cache )
		{
			kmalloc_cache_table[i].cache = cache;
		}
	}
}

};
};
