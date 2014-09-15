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

/* TSS manipulation */

#include <string.h>

#include <hotarubi/tss.h>
#include <hotarubi/macros.h>
#include <hotarubi/processor/core.h>
#include <hotarubi/processor/local_data.h>
#include <hotarubi/memory/physmm.h>
#include <hotarubi/memory/const.h>

LOCAL_DATA_INC( hotarubi/tss.h );
LOCAL_DATA_DEF( struct tss::tss *tss );

namespace tss
{
static struct tss _bsp_tss;

void
init( void )
{
	struct tss *tss = nullptr;

	if( processor::is_bsp() )
	{
		tss = &_bsp_tss;
	}
	else
	{
		/* AP - allocate dynamically */
	}

	memset( tss, 0, sizeof( struct tss ) );
	tss->rsp0   = PAGE_SIZE + ( uint64_t )memory::physmm::alloc_page();
	tss->rsp2   = PAGE_SIZE + ( uint64_t )memory::physmm::alloc_page();
	tss->ist[0] = PAGE_SIZE + ( uint64_t )memory::physmm::alloc_page();
	tss->ist[1] = PAGE_SIZE + ( uint64_t )memory::physmm::alloc_page();
	tss->ist[2] = PAGE_SIZE + ( uint64_t )memory::physmm::alloc_page();
	tss->ist[3] = PAGE_SIZE + ( uint64_t )memory::physmm::alloc_page();

	/* FIXME: verify those allocations! */
	processor::local_data()->tss = tss;
}

};
