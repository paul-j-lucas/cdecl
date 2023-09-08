/*
**      cdecl -- C gibberish translator
**      src/print.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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

// standard
#include <stdbool.h>
#include <stdio.h>

/**
 * @defgroup printing-errors-warnings-group Printing Hints, Errors, & Warnings
 * Functions for printing hints, errors, suggestions, and warning messages.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints an error message to standard error.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline is _not_ printed.
 *
 * @param ... The `printf()` arguments.
 *
 * @sa fl_print_error()
 * @sa #print_error_unknown_name()
 * @sa #print_warning()
 */
#define print_error(...) \
  fl_print_error( __FILE__, __LINE__, __VA_ARGS__ )

/**
 * Prints an "unknown <thing>" error message possibly followed by "did you mean
 * ...?" for types possibly meant.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline _is_ printed.
 *
 * @param LOC The location of \a SNAME.
 * @param SNAME The unknown name.
 *
 * @sa fl_print_error_unknown_name()
 * @sa #print_error()
 */
#define print_error_unknown_name(LOC,SNAME) \
  fl_print_error_unknown_name( __FILE__, __LINE__, (LOC), (SNAME) )

/**
 * Prints an warning message to standard error.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline is _not_ printed.
 *
 * @param ... The `printf()` arguments.
 *
 * @sa fl_print_warning()
 * @sa #print_error()
 */
#define print_warning(...) \
  fl_print_warning( __FILE__, __LINE__, __VA_ARGS__ )

/**
 * Parameters for the `print_*()` functions that would be too burdonsome to
 * pass to every function call.
 */
struct print_params {
  char const *command_line;             ///< Command from command line, if any.
  size_t      command_line_len;         ///< Length of `command_line`.
  char const *conf_path;                ///< Configuration file path, if any.
  size_t      inserted_len;             ///< Length of inserted string, if any.
};
typedef struct print_params print_params_t;

extern print_params_t print_params;     ///< Print parameters.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints an error message to standard error.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline is _not_ printed.
 * @note This function isn't normally called directly; use the #print_error()
 * macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of the error; may be NULL.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa fl_print_warning()
 * @sa #print_error()
 */
PJL_PRINTF_LIKE_FUNC(4)
void fl_print_error( char const *file, int line, c_loc_t const *loc,
                     char const *format, ... );

/**
 * Prints an "unknown <thing>" error message possibly followed by "did you mean
 * ...?" suggestions for things possibly meant.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline _is_ printed.
 * @note This function isn't normally called directly; use the
 * #print_error_unknown_name() macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of \a sname.
 * @param sname The unknown name.
 *
 * @sa fl_print_error()
 * @sa #print_error_unknown_name()
 * @sa print_suggestions()
 */
void fl_print_error_unknown_name( char const *file, int line,
                                  c_loc_t const *loc, c_sname_t const *sname );

/**
 * Prints a warning message to standard error.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline is _not_ printed.
 * @note This function isn't normally called directly; use the #print_warning()
 * macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of the warning; may be NULL.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa fl_print_error()
 * @sa #print_warning()
 */
PJL_PRINTF_LIKE_FUNC(4)
void fl_print_warning( char const *file, int line, c_loc_t const *loc,
                       char const *format, ... );

/**
 * If \ref opt_cdecl_debug is compiled in and enabled, prints \a file and \a
 * line to standard error in the form `"[<file>:<line>] "`; otherwise prints
 * nothing.
 *
 * @note A newline is _not_ printed.
 *
 * @param file The name of the file to print.
 * @param line The line number within \a file to print.
 */
void print_debug_file_line( char const *file, int line );

/**
 * Prints a hint message to standard error in the form:
 *
 *      ; did you mean ...?
 *
 * where `...` is the hint.
 *
 * @note A newline _is_ printed.
 *
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa print_suggestions()
 */
PJL_PRINTF_LIKE_FUNC(1)
void print_hint( char const *format, ... );

/**
 * If \a error_token is:
 *
 * + A C or C++ keyword:
 *     + And the oldest language in which \a error_token is a keyword is later
 *       than the current language, prints `"; not a keyword until"` followed
 *       by the name of said oldest language.
 *     + Otherwise prints `"(\"___\" is a keyword)"` where `___` is \a
 *       error_token.
 *
 * + A **cdecl** keyword, prints `"(\"___\" is a cdecl keyword)"` where `___`
 *   is \a error_token.
 *
 * + NULL, does nothing.
 *
 * @note A newline is _not_ printed.
 *
 * @param error_token The current error token or NULL.
 */
void print_is_a_keyword( char const *error_token );

/**
 * Prints the location of the error including:
 *
 *  + The error line (if neither a TTY nor interactive).
 *  + A `^` (in color, if possible and requested) under the offending token.
 *  + The error column.
 *
 * @note A newline is _not_ printed.
 *
 * @param loc The location to print.
 */
void print_loc( c_loc_t const *loc );

/**
 * If there is at least one "similar enough" suggestion for what \a
 * unknown_token might have meant, prints a message to standard error in the
 * form:
 *
 *      ; did you mean ...?
 *
 * where `...` is a a comma-separated list of one or more suggestions.  If
 * there are no suggestions that are "similar enough," prints nothing.
 *
 * @note A newline is _not_ printed.
 *
 * @param kinds The bitwise-or of the kind(s) of things possibly meant by \a
 * unknown_token.
 * @param unknown_token The unknown token.
 * @return Returns `true` only if any suggestions were printed.
 *
 * @sa print_hint()
 */
PJL_DISCARD
bool print_suggestions( dym_kind_t kinds, char const *unknown_token );

/**
 * Prints the type \a tdef how it was defined:
 *
 *  + If it was defined using pseudo-English, prints it as pseudo-English;
 *
 *  + If it was defined using `using`, \ref opt_using is `true`, and the
 *    current language is C++11 or later, prints it as a `using` declaration;
 *
 *  + Otherwise, prints it as gibberish.
 *
 * @note A newline _is_ printed.
 *
 * @param tdef The \ref c_typedef to print.
 * @param tout The `FILE` to print to.
 *
 * @sa c_typedef_english()
 * @sa c_typedef_gibberish()
 * @sa show_type()
 */
void print_type( c_typedef_t const *tdef, FILE *tout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_print_H */
/* vim:set et sw=2 ts=2: */
