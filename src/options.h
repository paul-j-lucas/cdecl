/*
**      cdecl -- C gibberish translator
**      src/options.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_options_H
#define cdecl_options_H

/**
 * @file
 * Contains global variables and functions for command-line options.
 */

// local
#include "config.h"                     /* must go first */
#include "lang.h"                       /* for lang_t */

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

///////////////////////////////////////////////////////////////////////////////

// extern option variables
extern bool         opt_debug;
extern char const  *opt_fin;
extern char const  *opt_fout;
extern bool         opt_interactive;
extern lang_t       opt_lang;
extern bool         opt_semicolon;
extern bool         opt_quiet;          // don't print the prompt

// other extern variables
extern FILE        *fin;                // file in
extern FILE        *fout;               // file out
#ifdef YYDEBUG
extern int          yydebug;            // yacc debugging
#endif /* YYDEBUG */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Initializes command-line option variables.
 * On return, \c *pargc and \c *pargv are updated to reflect the remaining
 * command-line with the options removed.
 *
 * @param pargc A pointer to the argument count from \c main().
 * @param pargv A pointer to the argument values from \c main().
 */
void options_init( int *pargc, char const ***pargv );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_options_H */
/* vim:set et sw=2 ts=2: */
