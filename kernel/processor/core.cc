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

#include <string.h>

#include <hotarubi/processor/core.h>
#include <hotarubi/processor/regs.h>
#include <hotarubi/processor/local_data.h>

#include <hotarubi/lock.h>
#include <hotarubi/log/log.h>

namespace processor
{
#define IA32_APCI_BASE     0x1b
#define IA32_GS_BASE       0xc0000101
#define IA32_KERNEL_GSBASE 0xc0000102

LOCAL_DATA_DEF( uint8_t id );

static SpinLock processor_accounting_lock;
static unsigned processor_active_count = 0;

/* local static allocation for the bootstrap processor */
static struct local_data _bsp_local_data;

static inline void
_cpuid( uint32_t eax, uint32_t ecx, uint32_t res[] )
{
	__asm__ __volatile__ (
		"cpuid"
		: "=a"( res[0] ), "=b"( res[1] ), "=c"( res[2] ), "=d"( res[3] )
		: "a"( eax ), "c"( ecx ) 
	);
}

bool is_bsp( void )
{
	return local_data()->id == 0;
}

void
init ( void )
{
	struct local_data *local = nullptr;

	processor_accounting_lock.lock();

	if( processor_active_count == 0 )
	{
		/* BSP */
		local = &_bsp_local_data;
	}
	else
	{
		/* AP - allocate from cache */
	}
	memset( local, 0, sizeof( struct local_data ) );

	local->_gs_self = ( uintptr_t )local;
	local->id       = processor_active_count++;

	processor_accounting_lock.unlock();

	regs::write_msr( IA32_GS_BASE      , ( uintptr_t )local );
	regs::write_msr( IA32_KERNEL_GSBASE, ( uintptr_t )local );

	gdt::init();
};

};
