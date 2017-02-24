/*
**      cdecl -- C gibberish translator
**      src/set.c
*/

// local
#include "config.h"                     /* must go first */
#include "options.h"

// standard
#include <stdio.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

// external variables
extern char const  *prompt;
extern char         prompt_buf[];

////////// extern functions ///////////////////////////////////////////////////

void set_option( char const *opt ) {
  if ( strcmp( opt, "create" ) == 0 )
    opt_make_c = true;
  else if ( strcmp( opt, "nocreate" ) == 0 )
    opt_make_c = false;

  else if ( strcmp( opt, "prompt" ) == 0 )
    prompt = prompt_buf;
  else if ( strcmp( opt, "noprompt" ) == 0 )
    prompt = "";

  else if ( strcmp( opt, "preansi" ) == 0 || strcmp( opt, "knr" ) == 0 )
    opt_lang = LANG_C_KNR;
  else if ( strcmp( opt, "ansi" ) == 0 )
    opt_lang = LANG_C_89;
  else if ( strcmp( opt, "c++" ) == 0 )
    opt_lang = LANG_CPP;
  else if ( strcmp( opt, "c++11" ) == 0 )
    opt_lang = LANG_CPP_11;

#ifdef WITH_CDECL_DEBUG
  else if ( strcmp( opt, "debug" ) == 0 )
    opt_debug = true;
  else if ( strcmp( opt, "nodebug" ) == 0 )
    opt_debug = false;
#endif /* WITH_CDECL_DEBUG */

#ifdef YYDEBUG
  else if ( strcmp( opt, "yydebug" ) == 0 )
    yydebug = true;
  else if ( strcmp( opt, "noyydebug" ) == 0 )
    yydebug = false;
#endif /* YYDEBUG */

  else {
    if ( strcmp( opt, "options" ) != 0 ) {
      printf( "\"%s\": unknown set option\n", opt );
    }
    printf( "Valid set options (and command line equivalents) are:\n" );
    printf( "  options\n" );
    printf( "  create (-c), nocreate\n" );
    printf( "  prompt, noprompt (-q)\n" );
#ifndef WITH_READLINE
    printf( "  interactive (-i), nointeractive\n" );
#endif /* WITH_READLINE */
    printf( "  preansi (-p), ansi (-a), or cplusplus (-+)\n" );
#ifdef WITH_CDECL_DEBUG
    printf( "  debug (-d), nodebug\n" );
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
    printf( "  yydebug (-D), noyydebug\n" );
#endif /* YYDEBUG */
    printf( "\nCurrent set values are:\n" );
    printf( "  %screate\n", opt_make_c ? "   " : " no" );
    printf( "  %sinteractive\n", opt_interactive ? "   " : " no" );
    printf( "  %sprompt\n", prompt[0] ? "   " : " no" );
    printf( "  lang=%s\n", lang_name( opt_lang ) );
#ifdef WITH_CDECL_DEBUG
    printf( "  %sdebug\n", opt_debug ? "   " : " no" );
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
    printf( "  %syydebug\n", yydebug ? "   " : " no" );
#endif /* YYDEBUG */
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
