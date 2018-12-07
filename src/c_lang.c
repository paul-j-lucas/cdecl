/*
**      cdecl -- C gibberish translator
**      src/c_lang.c
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

/**
 * @file
 * Defines functions for C/C++ language versions.
 */

// local
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_LANG_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_lang.h"
#include "options.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern constant definitions
/// @hideinitializer
c_lang_t const C_LANG[] = {
  { "ck&r",    LANG_C_KNR   },          // synonym for "knr"
  { "cknr",    LANG_C_KNR   },          // synonym for "knr"
  { "k&r",     LANG_C_KNR   },          // synonym for "knr"
  { "k&rc",    LANG_C_KNR   },          // synonym for "knr"
  { "knr",     LANG_C_KNR   },
  { "knrc",    LANG_C_KNR   },          // synonym for "knr"
  { "c",       LANG_C_NEW   },
  { "c89",     LANG_C_89,   },
  { "c95",     LANG_C_95    },
  { "c99",     LANG_C_99    },
  { "c11",     LANG_C_11    },
  { "c18",     LANG_C_18    },
  { "c++",     LANG_CPP_NEW },
  { "c++98",   LANG_CPP_98  },
  { "c++03",   LANG_CPP_03  },
  { "c++11",   LANG_CPP_11  },
  { "c++14",   LANG_CPP_14  },
  { "c++17",   LANG_CPP_17  },
  { "c++20",   LANG_CPP_20  },
  { NULL,      LANG_NONE    },
};

////////// extern functions ///////////////////////////////////////////////////

c_lang_id_t c_lang_find( char const *name ) {
  assert( name != NULL );

  // the list is small, so linear search is good enough
  for ( c_lang_t const *lang = C_LANG; lang->name != NULL; ++lang ) {
    if ( strcasecmp( name, lang->name ) == 0 )
      return lang->lang_id;
  } // for

  return LANG_NONE;
}

char const* c_lang_name( c_lang_id_t lang_id ) {
  switch ( lang_id ) {
    case LANG_NONE  : return "";
    case LANG_C_KNR : return "K&R C";
    case LANG_C_89  : return "C89";
    case LANG_C_95  : return "C95";
    case LANG_C_99  : return "C99";
    case LANG_C_11  : return "C11";
    case LANG_C_18  : return "C18";
    case LANG_CPP_98: return "C++98";
    case LANG_CPP_03: return "C++03";
    case LANG_CPP_11: return "C++11";
    case LANG_CPP_14: return "C++14";
    case LANG_CPP_17: return "C++17";
    case LANG_CPP_20: return "C++20";
    default:
      INTERNAL_ERR( "\"%d\": unexpected value for lang\n", (int)lang_id );
  } // switch
}

void c_lang_set( c_lang_id_t lang_id ) {
  assert( exactly_one_bit_set( lang_id ) );
  opt_lang = lang_id;

  bool const prompt_enabled =
    prompt[0] == NULL       // first time: cdecl_prompt_init() not called yet
    || *prompt[0] != '\0';

  cdecl_prompt_init();      // change prompt based on new language
  cdecl_prompt_enable( prompt_enabled );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
