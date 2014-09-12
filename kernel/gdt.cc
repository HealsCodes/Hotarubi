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

#include <string.h>
#include <hotarubi/gdt.h>
#include <hotarubi/processor.h>

namespace gdt
{
static void
_setup_descriptor( struct gdt_descriptor *gdt,
                   unsigned index, uintptr_t base, uint32_t limit,
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
	struct gdt_descriptor *gdt = processor::local_data()->gdt;
	struct gdt_pointer   *gdtr = &processor::local_data()->gdtr;

	memset( gdt, 0, sizeof( struct gdt_descriptor ) * GDT_DESCRIPTOR_COUNT );

	GDTSizeFlagsSet flags = kGDTOpSize64Bit | kGDTOpData32Bit | kGDTGranularity4KByte;

	_setup_descriptor( gdt, 0, 0, 0, kGDTTypeNull, kGDTFlagsNone );
	_setup_descriptor( gdt, 1, 0, 0xffffffff, kGDTTypeXR | kGDTPresent, flags );
	_setup_descriptor( gdt, 2, 0, 0xffffffff, kGDTTypeRW | kGDTPresent, flags );
	_setup_descriptor( gdt, 3, 0, 0xffffffff, kGDTTypeXR | kGDTAccessUsr | kGDTPresent, flags );
	_setup_descriptor( gdt, 4, 0, 0xffffffff, kGDTTypeRW | kGDTAccessUsr | kGDTPresent, flags );

	memset( gdtr, 0, sizeof( struct gdt_pointer ) );

	gdtr->limit   = sizeof( struct gdt_descriptor ) * GDT_DESCRIPTOR_COUNT - 1;
	gdtr->address = ( uintptr_t )gdt;

	/* reload the GDT */
	__asm__ __volatile__( "lgdt (%0)" :: "r"( gdtr ) );
	
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
