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
#include <hotarubi/tss.h>
#include <hotarubi/processor.h>
#include <hotarubi/macros.h>

LOCAL_DATA_INC( hotarubi/gdt.h );
LOCAL_DATA_DEF( struct gdt::gdt_descriptor gdt[GDT_DESCRIPTOR_COUNT] );

#define IA32_GS_BASE 0xc0000101

namespace gdt
{

static struct gdt_pointer _gdtr;

static void
_setup_descriptor( struct gdt_descriptor *gdt,
                   unsigned index, uintptr_t base, uint32_t limit,
                   GDTTypeSet type, GDTSizeFlagsSet flags )
{
	/* FIXME: check index for gdt bounds */
	auto descr = &gdt[index];

	memset( descr, 0, sizeof( struct gdt_descriptor ) );

	descr->limit_lo = ( limit >>  0 ) & 0xffff;
	descr->limit_hi = ( limit >> 16 ) & 0x0f;

	descr->base_lo  = ( base >>  0 ) & 0xffff;
	descr->base_mi  = ( base >> 16 ) & 0xff;
	descr->base_hi  = ( base >> 20 ) & 0xff;

	descr->type     = ( uint8_t )type;
	descr->granular = ( uint8_t )flags;
}

static void
_setup_extended_descriptor( struct gdt_descriptor *gdt,
                            unsigned index, uintptr_t base, uint32_t limit,
                            GDTTypeSet type, GDTSizeFlagsSet flags )
{
	/* FIXME: check index for gdt bounds */
	auto descr = ( struct gdt_descriptor_extended* )&gdt[index];

	memset( descr, 0, sizeof( struct gdt_descriptor_extended ) );

	descr->limit_lo = ( limit >>  0 ) & 0xffff;
	descr->limit_hi = ( limit >> 16 ) & 0x0f;

	descr->base_lo  = ( base >>  0 ) & 0xffff;
	descr->base_mi  = ( base >> 16 ) & 0xff;
	descr->base_hi  = ( base >> 24 ) & 0xff;
	descr->base_xt  = ( base >> 32 ) & 0xffffffff;

	descr->type     = ( uint8_t )type;
	descr->granular = ( uint8_t )flags;
}


static void
_setup_tss_descriptor( struct gdt_descriptor *gdt, unsigned index,
                       struct tss::tss *tss )
{
	auto base  = ( uintptr_t )tss;
	auto limit = sizeof( struct tss::tss ) - 1;

	_setup_extended_descriptor( gdt, index, base, limit, 
	                            kGDTTypeTSS | kGDTPresent | kGDTAccessUsr,
	                            kGDTFlagsNone );
}

void
init( void )
{
	auto core = processor::core::current();
	auto gdt = core->gdt;

	memset( gdt, 0, sizeof( struct gdt_descriptor ) * GDT_DESCRIPTOR_COUNT );

	GDTSizeFlagsSet flags = kGDTOpSize64Bit | kGDTOpData32Bit | kGDTGranularity4KByte;

	_setup_descriptor( gdt, 0, 0, 0, kGDTTypeNull, kGDTFlagsNone );
	_setup_descriptor( gdt, 1, 0, 0xffffffff, kGDTTypeXR | kGDTPresent, flags );
	_setup_descriptor( gdt, 2, 0, 0xffffffff, kGDTTypeRW | kGDTPresent, flags );
	_setup_descriptor( gdt, 3, 0, 0xffffffff, kGDTTypeXR | kGDTAccessUsr | kGDTPresent, flags );
	_setup_descriptor( gdt, 4, 0, 0xffffffff, kGDTTypeRW | kGDTAccessUsr | kGDTPresent, flags );

	_setup_tss_descriptor( gdt, 5, core->tss );

	memset( &_gdtr, 0, sizeof( struct gdt_pointer ) );

	_gdtr.limit   = sizeof( struct gdt_descriptor ) * GDT_DESCRIPTOR_COUNT - 1;
	_gdtr.address = ( uintptr_t )gdt;

	/* reload the GDT */
	__asm__ __volatile__( "lgdt (%0)" :: "r"( &_gdtr ) );
	
	/* update data segments (but keep GS.base) */
	__asm__ __volatile__(
		"mov %0, %%ds\n"
		"mov %0, %%es\n"
		"mov %0, %%fs\n"
		"mov %0, %%gs\n"
		"mov %0, %%ss\n"
		:: "r"( 0x10 ) );

	processor::regs::write_msr( IA32_GS_BASE, ( uintptr_t )core );

	/* jump over to the new code segment */
	__asm__ __volatile__(
		"push %0\n"
		"push $1f\n"
		"retfq\n"
		"1:\n"
		:: "I"( 0x08 )
	);

	/* load the TSS descriptor */
	__asm__ __volatile__( "ltr %%ax" :: "a"( 0x28 | 3 ) );
}

};
