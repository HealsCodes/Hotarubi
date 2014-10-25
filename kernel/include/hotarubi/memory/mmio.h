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

#include <bitmask.h>
#include <hotarubi/types.h>

#define __MMIO( x ) memory::mmio::Flags::k ##x

namespace memory
{
namespace mmio
{
	enum class Flags
	{
		kBusy    = ( 1 << 0 ),
		kShared  = ( 1 << 4 ),
		kMapped  = ( 1 << 5 ),
		/* type flags */
		kIOPort  = ( 1 << 6 ),
		kIOMem   = ( 1 << 7 ),

		is_bitmask
	};

	typedef struct resource *resource_t;

	resource_t request_region( const char *name, phys_addr_t start, size_t size,
	                           Flags flags );

	void release_region( resource_t *region );

	/* FIXME: there is no deactivate_region so it will stay mapped even after
	 *        being released.. */
	virt_addr_t activate_region( resource_t region );

	void init( void );
};
};

#endif
