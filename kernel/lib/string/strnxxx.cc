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

/* mostly POSIX compliant functions for string manipulation */

#include <cstdint>
#include <cstddef>

#include <hotarubi/macros.h>

extern "C" char*
_BUILTIN( strncat( char *s1, const char *s2, size_t n ) )
{
	if( s1 != 0 && s2 != 0 && n != 0)
	{
		char *p1 = &s1[ __builtin_strlen( s1 ) ];

		for( size_t i = 0; i < n && s2[i] != '\0'; ++i )
		{
			*p1++ = s2[i];
		}
		*p1 = '\0';
	}
	return s1;
}

extern "C" int
_BUILTIN( strncmp( const char *s1, const char *s2, size_t n ) )
{
	if( s1 == 0 || s2 == 0 || n == 0 )
	{
		return 0;
	}

	size_t i;
	const unsigned char *u1 = ( const unsigned char* )s1,
	                    *u2 = ( const unsigned char* )s2;

	for( i = 0; n > 1 && u1[i] != '\0' && u2[i] != '\0'; ++i, --n )
	{
		if( u1[i] != u2[i] )
		{
			return ( u1[i] < u2[i] ) ? -1 : 1;
		}
	}
	/* one of both strings reached '\0' */
	return ( u1[i] == u2[i] ) ? 0
	                          : ( ( u1[i] < u2[i] ) ? -1 : 1 );
}

extern "C" char*
_BUILTIN( strncpy( char *s1, const char *s2, size_t n ) )
{
	/* Credit where credit is due: this one is based on PCDLib */
	char *r = s1;

	while( ( n > 0 ) && ( *s1++ = *s2++ ) )
	{
		--n;
	}

	while( n-- > 1 )
	{
		*s1++ = '\0';
	}
	return r;
}
