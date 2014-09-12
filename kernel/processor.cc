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

#include <hotarubi/processor.h>
#include <hotarubi/lock.h>
#include <hotarubi/log/log.h>

namespace processor
{
#define MAX_PROCESSOR_COUNT 255
#define IA32_APCI_BASE 0x1b

static SpinLock processor_accounting_lock;
static unsigned processor_active_count = 0;
/* FIXME: this should be dynamically resized instead of being statically allocated */
static struct local_data *processor_local_data[MAX_PROCESSOR_COUNT];

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

/* while processor_nr() is public I don't think it would be wise to allow
 * others to mess with the data (.. at least not in a simple way) */
static inline void
_store_processor_nr( uint8_t nr )
{
	__asm__ __volatile__( "wrmsr" :: "c"( 0x174 ), "d"( 0 ), "a"( nr ) );
};

struct local_data*
local_data( void )
{
	/* FIXME: assert( processor_nr() < MAX_PROCESSOR_COUNT ); */
	/* FIXME: assert( processor_local_data[x] != nullptr ); */
	return processor_local_data[processor_nr()];
}

void
init ( void )
{
	processor_accounting_lock.lock();
	_store_processor_nr( processor_active_count++ );
	processor_accounting_lock.unlock();

	if( is_bsp() )
	{
		/* no need to hold the accounting_lock - if this is the BSP no other
		 * processors are active at this point */
		memset( processor_local_data, 0, sizeof( processor_local_data ) );
		memset( &_bsp_local_data, 0, sizeof( _bsp_local_data ) );

		processor_local_data[0] = &_bsp_local_data;
	}

	gdt::init();
};

};
