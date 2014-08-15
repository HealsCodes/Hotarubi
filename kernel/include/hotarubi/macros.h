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

/* generic macros to save some typing */

#ifndef MACROS_H
#define MACROS_H 1

#define BEGIN_C_DECL \
	#ifdef __cplusplus \
	extern "C" { \
	#endif

#define END_C_DECL \
	#ifdef __cplusplus \
	} \
	#endif

#ifndef RUN_TESTS
# define _BUILTIN(x) x
#else
# define _BUILTIN(x) builtin_ ##x
#endif

#endif
