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

/* management data for interrupt sources, targets, remaps etc. */

#ifndef __PROCESSOR_INTERRUPT_H
#define __PROCESSOR_INTERRUPT_H 1

#include <stdint.h>

namespace processor
{
	#define NUM_IRQ_ENTRIES 255

	enum IRQTriggerMode : uint8_t
	{
		kIRQTriggerConform  = 0x00,
		kIRQTriggerEdge     = 0x01,
		kIRQTriggerReserved = 0x02,
		kIRQTriggerLevel    = 0x03,
	};

	enum IRQPolarity : uint8_t
	{
		kIRQPolarityConform  = 0x00,
		kIRQPolarityHigh     = 0x01,
		kIRQPolarityReserved = 0x02,
		kIRQPolarityLow      = 0x03,
	};

	struct interrupt
	{
		uint8_t source;
		uint8_t target;
		IRQTriggerMode trigger;
		IRQPolarity    polarity;

		void setup( uint8_t src, uint8_t tgt, 
		            IRQTriggerMode trgr=kIRQTriggerConform,
		            IRQPolarity    pola=kIRQPolarityConform )
		{
			source   = src;
			target   = tgt;
			trigger  = trgr;
			polarity = pola;
		};

		static const char* str( IRQTriggerMode mode )
		{
			switch( mode )
			{
				case kIRQTriggerConform:  return "CF";
				case kIRQTriggerEdge:     return "ET";
				case kIRQTriggerReserved: return "RESERVED";
				case kIRQTriggerLevel:    return "LT";
			}
			return "??";
		}

		static const char* str( IRQPolarity polarity )
		{
			switch( polarity )
			{
				case kIRQPolarityConform:  return "CF";
				case kIRQPolarityLow:      return "LO";
				case kIRQPolarityReserved: return "RESERVED";
				case kIRQPolarityHigh:     return "HI";
			}
			return "??";
		}

		bool override( void )
		{
			return ( ( source   != target ) ||
			         ( trigger  != kIRQTriggerConform ) ||
			         ( polarity != kIRQPolarityConform ) );
		};
	};
};

#endif
