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

/* high-level language kernel-entry and main initialization */

#include <stdint.h>
#include <hotarubi/boot/multiboot.h>
#include <hotarubi/log/log.h>
#include <hotarubi/memory.h>
#include <hotarubi/processor.h>
#include <hotarubi/release.h>

extern "C" void _init( void );

extern "C" void
kernel_entry( uint32_t loader_magic, struct multiboot_info *multiboot_info )
{
	_init();

	log::init_printk();
	log::register_debug_output();

	log::printk( "%s\n", RELEASE_STRING );
	log::printk( "-- reached %s --\n", __FUNCTION__ );
	log::printk( "loader magic: %#08x\n", loader_magic );
	log::printk( "loader data : %p\n", multiboot_info );

	memory::physmm::init( multiboot_info );
	memory::virtmm::init();
	memory::cache::init();

	processor::init();
	__UNDER_CONSTRUCTION__;
}

extern "C" void
kernel_ap_entry( void )
{
	/* This is executed by application processors */
	processor::init();

	memory::virtmm::init_ap();

	__UNDER_CONSTRUCTION__;
}
