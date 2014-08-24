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

/* basic virtual memory layout definitions */

#ifndef __VMCONST_H
#define __VMCONST_H 1

#include <hotarubi/boot/bootmem.h>

#define PAGE_SIZE 0x1000

namespace memory
{
namespace virtmm
{

	enum VirtualMemoryRangeSet
	{
		kVMRangeUserBase    = 0x0000000000000000ULL,
		kVMRangeUserEnd     = 0x00007fffffffffffULL,

		kVMRangeGuard1Base  = 0xffff800000000000UL,
		kVMRangeGuard1End   = 0xffff80ffffffffffUL,

		kVMRangePhysMemBase = 0xffff880000000000UL,
		kVMRangePhysMemEnd  = 0xffffc7ffffffffffUL,

		kVMRangeGuard2Base  = 0xffffc80000000000UL,
		kVMRangeGuard2End   = 0xffffc8ffffffffffUL,

		kVMRangeHeapBase    = 0xffffc90000000000UL,
		kVMRangeHeapEnd     = 0xffffe8ffffffffffUL,

		kVMRangeIOMapBase   = 0xffffe90000000000UL,
		kVMRangeIOMapEnd    = 0xffffe9ffffffffffUL,

		kVMRangeKernelBase  = KERNEL_VMA,
	};
};
};

#endif
