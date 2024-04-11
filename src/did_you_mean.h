/*
**      cdecl -- C gibberish translator
**      src/did_you_mean.h
**
**      Copyright (C) 2020-2024  Paul J. Lucas
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
};
typedef struct did_you_mean did_you_mean_t;

/**
 * Kinds of things one might have meant.
 *
 * A bitwise-or of these may be used to specify more than one kind.
 */
enum dym_kind {
  DYM_NONE            = 0,              ///< Did you mean nothing.
  DYM_COMMANDS        = 1 << 0,         ///< Did you mean **cdecl** _command_?
  DYM_CLI_OPTIONS     = 1 << 1,         ///< Did you mean _CLI option_?
  DYM_SET_OPTIONS     = 1 << 2,         ///< Did you mean _set option_?
  DYM_C_ATTRIBUTES    = 1 << 3,         ///< Did you mean C/C++ _attribute_?
  DYM_C_KEYWORDS      = 1 << 4,         ///< Did you mean C/C++ _keyword_?
  DYM_C_MACROS        = 1 << 5,         ///< Did you mean C/C++ _macro_?
  DYM_C_TYPES         = 1 << 6,         ///< Did you mean C/C++ _type_?
  DYM_CDECL_KEYWORDS  = 1 << 7,         ///< Did you mean **cdecl** _keyword_?
};
typedef enum dym_kind dym_kind_t;

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
 * ...?" literals for \a unknown_literal.
 *
 * @param kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param unknown_literal The unknown literal.
 * @return Returns a pointer to an array of elements terminated by one having a
 * NULL `literal` pointer if there are suggestions or NULL if not.  The caller
 * is responsible for calling dym_free().
 *
 * @sa dym_free()
 */
NODISCARD
did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_literal );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_did_you_mean_H */
/* vim:set et sw=2 ts=2: */
