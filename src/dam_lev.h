/*
**      cdecl -- C gibberish translator
**      src/dam_lev.h
**
**      Copyright (C) 2020-2021  Paul J. Lucas, et al.
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

#ifndef cdecl_dam_lev_H
#define cdecl_dam_lev_H

/**
 * @file
 * Declares a function for calculating an _edit distance_ between two strings.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stddef.h>                     /* for size_t */

///////////////////////////////////////////////////////////////////////////////

typedef size_t dam_lev_t;               ///< Damerau-Levenshtein edit distance.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Implements the Damerau-Levenshtein algorithm to calculate an _edit distance_
 * between strings, i.e., how many letters need to be transposed within,
 * substituted within, deleted from, or added to \a source to get \a target.
 *
 * @param source The source string.
 * @param target The target string.
 * @return Returns said distance.
 *
 * @sa [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau–Levenshtein_distance)
 * @sa [Damerau–Levenshtein Edit Distance Explained](https://www.lemoda.net/text-fuzzy/damerau-levenshtein/)
 */
dam_lev_t dam_lev_dist( char const *source, char const *target );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_dam_lev_H */
/* vim:set et sw=2 ts=2: */
