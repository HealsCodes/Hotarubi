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

/* global type definitions and *stuff* */

#ifndef __TYPES_H
#define __TYPES_H 1

#include <stdint.h>
#include <stddef.h>

typedef uint64_t virt_addr_t;
typedef uint64_t phys_addr_t;

/* C++11 user defined literals for certain number systems / units */

/* time quantities */

constexpr uint64_t operator ""_s( unsigned long long seconds )
{
	return seconds * 1000 * 1000;
};

constexpr uint64_t operator ""_ms( unsigned long long milliseconds )
{
	return milliseconds * 1000;
};

constexpr uint64_t operator ""_us( unsigned long long microseconds )
{
	return microseconds;
};

#endif
