/*
**      cdecl -- C gibberish translator
**      src/options.h
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

#ifndef cdecl_options_H
#define cdecl_options_H

/**
 * @file
 * Declares global variables and functions for command-line options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * Convenience macro for iterating over all cdecl command-line options.
 *
 * @param VAR The `struct option` loop variable.
 *
 * @sa cli_option_next()
 */
#define FOREACH_CLI_OPTION(VAR) \
  for ( struct option const *VAR = NULL; (VAR = cli_option_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup cli-options-group Command-Line Options
 * Declares global variables and functions for command-line options.
 * @{
 */

// extern option variables
extern bool         opt_alt_tokens;     ///< Print alternative tokens?
#ifdef ENABLE_CDECL_DEBUG
extern bool         opt_cdecl_debug;    ///< Print JSON-like debug output?
#endif /* ENABLE_CDECL_DEBUG */
extern char const  *opt_conf_file;      ///< Configuration file path.
extern bool         opt_east_const;     ///< Print in "east const" form?
extern bool         opt_explain;        ///< Assume `explain` if no command?
extern c_graph_t    opt_graph;          ///< Di/Trigraph mode.
extern bool         opt_interactive;    ///< Interactive mode?
extern c_lang_id_t  opt_lang;           ///< Current language.
extern bool         opt_no_conf;        ///< Do not read configuration file.
extern bool         opt_prompt;         ///< Print the prompt?
extern bool         opt_semicolon;      ///< Print `;` at end of gibberish?
extern bool         opt_typedefs;       ///< Load C/C++ standard `typedef`s?

// other extern variables
extern FILE        *fin;                ///< File in.
extern FILE        *fout;               ///< File out.
#ifdef ENABLE_FLEX_DEBUG
extern int          yy_flex_debug;      ///< Flex variable for debugging.
#define opt_flex_debug  yy_flex_debug   ///< Print Flex debug output?
#endif /* ENABLE_FLEX_DEBUG */
#ifdef YYDEBUG
extern int          yydebug;            ///< Bison variable for debugging.
#define opt_bison_debug yydebug         ///< Print Bison debug output?
#endif /* YYDEBUG */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks if any explicit `int` is set.
 *
 * @return Returns `true` if at least one explicit `int` is set.
 *
 * @sa is_explicit_int()
 */
PJL_WARN_UNUSED_RESULT
bool any_explicit_int( void );

/**
 * Checks whether \a tid shall have `int` be printed explicitly for it.
 *
 * @param tid The integer type to check.
 * @return Returns `true` only if the type given by \a tid shall have `int`
 * printed explicitly.
 *
 * @sa any_explicit_int()
 */
PJL_WARN_UNUSED_RESULT
bool is_explicit_int( c_tid_t tid );

/**
 * Iterates to the next cdecl command-line option.
 *
 * @param opt A pointer to the previous option. For the first iteration, NULL
 * should be passed.
 * @return Returns the next command-line option or NULL for none.
 *
 * @sa #FOREACH_CLI_OPTION()
 */
PJL_WARN_UNUSED_RESULT
struct option const* cli_option_next( struct option const *opt );

/**
 * Initializes command-line option variables.
 * On return, `*pargc` and `*pargv` are updated to reflect the remaining
 * command-line with the options removed.
 *
 * @param pargc A pointer to the argument count from main().
 * @param pargv A pointer to the argument values from main().
 */
void options_init( int *pargc, char const **pargv[] );

/**
 * Parses the explicit `int` option.
 *
 * @param ei_format The null-terminated explicit `int` format string to parse.
 * Valid formats are:
 *      Format            | Meaning
 *      ------------------|----------------------------
 *         `i`            | All signed integer types.
 *      `u`               | All unsigned integer types.
 *      [`u`]{`isl`[`l`]} | Possibly `unsigned` `int`, `short`, `long`, or `long long`.
 * Multiple formats may be given, one immediately after the other, e.g., `usl`
 * means `unsigned short` and `long`.  Parsing is greedy so commas may be used
 * to separate formats.  For example, `ulll` is parsed as `unsigned long long`
 * and `long` whereas `ul,ll` is parsed as `unsigned long` and `long long`.
 * If invalid, an error message is printed to standard error.
 * @param loc The location of \a ei_format.  If not NULL and \a ei_format is
 * invalid, calls print_error(); if NULL and \a ei_format is invalid, calls
 * PMESSAGE_EXIT().
 * @return Returns `true` only if \a ei_format was parsed successfully.
 */
bool parse_explicit_int( char const *ei_format, c_loc_t const *loc );

/**
 * Prints the string representation of the explicit integer option.
 *
 * @param out The `FILE` to print to.
 */
void print_explicit_int( FILE *out );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_options_H */
/* vim:set et sw=2 ts=2: */
