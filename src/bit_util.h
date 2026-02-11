/*
**      cdecl -- C gibberish translator
**      src/bit_util.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef cdecl_bit_util_H
#define cdecl_bit_util_H

/**
 * @file
 * Declares bit utility macros and functions.
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdint.h>                     /* for uint*_t */

/// @endcond

/**
 * @defgroup bitutil-group Bit Utility Macros & Functions
 * Bit utility macros and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Gets a value where all bits that are greater than or equal to the one bit
 * set in \a N are also set, e.g., <code>%BITS_GE(0b00010000)</code> =
 * `0b11110000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GT()
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_GE(N)                (~BITS_LT( (N) ))

/**
 * Gets a value where all bits that are greater than the one bit set in \a N
 * are set, e.g., <code>%BITS_GT(0b00010000)</code> = `0b11100000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_GT(N)                (~BITS_LE( (N) ))

/**
 * Gets a value where all bits that are less than or equal to the one bit set
 * in \a N are also set, e.g., <code>%BITS_LE(0b00010000)</code> =
 * `0b00011111`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_GT()
 * @sa #BITS_LT()
 */
#define BITS_LE(N)                (BITS_LT( (N) ) | (N))

/**
 * Gets a value where all bits that are less than the one bit set in \a N are
 * set, e.g., <code>%BITS_LT(0b00010000)</code> = `0b00001111`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_GT()
 * @sa #BITS_LE()
 */
#define BITS_LT(N)                ((N) - 1u)

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whether \a n has either 0 or 1 bits set.
 *
 * @param n The number to check.
 * @return Returns `true` only if \a n has either 0 or 1 bits set.
 *
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD
inline bool is_01_bit( uint64_t n ) {
  return (n & (n - 1)) == 0;
}

/**
 * Checks whether there are 0 or more bits set in \a n that are only among the
 * bits set in \a set.
 *
 * @param n The bits to check.
 * @param set The bits to check against.
 * @return Returns `true` only if there are 0 or more bits set in \a n that are
 * only among the bits set in \a set.
 *
 * @sa is_01_bit()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD
inline bool is_0n_bit_only_in_set( uint64_t n, uint64_t set ) {
  return (n & set) == n;
}

/**
 * Checks whether \a n has exactly 1 bit is set.
 *
 * @param n The number to check.
 * @return Returns `true` only if \a n has exactly 1 bit set.
 *
 * @sa is_01_bit()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD
inline bool is_1_bit( uint64_t n ) {
  return n != 0 && is_01_bit( n );
}

/**
 * Checks whether \a n has exactly 1 bit set in \a set.
 *
 * @param n The number to check.
 * @param set The set of bits to check against.
 * @return Returns `true` only if \a n has exactly 1 bit set in \a set.
 *
 * @note There may be other bits set in \a n that are not in \a set.
 *
 * @sa is_01_bit()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD
inline bool is_1_bit_in_set( uint64_t n, uint64_t set ) {
  return is_1_bit( n & set );
}

/**
 * Checks whether \a n has exactly 1 bit set only in \a set.
 *
 * @param n The number to check.
 * @param set The set of bits to check against.
 * @return Returns `true` only if \a n has exactly 1 bit set only in \a set.
 *
 * @sa is_01_bit()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD
inline bool is_1_bit_only_in_set( uint64_t n, uint64_t set ) {
  return is_1_bit( n ) && is_1_bit_in_set( n, set );
}

/**
 * Checks whether \a n has one or more bits set that are only among the bits
 * set in \a set.
 *
 * @param n The bits to check.
 * @param set The bits to check against.
 * @return Returns `true` only if \a n has one or more bits set that are only
 * among the bits set in \a set.
 *
 * @sa is_01_bit()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 */
NODISCARD
inline bool is_1n_bit_only_in_set( uint64_t n, uint64_t set ) {
  return n != 0 && is_0n_bit_only_in_set( n, set );
}

/**
 * Gets the value of the least significant bit that's a 1 in \a n.
 * For example, for \a n of 12, returns 4.
 *
 * @param n The number to use.
 * @return Returns said value or 0 if \a n is 0.
 *
 * @sa ms_bit1_32()
 */
NODISCARD
inline uint32_t ls_bit1_32( uint32_t n ) {
  return n & -n;
}

/**
 * Gets the value of the most significant bit that's a 1 in \a n.
 * For example, for \a n of 12, returns 8.
 *
 * @param n The number to use.
 * @return Returns said value or 0 if \a n is 0.
 *
 * @sa ls_bit1_32()
 */
NODISCARD
uint32_t ms_bit1_32( uint32_t n );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_bit_util_H */
/* vim:set et sw=2 ts=2: */
