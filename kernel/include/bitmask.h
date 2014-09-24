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

/* Template based bitmask operators for enumerations.
 * This is based on http://en.wikipedia.org/wiki/Barton-Nackman and requires
 * the use of -ffriend-injection which I know is not that clean at all but
 * it produces much cleaner code than C++11 enum classes *sigh*
 */

#ifndef _BITMASK_H
#define _BITMASK_H 1

#include <stdint.h>

template <typename T>
class BitOps
{
	friend constexpr T operator &( T a, T b )
	{ 
		return ( T )( ( uintptr_t )a & ( uintptr_t ) b );
	};

	friend constexpr T operator |( T a, T b ) 
	{ 
		return ( T )( ( uintptr_t )a | ( uintptr_t ) b );
	};

	friend constexpr T operator ^( T a, T b )
	{ 
		return ( T )( ( uintptr_t )a ^ ( uintptr_t ) b ); 
	};

	friend constexpr T operator ~( T a )
	{ 
		return ( T )( ~( uintptr_t )a ); 
	};

	friend T& operator &=( T &a, T b )
	{ 
		a = a & b; return a; 
	};

	friend T& operator |=( T &a, T b )
	{ 
		a = a | b; return a;
	};

	friend T& operator ^=( T &a, T b )
	{ 
		a = a ^ b; return a;
	};
};

#define BITMASK( T ) namespace _detail { class BitOps##T : private BitOps<T>{}; }

#endif
