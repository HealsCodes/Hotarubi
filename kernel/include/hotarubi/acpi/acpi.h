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

/* ACPI interface */

#ifndef __ACPI_ACPI_H
#define __ACPI_ACPI_H 1

#include <hotarubi/processor/ioapic.h>
#include <hotarubi/processor/local_data.h>

namespace acpi
{
    void init_tables( void );
    void parse_madt( struct processor::local_data *&ap_data, uint32_t &core_count,
                     processor::ioapic *&ioapics, uint32_t &ioapic_count );
};

#endif
