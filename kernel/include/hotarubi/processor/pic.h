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

/* legacy 8259 PIC remap & disable 
 *
 * Don't read the below code in too much detail.. it's really only there
 * to be able to disable the blasted thing from inside the ACPI parser.
 */

#ifndef __PROCESSOR_PIC_H
#define __PROCESSOR_PIC_H 1

#include <hotarubi/io.h>
#include <hotarubi/log/log.h>

namespace processor
{
	namespace pic
	{
		inline void initialize( void )
		{
			uint8_t mask1, mask2;

			mask1 = io::inb( 0x21 );
			mask2 = io::inb( 0xa1 );

			/* send INIT + ICW4 */
			io::outb( 0x11, 0x20 );
			io::wait();
			io::outb( 0x11, 0xa0 );
			io::wait();

			/* set base IRQs to 33 & 41 */
			io::outb( 0x21, 0x21 );
			io::wait();
			io::outb( 0x29, 0xa1 );
			io::wait();

			/* setup cascatding */
			io::outb( 0x04, 0x21 );
			io::wait();
			io::outb( 0x02, 0xa1 );
			io::wait();

			/* send ICW_8086 */
			io::outb( 0x01, 0x21 );
			io::wait();
			io::outb( 0x01, 0xa1 );
			io::wait();

			/* reset masks */
			io::outb( mask1, 0x21 );
			io::outb( mask2, 0xa1 );
		};

		inline void disable( void )
		{
			io::outb( 0xff, 0x21 );
			io::outb( 0xff, 0xa1 );
			io::wait();
			io::outb( 0x20, 0x20 );
			io::wait();
			io::outb( 0xa0, 0x20 );
		};
	};
};

#endif
