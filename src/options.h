/*
**      cdecl -- C gibberish translator
**      src/options.h
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

#ifndef cdecl_options_H
#define cdecl_options_H

/**
 * @file
 * Declares global variables and functions for command-line options.
 */

// local
#include "config.h"                     /* must go first */
#include "c_lang.h"                     /* for c_lang_t */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup options-group Options
 * Declares global variables and functions for command-line options.
 * @{
 */

// extern option variables
extern char const  *opt_conf_file;      ///< Configuration file path.
#ifdef ENABLE_CDECL_DEBUG
extern bool         opt_debug;          ///< Generate JSON-like debug output?
#endif /* ENABLE_CDECL_DEBUG */
extern char const  *opt_fin;            ///< File in path.
extern char const  *opt_fout;           ///< File out path.
extern bool         opt_interactive;    ///< Interactive mode?
extern c_lang_t     opt_lang;           ///< Current language.
extern bool         opt_no_conf;        ///< Do not read configuration file.
extern bool         opt_semicolon;      ///< Print `;` at end of gibberish.
extern bool         opt_typedefs;       ///< Load C 99 standard `typedef`s.
extern bool         opt_quiet;          ///< Don't print the prompt.

// other extern variables
extern FILE        *fin;                ///< File in.
extern FILE        *fout;               ///< File out.
#ifdef YYDEBUG
extern int          yydebug;            ///< Bison debugging.
#endif /* YYDEBUG */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Initializes command-line option variables.
 * On return, `*pargc` and `*pargv` are updated to reflect the remaining
 * command-line with the options removed.
 *
 * @param pargc A pointer to the argument count from `main()`.
 * @param pargv A pointer to the argument values from `main()`.
 */
void options_init( int *pargc, char const ***pargv );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_options_H */
/* vim:set et sw=2 ts=2: */
