/*
**      PJL Library
**      src/did_you_mean.c
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
 * Defines types, constants, and functions for printing suggestions for "Did
 * you mean ...?"
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "did_you_mean.h"
#include "dam_lev.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdlib.h>
#include <string.h>

/// @endcond

/**
 * @addtogroup printing-suggestions-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Frees memory used by \a dym.
 *
 * @param dym A pointer to the first \ref did_you_mean to free and continuing
 * until \ref did_you_mean::known "known" is NULL.
 * @param cleanup_fn A pointer to the \ref dym_cleanup_fn_t to use.  May be
 * NULL.
 */
static void dym_cleanup_all( did_you_mean_t const *dym,
                             dym_cleanup_fn_t cleanup_fn ) {
  assert( dym != NULL );
  if ( cleanup_fn == NULL )
    return;
  while ( dym->known != NULL )
    (*cleanup_fn)( dym++ );
}

/**
 * Comparison function for two \ref did_you_mean objects.
 *
 * @param i_dym A pointer to the first \ref did_you_mean.
 * @param j_dym A pointer to the second \ref did_you_mean.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_dym is
 * less than, equal to, or greater than \a j_dym, respectively.
 */
NODISCARD
static int dym_cmp( did_you_mean_t const *i_dym, did_you_mean_t const *j_dym ) {
  ssize_t const cmp =
    STATIC_CAST( ssize_t, i_dym->dam_lev_dist ) -
    STATIC_CAST( ssize_t, j_dym->dam_lev_dist );
  return  cmp != 0 ? STATIC_CAST( int, cmp ) :
          strcmp( i_dym->known, j_dym->known );
}

////////// extern functions ///////////////////////////////////////////////////

void dym_free( did_you_mean_t const *dym_array, dym_cleanup_fn_t cleanup_fn ) {
  if ( dym_array != NULL ) {
    dym_cleanup_all( dym_array, cleanup_fn );
    FREE( dym_array );
  }
}

did_you_mean_t const* dym_new( char const *unknown,
                               dym_prep_fn_t prep_fn, void *prep_data,
                               dym_similar_fn_t similar_fn,
                               dym_cleanup_fn_t cleanup_fn ) {
  assert( unknown != NULL );
  assert( prep_fn != NULL );
  assert( similar_fn != NULL );

  // Pre-flight to calculate array size.
  size_t const dym_size = (*prep_fn)( /*dym=*/NULL, prep_data );
  if ( dym_size == 0 )
    return NULL;                        // LCOV_EXCL_LINE

  did_you_mean_t *const dym_array =
    calloc( dym_size + 1, sizeof( did_you_mean_t ) );

  (*prep_fn)( dym_array, prep_data );

  did_you_mean_t *dym;

  // calculate the source and maximum target lengths
  size_t const unknown_len = strlen( unknown );
  size_t max_known_len = 0;
  for ( dym = dym_array; dym->known != NULL; ++dym ) {
    dym->known_len = strlen( dym->known );
    if ( dym->known_len > max_known_len )
      max_known_len = dym->known_len;
  } // for

  /*
   * Adapted from the code:
   * <https://github.com/git/git/blob/3a0b884caba2752da0af626fb2de7d597c844e8b/help.c#L516>
   */

  // calculate Damerau-Levenshtein edit distance for all candidates
  void *const dam_lev_mem = dam_lev_new( unknown_len, max_known_len );
  for ( dym = dym_array; dym->known != NULL; ++dym ) {
    dym->dam_lev_dist = dam_lev_dist(
      dam_lev_mem, unknown, unknown_len, dym->known, dym->known_len
    );
  } // for
  free( dam_lev_mem );

  // sort by Damerau-Levenshtein distance
  qsort(
    dym_array, dym_size, sizeof dym_array[0],
    POINTER_CAST( qsort_cmp_fn_t, &dym_cmp )
  );

  if ( dym_array->dam_lev_dist > 0 ) {
    bool found_at_least_1 = false;

    for ( dym = dym_array; dym->known != NULL; ++dym ) {
      if ( !(*similar_fn)( dym ) )
        break;
      found_at_least_1 = true;
    } // for

    if ( found_at_least_1 ) {
      //
      // Free known literals past the best ones and set the one past the last
      // to NULL to mark the end.
      //
      dym_cleanup_all( dym, cleanup_fn );
      *dym = (did_you_mean_t){ 0 };
      return dym_array;
    }
  }
  else {
    //
    // This means unknown was an exact match for a known literal which means we
    // shouldn't suggest it for itself.
    //
  }

  dym_free( dym_array, cleanup_fn );
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
