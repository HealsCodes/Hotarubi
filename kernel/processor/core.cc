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
LOCAL_DATA_DEF( uint8_t id_[7] );

static spin_lock processor_accounting_lock;
static unsigned processor_active_count = 0;

/* local static allocation for the bootstrap processor */
struct local_data _bsp_local_data;
uint32_t          core_count     = 0;
uint32_t          ioapic_count   = 0;

/* local dynamic data */
interrupt         *interrupts    = nullptr;
pit               *pit_timer     = nullptr;
/* these will be initialized by ACPI */
struct local_data *ap_local_data = nullptr;
ioapic            *ioapics       = nullptr;


static inline void
_cpuid( uint32_t eax, uint32_t ecx, uint32_t res[] )
{
	__asm__ __volatile__ (
		"cpuid"
		: "=a"( res[0] ), "=b"( res[1] ), "=c"( res[2] ), "=d"( res[3] )
		: "a"( eax ), "c"( ecx ) 
	);
}

struct local_data*
core_data( unsigned core )
{
	if( core == 0 )
	{
		return &_bsp_local_data;
	}
	else
	{
		return ( core < core_count ) ? &ap_local_data[core - 1] : nullptr;
	}
}

bool
is_bsp( void )
{
	return local_data()->id == 0;
}

struct interrupt*
irqs( void )
{
	if( interrupts == nullptr )
	{
		interrupts = new struct interrupt[NUM_IRQ_ENTRIES];
		for( unsigned i = 0; i < NUM_IRQ_ENTRIES; ++i )
		{
			interrupts[i].setup( i, i );
		}
	}
	return interrupts;
}

struct interrupt*
irqs( unsigned n )
{
	return ( n < NUM_IRQ_ENTRIES ) ? &irqs()[n] : nullptr;
}

bool
route_isa_irq( uint8_t source, uint8_t target )
{
	auto irq = irqs( source );

	for( unsigned i = 0; i < ioapic_count; ++i )
	{
		if( ioapics[i].start() <= irq->target &&
		    ioapics[i].range() >= irq->target )
		{
			ioapics[i].set_route( irq->target, target, irq->trigger, irq->polarity );
			ioapics[i].set_mask( irq->target, false );
			return true;
		}
	}
	log::printk( "No IOAPIC to route ISA interrupt %u!\n", source );
	return false;
}

bool
set_nmi( uint8_t source, IRQTriggerMode trigger, IRQPolarity polarity )
{
	if( source < NUM_IRQ_ENTRIES )
	{
		for( uint32_t i = 0; i < ioapic_count; ++i )
		{
			if( interrupts[source].target >= ioapics[i].start() &&
			    interrupts[source].target <= ioapics[i].range() )
			{
				interrupts[source].trigger  = trigger;
				interrupts[source].polarity = polarity;

				ioapics[i].set_nmi( interrupts[source].target,
				                    interrupts[source].trigger,
				                    interrupts[source].polarity );
				return true;
			}
		}
	}
	return false;
}

pit*
timer( void )
{
	return pit_timer;
}

void
init( void )
{
	struct local_data *local = nullptr;

	processor_accounting_lock.lock();

	if( processor_active_count == 0 )
	{
		/* BSP */
		local = &_bsp_local_data;
		memset( local, 0, sizeof( struct local_data ) );
	}
	else
	{
		/* AP - allocate from cache */
		local = &ap_local_data[processor_active_count];
	}

	local->_gs_self = ( uintptr_t )local;
	local->id       = processor_active_count++;

	processor_accounting_lock.unlock();

	regs::write_msr( IA32_GS_BASE      , ( uintptr_t )local );
	regs::write_msr( IA32_KERNEL_GSBASE, ( uintptr_t )local );

	tss::init();
	gdt::init();
	idt::init();

	if( is_bsp() )
	{
		acpi::init_tables();
		acpi::parse_madt( ap_local_data, core_count, ioapics, ioapic_count );

		if( local_data()->lapic == nullptr )
		{
			log::printk( "No LAPIC available and this is the bootstrap processor!\n" );
			do{}while( 1 );
		}

		local_data()->lapic->init();
		pit_timer = new pit;
		pit_timer->init();
	}

	enable_interrupts();

	local_data()->lapic->init(); /* no-op if called twice from BSP */
	local_data()->lapic->calibrate();
}

};
