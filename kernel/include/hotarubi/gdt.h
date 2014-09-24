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

/* Global Descriptor Table manipulation */

#ifndef _GDT_H
#define _GDT_H 1

#include <stdint.h>
#include <bitmask.h>

namespace gdt
{

	#define GDT_DESCRIPTOR_COUNT 7

	enum GDTTypeSet
	{
		kGDTTypeNull            = 0,

		kGDTAccessSys           = 0,
		kGDTAccessUsr           = 0x60,
		kGDTPresent             = 0x80,

		kGDTTypeXO              = 0x18,
		kGDTTypeXR              = 0x1a,

		kGDTTypeRO              = 0x10,
		kGDTTypeRW              = 0x12,
		kGDTTypeStackRO         = 0x14,
		kGDTTypeStackRW         = 0x16,

		kGDTTypeTSS             = 0x09,
		kGDTTypeCallGate        = 0x0c
	};
	BITMASK( GDTTypeSet );

	enum GDTSizeFlagsSet
	{
		kGDTFlagsNone          = 0x00,

		kGDTGranularityByte    = 0x00,
		kGDTGranularity4KByte  = 0x08,

		kGDTOpData32Bit        = 0x00,
		kGDTOpData64Bit        = 0x04,
		kGDTOpSize32Bit        = 0x00,
		kGDTOpSize64Bit        = 0x02,
	};
	BITMASK( GDTSizeFlagsSet );

	#pragma pack( push, 1 )

	/* basic descriptor used in 32- and 64bit modes */
	struct gdt_descriptor
	{
		uint16_t limit_lo;
		uint16_t base_lo;
		uint8_t  base_mi;
		uint8_t  type;
		uint8_t  limit_hi : 4;
		uint8_t  granular : 4;
		uint8_t  base_hi;
	};

	/* extended descriptor used for 64bit TSS */
	struct gdt_descriptor_extended
	{
		uint16_t limit_lo;
		uint16_t base_lo;
		uint8_t  base_mi;
		uint8_t  type;
		uint8_t  limit_hi : 4;
		uint8_t  granular : 4;
		uint8_t  base_hi;
		uint32_t base_xt;
		uint32_t reserved_0;
	};

	struct gdt_pointer
	{
		uint16_t limit;
		uint64_t address;
	};

	#pragma pack( pop )

	void init( void );
};

#endif
