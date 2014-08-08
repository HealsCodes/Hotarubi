/*******************************************************************************

    Copyright (C) 2014  René 'Shirk' Köcher
 
    This file is part of Hotarubi.

    Hotarubi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

/* high-level language kernel-entry and main initialization */

#include <stdint.h>
#include <hotarubi/boot/multiboot.h>

extern "C" void _init( void );

extern "C" void
kernel_entry( uint32_t loader_magic, struct multiboot_header *multiboot_info )
{
	_init();

    /* dummy blink code, until there is something better to do here */
    volatile uint64_t *vram = (uint64_t*)0xffffffff800b8000;
	while( 1 )
	{
		vram[32]--;
	}
}
