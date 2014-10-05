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

/* methods dealing directly with the CPU or per-CPU local data */

#ifndef __PROCESSOR_CORE_H
#define __PROCESSOR_CORE_H 1

#include <hotarubi/types.h>

#include <hotarubi/processor/pit.h>
#include <hotarubi/processor/interrupt.h>

namespace processor
{
	inline uint64_t read_flags( void )
	{
		uint64_t r;
		__asm__ __volatile__( 
			"pushf\n"
			"pop %0\n"
			: "=r"( r )
		);
		return r;
	};

	inline void write_flags( uint64_t flags )
	{
		__asm__ __volatile__(
			"push %0\n"
			"popf \n"
			:: "r"( flags )
		);
	};

	inline void disable_interrupts( void )
	{
		__asm__ __volatile__( "cli" );
	};

	inline void enable_interrupts( void )
	{
		__asm__ __volatile__( "sti" );
	};

	inline struct local_data* local_data( void )
	{
		struct local_data *data = nullptr;
		__asm__ __volatile__( "mov %%gs:0(,1), %0" : "=r"( data ) );
		return data;
	}

	struct local_data* core_data( unsigned core );

	bool is_bsp( void );

	struct interrupt *irqs( void );
	struct interrupt *irqs( unsigned n );

	bool route_isa_irq( uint8_t source, uint8_t target );
	bool set_nmi( uint8_t source, IRQTriggerMode trigger, IRQPolarity polarity );

	pit *timer( void );
};

#endif
