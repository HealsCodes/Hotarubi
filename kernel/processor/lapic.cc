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

/* LAPIC handling */

#include <hotarubi/processor/lapic.h>

#include <hotarubi/macros.h>
#include <hotarubi/log/log.h>
#include <hotarubi/idt.h>
#include <hotarubi/io.h>
#include <hotarubi/processor/core.h>
#include <hotarubi/processor/regs.h>
#include <hotarubi/memory/page.h>

LOCAL_DATA_INC( hotarubi/processor/lapic.h );
LOCAL_DATA_DEF( processor::lapic *lapic );

namespace processor
{

#define IA32_APIC_BASE_MSR  0x0000001b

enum LAPICRegs : uint16_t
{
	kLAPICId                 = 0x020,
	kLAPICVersion            = 0x030,
	kLAPICTaskPrio           = 0x080,
	kLAPICArbitrationPrio    = 0x090,
	kLAPICProcessorPrio      = 0x0a0,
	kLAPICEOI                = 0x0b0,
	kLAPICRemoteRead         = 0x0c0,
	kLAPICLogicalDestination = 0x0d0,
	kLAPICDestinationFormat  = 0x0e0,
	kLAPICSpuriousInterrupt  = 0x0f0,
	kLAPICErrorStatus        = 0x280,
	kLAPICLvtCMCI            = 0x2f0,
	kLAPICInterruptCmdLo     = 0x300,
	kLAPICInterruptCmdHi     = 0x310,
	kLAPICLvtTimer           = 0x320,
	kLAPICLvtThermal         = 0x330,
	kLAPICLvtPerfMon         = 0x340,
	kLAPICLvtLINT0           = 0x350,
	kLAPICLvtLINT1           = 0x360,
	kLAPICLvtError           = 0x370,
	kLAPICTimerInitialCount  = 0x380,
	kLAPICTimerCurrentCount  = 0x390,
	kLAPICTimerDivider       = 0x3e0,
};

enum class LAPICDelivery : uint16_t
{
	kFixed  = 0x000,
	kSMI    = 0x200,
	kNMI    = 0x400,
	kExtINT = 0x700,
	kINIT   = 0x500,
	kSIPI   = 0x600,

