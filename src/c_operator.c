/*
**      cdecl -- C gibberish translator
**      src/c_operator.c
**
**      Copyright (C) 2018  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_OP_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "c_operator.h"
#include "util.h"

// standard
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of `c_operator` for all C/C++ operators indexed by a `c_oper_id`.
 *
 * @hideinitializer
 */
static c_operator_t const C_OPERATOR[] = {
  { "",     OP_NOT_OVERLOADABLE,  0, 0,                 LANG_NONE         },
  { "!",    OP_OVERLOADABLE,      0, 1,                 LANG_CPP_ALL      },
  { "!=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "%",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "%=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "&",    OP_OVERLOADABLE,      0, 2,                 LANG_CPP_ALL      },
  { "&&",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "&=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "()",   OP_MEMBER,            0, OP_ARGS_UNLIMITED, LANG_CPP_ALL      },
  { "*",    OP_OVERLOADABLE,      0, 2,                 LANG_CPP_ALL      },
  { "*=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "+",    OP_OVERLOADABLE,      0, 2,                 LANG_CPP_ALL      },
  { "++",   OP_OVERLOADABLE,      0, 2,                 LANG_CPP_ALL      },
  { "+=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { ",",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "-",    OP_OVERLOADABLE,      0, 2,                 LANG_CPP_ALL      },
  { "--",   OP_OVERLOADABLE,      0, 2,                 LANG_CPP_ALL      },
  { "-=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "->",   OP_MEMBER,            0, 0,                 LANG_CPP_ALL      },
  { "->*",  OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { ".",    OP_NOT_OVERLOADABLE,  0, 0,                 LANG_CPP_ALL      },
  { ".*",   OP_NOT_OVERLOADABLE,  0, 0,                 LANG_CPP_ALL      },
  { "/",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "/=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "::",   OP_NOT_OVERLOADABLE,  0, 0,                 LANG_CPP_ALL      },
  { "<",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "<<",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "<<=",  OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "<=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "<=>",  OP_OVERLOADABLE,      1, 2,                 LANG_MIN(CPP_20)  },
  { "=",    OP_MEMBER,            1, 1,                 LANG_CPP_ALL      },
  { "==",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { ">",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { ">=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { ">>",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { ">>=",  OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "?:",   OP_NOT_OVERLOADABLE,  0, 0,                 LANG_CPP_ALL      },
  { "[]",   OP_MEMBER,            1, 1,                 LANG_CPP_ALL      },
  { "^",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "^=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "|",    OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "|=",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "||",   OP_OVERLOADABLE,      1, 2,                 LANG_CPP_ALL      },
  { "~",    OP_OVERLOADABLE,      0, 1,                 LANG_CPP_ALL      },
};

////////// extern functions ///////////////////////////////////////////////////

c_operator_t const* op_get( c_oper_id_t oper_id ) {
  if ( unlikely( oper_id > OP_TILDE ) )
    INTERNAL_ERR( "\"%d\": unexpected value for operator\n", (int)oper_id );
  return &C_OPERATOR[ oper_id ];
}

unsigned op_get_overload( c_ast_t const *ast ) {
  assert( ast->kind == K_OPERATOR );

  //
  // If the operator is either member or non-member only, then it's that.
  //
  c_operator_t const *const op = op_get( ast->as.oper.oper_id );
  unsigned const op_overload_flags = op->flags & OP_MASK_OVERLOAD;
  switch ( op_overload_flags ) {
    case OP_MEMBER:
    case OP_NON_MEMBER:
      return op_overload_flags;
  } // switch

  //
  // Otherwise, the operator can be either: see if the user specified which one
  // explicitly.
  //
  unsigned const user_overload_flags = ast->as.oper.flags & OP_MASK_OVERLOAD;
  switch ( user_overload_flags ) {
    case OP_MEMBER:
    case OP_NON_MEMBER:
      return user_overload_flags;
  } // switch

  //
  // The user didn't specify either member or non-member explicitly: see if it
  // has a member-only or non-member-only type qualifier.
  //
  if ( (ast->type_id & T_MEMBER_ONLY) != T_NONE )
    return OP_MEMBER;
  if ( (ast->type_id & T_NON_MEMBER_ONLY) != T_NONE )
    return OP_NON_MEMBER;

  //
  // No such qualifier: try to infer whether it's a member or non-member based
  // on the number of arguments given.
  //
  unsigned const n_args = c_ast_args_len( ast );
  if ( n_args == op->args_min )
    return OP_MEMBER;
  if ( n_args == op->args_max )
    return OP_NON_MEMBER;

  //
  // We can't determine which one, so give up.
  //
  return OP_UNSPECIFIED;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
