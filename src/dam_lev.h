/*
**      PJL Library
**      src/dam_lev.h
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

#ifndef pjl_dam_lev_H
#define pjl_dam_lev_H

/**
 * @file
 * Declares a function for calculating an _edit distance_ between two strings.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stddef.h>                     /* for size_t */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Calculates the _Damerau-Levenshtein distance_ between two strings, that is
 * the number of letters that need to be transposed within, substituted within,
 * deleted from, or added to \a source to get \a target.
 *
 * @param working_mem A pointer to working memory returned by dam_lev_new().
 * @param source The source string.
 * @param source_len The length of \a source.
 * @param target The target string.
 * @param target_len The length of \a target.
 * @return Returns said distance.
 *
 * @sa [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau–Levenshtein_distance)
 * @sa [Damerau–Levenshtein Edit Distance Explained](https://www.lemoda.net/text-fuzzy/damerau-levenshtein/)
 */
NODISCARD
size_t dam_lev_dist( void *working_mem, char const *source, size_t source_len,
                     char const *target, size_t target_len );

/**
 * Allocates working memory for use with subsequent calls of dam_lev_dist().
 *
 * @remarks Typical use involves calling dam_lev_dist() in a loop to calculate
 * the edit distances between an unknown word and a set of words the user might
 * have meant, then sorting by distance.  This function allows the allocation
 * and deallocation of the temporary working memory used by dam_lev_dist() to
 * be hoisted out of the loop.
 *
 * @param max_source_len The maximum length of all source strings that will be
 * passed to subsequent calls of dam_lev_dist().
 * @param max_target_len The maximum length of all target strings that will be
 * passed to subsequent calls of dam_lev_dist().
 * @return Returns said working memory.  The caller is responsible for calling
 * **free**(3) on it.
 */
NODISCARD
void* dam_lev_new( size_t max_source_len, size_t max_target_len );

///////////////////////////////////////////////////////////////////////////////

#endif /* pjl_dam_lev_H */
/* vim:set et sw=2 ts=2: */
