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

/* Template based bitmask operators for enumerations.
 * Works on all enum class types marked with the 'is_bitmask' tag.
 */

#ifndef _BITMASK_H
#define _BITMASK_H 1

#include <cstdint>
#include <type_traits>

template <typename T>
struct is_bitmask
{
private:
	template<typename U> static std::true_type  eval( decltype( U::is_bitmask )* );
	template<typename>   static std::false_type eval( ... );

	typedef decltype( eval<T>( 0 ) ) bitmask;

public:
	static const bool value = std::is_enum<T>::value && bitmask::value;
};

template <typename T>
constexpr T operator &( T a, T b )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	return ( T )( ( typename std::underlying_type<T>::type )a
	              & ( typename std::underlying_type<T>::type )b );
};

template <typename T>
constexpr T operator |( T a, T b ) 
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	return ( T )( ( typename std::underlying_type<T>::type )a
	              | ( typename std::underlying_type<T>::type )b );
};

template <typename T>
constexpr T operator ^( T a, T b )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	return ( T )( ( typename std::underlying_type<T>::type )a
	              ^ ( typename std::underlying_type<T>::type )b );
};

template <typename T>
constexpr T operator ~( T a )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	return ( T )( ~( typename std::underlying_type<T>::type )a ); 
};

template <typename T>
inline T& operator &=( T &a, T b )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	a = a & b; return a; 
};

template <typename T>
inline T& operator |=( T &a, T b )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	a = a | b; return a;
};

template <typename T>
inline T& operator ^=( T &a, T b )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	a = a ^ b; return a;
};

template <typename T>
constexpr bool flag_set( T a, T b )
{
	static_assert( is_bitmask<T>::value, "enum class without 'is_bitmask' tag");
	return ( ( typename std::underlying_type<T>::type )a
	         & ( typename std::underlying_type<T>::type )b );
};

template <typename T>
constexpr typename std::underlying_type<T>::type numeric( T a )
{
	/* not explicitly a bitmask feature so check only for is_enum */
	static_assert( std::is_enum<T>::value, "enum class is required");
	return ( typename std::underlying_type<T>::type )a;
};

#endif
