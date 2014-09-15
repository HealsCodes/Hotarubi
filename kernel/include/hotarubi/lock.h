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

/* locking primitives */

#ifndef __LOCK_H
#define __LOCK_H

#include <stdint.h>
#include <hotarubi/processor/core.h>

class SpinLock
{
public:
	void lock( void )
	{
#ifdef KERNEL
		_isr_state = ( processor::read_flags() & ( 1 << 9 ) );
		processor::disable_interrupts();
#endif

		do {} while( __sync_lock_test_and_set( &_lock, 1 ) );
	}

	void unlock( void )
	{
		__sync_lock_release( &_lock );
#ifdef KERNEL
		if( _isr_state )
		{
			processor::enable_interrupts();
		}
#endif
	}

private:
	uint8_t _isr_state = 0;
	uint8_t _lock = 0;
};

#endif
