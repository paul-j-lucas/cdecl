/*
**      cdecl -- C gibberish translator
**      src/c_operator.c
**
**      Copyright (C) 2018-2021  Paul J. Lucas, et al.
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
 * Defines static data for C/C++ operators.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_OPERATOR_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_operator.h"
#include "c_ast.h"
#include "c_lang.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"

// standard
#include <assert.h>

#define UNLIMITED                 C_OP_PARAMS_UNLIMITED

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of `c_operator` for all C/C++ operators indexed by a `c_oper_id`.
 */
static c_operator_t const C_OPERATOR[] = {
  { "",         C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_NONE         },
  { L_NEW,      C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ANY      },
  { "new[]",    C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ANY      },
  { L_DELETE,   C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ANY      },
  { "delete[]", C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ANY      },
  { "!",        C_OP_OVERLOADABLE,      0, 1,         LANG_CPP_ANY      },
  { "!=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "%",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "%=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "&",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ANY      },
  { "&&",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "&=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "()",       C_OP_MEMBER,            0, UNLIMITED, LANG_CPP_ANY      },
  { "*",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ANY      },
  { "*=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "+",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ANY      },
  { "++",       C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ANY      },
  { "+=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { ",",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "-",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ANY      },
  { "--",       C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ANY      },
  { "-=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "->",       C_OP_MEMBER,            0, 0,         LANG_CPP_ANY      },
  { "->*",      C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { ".",        C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ANY      },
  { ".*",       C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ANY      },
  { "/",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "/=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "::",       C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ANY      },
  { "<",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "<<",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "<<=",      C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "<=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "<=>",      C_OP_OVERLOADABLE,      1, 2,         LANG_MIN(CPP_20)  },
  { "=",        C_OP_MEMBER,            1, 1,         LANG_CPP_ANY      },
  { "==",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { ">",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { ">=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { ">>",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { ">>=",      C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "?:",       C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ANY      },
  { "[]",       C_OP_MEMBER,            1, 1,         LANG_CPP_ANY      },
  { "^",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "^=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "|",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "|=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "||",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ANY      },
  { "~",        C_OP_OVERLOADABLE,      0, 1,         LANG_CPP_ANY      },
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets the alternative token of a C++ operator \a token.
 *
 * @param token The C++ operator token to get the alternative token for.
 * @return If we're emitting alternative tokens and if \a token is a token that
 * has an alternative token, returns said token; otherwise returns \a token as-
 * is.
 */
PJL_WARN_UNUSED_RESULT
static char const* alt_token_c( char const *token ) {
  assert( token != NULL );

  if ( opt_alt_tokens ) {
    switch ( token[0] ) {
      case '!': switch ( token[1] ) {
                  case '=': return L_NOT_EQ;
                  default : return L_NOT;
                }
      case '&': switch ( token[1] ) {
                  case '&': return L_AND;
                  case '=': return L_AND_EQ;
                  default : return L_BITAND;
                } // switch
      case '|': switch ( token[1] ) {
                  case '|': return L_OR;
                  case '=': return L_OR_EQ;
                  default : return L_BITOR;
                } // switch
      case '~': return L_COMPL;
      case '^': switch ( token[1] ) {
                  case '=': return L_XOR_EQ;
                  default : return L_XOR;
                } // switch
    } // switch
  }

  return token;
}

////////// extern functions ///////////////////////////////////////////////////

c_operator_t const* c_oper_get( c_oper_id_t oper_id ) {
  assert( oper_id <= C_OP_TILDE );
  return &C_OPERATOR[ oper_id ];
}

char const* c_oper_token_c( c_oper_id_t oper_id ) {
  return alt_token_c( graph_token_c( c_oper_get( oper_id )->name ) );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
