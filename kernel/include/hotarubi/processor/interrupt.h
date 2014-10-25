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

#include <hotarubi/types.h>

namespace processor
{
	#define NUM_IRQ_ENTRIES 255

	enum class TriggerMode : uint8_t
	{
		kConform  = 0x00,
		kEdge     = 0x01,
		kReserved = 0x02,
		kLevel    = 0x03,
	};

	enum Polarity : uint8_t
	{
		kConform  = 0x00,
		kHigh     = 0x01,
		kReserved = 0x02,
		kLow      = 0x03,
	};

	struct interrupt
	{
		uint8_t source;
		uint8_t target;
		TriggerMode trigger;
		Polarity    polarity;

		void setup( uint8_t src, uint8_t tgt, 
		            TriggerMode trgr=TriggerMode::kConform,
		            Polarity    pola=Polarity::kConform )
		{
			source   = src;
			target   = tgt;
			trigger  = trgr;
			polarity = pola;
		};

		static const char* str( TriggerMode mode )
		{
			switch( mode )
			{
				case TriggerMode::kConform:  return "CF";
				case TriggerMode::kEdge:     return "ET";
				case TriggerMode::kReserved: return "RESERVED";
				case TriggerMode::kLevel:    return "LT";
			}
			return "??";
		}

		static const char* str( Polarity polarity )
		{
			switch( polarity )
			{
				case Polarity::kConform:  return "CF";
				case Polarity::kLow:      return "LO";
				case Polarity::kReserved: return "RESERVED";
				case Polarity::kHigh:     return "HI";
			}
			return "??";
		}

		bool override( void )
		{
			return ( ( source   != target ) ||
			         ( trigger  != TriggerMode::kConform ) ||
			         ( polarity != Polarity::kConform ) );
		};
	};
};

#endif
