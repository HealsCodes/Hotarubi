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

#ifndef __PROCESSOR_PIT_H
#define __PROCESSOR_PIT_H 1

#include <hotarubi/types.h>

#include <hotarubi/idt.h>
#include <hotarubi/memory.h>

namespace processor
{
	class pit
	{
	public:
		typedef void (*timer_handler)( void );

		void init( void );

		void one_shot( uint64_t time, timer_handler fn );
		void periodic( uint64_t time, timer_handler fn );

	private:
		void _trigger( void );
		static void _irq_handler( idt::irq_stack_frame_t &frame );

		memory::mmio::resource_t _io_ports = nullptr;

		unsigned                 _vector = 0;
		timer_handler            _one_shot_handler = nullptr;
		timer_handler            _periodic_handler = nullptr;
	};
};

#endif
