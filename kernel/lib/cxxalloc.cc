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

/* C++ new / delete operators */

#include <hotarubi/types.h>
#include <hotarubi/memory/kmalloc.h>

void*
operator new( size_t size )
{
	return kmalloc( size );
}

void*
operator new( size_t, void *ptr ) noexcept
{
	return ptr;
}

void*
operator new[]( size_t size )
{
	return kmalloc( size );
}

void*
operator new[]( size_t, void *ptr ) noexcept
{
	return ptr;
}

void
operator delete( void *ptr ) noexcept
{
	kfree( ptr );
}

void
operator delete( void*, void* ) noexcept
{
	/* placement delete.. */
}

void
operator delete[]( void *ptr ) noexcept
{
	kfree( ptr );
}

void
operator delete[]( void*, void* ) noexcept
{
	/* placement delete */
}
