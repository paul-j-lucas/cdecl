/*
**      cdecl -- C gibberish translator
**      src/c_lang.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
 * Defines constants and functions for C/C++ language versions.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_LANG_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_lang.h"
#include "cdecl.h"
#include "options.h"
#include "prompt.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <string.h>

/// @endcond

/**
 * @addtogroup c-lang-vers-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of c_lang for all supported languages.
 *
 * @note The last entry is `{ NULL, false, LANG_NONE }`.
 */
static c_lang_t const C_LANG[] = {
  { "C",      false,  LANG_C_NEW   },
  { "CK&R",   true,   LANG_C_KNR   },
  { "CKNR",   true,   LANG_C_KNR   },
  { "CKR",    true,   LANG_C_KNR   },
  { "K&R",    true,   LANG_C_KNR   },
  { "K&RC",   false,  LANG_C_KNR   },
  { "KNR",    true,   LANG_C_KNR   },
  { "KNRC",   true,   LANG_C_KNR   },
  { "KR",     true,   LANG_C_KNR   },
  { "KRC",    true,   LANG_C_KNR   },
  { "C78",    true,   LANG_C_KNR   },
  { "C89",    false,  LANG_C_89,   },
  { "C90",    true,   LANG_C_89,   },
  { "C95",    false,  LANG_C_95    },
  { "C99",    false,  LANG_C_99    },
  { "C11",    false,  LANG_C_11    },
  { "C17",    false,  LANG_C_17    },
  { "C18",    true,   LANG_C_17    },
  { "C23",    false,  LANG_C_23    },
  { "C++",    false,  LANG_CPP_NEW },
  { "C++98",  false,  LANG_CPP_98  },
  { "C++03",  false,  LANG_CPP_03  },
  { "C++11",  false,  LANG_CPP_11  },
  { "C++14",  false,  LANG_CPP_14  },
  { "C++17",  false,  LANG_CPP_17  },
  { "C++20",  false,  LANG_CPP_20  },
  { "C++23",  false,  LANG_CPP_23  },
  { NULL,     false,  LANG_NONE    },
};

////////// extern functions ///////////////////////////////////////////////////

char const* c_lang___cplusplus( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( is_1_bit( lang_id ) );
  switch ( lang_id ) {
    case LANG_CPP_98:                   // Yes, this is correct.
    case LANG_CPP_03: return "199711L"; // And so is this.
    case LANG_CPP_11: return "201103L";
    case LANG_CPP_14: return "201402L";
    case LANG_CPP_17: return "201703L";
    case LANG_CPP_20: return "202002L";
    case LANG_CPP_23: return "202302L";
    default         : return NULL;
  } // switch
}

char const* c_lang___STDC_VERSION__( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( is_1_bit( lang_id ) );
  switch ( lang_id ) {
    case LANG_C_89:                     // Yes, this is correct.
    case LANG_C_95: return "199409L";
    case LANG_C_99: return "199901L";
    case LANG_C_11: return "201112L";
    case LANG_C_17: return "201710L";
    case LANG_C_23: return "202311L";
    default       : return NULL;
  }
}

c_lang_id_t c_lang_find( char const *name ) {
  assert( name != NULL );

  // the list is small, so linear search is good enough
  for ( c_lang_t const *lang = C_LANG; lang->name != NULL; ++lang ) {
    if ( strcasecmp( name, lang->name ) == 0 )
      return lang->lang_id;
  } // for

  return LANG_NONE;
}

char const* c_lang_literal( c_lang_lit_t const *ll ) {
  assert( ll != NULL );
  for ( ; ll->literal != NULL; ++ll ) {
    if ( opt_lang_is_any( ll->lang_ids ) )
      return ll->literal;
  } // for
  return NULL;                          // LCOV_EXCL_LINE
}

