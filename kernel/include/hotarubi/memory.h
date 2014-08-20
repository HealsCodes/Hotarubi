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

#include <hotarubi/boot/multiboot.h>

namespace memory
{
namespace physmm
{
	void init( const multiboot_info_t *boot_info );

	void set_physical_base_offset( const uint64_t offset );
	size_t free_memory_for_bootstrap( void );
	uint32_t free_page_count( void );

	void *alloc_page( void );
	void free_page( const void *page );
};
};

#endif
