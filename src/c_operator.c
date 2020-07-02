/*
**      cdecl -- C gibberish translator
**      src/c_operator.c
**
**      Copyright (C) 2018-2020  Paul J. Lucas, et al.
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
#define C_OP_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_operator.h"
#include "c_ast.h"
#include "c_lang.h"
#include "literals.h"

// standard
#include <assert.h>

#define UNLIMITED                 C_OP_ARGS_UNLIMITED

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of `c_operator` for all C/C++ operators indexed by a `c_oper_id`.
 */
static c_operator_t const C_OPERATOR[] = {
  { "",         C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_NONE         },
  { L_NEW,      C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ALL      },
  { "new[]",    C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ALL      },
  { L_DELETE,   C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ALL      },
  { "delete[]", C_OP_OVERLOADABLE,      1, UNLIMITED, LANG_CPP_ALL      },
  { "!",        C_OP_OVERLOADABLE,      0, 1,         LANG_CPP_ALL      },
  { "!=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "%",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "%=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "&",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ALL      },
  { "&&",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "&=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "()",       C_OP_MEMBER,            0, UNLIMITED, LANG_CPP_ALL      },
  { "*",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ALL      },
  { "*=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "+",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ALL      },
  { "++",       C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ALL      },
  { "+=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { ",",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "-",        C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ALL      },
  { "--",       C_OP_OVERLOADABLE,      0, 2,         LANG_CPP_ALL      },
  { "-=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "->",       C_OP_MEMBER,            0, 0,         LANG_CPP_ALL      },
  { "->*",      C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { ".",        C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ALL      },
  { ".*",       C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ALL      },
  { "/",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "/=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "::",       C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ALL      },
  { "<",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "<<",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "<<=",      C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "<=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "<=>",      C_OP_OVERLOADABLE,      1, 2,         LANG_MIN(CPP_20)  },
  { "=",        C_OP_MEMBER,            1, 1,         LANG_CPP_ALL      },
  { "==",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { ">",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { ">=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { ">>",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { ">>=",      C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "?:",       C_OP_NOT_OVERLOADABLE,  0, 0,         LANG_CPP_ALL      },
  { "[]",       C_OP_MEMBER,            1, 1,         LANG_CPP_ALL      },
  { "^",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "^=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "|",        C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "|=",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "||",       C_OP_OVERLOADABLE,      1, 2,         LANG_CPP_ALL      },
  { "~",        C_OP_OVERLOADABLE,      0, 1,         LANG_CPP_ALL      },
};

////////// extern functions ///////////////////////////////////////////////////

c_operator_t const* c_oper_get( c_oper_id_t oper_id ) {
  assert( oper_id <= C_OP_TILDE );
  return &C_OPERATOR[ oper_id ];
}

unsigned c_oper_get_overload( c_ast_t const *ast ) {
  assert( ast->kind_id == K_OPERATOR );

  //
  // If the operator is either member or non-member only, then it's that.
  //
  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );
  unsigned const op_overload_flags = op->flags & C_OP_MASK_OVERLOAD;
  switch ( op_overload_flags ) {
    case C_OP_MEMBER:
    case C_OP_NON_MEMBER:
      return op_overload_flags;
  } // switch

  //
  // Otherwise, the operator can be either: see if the user specified which one
  // explicitly.
  //
  unsigned const user_overload_flags = ast->as.oper.flags & C_OP_MASK_OVERLOAD;
  switch ( user_overload_flags ) {
    case C_OP_MEMBER:
    case C_OP_NON_MEMBER:
      return user_overload_flags;
  } // switch

  //
  // The user didn't specify either member or non-member explicitly: see if it
  // has a member-only or non-member-only type qualifier.
  //
  if ( (ast->type_id & T_MEMBER_FUNC_ONLY) != T_NONE )
    return C_OP_MEMBER;
  if ( (ast->type_id & T_NONMEMBER_FUNC_ONLY) != T_NONE )
    return C_OP_NON_MEMBER;

  //
  // Special case for new & delete operators: they're member operators if they
  // have a name (of a class) or declared static.
  //
  switch ( ast->as.oper.oper_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      return !c_ast_sname_empty( ast ) || (ast->type_id & T_STATIC) != T_NONE ?
        C_OP_MEMBER : C_OP_NON_MEMBER;
    default:
      /* suppress warning */;
  } // switch

  //
  // No such qualifier: try to infer whether it's a member or non-member based
  // on the number of arguments given.
  //
  size_t const n_args = c_ast_args_count( ast );
  if ( n_args == op->args_min )
    return C_OP_MEMBER;
  if ( n_args == op->args_max )
    return C_OP_NON_MEMBER;

  //
  // We can't determine which one, so give up.
  //
  return C_OP_UNSPECIFIED;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
