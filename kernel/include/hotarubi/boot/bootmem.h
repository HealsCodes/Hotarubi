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

/* Early page table definitions used by boot.S and other initialization code */

#ifndef __BOOTMEM_H
#define __BOOTMEM_H 1

#define BOOT_PML4       0x2000
#define BOOT_PDPT_1     0x3000
#define BOOT_PDPT_2     0x4000
#define BOOT_PDT        0x5000

#define BOOT_STACK_BASE 0x7000
#define BOOT_STACK_END  0x8000

#define BOOT_MAX_MAPPED 0x00c00000

#ifdef __ASSEMBLER__
# define ULL
#endif

#define KERNEL_LMA      0x0000000000100000ULL
#define KERNEL_VMA      0xffffffff80000000ULL

#ifdef __ASSEMBLER__
# undef ULL
#endif

#endif
