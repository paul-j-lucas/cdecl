/*
**      cdecl -- C gibberish translator
**      src/options.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_options_H
#define cdecl_options_H

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
extern bool         opt_make_c;
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
 *
 * @param argc The argument count from \c main().
 * @param argv The argument values from \c main().
 */
void options_init( int argc, char const *argv[] );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_options_H */
/* vim:set et sw=2 ts=2: */
