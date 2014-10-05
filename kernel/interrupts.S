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

/******************************************************************************/
.section .text
/******************************************************************************/

.macro interrupt_stub nr, has_ec=0, read_msr=0
.globl _interrupt_stub_\nr
.type  _interrupt_stub_\nr, @function

_interrupt_stub_\nr:
	.if \has_ec == 0
		pushq $0
	.endif
		pushq $\nr
		pushq $0
		pushq %rax
	.if \read_msr == 0
		lea L_interrupt_swapgs_fast, %rax
		jmp *%rax
	.else
		lea L_interrupt_swapgs_slow, %rax
		jmp *%rax
	.endif
	/* align to force equal sized stubs */
	.align 16
.endm

.macro interrupt_stub_base
		pushq $0
		pushq $0xffffffffdeadbeef
		pushq $0                           /* dummy push to get stack space  */
		pushq %rax
		movq  $0xffffffff0badf00d, %rax    /* fill in the 64bit user context */
		movq  %rax, 8(%rsp)
		lea L_interrupt_swapgs_fast, %rax
		jmp *%rax
	/* align to force equal sized stubs */
	.align 16
.endm

.macro interrupt_call_up
	pushq %rbp
	movq  %rsp, %rbp
	/* save GP regs */
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax
	pushq %rdi
	pushq %rsi
	/* save DS / ES */
	xorq  %rax, %rax
	movw  %ds, %ax
	pushq %rax
	/* save cr2 in case this was a page fault */
	movq  %cr2, %rax
	pushq %rax

	/* swith to kernel DS */
	movw  $0x10, %ax
	movw  %ax, %ds

	/* pass a pointer to the stacked registers (64bit calling convention) */
	movq  %rsp, %rdi

	/* locate the correct RIP */
	lea   _interrupt_pointer_table, %rbx
	movq  16(%rbp), %rcx            /* 16(rbp) is the irq number */
	movq  0(%rbx, %rcx, 8), %rax   /* offset %rcx into %rbx for the handler RIP */

	/* up call to the kernel */
	callq *%rax

	/* remove cr2 from the stack */
	addq $8, %rsp
	/* restore DS */
	popq %rax
	movw %ax, %ds
	/* restore GP regs */
	popq %rsi
	popq %rdi
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15
	popq %rbp
	/* remove the error code, user context and interrupt nr */
	addq $24, %rsp
.endm

/* system exceptions */

interrupt_stub  0, 0, 0 /* #DE  - divide error              */
interrupt_stub  1, 0, 1 /* #DB  - debug exception           */
interrupt_stub  2, 0, 1 /* #NMI - NMI interrupt             */
interrupt_stub  3, 0, 0 /* #BP  - breakpoint                */
interrupt_stub  4, 0, 0 /* #OF  - overflow                  */
interrupt_stub  5, 0, 0 /* #BR  - BOUND range exception     */
interrupt_stub  6, 0, 0 /* #UD  - invalid opcode            */
interrupt_stub  7, 0, 0 /* #NM  - device not available      */
interrupt_stub  8, 1, 0 /* #DF  - double fault              */
interrupt_stub  9, 0, 0 /* ---  - coprocessor overrun       */
interrupt_stub 10, 1, 0 /* #TS  - invalid TSS exception     */
interrupt_stub 11, 1, 0 /* #NP  - segment not present       */
interrupt_stub 12, 1, 0 /* #SF  - stack fault               */
interrupt_stub 13, 1, 0 /* #GP  - general protection fault  */
interrupt_stub 14, 1, 0 /* #PF  - page fault                */
interrupt_stub 16, 0, 0 /* #MF  - x87 floating point error  */
interrupt_stub 17, 1, 0 /* #AC  - alignment check exception */
interrupt_stub 18, 0, 1 /* #MC  - machine check exception   */
interrupt_stub 19, 0, 0 /* #XM  - SIMD floating point error */

interrupt_stub 32, 0, 0 /* syscall */

/* freely allocatable, dynamically patched stubs */

.globl _interrupt_stub_base
.type  _interrupt_stub_base, @function
.globl _interrupt_stub_size
.type  _interrupt_stub_size, @object

_interrupt_stub_base:
.rep 223
	interrupt_stub_base
.endr
_interrupt_stub_size:
	.quad ( _interrupt_stub_size - _interrupt_stub_base )

/* common handler code */

.globl L_interrupt_swapgs_fast
.type  L_interrupt_swapgs_fast, @function
L_interrupt_swapgs_fast:
	popq %rax
	/* 'fast path' - check CS on the stack */
	cmp $0x08, 24(%rsp)
	je  1f
	swapgs
1:
	interrupt_call_up

	cmp $0x08, 8(%rsp) /* 24 - irq_nr - error_code = 8 */
	je  2f
	swapgs
2:
	iretq

.globl L_interrupt_swapgs_slow
.type  L_interrupt_swapgs_slow, @function
L_interrupt_swapgs_slow:
	popq  %rax
	/* 'slow path' - read msr */
	pushq %rcx
	pushq %rax
	pushq %rdx
	movq $0xc0000101, %rcx
	rdmsr
	/* if SWAPGS was executed before then edx[31] is set ( 0xffff... ) */
	bt  $31, %edx

	popq %rdx
	popq %rax
	popq %rcx

	jc 1f

	swapgs
	interrupt_call_up
	swapgs
	iretq
1:
	interrupt_call_up
	iretq

/******************************************************************************/
.section .bss
/******************************************************************************/
.globl _interrupt_pointer_table
.type  _interrupt_pointer_table, @object
.comm  _interrupt_pointer_table, 2040,8 /* 57 * 8bytes */
