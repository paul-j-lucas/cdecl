/*
**      PJL Library
**      src/pjl_config.h
**
**      Copyright (C) 2018-2024  Paul J. Lucas
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

#ifndef pjl_config_H
#define pjl_config_H

/**
 * @file
 * Includes platform configuration information in the right order.  Always
 * `#include` this file rather than `config.h` directly.
 */

#ifdef cdecl_config_H
#error "Must #include pjl_config.h instead."
#endif /* cdecl_config_H */

// local
#include "config.h"                     /* must go first */
#ifdef VERSION
  // Undefine this since it clashes with our VERSION command-line option.  We
  // don't need this since PACKAGE_VERSION is also defined.
# undef VERSION
#endif /* VERSION */

// standard
#include <attribute.h>

////////// compiler attributes ////////////////////////////////////////////////

/**
 * Denote that a function's return value may be discarded without warning.
 *
 * @note There is no compiler attribute for this.  It's just a visual cue in
 * code that `NODISCARD` wasn't forgotten.
 */
#define PJL_DISCARD               /* nothing */

#ifdef HAVE___ATTRIBUTE__

/**
 * Denote a function declaration takes a `printf`-like format string followed
 * by a variable number of arguments.
 *
 * @param N The position (starting at 1) of the parameter that contains the
 * format string.
 */
#define PJL_PRINTF_LIKE_FUNC(N)   __attribute__((format(printf, (N), (N)+1)))

#endif /* HAVE___ATTRIBUTE__ */

/**
 * Define %PJL_TYPEOF to the supported `typeof` keyword, if any.
 */
#if defined HAVE_TYPEOF
# define PJL_TYPEOF               typeof
#elif defined HAVE___TYPEOF__
# define PJL_TYPEOF               __typeof__
#endif

#ifdef PJL_TYPEOF
/**
 * Discard the return value of a non-`void` function even if it was declared
 * with `NODISCARD`.
 *
 * @param FN_CALL The function call.
 */
#define PJL_DISCARD_RV(FN_CALL) \
  do { MAYBE_UNUSED PJL_TYPEOF(FN_CALL) _rv = (FN_CALL); } while (0)
#endif /* PJL_TYPEOF */

///////////////////////////////////////////////////////////////////////////////

#ifndef PJL_DISCARD_RV
# define PJL_DISCARD_RV(FN_CALL)  ((void)(FN_CALL))
#endif /* PJL_DISCARD_RV */

#ifndef PJL_PRINTF_LIKE_FUNC
# define PJL_PRINTF_LIKE_FUNC(N)  /* nothing */
#endif /* PJL_PRINTF_LIKE_FUNC */

///////////////////////////////////////////////////////////////////////////////

#endif /* pjl_config_H */
/* vim:set et sw=2 ts=2: */
