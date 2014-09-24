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

/* IO-Memory resource management */

#ifndef _MEMORY_MMIO_H
#define _MEMORY_MMIO_H 1

#include <stdint.h>
#include <stddef.h>
#include <bitmask.h>

#define __MMIO( x ) memory::mmio::kMMIOFlag ##x

namespace memory
{
namespace mmio
{
	enum MMIOFlagSet
	{
		kMMIOFlagBusy    = ( 1 << 0 ),
		kMMIOFlagMapped  = ( 1 << 5 ),
		/* type flags */
		kMMIOFlagIOPort  = ( 1 << 6 ),
		kMMIOFlagIOMem   = ( 1 << 7 ),
	};
	BITMASK( MMIOFlagSet );

	typedef struct resource *resource_t;

	resource_t request_region( const char *name, uintptr_t start, size_t size, 
	                           MMIOFlagSet flags );

	void release_region( resource_t *region );

	/* FIXME: there is no deactivate_region so it will stay mapped even after
	 *        being released.. */
	uintptr_t activate_region( resource_t region );

	void init( void );
};
};

#endif
