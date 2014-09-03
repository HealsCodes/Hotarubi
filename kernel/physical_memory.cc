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

#include <hotarubi/memory.h>
#include <hotarubi/vmconst.h>

#include <hotarubi/lock.h>
#include <hotarubi/log/log.h>

#ifdef KERNEL
extern "C" unsigned char __end[]; /* defined in link.ld */
#endif

namespace memory
{
namespace physmm
{

static uint32_t *memory_map_data = nullptr;
static uint32_t  memory_map_size = 0;
static uint32_t  memory_map_used = 0;
static uint64_t  memory_map_base = 0; /* offset applied to physical addresses */
static SpinLock  memory_map_lock;
uint64_t memory_upper_bound = 0;

static inline bool
_peek_used( uint64_t bit )
{
	if( bit > memory_map_size * 8 )
	{
		return true;
	}
	return memory_map_data[ bit / 32 ] & ( 1 << ( bit % 32 ) );
}

static inline void
_mark_used( uint64_t bit )
{
	if( ( bit < memory_map_size * 8 ) && _peek_used( bit ) == false )
	{
		memory_map_data[ bit / 32 ] |= ( 1 << ( bit % 32 ) );
		++memory_map_used;
	}
}

static inline void
_mark_free( uint64_t bit )
{
	if( ( bit < memory_map_size * 8 ) && _peek_used( bit ) == true )
	{
		memory_map_data[ bit / 32 ] &= ~( 1 << ( bit % 32 ) );
		--memory_map_used;
	}
}

static inline void
_mark_free_range( uint64_t addr, size_t len )
{
	uint64_t bit = addr / PAGE_SIZE;
	len = len / PAGE_SIZE;

	for( ; len > 0; --len )
	{
		_mark_free( bit++ );
	}
}

static inline void
_mark_used_range( uint64_t addr, size_t len )
{
	uint64_t bit = addr / PAGE_SIZE;
	len = len / PAGE_SIZE;

	for( ; len > 0; --len )
	{
		_mark_used( bit++ );
	}
}

static inline uint64_t
_search_first_free( void )
{
	for( size_t i = 0; i < memory_map_size / sizeof( memory_map_data[0] ); ++i )
	{
		if( memory_map_data[i] != 0xffffffff )
		{
			for( uint64_t bit = 0; bit < 32; ++bit )
			{
				if( _peek_used( i * 32 + bit ) == false )
				{
					return ( i * 32 + bit ) * PAGE_SIZE;
				}
			}
		}
	}
	return 0;
}

#ifdef KERNEL

void
init( const multiboot_info_t *boot_info )
{
	uint64_t mem_available = 0, first_free = 0, first_chunk_above_1mb = 0, last_chunk_end = 0;
	multiboot_memory_map_t *mem_map;

	/* I should go and kill one of the multiboot spec authors..
	 * map + map->size + sizeof( map->size ) -- what the hell did you smoke?
	 */
	log::printk( "loader provided memory map:\n" );
	log::printk( "-----------------------------------------\n" );
	mem_map = ( multiboot_memory_map_t* )( ( uint64_t ) boot_info->mmap_addr );
	do
	{
		if( mem_map->type == MULTIBOOT_MEMORY_AVAILABLE )
		{
			if( first_chunk_above_1mb == 0 && mem_map->addr >= 0x100000 )
			{
				first_chunk_above_1mb = mem_map->addr + mem_map->len;
				if( first_chunk_above_1mb > BOOT_MAX_MAPPED )
				{
					first_chunk_above_1mb = BOOT_MAX_MAPPED;
				}
			}
			if( last_chunk_end < mem_map->addr + mem_map-> len )
			{
				last_chunk_end = mem_map->addr + mem_map-> len;
			}
			mem_available += mem_map->len;
		}

		/*
		* #define E820_RAM	1
		* #define E820_RESERVED	2
		* #define E820_ACPI	3
		* #define E820_NVS	4
		* #define E820_UNUSABLE	5
		*/
		log::printk( "%#016llx - %#016llx, %s (%2d)\n",
		             mem_map->addr, mem_map->addr + mem_map->len,
		             mem_map->type == 1 ? "free" : "rsvd",
		             mem_map->type );

		mem_map = ( multiboot_memory_map_t* )( ( uintptr_t )mem_map + mem_map->size + sizeof( mem_map->size ) );
	} while( ( uintptr_t)mem_map < boot_info->mmap_addr + boot_info->mmap_length );

	memory_map_size = last_chunk_end / PAGE_SIZE / 8;
	memory_upper_bound = last_chunk_end;

	if( last_chunk_end < 0x1000000 )
	{
		/* FIXME: this *will fail* due to BIOS / chipset mappings
		 *        if there are exactly 16MB installed..
		 */
		log::printk( "\nPANIC: not enough physical RAM installed at least 16MB are required!\n" );
		do {} while( 1 );
	}

	log::printk( "-----------------------------------------\n" );
	log::printk( "%lu MB usable RAM\n", mem_available / 0x100000 );
	log::printk( "%u KB required for physical bitmap\n", memory_map_size / 1024 );
	log::printk( "-----------------------------------------\n" );

	/* check for the first free location that has enough space */
	first_free  = ( uintptr_t )__end - KERNEL_VMA;

	if( boot_info-> flags & MULTIBOOT_INFO_MODS )
	{
		multiboot_module_t *mod_list = ( multiboot_module_t* )( ( uintptr_t )boot_info->mods_addr );
		for( size_t i = 0; i < boot_info->mods_count; ++i )
		{
			if( mod_list[i].mod_start > first_free )
			{
				first_free = mod_list[i].mod_end;
				if( ( uintptr_t )mod_list[i].cmdline > first_free )
				{
					const char* cmdline = ( const char* )( ( uintptr_t )mod_list[i].cmdline );
					first_free = mod_list[i].cmdline + strlen( cmdline );
				}
			}
		}
	}

	if( first_free + memory_map_size > first_chunk_above_1mb )
	{
		// FIXME: I need a panic() method!
		log::printk( "\nPANIC: not enough free RAM to initialize memory bitmap!\n" );
		do {} while( 1 );
	}
	first_free += sizeof( uint64_t );

	memory_map_data = ( uint32_t* )first_free;
	memory_map_used = memory_map_size * 8;

	memset( memory_map_data, 0xff, memory_map_size );

	first_free += memory_map_size + PAGE_SIZE;
	first_free &= 0x7ffff000;

	/* second iteration, mark non-reserved memory above first_free as available */
	mem_map = ( multiboot_memory_map_t* )( ( uint64_t ) boot_info->mmap_addr );
	do
	{
		if( mem_map->addr < first_free && 
		    mem_map->addr + mem_map->len > first_free )
		{
			mem_map->len -= first_free - mem_map->addr;
			mem_map->addr = first_free;
		}

		if( mem_map->type == MULTIBOOT_MEMORY_AVAILABLE && 
		    mem_map->addr >= first_free )
		{
			_mark_free_range( mem_map->addr, mem_map->len );
		}
		mem_map = ( multiboot_memory_map_t* )( ( uintptr_t )mem_map + mem_map->size + sizeof( mem_map->size ) );
	} while( ( uintptr_t)mem_map < boot_info->mmap_addr + boot_info->mmap_length );

	log::printk( "Physical memory map at %p\n"
	             "-- %u entries, %u free (spanning %lu MB)\n",
	             memory_map_data,
	             memory_map_size * 8,
	             memory_map_size * 8 - memory_map_used,
	             ( (uint64_t) PAGE_SIZE * ( memory_map_size * 8 - memory_map_used ) / 0x100000 ) );
}

#endif

void
set_physical_base_offset( const uint64_t offset )
{
	/* change the expected location of the memory map and set the offset for
	 * physical addresses used alloc_page() and free_page()
	 */
	memory_map_data = ( uint32_t* )( ( uintptr_t )memory_map_data - memory_map_base + offset );
	memory_map_base = offset;

	log::printk( "Physical memory map relocated to %p\n", memory_map_data );
}

uint64_t 
physical_base_offset( void )
{
	return memory_map_base;
}

size_t
free_memory_for_bootstrap( void )
{
	size_t res = 0;
	for( uint64_t bit = BOOT_MAX_MAPPED / PAGE_SIZE; bit > 1; --bit )
	{
		if( _peek_used( bit ) == false )
		{
			res += PAGE_SIZE;
		}
	}
	return res;
}

uint32_t
free_page_count( void )
{
	return memory_map_size * 8 - memory_map_used;
}

void*
alloc_page( void )
{
	uint64_t addr;

	memory_map_lock.lock();
	if( free_page_count() == 0 )
	{
		memory_map_lock.unlock();
		return nullptr;
	}
	addr = _search_first_free();
	if( addr == 0 )
	{
		memory_map_lock.unlock();
		return nullptr;
	}

	_mark_used_range( addr, PAGE_SIZE );
	memory_map_lock.unlock();

	return ( void* )( addr + memory_map_base );
}

void
free_page( const void* page )
{
	uint64_t addr = ( uint64_t )page;

	if( addr >= memory_map_base )
	{
		addr -= memory_map_base;
	}

	if( addr < PAGE_SIZE )
	{
		/* prevent freeing of the first 4K */
		return;
	}
	if( addr / PAGE_SIZE / 32 + addr / PAGE_SIZE % 32 > memory_map_size * 8 )
	{
		/* prevent out-of-bounds access */
		return;
	}

	memory_map_lock.lock();
	_mark_free_range( addr, PAGE_SIZE );
	memory_map_lock.unlock();
}

};
};
