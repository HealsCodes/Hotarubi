/*******************************************************************************

    Copyright (C) 2015 René 'Shirk' Köcher
 
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

/* Templates for various useful iterator implementations */

#ifndef __ITERATORS_H
#define __ITERATORS_H 1

#include <hotarubi/types.h>

/* lambda based calculating iterator.
 * This iterator implementation relies on two user provided lambdas.
 * One for advancing the iterator (usid in operator++) and a second
 * for fetching a pointer to the current element the iterator is pointing at.
 *
 * Both lambdas are passed a user defined data pointer which can hold
 * arbitrary data and which may be modified by the advance lambda.
 */
template <typename T> class calc_iter
{
public:
	typedef void (*advance_fn)( uintptr_t & );
	typedef T* (*fetch_fn)( uintptr_t );

	calc_iter<T>( advance_fn advance, fetch_fn fetch, void *user_data )
	: _advance{advance}, _fetch{fetch}, _ptr{( uintptr_t )user_data} {};

	bool operator!=( const calc_iter<T>& other )
	{
		return _advance != other._advance ||
		       _fetch   != other._fetch   ||
		       _ptr     != other._ptr;
	};

	const calc_iter<T>& operator++()
	{
		_advance( _ptr );
		return *this;
	};

	T* operator*() const
	{
		return _fetch( _ptr );
	};

private:
	advance_fn _advance;
	fetch_fn   _fetch;
	uintptr_t  _ptr;
};

#endif

