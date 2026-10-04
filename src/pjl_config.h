/*
**      PJL Library
**      src/pjl_config.h
**
**      Copyright (C) 2018-2026  Paul J. Lucas
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

#ifndef pjl_config_h
#define pjl_config_h

/**
 * @file
 * This file:
 *
 *  + Includes Autotools' `config.h` without generating warnings.
 *  + Includes `attribute.h`.
 *  + Defines `likely` and `unlikely`.
 *  + Defines some additional compiler attributes.
 *
 * Always `#include` this file rather than `config.h` directly.
 */

#ifdef cdecl_config_h
# error "Must #include pjl_config.h instead."
#endif

// local
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include "config.h"
#pragma GCC diagnostic pop

#ifdef VERSION
  // Undefine this since it clashes with our VERSION command-line option.  We
  // don't need this since PACKAGE_VERSION is also defined.
# undef VERSION
#endif /* VERSION */

// standard
#include <attribute.h>

/**
 * Defined to an expression that evaluates to `true` only if we're running
 * genuine GNU **readline**(3) and not some other library emulating it.
 *
 * @remarks Some readline emulators, e.g., editline, have a bug that makes
 * color prompts not work correctly.  So, unless we know we're using genuine
 * GNU readline, use this to disable color prompts.
 *
 * @return Evaluates to `true` only if we're running genuine GNU readline.
 *
 * @sa [The GNU Readline
 * Library](https://tiswww.case.edu/php/chet/readline/rltop.html)
 * @sa http://stackoverflow.com/a/31333315/99089
 */
#if defined(WITH_READLINE) && HAVE_DECL_RL_GNU_READLINE_P
# include <stdio.h>                     /* needed by readline.h */
# include <readline/readline.h>         /* must go after stdio.h */
# define HAVE_GENUINE_GNU_READLINE      (rl_gnu_readline_p == 1)
#endif /* defined(WITH_READLINE) && HAVE_DECL_RL_GNU_READLINE_P */
#ifndef HAVE_GENUINE_GNU_READLINE
# define HAVE_GENUINE_GNU_READLINE      0
#endif /*  HAVE_GENUINE_GNU_READLINE */

////////// compiler attributes ////////////////////////////////////////////////

#ifdef HAVE___BUILTIN_EXPECT

/**
 * Specifies that \a EXPR is _very_ likely (as in 99.99% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
 *
 * @sa #unlikely()
 * @sa [Memory part 5: What programmers can do](http://lwn.net/Articles/255364/)
 */
#define likely(EXPR)              __builtin_expect( !!(EXPR), 1 )

/**
 * Specifies that \a EXPR is _very_ unlikely (as in .01% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
 *
 * @sa #likely()
 * @sa [Memory part 5: What programmers can do](http://lwn.net/Articles/255364/)
 */
#define unlikely(EXPR)            __builtin_expect( !!(EXPR), 0 )

#endif /* HAVE___BUILTIN_EXPECT */

/**
 * Denote that a function's return value may be ignored without warning.
 *
 * @note
 * There is no compiler attribute for this.  It's just a visual cue in code
 * that `NODISCARD` wasn't forgotten.
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

#ifdef HAVE_TYPEOF
/**
 * Discard the return value of a function even if it was declared with
 * `NODISCARD`.
 *
 * @param FN_CALL The function call.
 */
#define PJL_DISCARD_RV(FN_CALL) \
  do { MAYBE_UNUSED typeof(FN_CALL) _rv = (FN_CALL); } while (0)
#endif /* HAVE_TYPEOF */

///////////////////////////////////////////////////////////////////////////////

#ifndef likely
# define likely(EXPR)             (EXPR)
# define unlikely(EXPR)           (EXPR)
#endif /* likely */

#ifndef PJL_DISCARD_RV
# define PJL_DISCARD_RV(FN_CALL)  ((void)(FN_CALL))
#endif /* PJL_DISCARD_RV */

#ifndef PJL_PRINTF_LIKE_FUNC
# define PJL_PRINTF_LIKE_FUNC(N)  /* nothing */
#endif /* PJL_PRINTF_LIKE_FUNC */

///////////////////////////////////////////////////////////////////////////////

#endif /* pjl_config_h */
/* vim:set et sw=2 ts=2: */
