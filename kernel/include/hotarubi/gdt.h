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

#include <bitmask.h>
#include <hotarubi/types.h>

namespace gdt
{

	#define GDT_DESCRIPTOR_COUNT 7

	enum class Type
	{
		kNull         = 0x00,

		kAccessSys    = 0x00,
		kAccessUsr    = 0x60,
		kPresent      = 0x80,

		kTypeXO       = 0x18,
		kTypeXR       = 0x1a,

		kTypeRO       = 0x10,
		kTypeRW       = 0x12,
		kTypeStackRO  = 0x14,
		kTypeStackRW  = 0x16,

		kTypeTSS      = 0x09,
		kTypeCallGate = 0x0c,

		is_bitmask
	};

	enum class Flag
	{
		kNone               = 0x00,

		kGranularityByte    = 0x00,
		kGranularity4KByte  = 0x08,

		kOpData32Bit        = 0x00,
		kOpData64Bit        = 0x04,
		kOpSize32Bit        = 0x00,
		kOpSize64Bit        = 0x02,

		is_bitmask
	};

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

	#pragma pack( push, 1 )
	struct gdt_pointer
	{
		uint16_t limit;
		uint64_t address;
	};
	#pragma pack( pop )

	void init( void );
};

#endif
