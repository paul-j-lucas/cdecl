/*
**      PJL Library
**      src/dam_lev.c
**
**      Copyright (C) 2020-2025  Paul J. Lucas
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
#include "util.h"

// standard
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <string.h>

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets the minimum of \a i or \a j.
 *
 * @param i The first number.
 * @param j The second number
 * @return Returns the minimum of \a i or \a j.
 */
NODISCARD
static inline size_t min_dist( size_t i, size_t j ) {
  return i < j ? i : j;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Dynamically allocates a two-dimensional matrix [\a idim][\a jdim] elements
 * of size \a esize.
 *
 * @param esize The size in bytes of a single element.
 * @param ealign The alignment of a single element.
 * @param idim The number of elements in the _i_ dimension.
 * @param jdim The number of elements in the _j_ dimension.
 * @return Returns a pointer to a new two-dimensional matrix that may be cast
 * to `T**` where `T` is the type of element.  The caller is responsible for
 * freeing it via **free**(3).
 */
NODISCARD
static void** matrix2d_new( size_t esize, size_t ealign, size_t idim,
                            size_t jdim ) {
  // ensure &elements[0] is suitably aligned
  size_t const ptrs_size = round_up_pow_2( sizeof(void*) * idim, ealign );
  size_t const row_size = esize * jdim;
  // allocate the row pointers followed by the elements
  void **const rows = MALLOC( char, ptrs_size + idim * row_size );
  char *const elements = POINTER_CAST( char*, rows ) + ptrs_size;
  for ( size_t i = 0; i < idim; ++i )
    rows[i] = &elements[ i * row_size ];
  return rows;
}

////////// extern functions ///////////////////////////////////////////////////

size_t dam_lev_dist( void *working_mem, char const *source, size_t slen,
                     char const *target, size_t tlen ) {
  assert( working_mem != NULL );
  assert( source != NULL );
  assert( target != NULL );

  /*
   * Adapted from the code:
   * <https://gist.github.com/badocelot/5331587>
   */

  if ( slen == 0 )
    return tlen;                        // LCOV_EXCL_LINE
  if ( tlen == 0 )
    return slen;                        // LCOV_EXCL_LINE

  //
  // The zeroth row and column are for infinity; the last row and column are
  // extras with higher-than-possible distances to prevent erroneous detection
  // of transpositions that would be outside the bounds of the strings.
  //
  size_t const inf = slen + tlen;
  size_t *const *const dist_matrix = POINTER_CAST( size_t**, working_mem );
  dist_matrix[0][0] = inf;
  for ( size_t i = 0; i <= slen; ++i ) {
    dist_matrix[i+1][1] = i;
    dist_matrix[i+1][0] = inf;
  } // for
  for ( size_t j = 0; j <= tlen; ++j ) {
    dist_matrix[1][j+1] = j;
    dist_matrix[0][j+1] = inf;
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
      size_t const ins_dist = dist_matrix[ row   ][ col+1 ] + 1;
      size_t const del_dist = dist_matrix[ row+1 ][ col   ] + 1;
      size_t const sub_dist = dist_matrix[ row   ][ col   ] + !match;
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
      size_t const xpos_dist = dist_matrix[ last_match_row ][ last_match_col ]
        + (row - last_match_row - 1)
        + (col - last_match_col - 1)
        + 1;

      // Use the minimum distance.
      size_t dist_min = min_dist( ins_dist, del_dist );
             dist_min = min_dist( dist_min, sub_dist );
             dist_min = min_dist( dist_min, xpos_dist );
      dist_matrix[ row+1 ][ col+1 ] = dist_min;

      if ( match )
        last_match_col = col;
    } // for

    last_row[ (unsigned char)sc ] = row;
  } // for

  return dist_matrix[ slen+1 ][ tlen+1 ];
}

void* dam_lev_new( size_t max_source_len, size_t max_target_len ) {
  return matrix2d_new(
    sizeof(size_t), alignof(size_t), max_source_len + 2, max_target_len + 2
  );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
