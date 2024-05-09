/*
**      cdecl -- C gibberish translator
**      src/print.h
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for FILE */

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
  bool        opt_no_print_input_line;  ///< Don't print input line before `^`.
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
 * + A macro, prints `"(\"___\" is a macro)"` where `___` is \a error_token.
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
 * @param error_token The current error token. May be NULL.
 */
void print_error_token_is_a( char const *error_token );

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

///////////////////////////////////////////////////////////////////////////////

/** @} */

/**
 * @defgroup printing-c-types-group Printing C/C++ Types
 * Functions for printing C/C++ types.
 *
 * @sa \ref showing-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints the name of the kind of \a ast (or of the underlying AST, if any); if
 * \a ast is of kind #K_TYPEDEF, also prints `type` followed by `(aka` followed
 * by the underlying type in either pseudo-English or gibberish (depending on
 * how it was declared).
 *
 * For example, if a type was declared in pseudo-English like:
 *
 *      define RI as reference to int
 *
 * prints `type "RI" (aka "reference to integer")`, that is the type name
 * followed by `(aka` and the underlying type in pseudo-English.
 *
 * However, if the underlying type was declared in gibberish like:
 *
 *      using RI = int&
 *
 * prints `type "RI" (aka "int&")`, that is the type name followed by `(aka`
 * and the underlying type in gibberish.
 *
 * @note A newline is _not_ printed.
 *
 * @param ast The \ref c_ast to print.
 * @param fout The `FILE` to print to.
 *
 * @sa print_ast_type_aka()
 */
void print_ast_kind_aka( c_ast_t const *ast, FILE *fout );

/**
 * If \a ast is:
 *
 *  + A #K_TYPEDEF, prints the name of the type followed by `(aka` followed by
 *    the underlying type in either pseudo-English or gibberish (depending on
 *    how it was declared).  For example, if a type was declared in pseudo-
 *    English like:
 *
 *          define RI as reference to int
 *
 *    prints `"RI" (aka "reference to integer")`, that is the type name
 *    followed by `(aka` and the underlying type in pseudo-English.
 *
 *    However, if the underlying type was declared in gibberish like:
 *
 *          using RI = int&
 *
 *    prints `"RI" (aka "int&")`, that is the type name followed by `(aka` and
 *    the underlying type in gibberish.
 *
 *  + Otherwise prints only the type of \a ast either in pseudo-English or
 *    gibberish without its name, for example `"reference to integer"` or
 *    `"int&"`.
 *
 * @note A newline is _not_ printed.
 *
 * @param ast The \ref c_ast to print.
 * @param fout The `FILE` to print to.
 *
 * @sa c_ast_english()
 * @sa c_ast_gibberish()
 * @sa c_typedef_english()
 * @sa c_typedef_gibberish()
 * @sa opt_english_types
 * @sa print_ast_kind_aka()
 * @sa print_type_ast()
 * @sa print_type_decl()
 * @sa show_type()
 */
void print_ast_type_aka( c_ast_t const *ast, FILE *fout );

/**
 * Prints _only_ the underlying type of \a tdef either as pseudo-English via
 * c_ast_english() or gibberish via c_ast_gibberish() depending on \a tdef's
 * \ref c_typedef::decl_flags "decl_flags".
 *
 * @note A newline is _not_ printed.
 *
 * @param tdef The \ref c_typedef whose \ref c_typedef::ast "ast" to print.
 * @param fout The `FILE` to print to.
 *
 * @sa c_ast_english()
 * @sa c_ast_gibberish()
 * @sa c_typedef_english()
 * @sa c_typedef_gibberish()
 * @sa print_ast_type_aka()
 * @sa print_type_decl()
 * @sa show_type()
 */
void print_type_ast( c_typedef_t const *tdef, FILE *fout );

/**
 * Prints \a tdef as a full type declaration either in pseudo-English via
 * c_typedef_english() or gibberish via c_typedef_gibberish() according to \a
 * decl_flags.
 *
 * @note A newline is _not_ printed.
 *
 * @param tdef The \ref c_typedef to print.
 * @param decl_flags The declaration flags to use (overriding \a tdef's \ref
 * c_typedef::decl_flags "decl_flags").
 * @param fout The `FILE` to print to.
 *
 * @sa c_typedef_english()
 * @sa c_typedef_gibberish()
 * @sa print_ast_type_aka()
 * @sa print_type_ast()
 * @sa show_type()
 */
void print_type_decl( c_typedef_t const *tdef, unsigned decl_flags,
                      FILE *fout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_print_H */
/* vim:set et sw=2 ts=2: */
