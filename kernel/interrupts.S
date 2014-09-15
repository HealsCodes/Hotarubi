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
	.else
		/* keep the byte size equal */
		nop
		nop
	.endif
		pushq $\nr
	.if \read_msr == 0
		jmp L_interrupt_swapgs_fast
	.else
		jmp L_interrupt_swapgs_slow
	.endif
.endm

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

L_interrupt_swapgs_fast:
	/* 'fast path' - check CS on the stack */
	cmp   $0x08, 24(%rsp)
	je 1f
	swapgs
1:
	jmp L_interrupt_call_up

L_interrupt_swapgs_slow:
	/* 'slow path' - read msr */
	pushq %rcx
	pushq %rax
	pushq %rdx
	movq $0xc0000101, %rcx
	rdmsr
	/* if SWAPGS was executed before then ecx[31] is set ( 0xffff... ) */
	bt  $31, %ecx
	jc 1f
	swapgs
1:
	popq %rdx
	popq %rax
	popq %rcx
	jmp L_interrupt_call_up

L_interrupt_call_up:
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

	/* locate the correct RIP */
	lea   _interrupt_pointer_table, %rbx
	movq  8(%rbp), %rcx            /* 8(rbp) is the irq number */
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
	/* remove the error code and interrupt nr */
	addq $16, %rsp
	iretq

/******************************************************************************/
.section .bss
/******************************************************************************/
.globl _interrupt_pointer_table
.type  _interrupt_pointer_table, @object
.comm  _interrupt_pointer_table, 392,8 /* 33 * 8bytes */
