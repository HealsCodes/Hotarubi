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

#include <atomic>

class spin_lock
{
public:
	spin_lock() : _lock{ATOMIC_FLAG_INIT} {};

	void lock( void )
	{
		do {} while( _lock.test_and_set( std::memory_order_acquire ) );
	}

	void unlock( void )
	{
		_lock.clear( std::memory_order_release );
	}

private:
	std::atomic_flag _lock = ATOMIC_FLAG_INIT;
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

class scoped_lock
{
public:
#if defined( __GNUC__ ) && __GNUC__ == 4 && __GNUC_MINOR__ < 9
	/* see: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50025 */
	scoped_lock( spin_lock &lock ) : _lock( lock )
#else
	scoped_lock( spin_lock &lock ) : _lock{lock}
#endif
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
