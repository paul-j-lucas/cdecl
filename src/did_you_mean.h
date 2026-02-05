/*
**      PJL Library
**      src/did_you_mean.h
**
**      Copyright (C) 2020-2026  Paul J. Lucas
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

#ifndef pjl_did_you_mean_H
#define pjl_did_you_mean_H

/**
 * @file
 * Declares macros, types, and functions for printing suggestions for "Did you
 * mean ...?"
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/**
 * @defgroup printing-suggestions-group Printing Suggestions
 * Macros, types, and functions for printing suggestions for "Did you mean
 * ...?"
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Data structure to hold a "Did you mean ...?" suggestion.
 */
struct did_you_mean {
  char const *known;                    ///< Known candidate.
  size_t      known_len;                ///< Length of \ref known.
  size_t      dam_lev_dist;             ///< Damerau-Levenshtein edit distance.
  void       *user_data;                ///< Optional user data.
};
typedef struct did_you_mean did_you_mean_t;

/**
 * The signature for a function to clean-up a \ref did_you_mean structure,
 * specifically \ref did_you_mean::known "known" (if dynamically allocated) and
 * \ref did_you_mean::user_data "user_data".
 *
 * @param dym A pointer to a \ref did_you_mean structure to clean up.
 */
typedef void (*dym_cleanup_fn_t)( did_you_mean_t const *dym );

/**
 * The signature for a function to determine whether \a dym is similar enough
 * to the unknown literal given to dym_calc().
 *
 * @param dym A pointer to the \ref did_you_mean structure to check.
 * @return Returns `true` only if \a dym is similar enough to the unknown
 * literal.
 */
typedef bool (*dym_similar_fn_t)( did_you_mean_t const *dym );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Given an array of \ref did_you_mean suggestions for \a unknown, calculates a
 * Damerau-Levenshtein edit distance for each suggestion, sorts the array by
 * it, and removes suggestions that are not similar enough according to \a
 * similar_fn.
 *
 * @param unknown The unknown literal.
 * @param dym_array The array of \ref did_you_mean to use.  May be NULL.  If
 * not, each element _must_ have \ref did_you_mean::known "known" point to a
 * suggestion except the last _must_ be NULL.  The \ref did_you_mean::user_data
 * "user_data" member may be used for any purpose.
 * @param similar_fn A pointer to a \ref dym_similar_fn_t function to use.
 * @param cleanup_fn A pointer to a function to clean-up each element of \a
 * dym_array.  May be NULL if no clean-up is needed.
 * @return Returns `true` only if there is at least one suggestion according to
 * \a similar_fn; `false` otherwise.
 *
 * @sa dym_free()
 */
NODISCARD
bool dym_calc( char const *unknown, did_you_mean_t *dym_array,
              dym_similar_fn_t similar_fn, dym_cleanup_fn_t cleanup_fn );

/**
 * Frees all memory used by \a dym_array _including_ \a dym_array itself.
 *
 * @param dym_array The \ref did_you_mean array to free.  If NULL, does
 * nothing.
 * @param cleanup_fn A pointer to a function to clean-up a \ref did_you_mean
 * structure.  May be NULL.
 *
 * @sa dym_calc()
 */
void dym_free( did_you_mean_t const *dym_array, dym_cleanup_fn_t cleanup_fn );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* pjl_did_you_mean_H */
/* vim:set et sw=2 ts=2: */
