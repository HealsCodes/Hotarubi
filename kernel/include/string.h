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

#ifndef STRING_H
#define STRING_H 1

#include <stddef.h>
#include <hotarubi/macros.h>

BEGIN_C_DECL

/* prefer GCC built in functions (use fallbacks from memxxx.cc otherwise)*/
#define memchr( s, c, n )     __builtin_memchr( s, c, n )
#define memcmp( s1, s2, n )   __builtin_memcmp( s1, s2, n )
#define memcpy( s1, s2, n )   __builtin_memcpy( s1, s2, n )
#define memset( s, c, n )     __builtin_memset( s, c, n )

/* declare prototypes for everything GCC has not built in support for */
void* memmove( void *s1, void *s2, size_t n );

/* prefer GCC built in functions (or use fallbacks from strxxx.cc / strnxxx.cc) */
#define strcat( s1, s2 )     __builtin_strcat( s1, s2 )
#define strchr( s, c )       __builtin_strchr( s, c )  
#define strcmp( s1, s2 )     __builtin_strcmp( s1, s2 )
#define strcpy( s1, s2 )     __builtin_strcpy( s1, s2 )
#define strlen( s )          __builtin_strlen( s )     
#define strspn( s1, s2 )     __builtin_strspn( s1, s2 )
#define strstr( s1, s2 )     __builtin_strstr( s1, s2 )
#define strpbrk( s1, s2 )    __builtin_strpbrk( s1, s2 )
#define strcspn( s1, s2 )    __builtin_strcspn( s1, s2 )
#define strrchr( s, c )      __builtin_strrchr( s, c )  
#define strncat( s1, s2, n ) __builtin_strncat( s1, s2, n )
#define strncmp( s1, s2, n ) __builtin_strncmp( s1, s2, n )
#define strncpy( s1, s2, n ) __builtin_strncpy( s1, s2, n )

END_C_DECL

#endif
