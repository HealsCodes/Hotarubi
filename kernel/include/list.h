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

/* singly and doubly-linked lists */

#ifndef _LIST_H
#define _LIST_H 1

#include <stdint.h>

/* list_* and LIST_* -- doubly linked lists
 *
 * The list_* functions and macros provide a doubly-linked inline list
 * with O(1) support for member insertion at either the list head or tail as
 * well as member deletion.
 * 
 * LIST_FOREACH_* macros provide a shortcut to iterate over the members of the
 * list in forward- as well as reverse order including support for item deletion
 * using the _MUTABLE versions.
 */

struct list_head
{
	struct list_head *prev;
	struct list_head *next;
};

#define LIST_HEAD( name ) struct list_head name
#define LIST_LINK( name ) struct list_head name
#define LIST_INIT( name ) { &(name), &(name) }

#define INIT_LIST( name ) name = LIST_INIT( name )

#define LIST_ENTRY( elem, type, link ) \
	( ( type * )( ( uintptr_t )elem - __builtin_offsetof( type, link ) ) )

/* shortcut to access the head entry */
#define LIST_HEAD_ENTRY( head, type, link ) LIST_ENTRY( ( head )->next, type, link )

/* shortcut to access the tail entry */
#define LIST_TAIL_ENTRY( head, type, link ) LIST_ENTRY( ( head )->prev, type, link )

/* iterate over each link in the list */
#define LIST_FOREACH( ptr, head ) \
	for( struct list_head *ptr = ( head )->next; \
	     ptr != ( head ); \
	     ptr  = ptr->next )

/* iterate over each link in the list in reverse order */
#define LIST_FOREACH_REVERSE( ptr, head ) \
	for( struct list_head *ptr = ( head )->prev; \
	     ptr != ( head ); \
	     ptr  = ptr->prev )

/* iterate over each link in the list with support for the deletion of links */
#define LIST_FOREACH_MUTABLE( ptr, head ) \
	for( struct list_head *ptr = ( head )->next, \
		                  *ref = ptr->next; \
		 ptr != ( head ); \
		 ptr  = ref, \
		 ref  = ptr->next )

/* iterate over each link in the list in reverse order with support for the
 * deletion of links */
#define LIST_FOREACH_MUTABLE_REVERSE( ptr, head ) \
	for( struct list_head *ptr = ( head )->prev, \
		                  *ref = ptr->prev; \
		ptr != ( head ); \
		ptr  = ref, \
		ref  = ptr->prev )

static inline void __list_add( struct list_head *elem, struct list_head *prev, 
                                struct list_head *next )
{
	elem->next = next;
	elem->prev = prev;
	prev->next = elem;
	next->prev = elem;
};

static inline void __list_del( struct list_head *elem )
{
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
};

static inline void list_add( struct list_head *head, struct list_head *elem )
{
	__list_add( elem, head, head->next );
};

static inline void list_add_tail( struct list_head *head, struct list_head *elem )
{
	__list_add( elem, head->prev, head );
};

static inline void list_del( struct list_head *elem )
{
	__list_del( elem );
	elem->prev = nullptr;
	elem->next = nullptr;
};

static inline bool list_empty( struct list_head *head )
{
	return head->next == head;
};

/* slist_* and SLIST_* -- singly linked lists
 *
 * The slist_* functions and macros provide a singly-linked inline list
 * with O(1) support for member insertion at the list head and 
 * O(n) insertion at the list tail as well as member deletion.
 * 
 * The SLIST_FOREACH_* macro provides a shortcut to iterate over the members of 
 * the list with support for item deletion by using the _MUTABLE version.
 */
struct slist_head
{
	struct slist_head *next;
};

#define SLIST_HEAD( name ) struct slist_head name
#define SLIST_LINK( name ) struct slist_head name
#define SLIST_INIT( name ) { &name }

#define INIT_SLIST( name ) name = SLIST_INIT( name )

#define SLIST_ENTRY( elem, type, link ) \
	( ( type * )( ( uintptr_t )elem - __builtin_offsetof( type, link ) ) )

/* shortcut to access the head entry */
#define SLIST_HEAD_ENTRY( head, type, link ) LIST_ENTRY( ( head )->next, type, link )

