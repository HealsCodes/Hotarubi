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

/* ACPI interface */

#include <hotarubi/acpi/acpi.h>
#include <hotarubi/acpi/tables.h>

#include <hotarubi/processor/core.h>
#include <hotarubi/processor/pic.h>
#include <hotarubi/processor/ioapic.h>
#include <hotarubi/processor/interrupt.h>
#include <hotarubi/processor/local_data.h>

#include <hotarubi/io.h>
#include <hotarubi/memory.h>
#include <hotarubi/log/log.h>

namespace acpi
{

/* TODO: collect the size of all tables etc. and reserve an IO-region */
//static memory::mmio::resource_t acpi_mem = nullptr;
static struct rsdt *rsdt = nullptr;
static struct xsdt *xsdt = nullptr;

static system_descriptor_table*
_get_table( const char *signature )
{
	system_descriptor_table *res = nullptr;

	if( rsdt != nullptr )
	{
		size_t max_entry  = rsdt->length - sizeof( struct rsdt ) + sizeof( rsdt->entries[0] );
		       max_entry /= sizeof( rsdt->entries[0] );

		for( size_t i = 0; i < max_entry; ++i )
		{
			res = ( system_descriptor_table* )__VA( rsdt->entries[i] );
			if( res->check( signature ) )
			{
				break;
			}
		} 
	}
	else if( xsdt != nullptr )
	{
		size_t max_entry  = xsdt->length - sizeof( struct xsdt ) + sizeof( xsdt->entries[0] );
		       max_entry /= sizeof( xsdt->entries[0] );

		for( size_t i = 0; i < max_entry; ++i )
		{
			res = ( system_descriptor_table* )__VA( xsdt->entries[i] );
			if( res->check( signature ) )
			{
				break;
			}
		} 
	}
	return res;
}

void
parse_madt( struct processor::local_data *&ap_data, uint32_t &core_count,
            processor::ioapic *&ioapics, uint32_t &ioapic_count  )
{
	auto madt = ( struct madt* )_get_table( ACPI_MADT_SIG );
	if( madt == nullptr )
	{
		log::printk( "acpi: No MADT table available!\n" );
		do {} while( 0 );
	}


	core_count = 0;
	ioapic_count = 0;

	uint32_t lapic_base = 0;
	auto entry = &madt->entries[0];
	/* first run: count the system resources */
	while( ( uintptr_t )entry - ( uintptr_t )madt < madt->length )
	{
		switch( entry->type )
		{
			case kMADTEntryLAPIC:
				++core_count;
				break;

			case kMADTEntryIOAPIC:
				++ioapic_count;
				break;

			case kMADTEntrySourceOverride:
			{
				auto desc     = ( madt_source_override* )entry;
				auto trigger  = processor::IRQTriggerMode( desc->flags.trigger );
				auto polarity = processor::IRQPolarity( desc->flags.polarity );

				processor::irqs()[desc->source_irq].setup( desc->source_irq,
				                                           desc->global_irq, 
				                                           trigger, polarity );
				break;
			}

			case kMADTEntryLAPIAddressOverride:
			{
				auto desc = ( madt_lapic_address_override* )entry;
				lapic_base = desc->lapic_base;
				break;
			}

			default:
				break;
		}
		entry = ( madt_entry* )( ( uintptr_t )entry + entry->length );
	}

	if( core_count > 0 )
	{
		log::printk( "acpi: detected %i processors\n", core_count );
		ap_data = new struct processor::local_data[core_count - 1];
	}

	if( ioapic_count > 0 )
	{
		log::printk( "acpi: detected %i IOAPICs\n", ioapic_count );
		ioapics = new processor::ioapic[ioapic_count];
	}

	/* second run: configure them */
	ioapic_count = 0;
	entry = &madt->entries[0];
	while( ( uintptr_t )entry - ( uintptr_t )madt < madt->length )
	{
		switch( entry->type )
		{
			case kMADTEntryLAPIC:
			{
				auto desc   = ( madt_lapic_entry* )entry;
				auto lapic  = new processor::lapic( desc->apic_id, lapic_base );
				
				processor::core_data( desc->processor_id )->lapic = lapic;
				log::printk( "acpi: detected LAPIC %d for processor %d\n",
				             desc->apic_id, desc->processor_id );
				break;
			}

			case kMADTEntryIOAPIC:
			{
				auto desc   = ( madt_ioapic_entry* )entry;
				auto ioapic = &ioapics[ioapic_count++];

				ioapic->init( desc->address, desc->base_irq );
				log::printk( "acpi: detected IOAPIC %d, version %d handling IRQ %02x => %02x\n",
				             ioapic->id(), ioapic->version(),
				             ioapic->start(), ioapic->range() );
				break;
			}

			default:
				break;
		}
		entry = ( madt_entry* )( ( uintptr_t )entry + entry->length );
	}

	for( auto i = 0; i < NUM_IRQ_ENTRIES; ++i )
	{
		auto irq = processor::irqs()[i];
		if( irq.override() )
		{
			log::printk( "acpi: override IRQ %02x => %02x, %2s, %2s\n",
			             i, irq.target,
			             irq.str( irq.trigger ),
			             irq.str( irq.polarity ) );
		}
	}

	/* third run: collect and apply global and lapic nmi overrides */
	entry = &madt->entries[0];
	while( ( uintptr_t )entry - ( uintptr_t )madt < madt->length )
	{
		switch( entry->type )
		{
			case kMADTEntryNMISource:
			{
				auto desc = ( madt_nmi_source* )entry;
				auto trigger  = processor::IRQTriggerMode( desc->flags.trigger );
				auto polarity = processor::IRQPolarity( desc->flags.polarity );

				log::printk( "acpi: IRQ %02x, global NMI\n", desc->global_irq );
				if( processor::set_nmi( desc->global_irq, trigger, polarity ) == false )
				{
					log::printk( "acpi: no IOAPIC available to handle this NMI!\n" );
				}
				break;
			}

			case kMADTEntryLAPICNMI:
			{
				auto desc = ( madt_lapic_nmi* )entry;
				auto trigger  = processor::IRQTriggerMode( desc->flags.trigger );
				auto polarity = processor::IRQPolarity( desc->flags.polarity );

				log::printk( "acpi: LINT%d, NMI ", desc->lint);
				if( desc->processor_id == 255 )
				{
					log::printk( " on all LAPICs\n" );
					for( uint32_t i = 0; i < core_count; ++i )
					{
						auto lapic = processor::core_data( i )->lapic;

						if( lapic != nullptr )
						{
							lapic->set_nmi( desc->lint, trigger, polarity );
						}
					}
				}
				else
				{
					if( desc->processor_id > core_count )
					{
						log::printk( " -- with unknown procossor %d\n",
						             desc->processor_id );
						break;
					}

					auto lapic = processor::core_data( desc->processor_id )->lapic;
					if( lapic != nullptr )
					{
						log::printk( " on processor %d\n", desc->processor_id );
						lapic->set_nmi( desc->lint, trigger, polarity );
					}
				}
				break;
			}

			default:
				break;
		}
		entry = ( madt_entry* )( ( uintptr_t )entry + entry->length );
	}

	if( madt->flags & 1 )
	{
		log::printk( "acpi: disabling 8259 PICs\n" );
		processor::pic::initialize();
		processor::pic::disable();
	}
}

void
init_tables( void )
{
	/* locate the rsdp in low mem (reading the EBDA location from BDA:0x40e )*/
	uint8_t *mem = ( uint8_t* )__VA( 0x00000000 );

	log::printk( "Parsing ACPI tables..\n" );
	for( size_t i = ( mem[0x040e] >> 4 ); i < 0x100000; i += 2 )
	{
		auto rsd_ptr = ( rsdp* )&mem[i];
		if( rsd_ptr->check() )
		{
			if( rsd_ptr->xsdt_ptr == 0 )
			{
				log::printk( "acpi: no valid XSDT, using RSDT\n" );
				rsdt = ( struct rsdt* )__VA( rsd_ptr->rsdt_ptr );
				if( !rsdt->check( ACPI_RSDT_SIG ) )
				{
					/* FIXME: PANIC */
					log::printk( "acpi: RSDT is INVALID!\n" );
					do {} while( 0 );
				}
				log::printk( "acpi: RSDT version %d (%6.6s)\n",
				             rsdt->revision, rsdt->oem_id );
			}
			else
			{
				xsdt = ( struct xsdt* )__VA( rsd_ptr->xsdt_ptr );
				if( !xsdt->check( ACPI_XSDT_SIG ) )
				{
					/* FIXME: PANIC */
					log::printk( "acpi: XSDT is INVALID!\n" );
					do {} while( 0 );
				}
				log::printk( "acpi: XSDT version %d (%6.6s)\n",
				             xsdt->revision, xsdt->oem_id );
			}
		}
		rsd_ptr = nullptr;
	}
}

};
