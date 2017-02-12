/*
**      cdecl -- C gibberish translator
**      src/options.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_options_H
#define cdecl_options_H

// local
#include "config.h"

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

///////////////////////////////////////////////////////////////////////////////

/**
 * Language.
 */
enum lang {
  LANG_NONE   = '0',
  LANG_C_KNR  = 'K',
  LANG_C_ANSI = 'A',                    // aka, C89
  LANG_C_99   = '9',
  LANG_CXX    = '+',
};
typedef enum lang lang_t;

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

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the corresponding name of the long option for the given short option.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns the said option.
 */
char const* get_long_opt( char short_opt );

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
