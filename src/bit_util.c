/*
**      cdecl -- C gibberish translator
**      src/bit_util.c
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

/**
 * @file
 * Defines bit utility functions.
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "bit_util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdint.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

uint32_t ms_bit1_32( uint32_t n ) {
  n |= n >>  1;                         // "smear" MSB to the right
  n |= n >>  2;
  n |= n >>  4;
  n |= n >>  8;
  n |= n >> 16;
  return n ^ (n >> 1);                  // isolate MSB
}

///////////////////////////////////////////////////////////////////////////////

extern inline bool is_01_bit( uint64_t );
extern inline bool is_0n_bit_only_in_set( uint64_t, uint64_t );
extern inline bool is_1_bit( uint64_t );
extern inline bool is_1_bit_in_set( uint64_t, uint64_t );
extern inline bool is_1_bit_only_in_set( uint64_t, uint64_t );
extern inline bool is_1n_bit_only_in_set( uint64_t, uint64_t );
extern inline uint32_t ls_bit1_32( uint32_t );

/* vim:set et sw=2 ts=2: */
