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

/* IO-Memory resource management */

#include <list.h>
#include <hotarubi/lock.h>

#include <hotarubi/memory/mmio.h>

#include <hotarubi/memory/virtmm.h>
#include <hotarubi/memory/const.h>
#include <hotarubi/memory/page.h>

namespace memory
{
namespace mmio
{
struct resource
{
	const char *name;

	uintptr_t start;
	uintptr_t range;

	MMIOFlagSet flags;

	struct resource *parent;

	LIST_LINK( siblings );
	LIST_HEAD( children );
};

static SpinLock mmio_resource_lock;
static struct resource mmio_mem_root  = { "IO mem", 0, 0xffffffff, kMMIOFlagIOMem, nullptr, { nullptr, nullptr }, { nullptr, nullptr } };
static struct resource mmio_port_root = { "IO ports", 0, 0xffff, kMMIOFlagIOPort, nullptr, { nullptr, nullptr }, { nullptr, nullptr } };

static resource_t
_check_resource( resource_t root, uintptr_t start, uintptr_t range )
{
	/* check if we overlapp root */
	if( ( ( start < root->start && range > root->start ) ||   /* overlaps start */
		  ( start < root->range && range > root->range ) ) || /* overlaps range */
		( ( start >= root->start && range <= root->range && root->flags & kMMIOFlagBusy ) ) ) /* fits but is busy */
	{
		return root;
	}

	/* check if we overlap any children */
	LIST_FOREACH( ptr, &root->children )
	{
		resource_t child = LIST_ENTRY( ptr, struct resource, siblings );
		if( _check_resource( child, start, range ) != nullptr )
		{
			return child;
		}
	}
	return nullptr;
}

static bool
_possible_sibling( resource_t root, resource_t next )
{
	return ( next->range < root->start ) || ( next->start > root->range );
}

static bool
_add_nested( resource_t root, resource_t child )
{
	if( _possible_sibling( root, child ) )
	{
		LIST_FOREACH( ptr, &root->siblings )
		{
			resource_t sibling = LIST_ENTRY( ptr, struct resource, siblings );
			if( sibling->start > child->start )
			{
				list_add_before( &sibling->siblings, &child->siblings );
				child->parent = sibling->parent;
				return true;
			}
		}
		if( root->start > child->start )
		{
			list_add_before( &root->siblings, &child->siblings );
		}
		else
		{
			list_add_after( &root->siblings, &child->siblings );
		}
		child->parent = root->parent;
		return true;
	}
	else
	{
		if( list_empty( &root->children ) )
		{
			list_add( &root->children, &child->siblings );
			child->parent = root;
			return true;
		}
		else
		{
			LIST_FOREACH( ptr, &root->children )
			{
				if( _add_nested( LIST_ENTRY( ptr, struct resource, siblings ), child ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}

static resource_t
_request_resource( resource_t root, resource_t request )
{
	resource_t collision = nullptr;
	collision = _check_resource( root, request->start, request->range );
	if( collision == nullptr )
	{
		if( _add_nested( root, request ) )
		{
			return request;
		}
	}
	return collision;
}

static void
_release_resource( resource_t region )
{
	if( region->parent == nullptr )
	{
		return;
	}

	if( list_empty( &region->siblings ) == false )
	{
		LIST_FOREACH( ptr, &region->children )
		{
			LIST_ENTRY( ptr, struct resource, siblings )->parent = region->parent;
		}
		list_splice( &region->children, &region->siblings );
	}
	list_del( &region->siblings );
}

resource_t
request_region( const char *name, uintptr_t start, size_t size, MMIOFlagSet flags )
{
	resource_t request = new struct resource;

	request->name  = name;
	request->start = start;
	request->range = start + size;
	request->flags = flags;
	request->siblings.prev = nullptr;
	request->siblings.next = nullptr;
	INIT_LIST( request->children );

	mmio_resource_lock.lock();
	resource_t root = ( ( flags & kMMIOFlagIOPort ) ? &mmio_port_root
	                                                 : &mmio_mem_root );

	if( _request_resource( root, request ) != request )
	{
		delete request;
		request = nullptr;
	}
	mmio_resource_lock.unlock();
	return request;
}

void
release_region( resource_t *region )
{
	mmio_resource_lock.lock();
	_release_resource( *region );
	delete *region;
	*region = nullptr;
	mmio_resource_lock.unlock();
}

uintptr_t
activate_region( resource_t region )
{
	if( region->flags & kMMIOFlagIOPort )
	{
		/* IO-Ports are not mapped.. */
		return region->start;
	}

#ifdef KERNEL
	/* check if any parent is mapped */
	mmio_resource_lock.lock();
	resource_t tmp = region;
	do {
		if( tmp->flags & kMMIOFlagMapped )
		{
			goto out_mapped;
		}
		tmp = tmp->parent;
	} while( tmp != nullptr );

	/* map the region (or the closest boundary of it..) */
	for( size_t i = 0; i < region->range - region->start; i += PAGE_SIZE )
	{
		virtmm::map_fixed( virtmm::kVMRangeIOMapBase + region->start + i,
		                   region->start + i,
		                   __VPF( Writable ) | __VPF( CacheDisable ) );
	}

out_mapped:
	region->flags |= kMMIOFlagMapped;
	mmio_resource_lock.unlock();
#endif
	return ( uintptr_t )( virtmm::kVMRangeIOMapBase + region->start );
}

void
init( void )
{
	INIT_LIST( mmio_mem_root.children );
	INIT_LIST( mmio_port_root.children );
}

};
};
