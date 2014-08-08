/*******************************************************************************

    Copyright (C) 2014  René 'Shirk' Köcher
 
    This file is part of Hotarubi.

    Hotarubi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

/*
 * C++ runtime initialization and destruction
 * ..mostly functions required by the `Itanium C++ Application Binary Interface'
 */
#include <stddef.h>

#define CXXRT_MAX_ATEXIT_FUNCS 256 /* who want's 256 global class instances? */

/* collected destructor list needed by __cxa_finalize */
struct itanium_cxa_atexit_fn_t
{
	void ( *destructor_func )( void * );
	void *p_obj;
	void *p_dso;
} __atexit_funcs[CXXRT_MAX_ATEXIT_FUNCS];

size_t __atexit_func_count = 0;

/* prototype declarations */
extern "C" int __cxa_atexit( void ( * )( void * ), void *, void * );
extern "C" void __cxa_finalize( void * );
extern "C" void __cxa_pure_virtual( void );

extern "C" int
__cxa_atexit( void ( *destructor_func )( void * ), void *p_obj, void *p_dso )
{
	/* FIXME: should this code be SMP/thread save? */
	if (CXXRT_MAX_ATEXIT_FUNCS <= __atexit_func_count)
	{
		return -1;
	}

	__atexit_funcs[__atexit_func_count].destructor_func = destructor_func;
	__atexit_funcs[__atexit_func_count].p_obj = p_obj;
	__atexit_funcs[__atexit_func_count].p_dso = p_dso;
	__atexit_func_count++;

	return 0;
}

extern "C" void
__cxa_finalize( void *destructor_func )
{
	/* FIXME: This code is modeled after a sample implementation
	 * FIXME: on wiki.osdev.org and it shares the same 'bug'..
	 * FIXME: Destructores are qeued linear and they need to be destroyed
	 * FIXME: in reverse order - last destructor first and so on.
	 * FIXME: Since cxa_atexit above does no hole filling we have to take
	 * FIXME: care to not call a NULL destructor!
	 */
	size_t i = __atexit_func_count;

	if ( NULL == destructor_func )
	{
		/* a request to finalize NULL means we should just
		 * finalize __everything__ >:) !!
		 */
		while ( --i )
		{
			if( NULL != __atexit_funcs[i].destructor_func )
			{
				( *__atexit_funcs[i].destructor_func )( __atexit_funcs[i].p_obj );
			}
		}
		return;
	}

	/* .. otherwise destroy the matching object(s) __once__ */
	while( --i )
	{
		if( destructor_func == (void*)__atexit_funcs[i].destructor_func )
		{
			( *__atexit_funcs[i].destructor_func )( __atexit_funcs[i].p_obj );
			__atexit_funcs[i].destructor_func = NULL;
		}
	}
}

extern "C" void
__cxa_pure_virtual( void )
{
	/* FIXME: panic() ? */
}