/* iterate over each link in the list */
#define SLIST_FOREACH( ptr, head ) \
	for( struct slist_head *ptr = ( head )->next; \
	     ptr != ( head ); \
	     ptr  = ptr->next )

#define SLIST_FOREACH_MUTABLE( ptr, head ) \
	for( struct slist_head *ptr = ( head )->next, \
		                   *ref = ptr->next; \
		ptr != ( head ); \
		ptr  = ref, \
		ref  = ptr->next )

static inline bool slist_empty( struct slist_head *head )
{
	return head->next == head;
};

static inline void slist_add( struct slist_head *head, struct slist_head *elem )
{
	elem->next = head->next;
	head->next = elem;
};

static inline void slist_add_tail( struct slist_head *head, struct slist_head *elem )
{
	if( slist_empty( head ) )
	{
		slist_add( head, elem );
	}
	else
	{
		SLIST_FOREACH( ptr, head )
		{
			if( ptr->next == nullptr )
			{
				ptr->next = elem;
				elem->next = nullptr;
				break;
			}
		}
	}
};

static inline void slist_del( struct slist_head *head, struct slist_head *elem )
{
	if( head->next == elem )
	{
		head->next = elem->next;
		elem->next = nullptr;
	}
	else
	{
		SLIST_FOREACH( ptr, head )
		{
			if( ptr->next == elem )
			{
				ptr->next = elem->next;
				elem->next = nullptr;
				break;
			}
		}
	}
};

/* hlist_* and HLIST_* -- doubly linked lists with single head
 *
 * The hlist_* functions and macros provide a doubly-linked inline list
 * with O(1) support for member insertion at the list head and member deletion.
 * 
 * HLIST_FOREACH_* macros provide a shortcut to iterate over the members of the
 * list including support for item deletion the using the _MUTABLE version.
 *
 * hlist_* is intended as a compromisse between the insertion / deletion speed 
 * of a doubly linked list and the size efficiency of the slist_head.
 * The main purpose is the use in linked hash tables where it reduces the space
 * required to store a huge ammount of list heads.
 */

struct hlist_head
{
	struct hlist_node *next;
};

struct hlist_node
{
	struct hlist_node *next;   /* pointer to next node */
	struct hlist_node **pprev; /* pointer to next field of the previous node */
};

#define HLIST_HEAD( name ) struct hlist_head name
#define HLIST_NODE( name ) struct hlist_node name
#define HLIST_HEAD_INIT( name ) { nullptr }
#define HLIST_NODE_INIT( name ) { nullptr, nullptr }

#define INIT_HLIST_HEAD( name ) name = HLIST_HEAD_INIT( name )
#define INIT_HLIST_NODE( name ) name = HLIST_NODE_INIT( name )

#define HLIST_ENTRY( elem, type, link ) \
	( ( type * )( ( uintptr_t )elem - __builtin_offsetof( type, link ) ) )

/* shortcut to access the head entry */
#define HLIST_HEAD_ENTRY( head, type, link ) HLIST_ENTRY( ( head )->next, type, link )

/* iterate over each link in the list */
#define HLIST_FOREACH( ptr, head ) \
	for( struct hlist_node *ptr = ( head )->next; \
	     ptr != nullptr; \
	     ptr  = ptr->next )

/* iterate over each link in the list with support for the deletion of links */
#define HLIST_FOREACH_MUTABLE( ptr, head ) \
	for( struct hlist_node *ptr = ( head )->next, \
		                   *ref = ptr->next; \
		 ptr != nullptr && ( { ref = ptr->next; 1; } ); \
		 ptr  = ref )

static inline void hlist_add( struct hlist_head *head, struct hlist_node *elem )
{
	struct hlist_node *first = head->next;
	elem->next = first;
	if( first != nullptr )
	{
		first->pprev = &elem->next;
	}
	head->next  = elem;
	elem->pprev = &head->next;
};

static inline void hlist_del( struct hlist_node *elem )
{
	struct hlist_node *next   = elem->next,
	                  **pprev = elem->pprev;

	*pprev = elem->next;
	if( next )
	{
		next->pprev = pprev;
	}
	elem->next  = nullptr;
	elem->pprev = nullptr;
};

static inline bool hlist_empty( struct hlist_head *head )
{
	return head->next == nullptr;
};

#endif
