/*
**      cdecl -- C gibberish translator
**      src/dam_lev.c
**
**      Copyright (C) 2020  Paul J. Lucas, et al.
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
 * Defines a function for calculating an _edit distance_ between two strings.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "dam_lev.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <string.h>

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets the minimum of \a i or \a j.
 *
 * @param i The first number.
 * @param j The second number
 * @return Returns the minimum of \a i or \a j.
 */
static inline dam_lev_t min_dist( dam_lev_t i, dam_lev_t j ) {
  return i < j ? i : j;
}

////////// extern functions ///////////////////////////////////////////////////

dam_lev_t dam_lev_dist( char const *source, char const *target ) {
  assert( source != NULL );
  assert( target != NULL );

  /*
   * Adapted from the code:
   * <https://gist.github.com/badocelot/5331587>
   */

  size_t const slen = strlen( source );
  size_t const tlen = strlen( target );

  if ( slen == 0 )
    return tlen;
  if ( tlen == 0 )
    return slen;

  //
  // The zeroth row and column are for infinity; the last row and column are
  // extras with higher-than-possible distances to prevent erroneous detection
  // of transpositions that would be outside the bounds of the strings.
  //
  dam_lev_t m[ slen + 2 ][ tlen + 2 ];
  size_t const inf = slen + tlen;
  m[0][0] = inf;
  for ( size_t i = 0; i <= slen; ++i ) {
    m[i+1][1] = i;
    m[i+1][0] = inf;
  } // for
  for ( size_t j = 0; j <= tlen; ++j ) {
    m[1][j+1] = j;
    m[0][j+1] = inf;
  } // for

  // Map from a character to the row where it last appeared in source.
  size_t last_row[256] = { 0 };

  for ( size_t row = 1; row <= slen; ++row ) {
    char const sc = source[ row - 1 ];

    // The last column in the current row where the character in source matched
    // the character in target; as with last_row, zero denotes no match yet.
    size_t last_match_col = 0;

    for ( size_t col = 1; col <= tlen; ++col ) {
      char const tc = target[ col - 1 ];

      // The last place in source where we can find the current character in
      // target.
      size_t const last_match_row = last_row[ (unsigned char)tc ];

      bool const match = sc == tc;

      // Calculate the distances of all possible operations.
      dam_lev_t const dist_ins = m[ row   ][ col+1 ] + 1;
      dam_lev_t const dist_del = m[ row+1 ][ col   ] + 1;
      dam_lev_t const dist_sub = m[ row   ][ col   ] + !match;
      //
      // Calculate the cost of a transposition between the current character in
      // target and the last character found in both strings.
      //
      // All characters between these two are treated as either additions or
      // deletions.
      //
      // Note: Damerau-Levenshtein allows for either additions OR deletions
      // between the transposed characters, but NOT both. This is not
      // explicitly prevented, but if both additions and deletions would be
      // required between transposed characters -- that is if neither:
      //
      //     (row - last_matching_row - 1)
      //
      // nor:
      //
      //     (col - last_match_rol - 1)
      //
      // is zero -- then adding together both addition and deletion costs will
      // cause the total cost of a transposition to become higher than any
      // other operation's cost.
      //
      dam_lev_t const dist_xpos = m[ last_match_row ][ last_match_col ]
        + (row - last_match_row - 1)
        + (col - last_match_col - 1) + 1;

      // Use the minimum distance.
      dam_lev_t dist_min = min_dist( dist_ins, dist_del );
                dist_min = min_dist( dist_min, dist_sub );
                dist_min = min_dist( dist_min, dist_xpos );
      m[ row+1 ][ col+1 ] = dist_min;

      if ( match )
        last_match_col = col;
    } // for

    last_row[ (unsigned char)sc ] = row;
  } // for

  return m[ slen+1 ][ tlen+1 ];
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
