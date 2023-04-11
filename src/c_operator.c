/*
**      cdecl -- C gibberish translator
**      src/c_operator.c
**
**      Copyright (C) 2018-2023  Paul J. Lucas
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
 * Defines data and functions for C/C++ operators.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_OPERATOR_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_operator.h"
#include "c_ast.h"
#include "c_lang.h"
#include "gibberish.h"
#include "literals.h"
#include "util.h"

// standard
#include <assert.h>

/// @cond DOXYGEN_IGNORE
#define MBR                       C_OPER_MEMBER
#define OVR                       C_OPER_OVERLOADABLE
#define UNL                       C_OPER_PARAMS_UNLIMITED
#define XXX                       C_OPER_NOT_OVERLOADABLE
/// @endcond

/**
 * @addtogroup cpp-operators-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of c_operator for all C++ operators.
 *
 * @note Operators are in \ref c_oper_id order.
 */
static c_operator_t const C_OPERATOR[] = {
  { C_OP_NONE,                "none",     LANG_NONE,        XXX, 0, 0   },
  { C_OP_CO_AWAIT,            L_co_await, LANG_CPP_MIN(20), OVR, 0, 1   },
  { C_OP_NEW,                 L_new,      LANG_CPP_ANY,     OVR, 1, UNL },
  { C_OP_NEW_ARRAY,           "new[]",    LANG_CPP_ANY,     OVR, 1, UNL },
  { C_OP_DELETE,              L_delete,   LANG_CPP_ANY,     OVR, 1, UNL },
  { C_OP_DELETE_ARRAY,        "delete[]", LANG_CPP_ANY,     OVR, 1, UNL },
  { C_OP_EXCLAM,              "!",        LANG_CPP_ANY,     OVR, 0, 1   },
  { C_OP_EXCLAM_EQUAL,        "!=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PERCENT,             "%",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PERCENT_EQUAL,       "%=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_AMPER,               "&",        LANG_CPP_ANY,     OVR, 0, 2   },
  { C_OP_AMPER2,              "&&",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_AMPER_EQUAL,         "&=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PARENS,              "()",       LANG_CPP_ANY,     MBR, 0, UNL },
  { C_OP_STAR,                "*",        LANG_CPP_ANY,     OVR, 0, 2   },
  { C_OP_STAR_EQUAL,          "*=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PLUS,                "+",        LANG_CPP_ANY,     OVR, 0, 2   },
  { C_OP_PLUS2,               "++",       LANG_CPP_ANY,     OVR, 0, 2   },
  { C_OP_PLUS_EQUAL,          "+=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_COMMA,               ",",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_MINUS,               "-",        LANG_CPP_ANY,     OVR, 0, 2   },
  { C_OP_MINUS2,              "--",       LANG_CPP_ANY,     OVR, 0, 2   },
  { C_OP_MINUS_EQUAL,         "-=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_ARROW,               "->",       LANG_CPP_ANY,     MBR, 0, 0   },
  { C_OP_ARROW_STAR,          "->*",      LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_DOT,                 ".",        LANG_CPP_ANY,     XXX, 0, 0   },
  { C_OP_DOT_STAR,            ".*",       LANG_CPP_ANY,     XXX, 0, 0   },
  { C_OP_SLASH,               "/",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_SLASH_EQUAL,         "/=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_COLON2,              "::",       LANG_CPP_ANY,     XXX, 0, 0   },
  { C_OP_LESS,                "<",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_LESS2,               "<<",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_LESS2_EQUAL,         "<<=",      LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_LESS_EQUAL,          "<=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_LESS_EQUAL_GREATER,  "<=>",      LANG_CPP_MIN(20), OVR, 1, 2   },
  { C_OP_EQUAL,               "=",        LANG_CPP_ANY,     MBR, 1, 1   },
  { C_OP_EQUAL2,              "==",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_GREATER,             ">",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_GREATER_EQUAL,       ">=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_GREATER2,            ">>",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_GREATER2_EQUAL,      ">>=",      LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_QMARK_COLON,         "?:",       LANG_CPP_ANY,     XXX, 0, 0   },
  { C_OP_BRACKETS,            "[]",       LANG_CPP_MAX(20), MBR, 1, 1   },
  { C_OP_BRACKETS,            "[]",       LANG_CPP_MIN(23), MBR, 0, UNL },
  { C_OP_CARET,               "^",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_CARET_EQUAL,         "^=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PIPE,                "|",        LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PIPE_EQUAL,          "|=",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_PIPE2,               "||",       LANG_CPP_ANY,     OVR, 1, 2   },
  { C_OP_TILDE,               "~",        LANG_CPP_ANY,     OVR, 0, 1   },
};

////////// extern functions ///////////////////////////////////////////////////

c_operator_t const* c_oper_get( c_oper_id_t oper_id ) {
  assert( oper_id >= C_OP_NONE && oper_id <= C_OP_TILDE );

  c_operator_t const *best_op = NULL;

  //
  // We can't just use oper_id as a direct index since operator[] has multiple
  // entries, but we can start looking there.
  //
  for ( c_operator_t const *op = C_OPERATOR + oper_id;
        op < C_OPERATOR + ARRAY_SIZE( C_OPERATOR );
        ++op ) {
    if ( op->oper_id < oper_id )
      continue;
    if ( op->oper_id > oper_id )        // the array is sorted
      break;
    if ( opt_lang_is_any( op->lang_ids ) )
      return op;
    //
    // We found the operator, but the entry isn't supported for the current
    // language, so keep looking for one that is.  However, make a note of the
    // current entry and return it if we don't find a better entry since we
    // always have to return non-NULL.  The code in c_ast_check_oper() will
    // deal with an unsupported language.
    //
    best_op = op;
  } // for

  assert( best_op != NULL );
  return best_op;
}

char const* c_oper_token_c( c_oper_id_t oper_id ) {
  char const *const literal = c_oper_get( oper_id )->literal;
  char const *const alt_token = alt_token_c( literal );
  return alt_token != literal ? alt_token : graph_token_c( literal );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
