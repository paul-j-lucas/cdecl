/*
**      cdecl -- C gibberish translator
**      src/cdecl.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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

#ifndef cdecl_cdecl_H
#define cdecl_cdecl_H

/**
 * @file
 * Includes platform configuration information in the right order.  Always
 * `#include` this file rather than `config.h` directly.  Additionally declares
 * miscellaneous macros and global variables.
 */

/// @cond DOXYGEN_IGNORE

#ifdef __APPLE__
//
// From config.h:
//
//    Suppress extern inline (with or without __attribute__ ((__gnu_inline__)))
//    on configurations that mistakenly use 'static inline' to implement
//    functions or macros in standard C headers like <ctype.h>.  For example,
//    if isdigit is mistakenly implemented via a static inline function, a
//    program containing an extern inline function that calls isdigit may not
//    work since the C standard prohibits extern inline functions from calling
//    static functions (ISO C 99 section 6.7.4.(3).  This bug is known to occur
//    on:
//
//      OS X 10.8 and earlier; see:
//      https://lists.gnu.org/r/bug-gnulib/2012-12/msg00023.html
//
//      ...
//
//    MacOS X 10.9 and later define __header_inline indicating the bug is fixed
//    for C and for clang but remains for g++; see
//    <https://trac.macports.org/ticket/41033>.
//
// Mac OS X 10.9 and later define __header_inline in sys/cdefs.h that is
// included from stdlib.h, so include the latter to define it if applicable.
// This MUST be #include'd before config.h since it tests for __header_inline.
//
# include <stdlib.h>
#endif

/// @endcond

// local
#include "config.h"                     /* must go first */
#include "typedefs.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/** Default configuration file name. */
#define CONF_FILE_NAME_DEFAULT    "." PACKAGE "rc"

/** Program name when composing or deciphering C++. */
#define CPPDECL                   "c++decl"

// extern variables
extern c_mode_t     c_mode;             ///< Parsing english or gibberish?
extern c_init_t     c_init;             ///< Initialization state.
extern char const  *command_line;       ///< Command from command line, if any.
extern size_t       command_line_len;   ///< Length of `command_line`.
extern bool         is_input_a_tty;     ///< Is our input from a TTY?
extern char const  *me;                 ///< Program name.

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_cdecl_H */
/* vim:set et sw=2 ts=2: */
