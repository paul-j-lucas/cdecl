/*
**      cdecl -- C gibberish translator
**      src/did_you_mean.h
**
**      Copyright (C) 2020-2022  Paul J. Lucas
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

#ifndef cdecl_did_you_mean_H
#define cdecl_did_you_mean_H

/**
 * @file
 * Declares types, constants, and functions for implementing "Did you mean
 * ...?" suggestions.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stddef.h>                     /* for size_t */

///////////////////////////////////////////////////////////////////////////////

/**
 * Data structure to hold a "Did you mean ...?" token.
 */
struct did_you_mean {
  char const *token;                    ///< Candidate token.
  size_t      dam_lev_dist;             ///< Damerau-Levenshtein edit distance.
};
typedef struct did_you_mean did_you_mean_t;

/**
 * The bitwise-or of kinds of things one might have meant.
 *
 * @sa #DYM_COMMANDS
 * @sa #DYM_CLI_OPTIONS
 * @sa #DYM_SET_OPTIONS
 * @sa #DYM_C_ATTRIBUTES
 * @sa #DYM_C_KEYWORDS
 * @sa #DYM_C_TYPES
 * @sa #DYM_CDECL_KEYWORDS
 */
typedef unsigned dym_kind_t;

#define DYM_NONE            0u          /**< Did you mean nothing. */
#define DYM_COMMANDS        (1u << 0)   /**< Did you mean cdecl _command_? */
#define DYM_CLI_OPTIONS     (1u << 1)   /**< Did you mean _CLI option_? */
#define DYM_SET_OPTIONS     (1u << 2)   /**< Did you mean _set option_? */
#define DYM_C_ATTRIBUTES    (1u << 3)   /**< Did you mean C/C++ _attribute_? */
#define DYM_C_KEYWORDS      (1u << 4)   /**< Did you mean C/C++ _keyword_? */
#define DYM_C_TYPES         (1u << 5)   /**< Did you mean C/C++ _type_? */
#define DYM_CDECL_KEYWORDS  (1u << 6)   /**< Did you mean cdecl _keyword_? */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all memory used by \a dym_array _including_ \a dym_array itself.
 *
 * @param dym_array The \ref did_you_mean array to free.  If NULL, does
 * nothing.
 */
void dym_free( did_you_mean_t const *dym_array );

/**
 * Creates a new array of \ref did_you_mean elements containing "Did you mean
 * ...?" tokens for \a unknown_token.
 *
 * @param kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param unknown_token The unknown token.
 * @return Returns a pointer to an array of elements terminated by one having a
 * NULL `token` pointer if there are suggestions or NULL if not.
 *
 * @sa dym_free()
 */
PJL_WARN_UNUSED_RESULT
did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_token );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_did_you_mean_H */
/* vim:set et sw=2 ts=2: */
