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
  LANG_C_89   = '8',                    // ISO/IEC 9899:1990
  LANG_C_95   = '5',                    // ISO/IEC 9899/AMD1:1995
  LANG_C_99   = '9',                    // ISO/IEC 9899:1999
  LANG_C_11   = '1',                    // ISO/IEC 9899:2011/Cor
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
 * Gets the printable name of the given language.
 *
 * @param lang The language to get the name of.
 * @return Returns said name.
 */
char const* lang_name( lang_t lang );

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
