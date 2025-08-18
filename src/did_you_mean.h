/*
**      PJL Library
**      src/did_you_mean.h
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
#include <stddef.h>                     /* for size_t */

/**
 * @defgroup printing-suggestions-group Printing Suggestions
 * Macros, types, and functions for printing suggestions for "Did you mean
 * ...?"
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Data structure to hold a "Did you mean ...?" literal.
 */
struct did_you_mean {
  char const *literal;                  ///< Candidate literal.
  size_t      literal_len;              ///< Length of \ref literal.
  size_t      dam_lev_dist;             ///< Damerau-Levenshtein edit distance.
  void       *user_data;                ///< Optional user data.
};
typedef struct did_you_mean did_you_mean_t;

/**
 * The signature for a function to clean-up a \ref did_you_mean structure,
 * specifically the \ref did_you_mean::user_data "user_data" member.
 *
 * @param dym A pointer to a \ref did_you_mean structure to clean up.
 */
typedef void (*dym_cleanup_fn_t)( did_you_mean_t const *dym );

/**
 * The signature for a function to prepare \ref did_you_mean elements of an
 * array thereof.
 *
 * @remarks The function _must_ set \ref did_you_mean::literal "literal" to a
 * candate literal; it may set \ref did_you_mean::user_data to anything.  Other
 * members of \ref did_you_mean are set by dym_new().
 *
 * @param dym_array If NULL, the function _must_ only return the size of the
 * \ref did_you_mean array to allocate (not including the terminating element
 * containing a NULL \ref did_you_mean::literal "literal"; otherwise the
 * function _must_ set the \ref did_you_mean::literal "literal" member of every
 * array element.

 * @param prep_data Optional data passed to the function.
 * @return If \a dym_array is NULL, returns the size of the \ref did_you_mean
 * array to allocate; otherwise the return value is unspecified.
 */
typedef size_t (*dym_prep_fn_t)( did_you_mean_t *dym_array, void *prep_data );

/**
 * The signature for a function to determine whether \a dym is similar enough
 * to the unknown literal given to dym_new().
 *
 * @param dym A pointer to the \ref did_you_mean structure to check.
 * @return Returns `true` only if \a dym is similar enough to the unknown
 * literal.
 */
typedef bool (*dym_similar_fn_t)( did_you_mean_t const *dym );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all memory used by \a dym_array _including_ \a dym_array itself.
 *
 * @param dym_array The \ref did_you_mean array to free.  If NULL, does
 * nothing.
 * @param cleanup_fn A pointer to a function to clean-up a \ref did_you_mean
 * structure.  May be NULL.
 *
 * @sa dym_new()
 */
void dym_free( did_you_mean_t const *dym_array, dym_cleanup_fn_t cleanup_fn );

/**
 * Creates a new array of \ref did_you_mean elements containing "Did you mean
 * ...?" suggestion literals for \a unknown_literal.
 *
 * @param unknown_literal The unknown literal.
 * @param prep_fn A pointer to a \ref dym_prep_fn_t function to use.
 * @param prep_data Optional data passed to \a prep_fn that it may use for any
 * purpose.
 * @param similar_fn A pointer to a \ref dym_prep_fn_t function to use.
 * @param cleanup_fn A pointer to a function to clean-up the \ref
 * did_you_mean::user_data "user_data" member of a \ref did_you_mean structure.
 * May be NULL.
 * @return Returns a pointer to an array of elements terminated by one having a
 * NULL \ref did_you_mean::literal "literal" if there are suggestions or NULL
 * if not.  The caller is responsible for calling dym_free().
 *
 * @sa dym_free()
 */
NODISCARD
did_you_mean_t const* dym_new( char const *unknown_literal,
                               dym_prep_fn_t prep_fn, void *prep_data,
                               dym_similar_fn_t similar_fn,
                               dym_cleanup_fn_t cleanup_fn );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* pjl_did_you_mean_H */
/* vim:set et sw=2 ts=2: */
