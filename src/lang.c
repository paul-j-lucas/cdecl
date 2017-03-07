/*
**      cdecl -- C gibberish translator
**      src/lang.c
*/

// local
#include "config.h"                     /* must go first */
#include "common.h"
#include "lang.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

struct lang_map {
  char const *name;
  lang_t      lang;
};
typedef struct lang_map lang_map_t;

static lang_map_t const LANG_MAP[] = {
  { "cknr",   LANG_C_KNR    },          // synonym for "knr"
  { "knr",    LANG_C_KNR    },
  { "knrc",   LANG_C_KNR    },          // synonym for "knr"
  { "c",      LANG_C_MAX    },
  { "c89",    LANG_C_89     },
  { "c95",    LANG_C_95     },
  { "c99",    LANG_C_99     },
  { "c11",    LANG_C_11     },
  { "c++",    LANG_CPP_MAX  },
  { "c++98",  LANG_CPP_03   },          // C++03 added no new decl. syntax
  { "c++03",  LANG_CPP_03   },
  { "c++11",  LANG_CPP_11   },
};

////////// extern functions ///////////////////////////////////////////////////

lang_t lang_find( char const *s ) {
  assert( s );
  for ( size_t i = 0; i < ARRAY_SIZE( LANG_MAP ); ++i ) {
    if ( strcasecmp( s, LANG_MAP[i].name ) == 0 )
      return LANG_MAP[i].lang;
  } // for
  return LANG_NONE;
}

char const* lang_name( lang_t lang ) {
  switch ( lang ) {
    case LANG_NONE  : return "";
    case LANG_C_KNR : return "K&R C";
    case LANG_C_89  : return "C89";
    case LANG_C_95  : return "C95";
    case LANG_C_99  : return "C99";
    case LANG_C_11  : return "C11";
    case LANG_CPP_03: return "C++03";
    case LANG_CPP_11: return "C++11";
    default:
      INTERNAL_ERR( "\"%d\": unexpected value for lang\n", (int)lang );
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
