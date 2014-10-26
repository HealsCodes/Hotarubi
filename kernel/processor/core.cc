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

#include <string.h>

#include <hotarubi/processor/core.h>
#include <hotarubi/processor/regs.h>
#include <hotarubi/processor/pit.h>
#include <hotarubi/processor/ioapic.h>
#include <hotarubi/processor/lapic.h>
#include <hotarubi/processor/interrupt.h>
#include <hotarubi/processor/local_data.h>

#include <hotarubi/acpi/acpi.h>

#include <hotarubi/lock.h>
#include <hotarubi/log/log.h>

#include <hotarubi/gdt.h>
#include <hotarubi/idt.h>
#include <hotarubi/tss.h>

namespace processor
{
#define IA32_GS_BASE       0xc0000101
#define IA32_KERNEL_GSBASE 0xc0000102

LOCAL_DATA_DEF( uint8_t id );

static spin_lock _processor_accounting_lock;
static unsigned  _processor_active_count = 0;

/* local static allocation for the bootstrap processor */
core               _bsp;
/* these will be initialized by ACPI */
core              *_aps          = nullptr;
ioapic            *_ioapics      = nullptr;
uint32_t           _core_count   = 0;
uint32_t           _ioapic_count = 0;
/* local dynamic data */
interrupt         *_interrupts   = nullptr;
pit               *_pit          = nullptr;

static inline void
_cpuid( uint32_t eax, uint32_t ecx, uint32_t res[] )
{
	__asm__ __volatile__ (
		"cpuid"
		: "=a"( res[0] ), "=b"( res[1] ), "=c"( res[2] ), "=d"( res[3] )
		: "a"( eax ), "c"( ecx ) 
	);
}

core*
core::instance( unsigned core )
{
	if( core == 0 )
	{
		return &_bsp;
	}
	else
	{
		return ( core < _core_count ) ? &_aps[core - 1] : nullptr;
	}
}

bool
core::is_bsp( void )
{
	return ( current() == nullptr || current()->id == 0 );
}

struct interrupt*
core::irqs( void )
{
	if( _interrupts == nullptr )
	{
		_interrupts = new struct interrupt[NUM_IRQ_ENTRIES];
		for( unsigned i = 0; i < NUM_IRQ_ENTRIES; ++i )
		{
			_interrupts[i].setup( i, i );
		}
	}
	return _interrupts;
}

struct interrupt*
core::irqs( unsigned n )
{
	return ( n < NUM_IRQ_ENTRIES ) ? &irqs()[n] : nullptr;
}

bool
core::route_isa_irq( uint8_t source, uint8_t target )
{
	auto irq = irqs( source );

	if( irq != nullptr )
	{
		for( unsigned i = 0; i < _ioapic_count; ++i )
		{
			if( _ioapics[i].start() <= irq->target &&
			    _ioapics[i].range() >= irq->target )
			{
				_ioapics[i].set_route( irq->target, target,
				                       irq->trigger, irq->polarity );
				_ioapics[i].set_mask( irq->target, false );
				return true;
			}
		}
	}
	log::printk( "No IOAPIC to route ISA interrupt %u!\n", source );
	return false;
}

bool
core::set_nmi( uint8_t source, TriggerMode trigger, Polarity polarity )
{
	auto irq = irqs( source );

	if( irq != nullptr )
	{
		for( uint32_t i = 0; i < _ioapic_count; ++i )
		{
			if( _interrupts[source].target >= _ioapics[i].start() &&
			    _interrupts[source].target <= _ioapics[i].range() )
			{
				_interrupts[source].trigger  = trigger;
				_interrupts[source].polarity = polarity;

				_ioapics[i].set_nmi( _interrupts[source].target,
				                     _interrupts[source].trigger,
				                     _interrupts[source].polarity );
				return true;
			}
		}
	}
	log::printk( "No IOAPIC to handle NMI interrupt %u!\n", source );
	return false;
}

pit*
core::timer( void )
{
	return _pit;
}

void
init( void )
{
	core *local = nullptr;

	_processor_accounting_lock.lock();

	if( _processor_active_count == 0 )
	{
		/* BSP */
		local = &_bsp;
		/* FIXME: should be done in the constructor..
		 * memset( local, 0, sizeof( core_local_data ) ); */
	}
	else
	{
		/* AP - allocate from cache */
		local = &_aps[_processor_active_count];
	}

	local->_gs_self = ( uintptr_t )local;
	local->id       = _processor_active_count++;

	_processor_accounting_lock.unlock();

	regs::write_msr( IA32_GS_BASE      , ( uintptr_t )local );
	regs::write_msr( IA32_KERNEL_GSBASE, ( uintptr_t )local );

	tss::init();
	gdt::init();
	idt::init();

	if( core::is_bsp() )
	{
		acpi::init_tables();
		acpi::parse_madt( _aps, _core_count, _ioapics, _ioapic_count );

		if( core::current()->lapic == nullptr )
		{
			panic( "No LAPIC available and this is the bootstrap processor!" );
		}

		core::current()->lapic->init();
		_pit = new pit;
		_pit->init();
	}

	core::enable_interrupts();

	core::current()->lapic->init(); /* no-op if called twice from BSP */
	core::current()->lapic->calibrate();
}

};
