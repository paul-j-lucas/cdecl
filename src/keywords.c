/*
**      cdecl -- C gibberish translator
**      src/keywords.c
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
 * Defines functions for looking up C/C++ keyword information.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "keywords.h"
#include "lang.h"
#include "literals.h"
#include "types.h"
#include "parser.h"                     /* must go last */

// standard
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C/C++ keywords (relevant for declarations).
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,          Y_AUTO,         T_AUTO         },
  { L_CHAR,          Y_CHAR,         T_CHAR         },
  { L_DOUBLE,        Y_DOUBLE,       T_DOUBLE       },
  { L_EXTERN,        Y_EXTERN,       T_EXTERN       },
  { L_FLOAT,         Y_FLOAT,        T_FLOAT        },
  { L_INT,           Y_INT,          T_INT          },
  { L_LONG,          Y_LONG,         T_LONG         },
  { L_REGISTER,      Y_REGISTER,     T_REGISTER     },
  { L_SHORT,         Y_SHORT,        T_SHORT        },
  { L_STATIC,        Y_STATIC,       T_STATIC       },
  { L_STRUCT,        Y_STRUCT,       T_STRUCT       },
  { L_TYPEDEF,       Y_TYPEDEF,      T_TYPEDEF      },
  { L_UNION,         Y_UNION,        T_UNION        },
  { L_UNSIGNED,      Y_UNSIGNED,     T_UNSIGNED     },

  // C89
  { L_CONST,         Y_CONST,        T_CONST        },
  { L_ELLIPSIS,      Y_ELLIPSIS,     T_NONE         },
  { L_ENUM,          Y_ENUM,         T_ENUM         },
  { L_SIGNED,        Y_SIGNED,       T_SIGNED       },
  { L_VOID,          Y_VOID,         T_VOID         },
  { L_VOLATILE,      Y_VOLATILE,     T_VOLATILE     },

  // C99
  { L_BOOL,          Y_BOOL,         T_BOOL         },
  { L_COMPLEX,       Y_COMPLEX,      T_COMPLEX      },
  { L_RESTRICT,      Y_RESTRICT,     T_RESTRICT     },
  { L_WCHAR_T,       Y_WCHAR_T,      T_WCHAR_T      },

  // C11
  { L_NORETURN,      Y_NORETURN,     T_NONE         },
  { L__NORETURN,     Y_NORETURN,     T_NONE         },
  { L__THREAD_LOCAL, Y_THREAD_LOCAL, T_THREAD_LOCAL },

  // C++
  { L_CLASS,         Y_CLASS,        T_CLASS        },
  { L_VIRTUAL,       Y_VIRTUAL,      T_VIRTUAL      },

  // C11 & C++11
  { L_CHAR16_T,      Y_CHAR16_T,     T_CHAR16_T     },
  { L_CHAR32_T,      Y_CHAR32_T,     T_CHAR32_T     },
  { L_THREAD_LOCAL,  Y_THREAD_LOCAL, T_THREAD_LOCAL },

  // Apple extension
  { L___BLOCK,       Y___BLOCK,      T_BLOCK        },

  { NULL,            0,              T_NONE         }
};

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_keyword_find_literal( char const *literal ) {
  for ( c_keyword_t const *k = C_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( literal, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

c_keyword_t const* c_keyword_find_token( int y_token ) {
  for ( c_keyword_t const *k = C_KEYWORDS; k->literal; ++k ) {
    if ( y_token == k->y_token )
      return k;
  } // for
  return T_NONE;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
