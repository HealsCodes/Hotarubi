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

#include <hotarubi/processor/ioapic.h>
#include <hotarubi/log/log.h>

namespace processor
{

enum IOAPICRegs
{
	kIOAPICId          = 0x00,
	kIOAPICVersion     = 0x01,
	kIOAPICArbitration = 0x02,
	kIOAPICRedirectLo  = 0x10,
	kIOAPICRedirectHi  = 0x11,
};

enum class IOAPICDelivery : uint16_t
{
	kFixed    = 0x000,
	kLowPrio  = 0x100,
	kSMI      = 0x200,
	kNMI      = 0x400,
	kINIT     = 0x500,
	kExtINT   = 0x700,

	is_bitmask
};

ioapic::~ioapic()
{
	if( _io_mem != nullptr )
	{
		for( auto i = _base; i < _size; ++i )
		{
			/* mask all */
			set_mask( i, true );
		}
		memory::mmio::release_region( &_io_mem );
	}
}

void
ioapic::init( uint64_t address, uint8_t irq_base )
{
	if( _io_mem == nullptr )
	{
		_io_mem = memory::mmio::request_region( "IOAPIC", address, 0x10,
		                                        __MMIO( IOMem ) | __MMIO( Busy ) );
		if( _io_mem != nullptr )
		{
			_io_base = memory::mmio::activate_region( _io_mem );
			_id      = ( _read( kIOAPICId ) >> 24 ) & 0x0f;
			_base    = irq_base;
			_size    = ( _read( kIOAPICVersion ) >> 16 ) & 0xff;
			_version = ( _read( kIOAPICVersion ) >>  0 ) & 0xff;

			for( auto i = 0; i < _size; ++i )
			{
				/* mask all */
				uint32_t val_lo = _read( kIOAPICRedirectLo + i * 2 ) | ( 1 << 16 ),
				         val_hi = _read( kIOAPICRedirectHi + i * 2 );

				_write( kIOAPICRedirectLo + i * 2, val_lo );
				_write( kIOAPICRedirectHi + i * 2, val_hi );
			}
		}
	}
}

void
ioapic::set_route( uint8_t source, uint8_t target,
                   TriggerMode trigger, Polarity polarity )
{
	if( source < start() || source > range() )
	{
		return;
	}

	uint32_t val_lo = 0,
	         val_hi = 0;
//
//	if( ( val_lo & kIOAPICDeliverNMI ) != kIOAPICDeliverNMI )
//	{
//		/* set trigger / polarity for non-NMI IRQs */
		val_lo |= _irq_flags( trigger, polarity );
//	}
//	else
//	{
//		val_lo |= _irq_flags( kIRQTriggerEdge, kIRQPolarityLow );
//	}

	val_lo |= ( 1 << 11 );   /* logical delivery mode */
	val_lo |= ( 1 << 16 );   /* masked by default */
	val_lo |= target;        /* vector: target */
	val_hi |= 0xff000000UL;  /* broadcast to all LAPICs */

	_write( kIOAPICRedirectHi + source * 2, val_hi );
	_write( kIOAPICRedirectLo + source * 2, val_lo );
}

void
ioapic::set_mask( uint8_t source, bool masked )
{
	if( source < start() || source > range() )
	{
		return;
	}

	uint32_t val_lo = _read( kIOAPICRedirectLo + source * 2 ) & ~( 1 << 16 ),
	         val_hi = _read( kIOAPICRedirectHi + source * 2 );

//	if( ( val_lo & kIOAPICDeliverNMI ) != kIOAPICDeliverNMI )
//	{
//		/* no reason in trying to mask an NMI.. */
//		val_lo &= ~0x0000e700;
//		val_lo |= _irq_flags( kIRQTriggerConform, kIRQPolarityConform ) | ( masked << 16 );
//	}
	val_lo |= ( masked << 16 );
	_write( kIOAPICRedirectLo + source * 2, val_lo );
	_write( kIOAPICRedirectHi + source * 2, val_hi );
}

void
ioapic::set_nmi( uint8_t source, TriggerMode trigger, Polarity polarity )
{
	if( source < start() || source > range() )
	{
		return;
	}

	uint32_t val_lo = _read( kIOAPICRedirectLo + source * 2 ) & ~0x0000e700,
	         val_hi = _read( kIOAPICRedirectHi + source * 2 );

	val_lo |= _irq_flags( trigger, polarity ) | numeric( IOAPICDelivery::kNMI );
	_write( kIOAPICRedirectLo + source * 2, val_lo );
	_write( kIOAPICRedirectHi + source * 2, val_hi );
}

void
ioapic::clr_nmi( uint8_t source )
{
	if( source < start() || source > range() )
	{
		return;
	}
	uint32_t val_lo = _read( kIOAPICRedirectLo + source * 2 ) & numeric( ~IOAPICDelivery::kNMI ),
	         val_hi = _read( kIOAPICRedirectHi + source * 2 );

	_write( kIOAPICRedirectLo + source * 2, val_lo );
	_write( kIOAPICRedirectHi + source * 2, val_hi );
}

uint32_t
ioapic::_irq_flags( TriggerMode trigger, Polarity polarity )
{
	/* ISA interrupts are by default edge-triggered, active-high */
	uint32_t res = 0;
	switch( trigger )
	{
		case TriggerMode::kConform: /* FALL_THROUGH */
		case TriggerMode::kEdge:
			res &= ~( 1 << 15 );
			break;

		case TriggerMode::kLevel:
			res |= ( 1 << 15 );
			break;

		case TriggerMode::kReserved:
			break;
	}

	switch( polarity )
	{
		case Polarity::kConform: /* FALL_THROUGH */
		case Polarity::kHigh:
			res &= ~( 1 << 13 );
			break;

		case Polarity::kLow:
			res |= ( 1 << 13 );
			break;

		case Polarity::kReserved:
			break;
	}
	return res;
}

uint32_t 
ioapic::_read( uint8_t reg )
{
	auto iosel = ( volatile uint32_t* )_io_base;
	auto iowin = ( volatile uint32_t* )( _io_base + 0x10 );

	*iosel = reg;
	return *iowin;
}

void
ioapic::_write( uint8_t reg, uint32_t value )
{
	auto iosel = ( volatile uint32_t* )_io_base;
	auto iowin = ( volatile uint32_t* )( _io_base + 0x10 );

	*iosel = reg;
	__asm__ __volatile__("": : :"memory");
	*iowin = value;
	__asm__ __volatile__("": : :"memory");
}

};
