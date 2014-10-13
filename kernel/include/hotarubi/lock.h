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
#define __LOCK_H 1

#include <hotarubi/types.h>
#include <hotarubi/io.h>
#include <hotarubi/processor/core.h>

class spin_lock
{
public:
	spin_lock() : _lock{0} {};

	bool check( void ){ return _lock == 0; }

	void lock( void )
	{
		do {} while( __atomic_test_and_set( &_lock, __ATOMIC_ACQUIRE ) );
	}

	void unlock( void )
	{
		__atomic_clear( &_lock, __ATOMIC_RELEASE );
	}

private:
	volatile uint8_t _lock = 0;
};

class spin_lock_irqsafe : public spin_lock
{
public:
	spin_lock_irqsafe() : _isr_state{false} {};

	void lock( void )
	{
		spin_lock::lock();
#ifdef KERNEL
		_isr_state = ( processor::core::read_flags() & ( 1 << 9 ) );
		processor::core::disable_interrupts();
#endif
	}

	void unlock( void )
	{
#ifdef KERNEL
		if( _isr_state )
		{
			processor::core::enable_interrupts();
		}
#endif
		spin_lock::unlock();
	}

private:
	bool _isr_state = 0;
};

#ifdef KERNEL
/* see: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50025 */
class scoped_lock
{
public:
	scoped_lock( spin_lock &l ) : _lock{l}
	{
		_lock.lock();
	};

	~scoped_lock()
	{
		_lock.unlock();
	}
private:
	spin_lock &_lock;
};
#endif

#endif
