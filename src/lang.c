/*
**      cdecl -- C gibberish translator
**      src/lang.c
*/

// local
#include "config.h"                     /* must go first */
#include "lang.h"
#include "util.h"

// system
#include <stdlib.h>

////////// extern functions ///////////////////////////////////////////////////

char const* lang_name( lang_t lang ) {
  switch ( lang ) {
    case LANG_NONE  : return "";
    case LANG_C_KNR : return "K&R C";
    case LANG_C_89  : return "C89";
    case LANG_C_95  : return "C95";
    case LANG_C_99  : return "C99";
    case LANG_C_11  : return "C11";
    case LANG_CPP   : return "C++";
    case LANG_CPP_11: return "C++11";
    default:
      INTERNAL_ERR( "\"%d\": unexpected value for lang\n", (int)lang );
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
