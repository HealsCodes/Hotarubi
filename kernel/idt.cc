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

#include <stdint.h>
#include <string.h>
#include <bitmask.h>

#include <hotarubi/idt.h>
#include <hotarubi/lock.h>
#include <hotarubi/log/log.h>
#include <hotarubi/processor/core.h>

#define IDT_DESCRIPTORS 255
#define IDT_RESERVED    32

#define IDT_STUB_NR_MAGIC  0xdeadbeef
#define IDT_STUB_CX_MAGIC  0x0badf00d

#define IDT_STUB_FN_SLOWGS ( (uint32_t)( ( uintptr_t )L_interrupt_swapgs_fast ) & 0xffffffff )
#define IDT_STUB_FN_FASTGS ( (uint32_t)( ( uintptr_t )L_interrupt_swapgs_slow ) & 0xffffffff )

/* the following are declared in interrupts.S */
extern "C" uint8_t _interrupt_stub_0[] , _interrupt_stub_1[];
extern "C" uintptr_t _interrupt_pointer_table[IDT_DESCRIPTORS];

extern "C" uint8_t _interrupt_stub_base[];
extern "C" uint64_t _interrupt_stub_size;
extern "C" uint8_t L_interrupt_swapgs_fast[], L_interrupt_swapgs_slow[];

namespace idt
{
enum IDTTypeSet
{
	kIDTAccessSys           = 0,
	kIDTAccessUsr           = 0x60,
	kIDTPresent             = 0x80,

	kIDTTypeIRQ             = 0x0e,
	kIDTTypeTrap            = 0x0f,
};
BITMASK( IDTTypeSet );

enum IDTStackSet
{
	kIDTStackNone           = 0,

