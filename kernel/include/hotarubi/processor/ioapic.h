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

/* IOAPIC handling */

#ifndef __PROCESSOR_IOAPIC_H
#define __PROCESSOR_IOAPIC_H 1

#include <hotarubi/memory/mmio.h>
#include <hotarubi/processor/interrupt.h>

namespace processor
{
	class ioapic
	{
	public:
		~ioapic();

		void init( uint64_t address, uint8_t irq_base );
		
		void set_route( uint8_t source, uint8_t target,
		                TriggerMode trigger=TriggerMode::kConform,
		                Polarity polarity=Polarity::kConform );
		void set_mask( uint8_t source, bool masked );

		void set_nmi( uint8_t source, TriggerMode trigger, Polarity polarity );
		void clr_nmi( uint8_t source );

		uint8_t id( void ) const { return _id; };

		uint8_t start( void ) const { return _base; };
		uint8_t range( void ) const { return _base + _size; };
		uint8_t version( void ) const { return _version; };

	private:
		uint32_t _irq_flags( TriggerMode trigger, Polarity polarity );

		uint32_t _read( uint8_t reg );
		void     _write( uint8_t reg, uint32_t value );

		uint8_t                  _id   = 0xff;
		uint8_t                  _base = 0;
		uint8_t                  _size = 0;
		uint8_t                  _version = 0;

		memory::mmio::resource_t _io_mem  = nullptr;
		uintptr_t                _io_base = 0;
	};
};

#endif
