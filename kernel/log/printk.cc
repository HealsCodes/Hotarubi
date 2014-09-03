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

/* kernel logging via printk (needs work..) */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <_vcbprintf.h>

#include <hotarubi/io.h>
#include <hotarubi/lock.h>

namespace log
{

struct ring_buffer
{
	char *data;
	size_t in, out, len;
	SpinLock lock;

	size_t ( *callback )( void *p, const char *str, size_t n );
};

static char logbuffer[4096];
static struct ring_buffer logring;

static size_t
emit( void *p, const char *str, size_t n )
{
	struct ring_buffer *buffp = ( struct ring_buffer* )p;

	for( size_t i = 0; i < n; ++i, ++str )
	{
		buffp->data[ buffp->in++ ] = *str;

		if( buffp->in >= buffp->len )
		{
			buffp->in = 0;
		}
		if( buffp->out == buffp->in )
		{
			buffp->out++;
			if( buffp->out >= buffp->len )
			{
				buffp->out = 0;
			}
		}
	}
	return n;
}


/* FIXME: this is a hack that's used until I get a proppert log-and-notify system
 *        in place. It will echo the characters to the debug port and then call emit()
 */
static size_t
emit_early_debug( void *p, const char *str, size_t n )
{
	for( size_t i = 0; i < n; ++i )
	{
		io::outb( str[i], 0xe9 );
	}
	return emit( p, str, n );
}

void
init_printk( void )
{
	logring.data = logbuffer;
	logring.in  = 0;
	logring.out = 0;
	logring.len = sizeof( logbuffer );
	logring.callback = emit;

	memset( logbuffer, 0, sizeof( logbuffer ) );
}

int
printk( const char *fmt, ... )
{
	int rc;
	va_list ap;

	logring.lock.lock();

	va_start( ap, fmt );
	rc = _vcbprintf( &logring, logring.callback, fmt, ap );
	va_end( ap );

	logring.lock.unlock();
	return rc;
}

void
register_debug_output( void )
{
	logring.lock.lock();
	logring.callback = emit_early_debug;
	logring.lock.unlock();
}

void
unregister_debug_output( void )
{
	logring.lock.lock();
	logring.callback = emit;
	logring.lock.unlock();
}

};