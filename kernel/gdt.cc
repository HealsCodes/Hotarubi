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

#include <stdint.h>
#include <string.h>
#include <bitmask.h>

namespace GDT
{

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
BITMASK( GDTSizeFlagsSet )

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

static struct gdt_descriptor [[aligned(8)]] gdt[5];
static struct gdt_pointer [[aligned(4)]] gdtr;

static void
_setup_descriptor( unsigned index, uintptr_t base, uint32_t limit,
                   GDTTypeSet type, GDTSizeFlagsSet flags )
{
	/* FIXME: check index for gdt bounds */
	struct gdt_descriptor *descr = &gdt[index];

	memset( descr, 0, sizeof( struct gdt_descriptor ) );

	descr->limit_lo = ( limit >>  0 ) & 0xffff;
	descr->limit_hi = ( limit >> 16 ) & 0x0f;

	descr->base_lo  = ( base >>  0 ) & 0xffff;
	descr->base_mi  = ( base >> 16 ) & 0xff;
	descr->base_hi  = ( base >> 20 ) & 0xff;

	descr->type     = ( uint8_t )type;
	descr->granular = ( uint8_t )flags;
}

void
init( void )
{
	memset( gdt, 0, sizeof( gdt ) );

	GDTSizeFlagsSet flags = kGDTOpSize64Bit | kGDTOpData32Bit | kGDTGranularity4KByte;

	_setup_descriptor( 0, 0, 0, kGDTTypeNull, kGDTFlagsNone );
	_setup_descriptor( 1, 0, 0xffffffff, kGDTTypeXR | kGDTPresent, flags );
	_setup_descriptor( 2, 0, 0xffffffff, kGDTTypeRW | kGDTPresent, flags );
	_setup_descriptor( 3, 0, 0xffffffff, kGDTTypeXR | kGDTAccessUsr | kGDTPresent, flags );
	_setup_descriptor( 4, 0, 0xffffffff, kGDTTypeRW | kGDTAccessUsr | kGDTPresent, flags );

	memset( &gdtr, 0, sizeof( gdtr ) );

	gdtr.limit   = sizeof( gdt ) - 1;
	gdtr.address = ( uintptr_t )gdt;

	/* reload the GDT */
	__asm__ __volatile__( "lgdt (%0)" :: "r"( &gdtr ) );
	
	/* update data segments */
	__asm__ __volatile__( 
		"mov %0, %%ds\n"
		"mov %0, %%es\n"
		"mov %0, %%fs\n"
		"mov %0, %%gs\n"
		"mov %0, %%ss\n"
		:: "r"( 0x10 ) );

	/* jump over to the new code segment */
	__asm__ __volatile__(
		"push %0\n"
		"push $1f\n"
		"retfq\n"
		"1:\n"
		:: "I"( 0x08 )
	);
}

};
