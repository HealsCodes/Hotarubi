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

/* ACPI Table definitions */

#ifndef __ACPI_TABLES_H
#define __ACPI_TABLES_H 1

#include <string.h>
#include <hotarubi/types.h>

namespace acpi
{
	#define ACPI_RSDP_SIG "RSD PTR "
	#define ACPI_RSDT_SIG "RSDT"
	#define ACPI_XSDT_SIG "XSDT"
	#define ACPI_MADT_SIG "APIC"

	#pragma pack( push, 1 )

	struct rsdp
	{
		const char signature[8];
		uint8_t    checksum;
		const char oem_id[6];
		uint8_t    revision;
		uint32_t   rsdt_ptr;
		uint32_t   length;
		uint64_t   xsdt_ptr;
		uint8_t    ext_checksum;
		uint8_t    _reserved[3];

		bool check( void )
		{
			if( strncmp( signature, ACPI_RSDP_SIG, sizeof( signature ) ) == 0 )
			{
				/* validate base checksum */
				uint8_t sum = 0, *ptr = ( uint8_t* )&signature[0];
				for( size_t i = 0; i < 20; ++i )
				{
					sum += ptr[i];
				}
				if( sum == 0 )
				{
					/* validate ext_checksum */
					sum = 0;
					for( size_t i = 0; i < length; ++i )
					{
						sum += ptr[i];
					}
					return ( sum == 0 );
				}
			}
			return false;
		};
	};

	struct system_descriptor_table
	{
		const char signature[4];
		uint32_t   length;
		uint8_t    revision;
		uint8_t    checksum;
		const char oem_id[6];
		const char oem_table_id[8];
		uint32_t   oem_revison;
		const char creator_id[4];
		uint32_t   creator_revision;

		bool check( const char *target_sig )
		{
			if( strncmp( signature, target_sig, sizeof( signature ) ) == 0 )
			{
				/* validate checksum */
				uint8_t sum = 0, *ptr = ( uint8_t* )&signature[0];
				for( size_t i = 0; i < length; ++i )
				{
					sum += ptr[i];
				}
				return ( sum == 0 );
			}
			return false;
		};
	};

	struct rsdt : public system_descriptor_table
	{
		uint32_t entries[1];
	};

	struct xsdt : public system_descriptor_table
	{
		uint64_t entries[1];
	};

	enum MADTEntryType : uint8_t
	{
		kMADTEntryLAPIC               = 0,
		kMADTEntryIOAPIC              = 1,
		kMADTEntrySourceOverride      = 2,
		kMADTEntryNMISource           = 3,
		kMADTEntryLAPICNMI            = 4,
		kMADTEntryLAPIAddressOverride = 5,
		kMADTEntryLx2APIC             = 9,
		kMADTEntryLx2APICNMI          = 10,
	};

	enum MADTLAPICFlags : uint32_t
	{
		kMADTLAPICFlagEnabled  = ( 1 << 0 ),
		kMADTLAPICFlagsMask    = 0xfffffffe,
	};

	struct madt_mps_init_flags
	{
		uint16_t polarity  : 2;
		uint16_t trigger   : 2;
		uint16_t _reserved : 12;
	};

	struct madt_entry
	{
		MADTEntryType type;
		uint8_t       length;
	};

	struct madt_lapic_entry : public madt_entry
	{
		uint8_t        processor_id;
		uint8_t        apic_id;
		MADTLAPICFlags flags;
	};

	struct madt_ioapic_entry : public madt_entry
	{
		uint8_t  id;
		uint8_t  _reserved;
		uint32_t address;
		uint32_t base_irq;
	};

	struct madt_source_override : public madt_entry
	{
		uint8_t             bus;
		uint8_t             source_irq;
		uint32_t            global_irq;
		madt_mps_init_flags flags;
	};

	struct madt_nmi_source : public madt_entry
	{
		madt_mps_init_flags flags;
		uint32_t            global_irq;
	};

	struct madt_lapic_nmi : public madt_entry
	{
		uint8_t             processor_id;
		madt_mps_init_flags flags;
		uint8_t             lint;
	};

	struct madt_lapic_address_override : public madt_entry
	{
		uint16_t _reserved;
		uint64_t lapic_base;
	};

	struct madt : public system_descriptor_table
	{
		uint32_t          lapic_base;
		uint32_t          flags;
		struct madt_entry entries[1];
	};
};

#endif
