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

/* _vcbprintf as specified by PDClib */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#define MAX_NUMBER_LEN 130 /* 2^128 in binary notation */

static void
convert_number( char *s, size_t n, uintmax_t num, unsigned base, bool caps )
{
	const char  convtab_lower[] = "0123456789abcdef",
	            convtab_upper[] = "0123456789ABCDEF",
	           *convtab = ( caps ) ? convtab_upper
	                               : convtab_lower;

	size_t len = 0;
	uintmax_t tmp = num;

	if( n == 0 )
	{
		*s = '\0';
		return;
	}

	base = ( base < 2 ) ? 2 : ( ( base > 16 ) ? 16 : base );

	do
	{
		tmp /= base;
	} while( ++len, tmp );

	len = ( len > n ) ? n : len;
	for( size_t i = len; i > 0 ; --i )
	{
		s[i - 1] = convtab[ num % base ];
		num /= base;
	}
	s[len] = '\0';
}


/* pad_string is a small performance enhancement.
 * by allowing to padd strings of up to 40 characters with a single call to cb
 * the overall overhead of cb calls is reduced drastically.
 *
 * PDClib test suite without pad_string: 2568 calls to cb
 * PDClib test suite with pad_string   : 636 calls to cb
 */
static const char fill_space[] = "                                        ";
static const char fill_zero [] = "0000000000000000000000000000000000000000";

static inline int pad_string( bool use_zero, size_t len,
	                          void *p, size_t ( *cb )( void *, const char *, size_t ) )
{
	int rc = 0;
	const char *filler = ( use_zero ) ? fill_zero : fill_space;
	const size_t fill_size = strlen( filler );

	while( len )
	{
		if( len && len < fill_size )
		{
			if( cb( p, filler, len ) != len )
			{
				return -1;
			}
			rc += len;
			return rc;
		}
		else
		{
			if( cb( p, filler, fill_size ) != fill_size )
			{
				return -1;
			}
			len -= fill_size;
			rc += fill_size;
		}
	}
	return rc;
}