	is_bitmask = 0 // to avoid "not handled in case.."
};

static volatile bool _calibrated = false;

lapic::lapic( uint8_t id, uint32_t address )
: _init_id{id}, _init_addr{address}
{
}

lapic::~lapic()
{
	if( _io_mem != nullptr )
	{
		set_mask( LAPICInterrupt::kLINT0  , true );
		set_mask( LAPICInterrupt::kLINT1  , true );
		set_mask( LAPICInterrupt::kTimer  , true );
		set_mask( LAPICInterrupt::kPerfMon, true );
		set_mask( LAPICInterrupt::kThermal, true );
		set_mask( LAPICInterrupt::kError  , true );

		memory::mmio::release_region( &_io_mem );
	}
}

void
lapic::init( void )
{
	_init_addr = ( _init_addr == 0 ) ? ( regs::read_msr( IA32_APIC_BASE_MSR ) & ~0xfff )
	                                 : _init_addr;
	if( _io_mem == nullptr )
	{
		_io_mem = memory::mmio::request_region( "LAPIC", _init_addr, PAGE_SIZE,
		                                        __MMIO( IOMem )  |
		                                        __MMIO( Shared ) |
		                                        __MMIO( Busy ) );
		if( _io_mem != nullptr )
		{
			_io_base = memory::mmio::activate_region( _io_mem );

			_id      = _read( kLAPICId ) >> 24;
			_version = _read( kLAPICVersion ) & 0xff;

			if( _id != _init_id )
			{
				log::printk( "lapic: local ID %02x doesn't match expected %02x\n",
				             _id, _init_id );
			}

			accept_broadcast( true );
			{
				LAPICInterrupt vectors[]{
					LAPICInterrupt::kLINT0,
					LAPICInterrupt::kLINT1,
					LAPICInterrupt::kTimer,
					LAPICInterrupt::kThermal,
					LAPICInterrupt::kPerfMon,
					LAPICInterrupt::kError,
					LAPICInterrupt( 0 )
				};

				/* map LAPIC IRQs to 50+ */
				for( auto v = 0; vectors[v] != LAPICInterrupt( 0 ); ++v )
				{
					set_mask( vectors[v], true );
				}

				/* software enable the LAPIC */
				unsigned vector = 0;
				if( idt::register_irq_handler( vector, [](idt::irq_stack_frame_t&){} ) )
				{
					_write( kLAPICSpuriousInterrupt, vector | 0x100 );
				}

			}

			if( _init_nmi != 0xff )
			{
				_init_done = true;
				set_nmi( _init_nmi, _init_nmi_trigger, _init_nmi_polarity );
			}
		}
		eoi();
	}
}

void
lapic::calibrate( void )
{
	/* reset ticks/msec, set the counter to max and a divider of 1 */
	_calibrated = false;
	_ticks_per_msec = 1;
	_write( kLAPICTimerDivider, ( _read( kLAPICTimerDivider ) & 0xfffffff0 ) | 0x0b );
	_write( kLAPICTimerInitialCount, ( uint32_t )~0 );

	core::timer()->one_shot( 1_ms, [](){ _calibrated = true; } );

	/* poll until the PIT has fired */
	while( _calibrated == false )
	{
		__asm__ __volatile__( "hlt" );
	}

	auto current = _read( kLAPICTimerCurrentCount );
	_write( kLAPICTimerInitialCount, 0 );

	_ticks_per_msec = ( 0xffffffff - current + 1 );
	_ticks_per_msec = ( _ticks_per_msec > 0 ) ? _ticks_per_msec : 1;

	set_mask( LAPICInterrupt::kTimer, false );
	log::printk( "LAPIC %d: %u ticks/ms\n", _id, _ticks_per_msec );
}

void
lapic::set_route( LAPICInterrupt source, uint8_t target,
                  TriggerMode trigger, Polarity polarity )
{
	uint16_t reg = kLAPICLvtLINT0;
	switch( source )
	{
		case LAPICInterrupt::kLINT0:
			reg = kLAPICLvtLINT0;
			break;

		case LAPICInterrupt::kLINT1:
			reg = kLAPICLvtLINT1;
			break;

		case LAPICInterrupt::kTimer:
			reg = kLAPICLvtTimer;
			break;

		case LAPICInterrupt::kPerfMon:
			reg = kLAPICLvtPerfMon;
			break;

		case LAPICInterrupt::kThermal:
			reg = kLAPICLvtThermal;
			break;

		case LAPICInterrupt::kError:
			reg = kLAPICLvtError;
			break;
	}

	uint32_t val = _read( reg ) & 0xffffff00;
	_write( reg, val | _irq_flags( trigger, polarity ) | target );
}

void
lapic::set_timer( unsigned msec, bool repeat )
{
	uint32_t lvt = ( _read( kLAPICLvtTimer ) & ~( 1 << 16 | 1 << 17 ) ) | ( repeat << 17 );
	_write( kLAPICTimerInitialCount, ( msec / 1_ms ) * _ticks_per_msec );
	_write( kLAPICLvtTimer,  lvt );
}

void
lapic::clr_timer( void )
{
	set_mask( LAPICInterrupt::kTimer, true );
	_write( kLAPICTimerInitialCount, 0 );
}

void
lapic::set_nmi( unsigned lint_no, TriggerMode trigger, Polarity polarity )
{
	if( _init_done == false )
	{
		/* there is a good chance this will be called from ACPI code
		 * and at that point *this is not the correct LAPIC* */
		_init_nmi = lint_no;
		_init_nmi_trigger  = trigger;
		_init_nmi_polarity = polarity;
		return;
	}

	uint16_t reg = numeric( ( lint_no == 0 ) ? LAPICInterrupt::kLINT0 
	                                         : LAPICInterrupt::kLINT1 );
	uint32_t val = _read( reg ) & ~0x00000070; /* mask out the deliver mode */

	val |= _irq_flags( trigger, polarity ) | numeric( LAPICDelivery::kNMI );
	_write( reg, val );
}

void
lapic::clr_nmi( unsigned lint_no )
{
	uint16_t reg = numeric( ( lint_no == 0 ) ? LAPICInterrupt::kLINT0
	                                         : LAPICInterrupt::kLINT1 );
	_write( reg, _read( reg ) & numeric( ~LAPICDelivery::kNMI ) );
}

void
lapic::set_mask( LAPICInterrupt source, bool masked )
{
	uint16_t reg = kLAPICLvtLINT0;
	switch( source )
	{
		case LAPICInterrupt::kLINT0:
			reg = kLAPICLvtLINT0;
			break;

		case LAPICInterrupt::kLINT1:
			reg = kLAPICLvtLINT1;
			break;

		case LAPICInterrupt::kTimer:
			reg = kLAPICLvtTimer;
			break;

		case LAPICInterrupt::kPerfMon:
			reg = kLAPICLvtPerfMon;
			break;

		case LAPICInterrupt::kThermal:
			reg = kLAPICLvtThermal;
			break;

		case LAPICInterrupt::kError:
			reg = kLAPICLvtError;
			break;
	}

	if( masked )
	{
		_write( reg, _read( reg ) | ( 1 << 16 ) );
	}
	else
	{
		_write( reg, _read( reg ) & ~( 1 << 16 ) );
	}
}

void
lapic::accept_broadcast( bool accept )
{
	_write( kLAPICLogicalDestination, ( accept ) ? 0xff000000 : ( ( uint32_t )_id << 24 ) );
	_write( kLAPICDestinationFormat, _read( kLAPICDestinationFormat ) & 0xf0000000 );
}

void
lapic::send_ipi( uint8_t target, uint8_t vector )
{
	_send_ipi( target, LAPICBroadcast( 0 ), LAPICDelivery::kFixed, vector );
}

void
lapic::send_sipi( uint8_t target, uint8_t boot_vector )
{
	_send_ipi( target, LAPICBroadcast( 0 ), LAPICDelivery::kSIPI, boot_vector );
}

void
lapic::send_init( uint8_t target )
{
	_send_ipi( target, LAPICBroadcast( 0 ), LAPICDelivery::kINIT, 0 );
}

void
lapic::broadcast_ipi( LAPICBroadcast mode, uint8_t vector )
{
	_send_ipi( 0, mode, LAPICDelivery::kFixed, vector );
}

void
lapic::broadcast_init( LAPICBroadcast mode )
{
	_send_ipi( 0, mode, LAPICDelivery::kINIT, 0 );
}

void
lapic::eoi( void )
{
	_write( kLAPICEOI, 0 );
}

uint32_t
lapic::_irq_flags( TriggerMode trigger, Polarity polarity )
{
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
			res &= ~( 1 << 12 );
			break;

		case Polarity::kLow:
			res |= ( 1 << 12 );
			break;

		case Polarity::kReserved:
			break;
	}
	return res;
}

