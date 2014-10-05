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

#ifndef _TSS_H
#define _TSS_H 1

#include <bitmask.h>
#include <hotarubi/types.h>

namespace tss
{
	#pragma pack( push, 1 )
	struct tss
	{
		uint32_t _reserved_0;
		uint64_t rsp0;
		uint64_t rsp1;
		uint64_t rsp2;
		uint64_t _reserved_1;
		uint64_t ist[7];
		uint64_t _reserved_2;
		uint16_t _reserved_3;
		uint16_t io_map_base;
	};
	#pragma pack( pop )

	void init( void );
};

#endif
