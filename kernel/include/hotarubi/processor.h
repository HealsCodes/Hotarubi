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

#ifndef __PROCESSOR_H
#define __PROCESSOR_H 1

#include <stdint.h>
#include <hotarubi/gdt.h>

namespace processor
{
	struct local_data
	{
		struct gdt::gdt_pointer    [[aligned(8)]] gdtr;
		struct gdt::gdt_descriptor [[aligned(4)]] gdt[GDT_DESCRIPTOR_COUNT];
	};

	inline uint64_t read_flags( void )
	{
		uint64_t r;
		__asm__ __volatile__( 
			"pushf\n"
			"pop %0\n"
			: "=r"( r )
		);
		return r;
	};

	inline void write_flags( uint64_t flags )
	{
		__asm__ __volatile__(
			"push %0\n"
			"popf \n"
			:: "r"( flags )
		);
	};

	inline void disable_interrupts( void )
	{
		__asm__ __volatile__( "cli" );
	};

	inline void enable_interrupts( void )
	{
		__asm__ __volatile__( "sti" );
	};

	/* The next method abuses the SYSENTER_CS MSR.
	 * In legacy mode it's used to determine the CS selector for syscalls,
	 * however in longmode SYSENTER is illegal so it should be save to use
	 * this MSR as a per-cpu permanent (16bit) store */

	inline uint8_t processor_nr( void )
	{
		uint32_t tmp;
		__asm__ __volatile__( "rdmsr" : "=a"( tmp ) : "c"( 0x174 ) : "%rdx" );
		return tmp & 0xff;
	};

	inline bool is_bsp( void )
	{
		return processor_nr() == 0;
	};

	struct local_data* local_data( void );

	void init( void );

namespace regs
{
	inline uint64_t read_cr0( void )
	{
		uint64_t r;
		__asm__ __volatile__( "movq %%cr0, %0" : "=r"( r ) );
		return r;
	};

	inline uint64_t read_cr1( void )
	{
		uint64_t r;
		__asm__ __volatile__( "movq %%cr1, %0" : "=r"( r ) );
		return r;
	};

	inline uint64_t read_cr2( void )
	{
		uint64_t r;
		__asm__ __volatile__( "movq %%cr2, %0" : "=r"( r ) );
		return r;
	};

	inline uint64_t read_cr3( void )
	{
		uint64_t r;
		__asm__ __volatile__( "movq %%cr3, %0" : "=r"( r ) );
		return r;
	};

	inline uint64_t read_cr4( void )
	{
		uint64_t r;
		__asm__ __volatile__( "movq %%cr4, %0" : "=r"( r ) );
		return r;
	};

	inline void write_cr0( uint64_t val )
	{
		__asm__ __volatile__( "movq %0, %%cr0" :: "r"( val ) );
	};

	inline void write_cr1( uint64_t val )
	{
		__asm__ __volatile__( "movq %0, %%cr1" :: "r"( val ) );
	};

	inline void write_cr2( uint64_t val )
	{
		__asm__ __volatile__( "movq %0, %%cr2" :: "r"( val ) );
	};

	inline void write_cr3( uint64_t val )
	{
		__asm__ __volatile__( "movq %0, %%cr3" :: "r"( val ) );
	};

	inline void write_cr4( uint64_t val )
	{
		__asm__ __volatile__( "movq %0, %%cr4" :: "r"( val ) );
	};

	inline uint64_t read_msr( uint32_t reg )
	{
		uint32_t lo, hi;
		__asm__ __volatile__( "rdmsr" : "=d"( hi ), "=a"( lo ) : "c"( reg ) );

		return ( ( uint64_t ) hi << 32 ) | lo;
	};

	inline void write_msr( uint32_t reg, uint64_t val )
	{
		uint32_t lo = val & 0xffffffff, 
		         hi = val >> 32;

		__asm__ __volatile__( "wrmsr" :: "c"( reg ), "d"( hi ), "a"( lo ) );
	};
};
};

#endif