char const* c_lang_name( c_lang_id_t lang_id ) {
  assert( is_1_bit( lang_id & ~LANGX_MASK ) );
  switch ( lang_id ) {
    case LANG_NONE    : return "";      // LCOV_EXCL_LINE
    case LANG_C_KNR   : return "K&RC";
    case LANG_C_89    : return "C89";
    case LANG_C_95    : return "C95";
    case LANG_C_99    : return "C99";
    case LANG_C_99_EMC: return "C99 (with Embedded C extensions)";
    case LANG_C_99_UPC: return "C99 (with Unified Parallel C extensions)";
    case LANG_C_11    : return "C11";
    case LANG_C_17    : return "C17";
    case LANG_C_23    : return "C23";
    case LANG_CPP_98  : return "C++98";
    case LANG_CPP_03  : return "C++03";
    case LANG_CPP_11  : return "C++11";
    case LANG_CPP_14  : return "C++14";
    case LANG_CPP_17  : return "C++17";
    case LANG_CPP_20  : return "C++20";
    case LANG_CPP_23  : return "C++23";
  } // switch
  UNEXPECTED_INT_VALUE( lang_id );
}

c_lang_t const* c_lang_next( c_lang_t const *lang ) {
  return lang == NULL ? C_LANG : (++lang)->name == NULL ? NULL : lang;
}

char const* c_lang_coarse_name( c_lang_id_t lang_ids ) {
  bool const is_c   = (lang_ids & LANG_C_ANY  ) != LANG_NONE;
  bool const is_cpp = (lang_ids & LANG_CPP_ANY) != LANG_NONE;
  return is_c ^ is_cpp ? (is_c ? "C" : "C++") : NULL;
}

c_lang_id_t c_lang_is_one( c_lang_id_t lang_ids ) {
  bool const is_c   = (lang_ids & LANG_C_ANY  ) != LANG_NONE;
  bool const is_cpp = (lang_ids & LANG_CPP_ANY) != LANG_NONE;
  return is_c ^ is_cpp ? (is_c ? LANG_C_ANY : LANG_CPP_ANY) : LANG_NONE;
}

void c_lang_set( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( is_1_bit( lang_id ) );
  opt_lang_id = lang_id;
  cdecl_prompt_init();                  // change prompt based on new language
}

char const* c_lang_which( c_lang_id_t lang_ids ) {
  lang_ids &= ~LANGX_MASK;
  if ( lang_ids == LANG_NONE )
    return "";

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );
  c_lang_id_t which_lang_id;

  if ( is_1_bit( lang_ids ) ) {
    if ( opt_lang_id == lang_ids )
      return "";
    strbuf_putsn( &sbuf, " unless ", 8 );
    which_lang_id = lang_ids;
  }
  else {
    lang_ids &= OPT_LANG_IS( C_ANY ) ? LANG_C_ANY : LANG_CPP_ANY;
    if ( lang_ids == LANG_NONE )
      return OPT_LANG_IS( C_ANY ) ? " in C" : " in C++";

    which_lang_id = c_lang_oldest( lang_ids );
    if ( opt_lang_id < which_lang_id ) {
      strbuf_putsn( &sbuf, " until ", 7 );
    } else {
      strbuf_putsn( &sbuf, " since ", 7 );
      //
      // The newest language of langs_ids is the last language in which the
      // feature is legal, so we need the language after that to be the first
      // language in which the feature is illegal.
      //
      which_lang_id = c_lang_newest( lang_ids ) << 1;
      assert( which_lang_id != LANG_NONE );
    }
  }

  strbuf_puts( &sbuf, c_lang_name( which_lang_id ) );
  return sbuf.str;
}

c_lang_id_t is_reserved_name( char const *name ) {
  assert( name != NULL );

  if ( name[0] == '_' && (isupper( name[1] ) || name[1] == '_') )
    return LANG_ANY;

  if ( strstr( name, "__" ) != NULL )
    return LANG_CPP_ANY;

  return LANG_NONE;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
