/*
**      cdecl -- C gibberish translator
**      src/print.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

#ifndef cdecl_print_H
#define cdecl_print_H

/**
 * @file
 * Declares functions for printing error and warning messages.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "did_you_mean.h"
#include "types.h"                      /* for c_loc_t */

/**
 * @defgroup printing-errors-warnings-group Printing Hints, Errors, & Warnings
 * Functions for printing hints, errors, suggestions, and warning messages.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints an error message to standard error _not_ including a newline.
 * In debug mode, also prints the file & line where the function was called
 * from.
 *
 * @param ... The arguments for fl_print_error().
 *
 * @sa fl_print_error()
 * @sa print_error_unknown_name()
 */
#define print_error(...) \
  fl_print_error( __FILE__, __LINE__, __VA_ARGS__ )

/**
 * Prints an "unknown <thing>" error message possibly followed by "did you mean
 * ...?" for types possibly meant.
 * In debug mode, also prints the file & line where the function was called
 * from.
 *
 * @param LOC The location of \a SNAME.
 * @param SNAME The unknown name.
 *
 * @sa fl_print_error_unknown_name()
 * @sa print_error()
 */
#define print_error_unknown_name(LOC,SNAME) \
  fl_print_error_unknown_name( __FILE__, __LINE__, (LOC), (SNAME) )

/**
 * Prints an warning message to standard error _not_ including a newline.
 * In debug mode, also prints the file & line where the function was called
 * from.
 *
 * @param ... The arguments for fl_print_warning().
 *
 * @sa fl_print_warning()
 */
#define print_warning(...) \
  fl_print_warning( __FILE__, __LINE__, __VA_ARGS__ )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints an error message to standard error _not_ including a newline.
 * In debug mode, also prints the file & line where the function was called
 * from.
 *
 * @note
 * This function isn't normally called directly; use the #print_error() macro
 * instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of the error; may be NULL.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa #print_error()
 */
PJL_PRINTF_LIKE_FUNC(4)
void fl_print_error( char const *file, int line, c_loc_t const *loc,
                     char const *format, ... );

/**
 * Prints an "unknown <thing>" error message possibly followed by "did you mean
 * ...?" suggestions for things possibly meant.
 * In debug mode, also prints the file & line where the function was called
 * from.
 *
 * @note
 * This function isn't normally called directly; use the
 * #print_error_unknown_name() macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of \a sname.
 * @param sname The unknown name.
 *
 * @sa fl_print_error()
 * @sa print_error_unknown_name()
 * @sa print_suggestions()
 */
void fl_print_error_unknown_name( char const *file, int line,
                                  c_loc_t const *loc, c_sname_t const *sname );

/**
 * Prints a warning message to standard error _not_ including a newline.
 * In debug mode, also prints the file & line where the function was called
 * from.
 *
 * @note
 * This function isn't normally called directly; use the #print_warning() macro
 * instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of the warning; may be NULL.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa #print_warning()
 */
PJL_PRINTF_LIKE_FUNC(4)
void fl_print_warning( char const *file, int line, c_loc_t const *loc,
                       char const *format, ... );

/**
 * If <code>\ref opt_cdecl_debug</code> is compiled in and enabled, prints \a
 * file and \a line to standard error in the form `"[<file>:<line>] "`;
 * otherwise prints nothing.
 *
 * @note A newline is _not_ printed.
 *
 * @param file The name of the file to print.
 * @param line The line number within \a file to print.
 */
void print_debug_file_line( char const *file, int line );

/**
 * Prints a hint message to standard error in the form:
 * @code
 * ; did you mean ...?\n
 * @endcode
 * where `...` is the hint.
 *
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa print_suggestions()
 */
PJL_PRINTF_LIKE_FUNC(1)
void print_hint( char const *format, ... );

/**
 * Prints the location of the error including:
 *
 *  + The error line (if neither a TTY nor interactive).
 *  + A `^` (in color, if possible and requested) under the offending token.
 *  + The error column.
 *
 * @note
 * A newline is _not_ printed.
 *
 * @param loc The location to print.
 */
void print_loc( c_loc_t const *loc );

/**
 * If there is at least one "similar enough" suggestion for what \a
 * unknown_token might have meant, prints a message to standard error in the
 * form:
 * @code
 * ; did you mean ...?
 * @endcode
 * where `...` is a a comma-separated list of one or more suggestions.  If
 * there are no suggestions that are "similar enough," prints nothing.
 *
 * @note
 * A newline is _not_ printed.
 *
 * @param kinds The bitwise-or of the kind(s) of things possibly meant by \a
 * unknown_token.
 * @param unknown_token The unknown token.
 *
 * @sa print_hint()
 */
void print_suggestions( dym_kind_t kinds, char const *unknown_token );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_print_H */
/* vim:set et sw=2 ts=2: */
