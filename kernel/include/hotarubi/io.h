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

/* high-level wrappers for port, register and msr functions */

#ifndef __IO_H
#define __IO_H 1

#include <hotarubi/types.h>

namespace io
{
	inline void wait( void )
	{
		__asm__ __volatile__( "outb %%al, $0x80" :: "a"( 0 ) );
	};

	inline void outb( unsigned char val, unsigned short port )
	{
		__asm__ __volatile__( "outb %%al, %%dx" :: "a"( val ), "d"( port ) );
	};

	inline void outw( unsigned short val, unsigned short port )
	{
		__asm__ __volatile__( "outw %%ax, %%dx" :: "a"( val ), "d"( port ) );
	};

	inline void outl( unsigned int val, unsigned short port )
	{
		__asm__ __volatile__( "outl %%eax, %%dx" :: "a"( val ), "d"( port ) );
	};

	inline unsigned char inb( unsigned short port )
	{
		unsigned char r;
		__asm__ __volatile__( "inb %%dx" : "=a"( r ) : "d"( port ) );
		return r;
	};

	inline unsigned short inw( unsigned short port )
	{
		unsigned short r;
		__asm__ __volatile__( "inw %%dx" : "=a"( r ) : "d"( port ) );
		return r;
	};

	inline unsigned int inl( unsigned short port )
	{
		unsigned int r;
		__asm__ __volatile__( "inl %%dx" : "=a"( r ) : "d"( port ) );
		return r;
	};
};

#endif
