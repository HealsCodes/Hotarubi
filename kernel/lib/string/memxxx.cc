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

/* mostly POSIX compliant functions for memory manipulation */

#include <stdint.h>
#include <stddef.h>

#include <hotarubi/macros.h>

extern "C" void*
_BUILTIN( memchr( const void *s, int c, size_t n ) )
{
	size_t i;
	const unsigned char *search = ( unsigned char* )s;

	if( s == 0 || n == 0 )
	{
		return 0;
	}

	for( i = 0; i < n; ++ i )
	{
		if( search[i] == ( unsigned char )c )
		{
			return ( void * )&search[i];
		}
	}
	return 0;
}

extern "C" int
_BUILTIN( memcmp( const void *s1, const void *s2, size_t n ) )
{
	size_t i;
	const unsigned char *c1 = ( unsigned char* )s1,
	                    *c2 = ( unsigned char* )s2;

	if( s1 == 0 || s2 == 0 || n == 0 )
	{
		return 0;
	}

	for( i = 0; i < n; ++i )
	{
		if( c1[i] != c2[i] )
		{
			return ( c1[i] < c2[i] ) ? -1 : 1;
		}
	}
	return 0;
}

extern "C" void*
_BUILTIN( memcpy( void *s1, void *s2, size_t n ) )
{
	uint8_t *c1 = ( uint8_t* )s1,
	        *c2 = ( uint8_t* )s2;

	if( c1 != 0 && c2 != 0 && n > 0 )
	{
		for( ; n > 0; --n, *c1++ = *c2++ );
	}
	return s1;
}

extern "C" void*
_BUILTIN( memmove( void *s1, void *s2, size_t n ) )
{
	if( s1 < s2 )
	{
		/* overlap is not a problem in this case */
		return __builtin_memcpy( s1, s2, n );
	}
	else
	{
		if( s1 != 0 && s2 != 0 && n > 0 )
		{
			/* buffers may overlap, only copy the part that needs changing.. */
			uint8_t *c1 = ( uint8_t* )( ( ptrdiff_t )s1 + n ),
			        *c2 = ( uint8_t* )( ( ptrdiff_t )s2 + n );

			//c1 = &c1[n];
			//c2 = &c2[n];

			while( n-- )
			{
				/* and do it back to front */
				*--c1 = *--c2;
			}
		}
	}
	return s1;
}

extern "C" void*
_BUILTIN( memset( void *s, int c, size_t n ) )
{
	unsigned char *p = ( unsigned char* )s;

	for( ; n > 0; --n, p[n] = ( unsigned char )c );

	return s;
}
