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
_BUILTIN( strcat( char *s1, const char *s2 ) )
{
	if( s1 != 0 && s2 != 0 )
	{
		char *p1 = &s1[ __builtin_strlen( s1 ) ];

		for( size_t i = 0; s2[i] != '\0'; ++i )
		{
			*p1++ = s2[i];
		}
		*p1 = '\0';
	}
	return s1;
}

extern "C" char*
_BUILTIN( strchr( const char *s1, int c ) )
{
	if( s1 == 0 )
	{
		return 0;
	}

	for( size_t i = 0; s1[i] != '\0'; ++i )
	{
		if( s1[i] == ( char )c )
		{
			return ( char* )&s1[i];
		}
	}

	if( ( char )c == '\0' )
	{
		return ( char* )&s1[__builtin_strlen( s1 )];
	}
	return 0;
}

extern "C" char*
_BUILTIN( strrchr( const char *s1, int c ) )
{
	if( s1 == 0 )
	{
		return 0;
	}

	for( size_t i = __builtin_strlen( s1 ); i > 0; --i )
	{
		if( s1[i] == ( char )c )
		{
			return ( char* )&s1[i];
		}
	}
	if( *s1 == ( char )c )
	{
		return ( char* )s1;
	}
	return 0;
}

extern "C" int
_BUILTIN( strcmp( const char *s1, const char *s2 ) )
{
	if( s1 == 0 || s2 == 0 )
	{
		return 0;
	}

	size_t i;
	const unsigned char *u1 = ( unsigned char* )s1,
	                    *u2 = ( unsigned char* )s2;

	for( i = 0; u1[i] != '\0' && u2[i] != '\0'; ++i )
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
_BUILTIN( strcpy( char *s1, const char *s2 ) )
{
	return ( char* )__builtin_memcpy( s1, s2, __builtin_strlen( s2 ) + 1 );
}

extern "C" size_t
_BUILTIN( strcspn( const char *s1, const char *s2 ) )
{
	if( s1 == 0 || s2 == 0 )
	{
		return 0;
	}

	for( size_t i = 0; s1[i] != '\0'; ++i )
	{
		if( __builtin_strchr( s2, s1[i] ) != 0 )
		{
			return i;
		}
	}
	return __builtin_strlen( s1 );
}

extern "C" size_t
_BUILTIN( strlen( const char *s ) )
{
	if( s == 0 )
	{
		return 0;
	}

	size_t i;
	for( i = 0; s[i] != '\0'; ++i );

	return i;
}

extern "C" char*
_BUILTIN( strpbrk( const char *s1, const char *s2 ) )
{
	if( s1 == 0 || s2 == 0 )
	{
		return 0;
	}
	for( ; *s1 != '\0'; ++s1 )
	{
		const char *seek = s2;
		while( *seek != '\0' )
		{
			if( *s1 == *seek++ )
			{
				return ( char * )s1;
			}
		}
	}
	return ( char * )0;
}

extern "C" size_t
_BUILTIN( strspn( const char *s1, const char *s2 ) )
{
	for( size_t i = 0; s1[i] != '\0'; ++i )
	{
		if( __builtin_strchr( s2, s1[i] ) == 0 )
		{
			return i;
		}
	}
	return __builtin_strlen( s1 );
}

extern "C" char*
_BUILTIN( strstr( const char *s1, const char *s2 ) )
{
	if( s1 != 0 && s2 != 0 )
	{
		size_t l1 = __builtin_strlen( s1 );
		size_t l2 = __builtin_strlen( s2 );
		for( size_t i = 0; i < l1; ++i )
		{
			if( __builtin_strncmp( &s1[i], s2, l2 ) == 0 )
			{
				return ( char* )&s1[i];
			}
		}
	}
	return 0;
}