extern "C" int
_vcbprintf( void *p, size_t ( *cb )( void *, const char *, size_t ),
            const char *fmt, va_list ap )
{
#define PUTC( c ) if( cb( p, c, 1 ) != 1 ) { return -1; } else { ++produced; }
#define PUTS( s, l ) if( cb( p, s, l ) != l ){ return -1; } else { produced += ( l ); }

	enum length
	{
		kLengthChar     = 1,
		kLengthShort    = 2,
		kLengthInt      = 3,
		kLengthLong     = 4,
		kLengthLongLong = 5,
		kLengthIntMax   = 6,
		kLengthSizeT    = 7,
		kLengthPtrDiffT = 8,
	};

	struct length_map
	{
		const char  *str;
		size_t len;
		enum length key;
	};

 	struct length_map lenght_modifiers[] = {
 		/* NOTE: these must be sorted with longest strings first! */
		{ "hh", 2, kLengthChar     },
		{ "ll", 2, kLengthLongLong },
		{ "h" , 1, kLengthShort    },
		{ "l" , 1, kLengthLong     },
		{ "j" , 1, kLengthIntMax   },
		{ "z" , 1, kLengthSizeT    },
		{ "t" , 1, kLengthPtrDiffT },
		/* { "L" , kLengthLongDouble, 1 } */
	};

	struct flags
	{
		uint8_t separate_thousands : 1;
		uint8_t left_justify       : 1;
		uint8_t always_add_sign    : 1;
		uint8_t add_space          : 1;
		uint8_t alternate_form     : 1;
		uint8_t fill_with_zero     : 1;
		uint8_t _unused            : 2;
	};

	struct flags flags;
	size_t produced = 0;
	enum length length = kLengthInt;
	int width = 0, precision = 1;
	bool have_precision = false, have_dot = false;

	const char *format_start = fmt;

	while( *fmt != '\0' )
	{
		/* Collect and print anything until the first conversion.
		 * This was suggested by sortie on #osdev - thanks!
		 */
		if( *fmt != '%' )
		{
			size_t skiplen = 1;
			for( ; fmt[skiplen] != '%' && fmt[skiplen] != '\0'; ++skiplen );

			PUTS( fmt, skiplen );

			fmt += skiplen;
			if( *fmt == '\0' )
			{
				break;
			}
		}

		format_start = fmt++;
		memset( &flags, 0, sizeof( flags ) );

		if( *fmt == '%' )
		{
			PUTC( fmt++ );
			continue;
		}

		/* read flags */
		while( true )
		{
			switch( *(fmt++) )
			{
				case '\'':
					flags.separate_thousands = 1;
					continue;

				case '-':
					flags.left_justify = 1;
					flags.fill_with_zero = 0;
					continue;

				case '+':
					flags.always_add_sign = 1;
					continue;

				case ' ':
					flags.add_space = 1;
					continue;

				case '#':
					flags.alternate_form = 1;
					continue;

				case '0':
					if( !flags.left_justify )
					{
						flags.fill_with_zero = 1;
					}
					continue;

				default:
					break;
			}
			/* adjust for the last increment */
			--fmt;
			break;
		}
		/* read field width */
		width = 0;
		while( *fmt >= '0' && *fmt <= '9' )
		{
			width = width * 10 + *(fmt++) - '0';
		}

		if( *fmt == '*' )
		{
			++fmt;
			width = va_arg( ap, int );
			if( width < 0 )
			{
				width *= -1;
				flags.left_justify = 1;
			}
		}

		precision = 0;//~-1;
		have_dot = false;
		have_precision = false;
		if( *fmt == '.' )
		{
			have_dot = true;
			/* read precision */
			++fmt;
			precision = 0;
			while( *fmt >= '0' && *fmt <= '9' )
			{
				precision = precision * 10 + *(fmt++) - '0';
			}

			if( *fmt == '*' )
			{
				++fmt;
				precision = va_arg( ap, int );
				if( precision < 0 )
				{
					precision = 0;//~-1;
				}
			}
			have_precision = precision > 0;
			flags.fill_with_zero = ( have_precision ) ? 0 : flags.fill_with_zero;
		}

		/* check for length modifiers */
		length = kLengthInt;
		for( size_t i = 0; i < ( sizeof( lenght_modifiers ) / sizeof( lenght_modifiers[0] ) ); ++i )
		{
			if( strncmp( fmt, lenght_modifiers[i].str, lenght_modifiers[i].len ) == 0 )
			{
				length = lenght_modifiers[i].key;
				fmt += lenght_modifiers[i].len;
				break;
			}
		}

		/* do the conversion */
		switch( *(fmt++) )
		{
			case 'p': /* FALL THROUGH */
				flags.alternate_form = 1;
			case 'd': /* FALL THROUGH */
			case 'i': /* FALL THROUGH */
			case 'o': /* FALL THROUGH */
			case 'u': /* FALL THROUGH */
			case 'x': /* FALL THROUGH */
			case 'X': /* FALL THROUGH */
			{
				fmt--;
				/* handle number formatting */
				unsigned base;
				bool is_signed = false, negative = false;

				char numbuf[MAX_NUMBER_LEN + 1];
				const char *sign = "", *prefix = "";

				base = ( *fmt == 'o' ) ? 8 : ( ( *fmt == 'X' || *fmt == 'x' || *fmt == 'p' ) ? 16 : 10 );
				is_signed = ( *fmt == 'd' || *fmt == 'i' ) ? true : false;

				uintmax_t number = 0;
				if( is_signed )
				{
					intmax_t signum = 0;
					switch( length )
					{
						case kLengthChar:  /* FALL THROUGH */
						case kLengthShort: /* FALL THROUGH */
						case kLengthInt:
							signum = va_arg( ap, int );
							break;

						case kLengthLong:
							signum = va_arg( ap, long );
							break;

						case kLengthLongLong:
							signum = va_arg( ap, long long );
							break;

						case kLengthIntMax:
							signum = va_arg( ap, intmax_t );
							break;

						case kLengthSizeT:
							signum = va_arg( ap, size_t );
							break;

						case kLengthPtrDiffT:
							signum = va_arg( ap, ptrdiff_t );
							break;
					}

					number = ( negative = signum < 0 ) ? ( uintmax_t ) -signum
					                                   : ( uintmax_t )  signum;
				}
				else
				{
					negative = false;
					switch( length )
					{
						case kLengthChar:  /* FALL THROUGH */
						case kLengthShort: /* FALL THROUGH */
						case kLengthInt:
							number = va_arg( ap, unsigned int );
							break;

						case kLengthLong:
							number = va_arg( ap, unsigned long );
							break;

						case kLengthLongLong:
							number = va_arg( ap, unsigned long long );
							break;

						case kLengthIntMax:
							number = va_arg( ap, uintmax_t );
							break;

						case kLengthSizeT:
							number = va_arg( ap, size_t );
							break;

						case kLengthPtrDiffT:
							number = va_arg( ap, ptrdiff_t );
							break;
					}
				}
				memset( numbuf, 0, sizeof( numbuf ) );
				convert_number( numbuf, MAX_NUMBER_LEN, number, base, ( *fmt == 'X' ) );

				/* precision: minimal length of number (padded with zero if needed)
				 *     width: minimal length of number + prefix + sign
				 */
				int numlen = strlen( numbuf );
				int precision_with_prefix = ( precision < numlen ) ? numlen : precision;

				sign = 0;
				prefix = 0;

				if( is_signed )
				{
					if( negative || flags.always_add_sign || flags.add_space )
					{
						++precision_with_prefix;
						sign = ( negative ) ? "-"
						                    : ( ( flags.always_add_sign ) ? "+" : " " );
					}
				}
				else if( flags.alternate_form && number != 0 )
				{
					if( *fmt == 'o' )
					{
						prefix = "0";
						++precision_with_prefix;
					}
					else
					{
						prefix = ( *fmt == 'X' ) ? "0X" : "0x";
						precision_with_prefix += 2;
					}
				}

				int padd = precision_with_prefix;
				/* left aligned + optional prefix & sign */
				if( flags.left_justify == 0 )
				{
					if( flags.fill_with_zero )
					{
						if( sign )
						{
							PUTC( sign );
						}
						if( flags.alternate_form && prefix )
						{
							PUTS( prefix, strlen( prefix ) );
						}
					}
					if( padd < width )
					{
						if( pad_string( flags.fill_with_zero, width - padd, p, cb ) != width - padd )
						{
							return -1;
						}
						produced += width - padd;
					}
				}
				if( !flags.fill_with_zero )
				{
					if( sign )
					{
						PUTC( sign );
					}
					if( flags.alternate_form && prefix )
					{
						PUTS( prefix, strlen( prefix ) );
					}
				}
				if( ( have_precision && precision > 0 ) || !have_precision )
				{
					/* print at least $precision digits - fill with zeros if needed */
					padd = numlen;
					if( padd < precision )
					{
						if( pad_string( true, precision - padd, p, cb ) != precision - padd )
						{
							return -1;
						}
						produced += precision - padd;
					}
					
					if( have_dot && precision == 0 && number == 0 )
					{
						strncpy( numbuf, " \0", 2 );
						numlen = 1;
					}
					PUTS( numbuf, ( size_t )numlen )
				}
				if( width > precision_with_prefix && flags.left_justify )
				{
					if( pad_string( false, width - precision_with_prefix, p, cb ) != width - precision_with_prefix )
					{
						return -1;
					}
					produced += width - precision_with_prefix;
				}
			}
				fmt++;
				break;

			/*-- case f,F,e,E,g,G,a,A --*/
			case 'c':
			{
				char c = va_arg( ap, int );
				if( !flags.left_justify && width > 1)
				{
					if( pad_string( false, width - 1, p, cb ) != width - 1)
					{
						return -1;
					}
					produced += width - 1;
				}
				PUTC( &c );
				if( flags.left_justify && width > 1)
				{
					if( pad_string( false, width - 1, p, cb ) != width - 1 )
					{
						return -1;
					}
					produced += width - 1;
				}
			}
				break;

			case 's':
			{
				/* precision: max number of bytes */
				int padd = 0;
				const char *string = va_arg( ap, char* );

				if( !have_precision )
				{
					precision = have_dot ? 0 : strlen( string );
				}
				else
				{
					int len = strlen( string );
					precision = ( precision < len ) ? precision : len;
				}
				padd = precision;

				if( flags.left_justify == 0 && padd < width )
				{
					if( pad_string( false, width - padd, p, cb ) != width - padd )
					{
						return -1;
					}
					produced += width - padd;
				}

				if( precision )
				{
					PUTS( string, ( size_t )precision );
				}
				if( flags.left_justify && padd < width )
				{
					if( pad_string( false, width - padd, p, cb ) != width - padd )
					{
						return -1;
					}
					produced += width - padd;
				}
			}
				break;

			case 'n':
				switch( length )
				{
					case kLengthChar:
						*va_arg( ap, signed char* ) = ( signed char )produced;
						break;

					case kLengthShort:
						*va_arg( ap, short* ) = ( short )produced;
						break;

					case kLengthInt:
						*va_arg( ap, int* ) = ( int )produced;
						break;

					case kLengthLong:
						*va_arg( ap, long* ) = ( long )produced;
						break;

					case kLengthLongLong:
						*va_arg( ap, long long* ) = ( long long )produced;
						break;

					case kLengthIntMax:
						*va_arg( ap, intmax_t* ) = ( intmax_t )produced;
						break;

					case kLengthSizeT:
						*va_arg( ap, size_t* ) = produced;
						break;

					case kLengthPtrDiffT:
						*va_arg( ap, ptrdiff_t* ) = ( ptrdiff_t )produced;
						break;
				}
				continue;

			default:
			{
				/* invalid.. */
				size_t len = fmt - format_start;
				PUTS( format_start, len );
			}
		}
	}
	// FIXME: handle INT_MAX case
	return produced;
}

extern "C" int
_cbprintf( void *p, size_t ( *cb )( void *, const char *, size_t ),
            const char *fmt, ... )
{
	int rc;
	va_list ap;
	
	va_start( ap, fmt );
	rc = _vcbprintf( p, cb, fmt, ap );
	va_end( ap );

	return rc;
}
