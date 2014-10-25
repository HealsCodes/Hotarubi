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

#ifndef _MEMORY_KMALLOC_H
#define _MEMORY_KMALLOC_H 1

#include <cstddef>

namespace memory
{
namespace kmalloc
{
	void init( void );
};
};

void *kmalloc( size_t n );
void *krealloc( void *ptr, size_t n );
void kfree( void *ptr );

#endif
