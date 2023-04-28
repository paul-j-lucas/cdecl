/*
**      cdecl -- C gibberish translator
**      src/options.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas, et al.
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

#ifndef cdecl_options_H
#define cdecl_options_H

/**
 * @file
 * Declares global variables and functions for **cdecl** options.
 *
 * @sa cli_options.h
 * @sa set_options.h
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_kind.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup cdecl-options-group Cdecl Options
 * Global variables and functions for **cdecl** options.
 *
 * @sa \ref cli-options-group
 * @sa \ref set-options-group
 *
 * @{
 */

// extern option variables
extern bool         opt_alt_tokens;     ///< Print alternative tokens?
#ifdef YYDEBUG
                    /// Print Bison debug output?
#define             opt_bison_debug     yydebug
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
extern bool         opt_cdecl_debug;    ///< Print JSON-like debug output?
#endif /* ENABLE_CDECL_DEBUG */
extern char const  *opt_conf_path;      ///< Configuration file path.
extern bool         opt_east_const;     ///< Print in "east const" form?
extern bool         opt_echo_commands;  ///< Echo commands?
extern bool         opt_english_types;  ///< Print types in English, not C/C++.
extern bool         opt_explain;        ///< Assume `explain` if no command?
extern c_tid_t      opt_explicit_ecsu;  ///< Explicit `class`|`struct`|`union`?
#ifdef ENABLE_FLEX_DEBUG
                    /// Print Flex debug output?
#define             opt_flex_debug      yy_flex_debug
#endif /* ENABLE_FLEX_DEBUG */
extern c_graph_t    opt_graph;          ///< Di/Trigraph mode.
extern c_lang_id_t  opt_lang;           ///< Current language.
extern bool         opt_prompt;         ///< Print the prompt?
extern bool         opt_read_conf;      ///< Read configuration file?
extern bool         opt_semicolon;      ///< Print `;` at end of gibberish?
extern bool         opt_trailing_ret;   ///< Print trailing return type?
extern bool         opt_typedefs;       ///< Load C/C++ standard `typedef`s?
extern bool         opt_using;          ///< Print `using` in C++11 and later?
extern c_ast_kind_t opt_west_kinds;     ///< Kinds to print `*` and `&` "west."

// other extern variables
#ifdef ENABLE_FLEX_DEBUG
extern int          yy_flex_debug;      ///< Flex variable for debugging.
#endif /* ENABLE_FLEX_DEBUG */
#ifdef YYDEBUG
extern int          yydebug;            ///< Bison variable for debugging.
#endif /* YYDEBUG */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks if any explicit `int` is set.
 *
 * @return Returns `true` if at least one explicit `int` is set.
 *
 * @sa is_explicit_int()
 * @sa parse_explicit_int()
 * @sa print_explicit_int()
 */
NODISCARD
bool any_explicit_int( void );

/**
 * Checks whether \a btids shall have `int` be printed explicitly for it.
 *
 * @param btids The integer type to check.
 * @return Returns `true` only if the type given by \a btid shall have `int`
 * printed explicitly.
 *
 * @sa any_explicit_int()
 * @sa parse_explicit_int()
 * @sa print_explicit_int()
 */
NODISCARD
bool is_explicit_int( c_tid_t btids );

/**
 * Parses the explicit `enum`, `class`, `struct`, `union` option.
 *
 * @param ecsu_format The null-terminated explicit `enum`, `class`, `struct`,
 * `union` format string (case insensitive) to parse.  Valid formats are:
 *      Format | Meaning
 *      -------|--------
 *      `e`    | `enum`
 *      `c`    | `class`
 *      `s`    | `struct`
 *      `u`    | `union`
 * Multiple formats may be given, one immediately after the other, e.g., `su`
 * means `struct` and `union`.
 * @return Returns `true` only if \a ecsu_format was parsed successfully.
 *
 * @sa print_explicit_ecsu()
 */
NODISCARD
bool parse_explicit_ecsu( char const *ecsu_format );

/**
 * Parses the explicit `int` option.
 *
 * @param ei_format The null-terminated explicit `int` format string (case
 * insensitive) to parse.  Valid formats are:
 *      Format                    | Meaning
 *      --------------------------|----------------------------
 *         `i`                    | All signed integer types.
 *      `u`                       | All unsigned integer types.
 *      [`u`]{`i`\|`s`\|`l`[`l`]} | Possibly `unsigned` `int`, `short`, `long`, or `long long`.
 * Multiple formats may be given, one immediately after the other, e.g., `usl`
 * means `unsigned short` and `long`.  Parsing is greedy so commas may be used
 * to separate formats.  For example, `ulll` is parsed as `unsigned long long`
 * and `long` whereas `ul,ll` is parsed as `unsigned long` and `long long`.  If
 * invalid, an error message is printed to standard error.
 * @return Returns `true` only if \a ei_format was parsed successfully.
 *
 * @sa any_explicit_int()
 * @sa is_explicit_int()
 * @sa print_explicit_int()
 */
NODISCARD
bool parse_explicit_int( char const *ei_format );

/**
 * Parses the `west-pointer` option.
 *
 * @param wp_format The null-terminated west pointer format string to parse.
 * Valid formats are:
 *      Format | Meaning
 *      -------|-----------------------------------
 *      `b`    | Apple block return type.
 *      `f`    | Function (and pointer to function) return rype.
 *      `l`    | User-defined literal return type.
 *      `o`    | Operator return type.
 *      `r`    | All return types (same as `bflo`).
 *      `t`    | Non-return types.
 *      `*`    | All types (same as `bflot`).
 * Multiple formats may be given, one immediately after the other.
 * @return Returns `true` only if \a wp_format was parsed successfully.
 *
 * @sa print_west_pointer()
 */
NODISCARD
bool parse_west_pointer( char const *wp_format );

/**
 * Prints the string representation of the explicit `enum`, `class`, `struct`,
 * `union` option.
 *
 * @param out The `FILE` to print to.
 *
 * @sa parse_explicit_ecsu()
 */
void print_explicit_ecsu( FILE *out );

/**
 * Prints the string representation of the explicit integer option.
 *
 * @param out The `FILE` to print to.
 *
 * @sa any_explicit_int()
 * @sa is_explicit_int()
 * @sa parse_explicit_int()
 */
void print_explicit_int( FILE *out );

/**
 * Prints the string representation of the west pointer option.
 *
 * @param out The `FILE` to print to.
 *
 * @sa parse_west_pointer()
 */
void print_west_pointer( FILE *out );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_options_H */
/* vim:set et sw=2 ts=2: */
