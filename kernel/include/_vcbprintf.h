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

/* _(v)cbprintf as specified by PDClib */

#ifndef ___VCBPRINTF_H
#define ___VCBPRINTF_H 1

#include <cstdint>
#include <cstdarg>
#include <cstddef>

#include <hotarubi/macros.h>

BEGIN_C_DECL

int _vcbprintf( void *p, size_t ( *cb )( void *p, const char *str, size_t n ),
                const char *fmt, va_list ap );

int _cbprintf( void *p, size_t ( *cb )( void *p, const char *str, size_t n ),
               const char *fmt, ... ) __attribute__( ( format( printf, 3, 4 ) ) );

END_C_DECL

#endif