void
lapic::_send_ipi( uint8_t target, LAPICBroadcast mode, LAPICDelivery delivery,
                  uint8_t vector )
{
	uint32_t ipi_lo = 0, ipi_hi = 0;

	switch( mode )
	{
		case LAPICBroadcast::kAll:
		case LAPICBroadcast::kSelf:
		case LAPICBroadcast::kOthers:
			ipi_lo |= numeric( mode );
		break;

		default:
		ipi_hi = ( ( uint32_t )target << 24 );
		break;
	}

	ipi_lo |= numeric( delivery );

	switch( delivery )
	{
		case LAPICDelivery::kFixed: /* FALL_THROUGH */
		case LAPICDelivery::kSIPI:
			ipi_lo |= vector;
		break;

		case LAPICDelivery::kSMI: /* FALL_THROUGH */
		case LAPICDelivery::kNMI: /* FALL_THROUGH */
		case LAPICDelivery::kINIT:
			/* no vector / vector should be 00 */
		break;

		case LAPICDelivery::kExtINT:
			/* no-op, reserved */
		return;
	}

	ipi_lo |= ( 1 << 14 ); /* assert */
	if( ipi_hi )
	{
		_write( kLAPICInterruptCmdHi, ipi_hi );
	}
	_write( kLAPICInterruptCmdLo, ipi_lo );
}

uint32_t
lapic::_read( uint16_t reg )
{
	auto ioreg = ( volatile uint32_t* )( _io_base + reg );
	return *ioreg;
}

void
lapic::_write( uint16_t reg, uint32_t val )
{
	auto ioreg = ( volatile uint32_t* )( _io_base + reg );
	*ioreg = val;
	__asm__ __volatile__("": : :"memory");
}

};
