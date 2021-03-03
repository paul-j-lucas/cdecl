/*
**      cdecl -- C gibberish translator
**      src/c_lang.c
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

/**
 * @file
 * Defines functions for C/C++ language versions.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_LANG_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_lang.h"
#include "cdecl.h"
#include "options.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of `c_lang` for all supported languages. The last entry is
 * `{ NULL, false, LANG_NONE }`.
 */
static c_lang_t const C_LANG[] = {
  { "CK&R",   true,   LANG_C_KNR   },
  { "CKNR",   true,   LANG_C_KNR   },
  { "CKR",    true,   LANG_C_KNR   },
  { "K&R",    true,   LANG_C_KNR   },
  { "K&RC",   false,  LANG_C_KNR   },
  { "KNR",    true,   LANG_C_KNR   },
  { "KNRC",   true,   LANG_C_KNR   },
  { "KR",     true,   LANG_C_KNR   },
  { "KRC",    true,   LANG_C_KNR   },
  { "C",      false,  LANG_C_NEW   },
  { "C89",    false,  LANG_C_89,   },
  { "C90",    true,   LANG_C_89,   },
  { "C95",    false,  LANG_C_95    },
  { "C99",    false,  LANG_C_99    },
  { "C11",    false,  LANG_C_11    },
  { "C17",    false,  LANG_C_17    },
  { "C18",    true,   LANG_C_17    },
  { "C2X",    false,  LANG_C_2X    },
  { "C++",    false,  LANG_CPP_NEW },
  { "C++98",  false,  LANG_CPP_98  },
  { "C++03",  false,  LANG_CPP_03  },
  { "C++11",  false,  LANG_CPP_11  },
  { "C++14",  false,  LANG_CPP_14  },
  { "C++17",  false,  LANG_CPP_17  },
  { "C++20",  false,  LANG_CPP_20  },
  { NULL,     false,  LANG_NONE    },
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

char const* c_lang_literal( c_lang_lit_t const lang_lit[const] ) {
  for ( c_lang_lit_t const *ll = lang_lit; ; ++ll ) {
    if ( (ll->lang_ids & opt_lang) != LANG_NONE )
      return ll->literal;
  } // for
}

char const* c_lang_name( c_lang_id_t lang_id ) {
  assert( exactly_one_bit_set( lang_id & ~LANGX_MASK ) );
  switch ( lang_id ) {
    case LANG_NONE    : return "";
    case LANG_C_KNR   : return "K&RC";
    case LANG_C_89    : return "C89";
    case LANG_C_95    : return "C95";
    case LANG_C_99    : return "C99";
    case LANG_C_99_EMC: return "C99 (with Embedded C extensions)";
    case LANG_C_99_UPC: return "C99 (with Unified Parallel C extensions)";
    case LANG_C_11    : return "C11";
    case LANG_C_17    : return "C17";
    case LANG_C_2X    : return "C2X";
    case LANG_CPP_98  : return "C++98";
    case LANG_CPP_03  : return "C++03";
    case LANG_CPP_11  : return "C++11";
    case LANG_CPP_14  : return "C++14";
    case LANG_CPP_17  : return "C++17";
    case LANG_CPP_20  : return "C++20";
    default:
      UNEXPECTED_INT_VALUE( lang_id );
  } // switch
}

#define LANG_NAME_SIZE_MAX  40          /**< Length of #LANG_C_99_UPC. */

PJL_WARN_UNUSED_RESULT
c_lang_t const* c_lang_next( c_lang_t const *lang ) {
  if ( lang == NULL )
    lang = C_LANG;
  else if ( (++lang)->name == NULL )
    lang = NULL;
  return lang;
}

void c_lang_set( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( exactly_one_bit_set( lang_id ) );
  opt_lang = lang_id;

  bool const prompt_enabled =
    opt_prompt && (
    cdecl_prompt[0] == NULL || // first time: cdecl_prompt_init() not called yet
    cdecl_prompt[0][0] != '\0');

  cdecl_prompt_init();         // change prompt based on new language
  cdecl_prompt_enable( prompt_enabled );
}

char const* c_lang_which( c_lang_id_t lang_ids ) {
  if ( lang_ids == LANG_NONE )
    return "";

  c_lang_id_t const mask = OPT_LANG_IS(C_ANY) ? LANG_MASK_C : LANG_MASK_CPP;

  if ( (lang_ids & mask) == LANG_NONE )
    return OPT_LANG_IS(C_ANY) ? " in C" : " in C++";

  lang_ids &= mask;
  assert( lang_ids != LANG_NONE );

  static char buf[ 7/* until|since */ + LANG_NAME_SIZE_MAX + 1/*\0*/ ];

  c_lang_id_t const oldest_lang_id = c_lang_oldest( lang_ids );
  if ( opt_lang < oldest_lang_id ) {
    sprintf( buf, " until %s", c_lang_name( oldest_lang_id ) );
  } else {
    c_lang_id_t const since_lang_id = c_lang_newest( lang_ids ) << 1;
    assert( since_lang_id != LANG_NONE );
    sprintf( buf, " since %s", c_lang_name( since_lang_id ) );
  }

  return buf;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
