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
#include <hotarubi/processor/local_data.h>

namespace processor
{
	class core_local_data;
	class core : public core_local_data
	{
	public:
		static inline core* current( void )
		{
			core *data = nullptr;
			__asm__ __volatile__( "mov %%gs:0(,1), %0" : "=r"( data ) );
			return data;
		};

		static core* instance( unsigned core );
		static interrupt *irqs( void );
		static interrupt *irqs( unsigned n );

		static bool is_bsp( void );
		static bool route_isa_irq( uint8_t source, uint8_t target );
		static bool set_nmi( uint8_t source, IRQTriggerMode trigger,
		                                     IRQPolarity polarity );

		static pit *timer( void );

		static inline uint64_t read_flags( void )
		{
			uint64_t r;
			__asm__ __volatile__(
				"pushf\n"
				"pop %0\n"
				: "=r"( r )
			);
			return r;
		};

		static inline void write_flags( uint64_t flags )
		{
			__asm__ __volatile__(
				"push %0\n"
				"popf \n"
				:: "r"( flags )
			);
		};

		static inline void disable_interrupts( void )
		{
			__asm__ __volatile__( "cli" );
		};

		static inline void enable_interrupts( void )
		{
			__asm__ __volatile__( "sti" );
		};
	};
};

#endif