	kIDTStack1              = 1,
	kIDTStack2              = 2,
	kIDTStack3              = 3,
	kIDTStack4              = 4,
	kIDTStack5              = 5,
	kIDTStack6              = 6,
	kIDTStack7              = 7,
};
BITMASK( IDTStackSet );

#pragma pack( push, 1 )

/* extended descriptor used for 64bit TSS */
struct idt_descriptor
{
	uint16_t target_lo;
	uint16_t selector;
	uint8_t  ist;
	uint8_t  type;
	uint16_t target_hi;
	uint32_t target_xt;
	uint32_t _reserved_0;
};

struct idt_pointer
{
	uint16_t limit;
	uint64_t address;
};

#pragma pack( pop )

static struct idt_descriptor idt[IDT_DESCRIPTORS];
static struct idt_pointer    idtr;

/* offsets used when patching dynamic stubs */
static size_t patch_index = 0, patch_ctxt = 0, patch_jump = 0;

static spin_lock patch_handler_lock, patch_system_lock;

static void
_default_irq_stub( irq_stack_frame_t &stack )
{
	log::printk( "\n--- call to default irq stub ---\n"
	             "- source irq: %ld, error code: %d\n"
	             "rip: %016lx\n"
	             "rsp: %016lx rflags: %016lx\n"
	             "rax: %016lx rbx: %016lx rcx: %016lx\n"
	             "rdx: %016lx rsi: %016lx rdi: %016lx\n"
	             "rbp: %016lx r8 : %016lx r9 : %016lx\n"
	             "r10: %016lx r11: %016lx r12: %016lx\n"
	             "r13: %016lx r14: %016lx r15: %016lx\n"
	             "cr2: %016lx\n"
	             "cs: %04x ds: %04x ss: %04x\n\n",
	             stack._irq_nr, stack.error_code,
	             stack.rip, stack.rsp, stack.rflags,
	             stack.rax, stack.rbx, stack.rcx,
	             stack.rdx, stack.rsi, stack.rdi,
	             stack.rbp, stack.r8 , stack.r9 ,
	             stack.r10, stack.r11, stack.r12,
	             stack.r13, stack.r14, stack.r15,
	             stack.cr2, stack.cs, stack.ds, stack.ss );

	do { __asm__ __volatile__( "hlt" ); } while( 1 );
}

static void
_setup_idt_descriptor( struct idt_descriptor *idt, unsigned index,
                       uintptr_t target, uint16_t selector,
                       IDTStackSet stack, IDTTypeSet type )
{
	struct idt_descriptor *descr = &idt[index];

	memset( descr, 0, sizeof( struct idt_descriptor ) );

	descr->target_lo = ( target >>  0 ) & 0xffff;
	descr->target_hi = ( target >> 16 ) & 0xffff;
	descr->target_xt = ( target >> 32 ) & 0xffffffff;
	descr->selector  = selector;
	descr->type      = type;
	descr->ist       = stack;
}

static bool
_detect_patch_offsets( void )
{
	auto stub = ( uint8_t* )( ( uintptr_t )_interrupt_stub_base );
	auto size = _interrupt_stub_size / ( IDT_DESCRIPTORS - IDT_RESERVED );

	for( size_t i = 0; i < size; ++i )
	{
		uint32_t *magic = ( uint32_t* )&stub[i];
		if( *magic == IDT_STUB_NR_MAGIC )
		{
			patch_index = i;
		}
		else if( *magic == IDT_STUB_CX_MAGIC )
		{
			patch_ctxt = i;
		}
		else if( *magic == IDT_STUB_FN_SLOWGS ||
		         *magic == IDT_STUB_FN_FASTGS )
		{
			patch_jump = i;
		}
	}
	return ( patch_index > 0 && patch_ctxt > 0 && patch_jump > 0 );
}

bool
register_system_handler( unsigned index, irq_handler_fn fn )
{
	scoped_lock lock( patch_system_lock );

	if( index <= IDT_RESERVED )
	{
		if( _interrupt_pointer_table[index] == 0 ||
			_interrupt_pointer_table[index] == ( uintptr_t )_default_irq_stub )
		{
			_interrupt_pointer_table[index] = ( uintptr_t )fn;
			return true;
		}
	}
	return false;
}

bool
register_irq_handler( unsigned &index, irq_handler_fn fn,
                      uint64_t ucontext, bool swapgs_fast )
{
	scoped_lock lock( patch_handler_lock);

	auto stub = ( uint8_t* )( ( uintptr_t )_interrupt_stub_base );
	auto size = _interrupt_stub_size / ( IDT_DESCRIPTORS - IDT_RESERVED );

	for( auto i = 1; i < IDT_DESCRIPTORS - IDT_RESERVED; ++i )
	{
		auto nr      = ( uint32_t* )&stub[patch_index];
		auto context = ( uint64_t* )&stub[patch_ctxt];
		auto handler = ( uint32_t* )&stub[patch_jump];

		if( *nr == IDT_STUB_NR_MAGIC )
		{
			*nr = IDT_RESERVED + i;
			*context = ucontext;
			if( swapgs_fast )
			{
				*handler = IDT_STUB_FN_FASTGS;
			}
			else
			{
				*handler = IDT_STUB_FN_SLOWGS;
			}

			_interrupt_pointer_table[IDT_RESERVED + i] = ( uintptr_t )fn;
			_setup_idt_descriptor( idt, IDT_RESERVED + i, 
			                       ( uintptr_t )stub,
			                       0x08, 
			                       kIDTStackNone,
			                       kIDTTypeIRQ | kIDTPresent | kIDTAccessSys );

			index = IDT_RESERVED + i;
			return true;
		}
		stub = ( uint8_t* )( ( uintptr_t )stub + size );
	}
	return false;
}

void
release_irq_handler( unsigned index )
{
	scoped_lock lock( patch_handler_lock);

	if( index <= IDT_RESERVED || index > IDT_DESCRIPTORS )
	{
		return;
	}

	auto size = _interrupt_stub_size / ( IDT_DESCRIPTORS - IDT_RESERVED );
	auto stub = ( uint8_t* )( ( uintptr_t )_interrupt_stub_base + size * index );

	auto nr      = ( uint32_t* )&stub[patch_index];
	auto context = ( uint32_t* )&stub[patch_ctxt];
	auto handler = ( uint32_t* )&stub[patch_jump];

	*nr = IDT_STUB_NR_MAGIC;
	*handler = IDT_STUB_FN_FASTGS;

	context[0] = IDT_STUB_CX_MAGIC;
	context[1] = 0xffffffff;

	_interrupt_pointer_table[index] = ( uintptr_t )0xbadf00d;
	idt[index].type &= ~kIDTPresent;
}

void
init( void )
{
	if( processor::is_bsp() )
	{
		uintptr_t stub_size = ( ( uintptr_t )_interrupt_stub_1 ) - ( ( uintptr_t )_interrupt_stub_0 );
		uintptr_t stub_base = ( ( uintptr_t )_interrupt_stub_0 );

		log::printk( "Initializing interrupt descriptor table with %d entries..\n",
		             IDT_DESCRIPTORS );

		if( _detect_patch_offsets() == false )
		{
			log::printk( "Unable to determine interrupt stub patch offsets!\n" );
			do{}while( 1 );
		}

		memset( idt, 0, sizeof( idt ) );
		for( auto i = 0; i < IDT_DESCRIPTORS; ++i )
		{
			if( i <= IDT_RESERVED )
			{
				register_system_handler( i, _default_irq_stub );
			}

			IDTTypeSet  type;
			IDTStackSet stack;
			switch( i )
			{
				case  1: /* #DB  */
					type  = kIDTTypeIRQ | kIDTPresent | kIDTAccessSys;
					stack = kIDTStack1;
					break;

				case  2: /* #NMI */
					type  = kIDTTypeIRQ | kIDTPresent | kIDTAccessSys;
					stack = kIDTStack2;
					break;

				case  8: /* #DF  */
					type  = kIDTTypeIRQ | kIDTPresent | kIDTAccessSys;
					stack = kIDTStack3;
					break;

				case 18: /* #MC  */
					type  = kIDTTypeIRQ | kIDTPresent | kIDTAccessSys;
					stack = kIDTStack4;
					break;

				case 15:        /* not used */
				case 20 ... 31: /* not used */
					continue;

				case 32: /* syscall */
					type  = kIDTTypeIRQ | kIDTPresent | kIDTAccessUsr;
					stack = kIDTStackNone;
					break;

				case 33 ... IDT_DESCRIPTORS:
					type  = kIDTTypeIRQ | kIDTAccessSys;
					stack = kIDTStackNone;
					break;

				default:
					type  = kIDTTypeIRQ | kIDTPresent | kIDTAccessSys;
					stack = kIDTStackNone;
					break;
			}
			_setup_idt_descriptor( idt, i, stub_base, 0x08, stack, type );
			stub_base += ( i > IDT_RESERVED ) ? 0 : stub_size;
		}

		memset( &idtr, 0, sizeof( idtr ) );
		idtr.limit   = sizeof( idt ) - 1;
		idtr.address = ( uintptr_t )idt;

		log::printk( "Reloading interrupt descriptor table..\n" );
	}

	/* reload the IDT */
	__asm__ __volatile__( "lidt (%0)" :: "r"( &idtr ) );
};
};
