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

/* Interrupt Descriptor Table manpulation */

#ifndef _IDT_H
#define _IDT_H 1

#include <hotarubi/types.h>

namespace idt
{
	#pragma pack( push, 1 )
	struct irq_stack_frame
	{
		uint64_t cr2;
		uint16_t ds;
		uint16_t _padd_0;
		uint32_t _padd_1;
		uint64_t rsi;
		uint64_t rdi;
		uint64_t rax;
		uint64_t rbx;
		uint64_t rcx;
		uint64_t rdx;
		uint64_t r8;
		uint64_t r9;
		uint64_t r10;
		uint64_t r11;
		uint64_t r12;
		uint64_t r13;
		uint64_t r14;
		uint64_t r15;
		uint64_t rbp;
		uint64_t _context;
		uint64_t _irq_nr;
		uint32_t error_code;
		uint32_t _padd_2;
		uint64_t rip;
		uint16_t cs;
		uint16_t _padd_3;
		uint32_t _padd_4;
		uint64_t rflags;
		uint64_t rsp;
		uint16_t ss;
		uint16_t _padd_5;
		uint32_t _padd_6;
	};
	typedef struct irq_stack_frame irq_stack_frame_t;
	#pragma pack( pop )

	typedef void ( *irq_handler_fn )( irq_stack_frame_t &stack_frame );

	bool register_system_handler( unsigned index, irq_handler_fn fn );
	bool register_irq_handler( unsigned &index, irq_handler_fn fn,
	                           uint64_t context=0, bool swapgs_fast=true );
	void release_irq_handler( unsigned index );

	void init( void );
};

#endif
