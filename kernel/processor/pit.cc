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

/* 8253/8254 PIT driver */

#include <hotarubi/processor/pit.h>

#include <hotarubi/io.h>
#include <hotarubi/log/log.h>

#include <hotarubi/processor/core.h>

namespace processor
{

#define PIT_CMD  0x43
#define PIT_DATA 0x40

#define PIT_MODE_SINGLE 0x30 /* single trigger on zero, channel 0, binary */
#define PIT_MODE_PERIOD 0x34 /* square-wave generator, channel 0, binary  */

#define PIT_FREQUENCY 1193180

void
pit::init( void )
{
	_io_ports = memory::mmio::request_region( "PIT", 0x40, 3, __MMIO( IOPort ) | __MMIO( Busy ) );
	if( _io_ports != nullptr )
	{
		memory::mmio::activate_region( _io_ports );
		if( idt::register_irq_handler( _vector, _irq_handler, ( uint64_t )this ) )
		{
			core::route_isa_irq( 0, _vector );
			one_shot( 200_us, nullptr );

			core::enable_interrupts();
			__asm__ __volatile__( "hlt" );
			core::disable_interrupts();
			core::current()->lapic->eoi();

			log::printk( "PIC initialized\n" );
		}
	}
}

void
pit::one_shot( uint64_t time, timer_handler fn )
{
	_periodic_handler = nullptr;
	_one_shot_handler = fn;

	if( time > 1_s / 19 )
	{
		time = 1_s / 19;
	}
	uint64_t hz = 1_s / time;
	uint64_t divider = PIT_FREQUENCY / hz;

	io::outb( PIT_MODE_SINGLE, PIT_CMD );
	io::outb( ( divider >> 0 ) & 0xff, PIT_DATA );
	io::outb( ( divider >> 8 ) & 0xff, PIT_DATA );
}

void
pit::periodic( uint64_t time, timer_handler fn )
{
	_periodic_handler = fn;
	_one_shot_handler = nullptr;

	uint64_t hz = 1_s / time;
	uint64_t divider = PIT_FREQUENCY / hz;

	io::outb( PIT_MODE_PERIOD, PIT_CMD );
	io::outb( ( divider >> 0 ) & 0xff, PIT_DATA );
	io::outb( ( divider >> 8 ) & 0xff, PIT_DATA );
}

void
pit::_trigger( void )
{
	if( _one_shot_handler )
	{
		_one_shot_handler();
		_one_shot_handler = nullptr;
	}
	else if( _periodic_handler )
	{
		_periodic_handler();
	}
}

void
pit::_irq_handler( idt::irq_stack_frame_t &frame )
{
	if( frame._context != 0 )
	{
		( ( pit* )frame._context )->_trigger();
	}
}

};
