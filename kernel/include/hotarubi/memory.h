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

#ifndef __MEMORY_H
#define __MEMORY_H 1

#include <hotarubi/memory/physmm.h>
#include <hotarubi/memory/virtmm.h>
#include <hotarubi/memory/cache.h>
#include <hotarubi/memory/const.h>
#include <hotarubi/memory/kmalloc.h>
#include <hotarubi/memory/mmio.h>

namespace memory
{
	inline void init( struct multiboot_info *multiboot_info )
	{
		/* make sure these are called in correct order */
		physmm::init( multiboot_info );
		virtmm::init();
		cache::init();
		kmalloc::init();
        mmio::init();
	};

    inline void init_ap( void )
    {
        virtmm::init_ap();
    };
};

#endif
