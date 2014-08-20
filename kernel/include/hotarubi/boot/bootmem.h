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

#ifndef __ASSEMBLER__
extern "C" unsigned char BOOT_PML4[];
extern "C" unsigned char BOOT_PDPT_1[];
extern "C" unsigned char BOOT_PDPT_2[];
extern "C" unsigned char BOOT_PDT[];
#endif

#define BOOT_MAX_MAPPED 0x3ff00000

#ifdef __ASSEMBLER__
# define ULL
#endif

#define KERNEL_LMA      0x0000000000100000ULL
#define KERNEL_VMA      0xffffffff80000000ULL

#ifdef __ASSEMBLER__
# undef ULL
#endif

#endif
