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

/*  */
#ifndef _LOG_H
#define _LOG_H 1

#include <stdint.h>
#include <stddef.h>

#define __UNDER_CONSTRUCTION__ do { \
    	log::printk( "\n\n-- reached under construction area in %s:%d, idling --\n ", \
    	             __FILE__, __LINE__ ); \
    	for( ; ; ){ __asm__ __volatile__( "hlt" ); }; \
    } while( 0 )

namespace log
{
	void init_printk( void );
	int printk( const char *fmt, ... ) __attribute__( ( format( printf, 1, 2 ) ) );

	void register_debug_output( void );
	void unregister_debug_output( void );
};

#endif
