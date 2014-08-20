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

#include <hotarubi/boot/bootmem.h>
#include <hotarubi/boot/multiboot.h>

.intel_syntax noprefix

/******************************************************************************/
.section .multiboot
/******************************************************************************/

#define MULTIBOOT_FLAGS  ( MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO )
#define MULTIBOOT_CHKSUM -( MULTIBOOT_HEADER_MAGIC + MULTIBOOT_FLAGS )
.globl multiboot
.type multiboot, @object
.align 4
multiboot:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_FLAGS
	.long MULTIBOOT_CHKSUM

/******************************************************************************/
.section .text
/******************************************************************************/
.extern kernel_entry, __bss, __ebss

.code32
.globl kickstart
.type kickstart, @function
kickstart:
	cli
	lea edi, _MBD
	mov [edi + 0], eax
	mov [edi + 4], ebx

	/* quick plausibility check */
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000000
	jbe L_failed_64bit_capable
	mov eax, 0x80000001
	cpuid
	bt  edx, 29
	jnc L_failed_64bit_capable

	/* good to go */
	lgdt [_GDTR]
	jmp 0x08:L_flush_gdt_32
	hlt

L_failed_64bit_capable:
	mov ebx, 0xb8000
	mov word ptr[ebx], 0x1f4c
	hlt
1:
	jmp 1b

L_flush_gdt_32:
	mov ax, 0x10
	mov ds, ax
	mov ss, ax

	cmp dword ptr[_BSP], 1
	jne L_enable_paging

L_setup_paging:
	mov edi, BOOT_PML4
	mov ecx, BOOT_PDPT_2
	add ecx, 0x1000
	sub ecx, edi
	shr ecx, 2
	xor eax, eax
	rep stosd

	mov ebx, BOOT_PML4
	mov dword ptr[ebx + 0]        , (BOOT_PDPT_1 | 0x01)
	mov dword ptr[ebx + 0x1ff * 8], (BOOT_PDPT_2 | 0x01)

	mov ebx, BOOT_PDPT_1
	mov edi, BOOT_PDPT_2
	mov dword ptr[ebx + 0]        , (BOOT_PDT | 0x01)
	mov dword ptr[edi + 0x1fe * 8], (BOOT_PDT | 0x01)

	mov ebx, BOOT_PDT
	mov dword ptr[ebx + 0 * 8], (0x00000000 | 0x83)
	mov dword ptr[ebx + 1 * 8], (0x00200000 | 0x83)
	mov dword ptr[ebx + 2 * 8], (0x00400000 | 0x83)
	mov dword ptr[ebx + 3 * 8], (0x00600000 | 0x83)
	mov dword ptr[ebx + 4 * 8], (0x00800000 | 0x83)
	mov dword ptr[ebx + 5 * 8], (0x00a00000 | 0x83)
	mov dword ptr[ebx + 6 * 8], (0x00c00000 | 0x83)
	mov dword ptr[ebx + 7 * 8], (0x00e00000 | 0x83)

L_enable_paging:
	mov eax, BOOT_PML4
	mov cr3, eax

	mov eax, cr4
	or  eax, 1 << 5
	mov cr4, eax

	mov ecx, 0xc0000080
	rdmsr
	or  eax, 1 << 8
	wrmsr

	mov eax, cr0
	or  eax, 1 << 31
	mov cr0, eax
	jmp 0x08:L_paging_enabled
	hlt

L_paging_enabled:
	/* check if EFER.LMA is active too */
	mov ecx, 0xc0000080
	rdmsr
	bt  eax, 10
	jnc L_failed_64bit_enable

	lgdt [_GDTR]
	jmp 0x18:L_long_mode_enabled
	hlt

L_failed_64bit_enable:
	mov ebx, 0xb8000
	mov word ptr[ebx], 0x1fac
1:
	hlt
	jmp 1b

.code64
L_long_mode_enabled:
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	cmp dword ptr[_BSP], 1
	jne L_jump_to_64

	/* BSP passed through here - zero the flag */
	mov dword ptr[_BSP], 0

	/* memory at KERNEL_VMA is now accessible - clean out BSS */
L_clear_bss:
	lea rdi, __bss
	lea rcx, __ebss
	sub rcx, rdi
	shr rcx, 3
	xor rax, rax
	rep stosq

L_jump_to_64:
	/* FIXME: APs will use this stack too? */
	lea rsp, __boot_stack_top 
	mov edi, dword ptr[_MBD + 0]
	mov esi, dword ptr[_MBD + 4]
	lea rax, kernel_entry
	jmp rax

1:
	hlt
	jmp 1b
/******************************************************************************/
.section .data
/******************************************************************************/
.globl _BSP
.type _BSP, @object
_BSP:
	.long 1
_MBD:
	.quad 0x0000000000000000
_GDT:
	.quad 0x0000000000000000
	.quad 0x00cf9a000000ffff
	.quad 0x00cf92000000ffff
	.quad 0x00a09a0000000000
	.quad 0x00a0920000000000
_GDTR:
	.word (_GDTR - _GDT) - 1
	.quad _GDT
