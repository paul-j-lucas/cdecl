/*
**      cdecl -- C gibberish translator
**      src/c_operator.c
**
**      Copyright (C) 2018-2024  Paul J. Lucas
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
#include "literals.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */

/// @cond DOXYGEN_IGNORE
#define EIT                       C_OVERLOAD_EITHER
#define MBR                       C_OVERLOAD_MEMBER
#define UNL                       C_OP_PARAMS_UNLIMITED
#define XXX                       C_OVERLOAD_NONE
/// @endcond

/**
 * @addtogroup cpp-operators-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of c_operator for all C++ operators.
 *
 * @note Operators are in \ref c_op_id order.
 */
static c_operator_t const C_OPERATOR[] = {
  { C_OP_NONE,                  "none",     LANG_NONE,        XXX, 0, 0   },
  { C_OP_CO_AWAIT,              L_co_await, LANG_COROUTINES,  EIT, 0, 1   },
  { C_OP_NEW,                   L_new,      LANG_operator,    EIT, 1, UNL },
  { C_OP_NEW_ARRAY,             "new[]",    LANG_operator,    EIT, 1, UNL },
  { C_OP_DELETE,                L_delete,   LANG_operator,    EIT, 1, UNL },
  { C_OP_DELETE_ARRAY,          "delete[]", LANG_operator,    EIT, 1, UNL },
  { C_OP_EXCLAM,                "!",        LANG_operator,    EIT, 0, 1   },
  { C_OP_EXCLAM_EQUAL,          "!=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_PERCENT,               "%",        LANG_operator,    EIT, 1, 2   },
  { C_OP_PERCENT_EQUAL,         "%=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_AMPER,                 "&",        LANG_operator,    EIT, 0, 2   },
  { C_OP_AMPER_AMPER,           "&&",       LANG_operator,    EIT, 1, 2   },
  { C_OP_AMPER_EQUAL,           "&=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_PARENS,                "()",       LANG_operator,    MBR, 0, UNL },
  { C_OP_STAR,                  "*",        LANG_operator,    EIT, 0, 2   },
  { C_OP_STAR_EQUAL,            "*=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_PLUS,                  "+",        LANG_operator,    EIT, 0, 2   },
  { C_OP_PLUS_PLUS,             "++",       LANG_operator,    EIT, 0, 2   },
  { C_OP_PLUS_EQUAL,            "+=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_COMMA,                 ",",        LANG_operator,    EIT, 1, 2   },
  { C_OP_MINUS,                 "-",        LANG_operator,    EIT, 0, 2   },
  { C_OP_MINUS_MINUS,           "--",       LANG_operator,    EIT, 0, 2   },
  { C_OP_MINUS_EQUAL,           "-=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_MINUS_GREATER,         "->",       LANG_operator,    MBR, 0, 0   },
  { C_OP_MINUS_GREATER_STAR,    "->*",      LANG_operator,    EIT, 1, 2   },
  { C_OP_DOT,                   ".",        LANG_operator,    XXX, 0, 0   },
  { C_OP_DOT_STAR,              ".*",       LANG_operator,    XXX, 0, 0   },
  { C_OP_SLASH,                 "/",        LANG_operator,    EIT, 1, 2   },
  { C_OP_SLASH_EQUAL,           "/=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_COLON_COLON,           "::",       LANG_operator,    XXX, 0, 0   },
  { C_OP_LESS,                  "<",        LANG_operator,    EIT, 1, 2   },
  { C_OP_LESS_LESS,             "<<",       LANG_operator,    EIT, 1, 2   },
  { C_OP_LESS_LESS_EQUAL,       "<<=",      LANG_operator,    EIT, 1, 2   },
  { C_OP_LESS_EQUAL,            "<=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_LESS_EQUAL_GREATER,    "<=>",      LANG_LESS_EQUAL_GREATER,
                                                              EIT, 1, 2   },
  { C_OP_EQUAL,                 "=",        LANG_operator,    MBR, 1, 1   },
  { C_OP_EQUAL_EQUAL,           "==",       LANG_operator,    EIT, 1, 2   },
  { C_OP_GREATER,               ">",        LANG_operator,    EIT, 1, 2   },
  { C_OP_GREATER_EQUAL,         ">=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_GREATER_GREATER,       ">>",       LANG_operator,    EIT, 1, 2   },
  { C_OP_GREATER_GREATER_EQUAL, ">>=",      LANG_operator,    EIT, 1, 2   },
  { C_OP_QMARK_COLON,           "?:",       LANG_operator,    XXX, 0, 0   },
  { C_OP_BRACKETS,              "[]",       LANG_1_ARY_OP_BRACKETS,
                                                              MBR, 1, 1   },
  { C_OP_BRACKETS,              "[]",       LANG_N_ARY_OP_BRACKETS,
                                                              MBR, 0, UNL },
  { C_OP_CARET,                 "^",        LANG_operator,    EIT, 1, 2   },
  { C_OP_CARET_EQUAL,           "^=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_PIPE,                  "|",        LANG_operator,    EIT, 1, 2   },
  { C_OP_PIPE_EQUAL,            "|=",       LANG_operator,    EIT, 1, 2   },
  { C_OP_PIPE_PIPE,             "||",       LANG_operator,    EIT, 1, 2   },
  { C_OP_TILDE,                 "~",        LANG_operator,    EIT, 0, 1   },
};

////////// extern functions ///////////////////////////////////////////////////

c_operator_t const* c_op_get( c_op_id_t op_id ) {
  assert( op_id >= C_OP_NONE && op_id <= C_OP_TILDE );

  c_operator_t const *best_op = NULL;

  //
  // We can't just use op_id as a direct index since operator[] has multiple
  // entries, but we can start looking there.
  //
  for ( c_operator_t const *op = C_OPERATOR + op_id;
        op < C_OPERATOR + ARRAY_SIZE( C_OPERATOR );
        ++op ) {
    if ( op->op_id < op_id )
      continue;
    if ( op->op_id > op_id )            // the array is sorted
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

bool c_op_is_new_delete( c_op_id_t op_id ) {
  switch ( op_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      return true;
    default:
      return false;
  } // switch
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
