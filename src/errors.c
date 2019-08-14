/*
**      cdecl -- C gibberish translator
**      src/errors.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
 * Defines functions for performing semantic checks on an AST.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_type.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>

#define T_NOT_FUNC_LIKE \
  (T_AUTO_C | T_BLOCK | T_MUTABLE | T_REGISTER | T_THREAD_LOCAL)

// local constants
static bool const VISITOR_ERROR_FOUND     = true;
static bool const VISITOR_ERROR_NOT_FOUND = false;

/// @endcond

// local functions
static bool c_ast_check_oper_args( c_ast_t const* );
static bool c_ast_check_ret_type( c_ast_t const* );
static bool c_ast_visitor_error( c_ast_t*, void* );
static bool c_ast_visitor_type( c_ast_t*, void* );
static bool error_kind_not_cast_into( c_ast_t const*, char const* );
static bool error_kind_not_supported( c_ast_t const* );
static bool error_kind_not_type( c_ast_t const*, c_type_id_t );
static bool error_kind_to_type( c_ast_t const*, c_type_id_t );
static bool error_unknown_type( c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets the alignas literal for the current language.
 *
 * @return Returns either `_Alignas` (for C) or `alignas` (for C++).
 */
static inline char const* alignas_lang( void ) {
  return C_LANG_IS_CPP() ? L_ALIGNAS : L__ALIGNAS;
}

/**
 * Simple wrapper around `c_ast_found()`.
 *
 * @param ast The `c_ast` to check.
 * @param visitor The visitor to use.
 * @param data Optional data passed to `c_ast_visit()`.
 * @return Returns `true` only if all checks passed.
 */
static inline bool c_ast_check_visitor( c_ast_t const *ast,
                                        c_ast_visitor_t visitor,
                                        void *data ) {
  return !c_ast_found( ast, V_DOWN, visitor, data );
}

/**
 * Returns an "s" or not based on \a n to pluralize a word.
 *
 * @param n A quantity.
 * @return Returns the empty string only if \a n == 1; otherwise returns "s".
 */
static inline char const* plural_s( uint64_t n ) {
  return n == 1 ? "" : "s";
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks the `alignas` of an AST for errors.
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_alignas( c_ast_t *ast ) {
  assert( ast != NULL );
  assert( ast->align.kind != ALIGNAS_NONE );

  if ( (ast->type_id & T_REGISTER) != T_NONE ) {
    print_error( &ast->loc,
      "\"%s\" can not be combined with \"%s\"", alignas_lang(), L_REGISTER
    );
    return false;
  }

  if ( (ast->kind & K_FUNCTION_LIKE) != K_NONE ) {
    print_error( &ast->loc,
      "%s can not be %s", c_kind_name( ast->kind ), L_ALIGNED
    );
    return false;
  }

  switch ( ast->align.kind ) {
    case ALIGNAS_NONE:
      break;
    case ALIGNAS_EXPR: {
      unsigned const alignment = ast->align.as.expr;
      if ( !at_most_one_bit_set( alignment ) ) {
        print_error( &ast->loc,
          "\"%u\": alignment is not a power of 2", alignment
        );
        return false;
      }
      break;
    }
    case ALIGNAS_TYPE:
      if ( !c_ast_check( ast->align.as.type_ast, CHECK_DECL ) )
        return false;
      break;
  } // switch

  return true;
}

/**
 * Checks an array AST for errors.
 *
 * @param ast The array `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_array( c_ast_t const *ast, bool is_func_arg ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );

  if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE ) {
    if ( (opt_lang & (LANG_MIN(C_99) & ~LANG_CPP_ALL)) == LANG_NONE ) {
      print_error( &ast->loc,
        "variable length arrays not supported in %s",
        C_LANG_NAME()
      );
      return false;
    }
    if ( !is_func_arg ) {
      print_error( &ast->loc,
        "variable length arrays are illegal outside of function arguments"
      );
      return false;
    }
  }

  if ( ast->as.array.type_id != T_NONE ) {
    if ( (opt_lang & (LANG_MIN(C_99) & ~LANG_CPP_ALL)) == LANG_NONE ) {
      print_error( &ast->loc,
        "\"%s\" arrays not supported in %s",
        c_type_name_error( ast->as.array.type_id ),
        C_LANG_NAME()
      );
      return false;
    }
    if ( !is_func_arg ) {
      print_error( &ast->loc,
        "\"%s\" arrays are illegal outside of function arguments",
        c_type_name_error( ast->as.array.type_id )
      );
      return false;
    }
  }

  c_ast_t const *const of_ast = ast->as.array.of_ast;
  switch ( of_ast->kind ) {
    case K_BUILTIN:
      if ( (of_ast->type_id & T_VOID) != T_NONE ) {
        print_error( &ast->loc, "array of %s", L_VOID );
        print_hint( "array of pointer to %s", L_VOID );
        return false;
      }
      if ( (of_ast->type_id & T_REGISTER) != T_NONE ) {
        error_kind_not_type( ast, T_REGISTER );
        return false;
      }
      break;
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
      print_error( &ast->loc, "array of %s", c_kind_name( of_ast->kind ) );
      print_hint( "array of pointer to function" );
      return false;
    case K_NAME:
      error_unknown_type( of_ast );
      return false;
    default:
      /* suppress warning */;
  } // switch

  return true;
}

/**
 * Checks a built-in type AST for errors.
 *
 * @param ast The built-in `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_builtin( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );

  if ( (ast->parent == NULL ||
        ast->parent->kind != K_USER_DEF_CONVERSION) &&
        (ast->type_id & T_MASK_TYPE) == T_NONE ) {
    print_error( &ast->loc,
      "implicit \"%s\" is illegal in %s", L_INT, C_LANG_NAME()
    );
    return false;
  }

  if ( (ast->type_id & T_VOID) != T_NONE && ast->parent == NULL ) {
    print_error( &ast->loc, "variable of %s", L_VOID );
    print_hint( "pointer to %s", L_VOID );
    return false;
  }

  if ( (ast->type_id & T_INLINE) != T_NONE && opt_lang < LANG_CPP_17 ) {
    print_error( &ast->loc,
      "%s variables not supported in %s", L_INLINE, C_LANG_NAME()
    );
    return false;
  }

  return true;
}

/**
 * Performs additional checks on an entire AST for semantic errors when
 * casting.
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );

  c_ast_t const *const storage_ast =
    c_ast_find_type( nonconst_ast, V_DOWN, T_MASK_STORAGE );

  if ( storage_ast != NULL ) {
    c_type_id_t const storage_type = storage_ast->type_id & T_MASK_STORAGE;
    print_error( &ast->loc,
      "can not cast into %s", c_type_name_error( storage_type )
    );
    return false;
  }

  switch ( ast->kind ) {
    case K_ARRAY:
      return error_kind_not_cast_into( ast, "pointer" );
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
      return error_kind_not_cast_into( ast, "pointer to function" );
    default:
      return true;
  } // switch
}

/**
 * Checks a constructor or destructor AST for errors.
 *
 * @param The constructor or destructor `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_ctor_dtor( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE );

  if ( c_ast_sname_count( ast ) > 1 && !c_ast_sname_is_ctor( ast ) ) {
    print_error( &ast->loc,
      "\"%s\", \"%s\": class and %s names don't match",
      c_ast_sname_name_atr( ast, 1 ), c_ast_sname_local_name( ast ),
      c_kind_name( ast->kind )
    );
    return false;
  }

  return true;
}

/**
 * Checks an `enum`, `class`, `struct`, or `union` AST for errors.
 *
 * @param ast The `enum`, `class`, `struct`, or union `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_ecsu( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_ENUM_CLASS_STRUCT_UNION );

  if ( (ast->type_id & T_CLASS_STRUCT_UNION) != T_NONE &&
       (ast->type_id & T_REGISTER) != T_NONE ) {
    error_kind_not_type( ast, T_REGISTER );
    return false;
  }

  if ( c_mode == MODE_GIBBERISH_TO_ENGLISH &&
       (ast->type_id & T_ENUM) != T_NONE &&
       (ast->type_id & (T_STRUCT | T_CLASS)) != T_NONE ) {
    print_error( &ast->loc,
      "\"%s\": %s classes must just use \"%s\"",
      c_type_name_error( ast->type_id ), L_ENUM, L_ENUM
    );
    return false;
  }

  return true;
}

/**
 * Checks an entire AST for semantic errors.
 *
 * @param ast The `c_ast` to check.
 * @param is_func_arg If `true`, we're checking a function argument.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_errors( c_ast_t const *ast, bool is_func_arg ) {
  assert( ast != NULL );
  // check in major-to-minor error order
  void *const data = REINTERPRET_CAST( void*, is_func_arg );
  if ( !c_ast_check_visitor( ast, c_ast_visitor_error, data ) )
    return false;
  if ( !c_ast_check_visitor( ast, c_ast_visitor_type, data ) )
    return false;
  return true;
}

/**
 * Checks a function-like AST for errors.
 *
 * @param ast The function-like `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_func( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & K_FUNCTION_LIKE) != K_NONE );

  if ( (ast->type_id & T_ANY_REFERENCE) != T_NONE ) {
    if ( opt_lang < LANG_CPP_11 ) {
      print_error( &ast->loc,
        "%s qualified %ss not supported in %s",
        L_REFERENCE, c_kind_name( ast->kind ), C_LANG_NAME()
      );
      return false;
    }
    if ( (ast->type_id & (T_EXTERN | T_STATIC)) != T_NONE ) {
      print_error( &ast->loc,
        "%s qualified %ss can not be %s",
        L_REFERENCE, c_kind_name( ast->kind ),
        c_type_name_error( ast->type_id & (T_EXTERN | T_STATIC) )
      );
      return false;
    }
  }

  if ( C_LANG_IS_CPP() ) {
    c_type_id_t const member_types = ast->type_id & T_MEMBER_ONLY;
    c_type_id_t const non_member_types = ast->type_id & T_NON_MEMBER_ONLY;
    if ( member_types != T_NONE && non_member_types != T_NONE ) {
      char const *const member_types_names =
        FREE_STRDUP_LATER( c_type_name_error( member_types ) );
      print_error( &ast->loc,
        "%ss can not be %s and %s",
        c_kind_name( ast->kind ),
        member_types_names,
        c_type_name_error( non_member_types )
      );
      return false;
    }

    unsigned const user_overload_flags =
      ast->as.func.flags & C_FUNC_MASK_MEMBER;
    switch ( user_overload_flags ) {
      case C_FUNC_MEMBER:
        if ( non_member_types != T_NONE ) {
          print_error( &ast->loc,
            "%s %ss can not be %s",
            L_MEMBER, c_kind_name( ast->kind ),
            c_type_name_error( non_member_types )
          );
          return false;
        }
        break;
      case C_FUNC_NON_MEMBER:
        if ( member_types != T_NONE ) {
          print_error( &ast->loc,
            "%s %ss can not be %s",
            L_NON_MEMBER, c_kind_name( ast->kind ),
            c_type_name_error( member_types )
          );
          return false;
        }
        break;
    } // switch

    if ( (ast->type_id & (T_DEFAULT | T_DELETE)) != T_NONE ) {
      switch ( ast->kind ) {
        case K_OPERATOR: {              // C& operator=(C const&)
          if ( ast->as.oper.oper_id != OP_EQ )
            goto only_special;
          c_ast_t const *ret_ast = ast->as.oper.ret_ast;
          if ( !c_ast_is_ref_to_type( ret_ast, T_CLASS_STRUCT_UNION ) )
            goto only_special;
          // FALLTHROUGH
        case K_CONSTRUCTOR:             // C(C const&)
          if ( c_ast_args_count( ast ) != 1 )
            goto only_special;
          c_ast_t const *arg_ast = c_ast_arg_ast( c_ast_args( ast ) );
          if ( !c_ast_is_ref_to_type( arg_ast, T_CLASS_STRUCT_UNION ) )
            goto only_special;
          if ( ast->kind == K_OPERATOR ) {
            //
            // For C& operator=(C const&), the argument and the return type
            // must both be a reference to the same class, struct, or union.
            //
            arg_ast = c_ast_unreference( arg_ast );
            ret_ast = c_ast_unreference( ret_ast );
            if ( arg_ast != ret_ast )
              goto only_special;
          }
          break;
        }
        default:
          goto only_special;
      } // switch
    }

    if ( (ast->type_id & T_VIRTUAL) != T_NONE ) {
      if ( c_ast_sname_count( ast ) > 1 ) {
        print_error( &ast->loc,
          "\"%s\": %s can not be used in file-scoped %ss",
          c_ast_sname_full_name( ast ), L_VIRTUAL, c_kind_name( ast->kind )
        );
        return false;
      }
    }
    else if ( (ast->type_id & T_PURE_VIRTUAL) != T_NONE ) {
      print_error( &ast->loc,
        "non-%s %ss can not be %s",
        L_VIRTUAL, c_kind_name( ast->kind ), L_PURE
      );
      return false;
    }
  }
  else {
    c_type_id_t const qual_type = ast->type_id & T_MASK_QUALIFIER;
    if ( qual_type != T_NONE ) {
      print_error( &ast->loc,
        "\"%s\" %ss not supported in %s",
        c_type_name_error( qual_type ),
        c_kind_name( ast->kind ),
        C_LANG_NAME()
      );
      return false;
    }
  }

  return true;

only_special:
  print_error( &ast->loc,
    "\"%s\" can be used only for special member functions",
    c_type_name_error( ast->type_id )
  );
  return false;
}

/**
 * Checks all function-like arguments for semantic errors.
 *
 * @param ast The function-like `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_func_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & K_FUNCTION_LIKE) != K_NONE );
  assert( opt_lang != LANG_C_KNR );

  c_ast_t const *variadic_ast = NULL, *void_ast = NULL;
  unsigned n_args = 0;

  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( ++n_args > 1 && void_ast != NULL )
      goto only_void;

    c_ast_t const *const arg_ast = c_ast_arg_ast( arg );

    if ( c_ast_sname_count( arg_ast ) > 1 ) {
      print_error( &arg_ast->loc, "argument names can not be scoped" );
      return false;
    }

    switch ( arg_ast->kind ) {
      case K_BUILTIN:
        if ( (arg_ast->type_id & T_AUTO_CPP_11) != T_NONE ) {
          print_error( &arg_ast->loc, "arguments can not be %s", L_AUTO );
          return false;
        }
        if ( (arg_ast->type_id & T_VOID) != T_NONE ) {
          //
          // Ordinarily, void arguments are invalid; but a single void function
          // "argument" is valid (as long as it doesn't have a name).
          //
          if ( !c_ast_sname_empty( arg_ast ) ) {
            print_error( &arg_ast->loc, "arguments can not be %s", L_VOID );
            return false;
          }
          void_ast = arg_ast;
          if ( n_args > 1 )
            goto only_void;
          continue;
        }
        break;

      case K_NAME:
        if ( C_LANG_IS_CPP() ) {
          print_error( &arg_ast->loc, "C++ requires type specifier" );
          return false;
        }
        break;

      case K_VARIADIC:
        if ( ast->kind == K_OPERATOR && ast->as.oper.oper_id != OP_PARENS ) {
          print_error( &arg_ast->loc,
            "%s %s can not have a variadic argument",
            L_OPERATOR, op_get( ast->as.oper.oper_id )->name
          );
          return false;
        }
        if ( arg->next != NULL ) {
          print_error( &arg_ast->loc, "variadic specifier must be last" );
          return false;
        }
        variadic_ast = arg_ast;
        continue;

      default:
        /* suppress warning */;
    } // switch

    c_type_id_t const storage_type =
      arg_ast->type_id & (T_MASK_STORAGE & ~T_REGISTER);
    if ( storage_type != T_NONE ) {
      print_error( &arg_ast->loc,
        "%s arguments can not be %s",
        c_kind_name( ast->kind ),
        c_type_name_error( storage_type )
      );
      return false;
    }

    if ( !c_ast_check_errors( arg_ast, true ) )
      return false;
  } // for

  if ( ast->kind == K_OPERATOR && !c_ast_check_oper_args( ast ) )
    return false;

  if ( variadic_ast != NULL && n_args == 1 ) {
    print_error( &variadic_ast->loc,
      "variadic specifier can not be only argument"
    );
    return false;
  }

  return true;

only_void:
  print_error( &void_ast->loc, "\"void\" must be only parameter if specified" );
  return false;
}

/**
 * Checks all function arguments for semantic errors in K&R C.
 *
 * @param ast The function-like `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_func_args_knr( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & K_FUNCTION_LIKE) != K_NONE );
  assert( opt_lang == LANG_C_KNR );

  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    c_ast_t const *const arg_ast = c_ast_arg_ast( arg );
    switch ( arg_ast->kind ) {
      case K_NAME:
        break;
      case K_PLACEHOLDER:
        assert( arg_ast->kind != K_PLACEHOLDER );
      default:
        print_error( &arg_ast->loc,
          "function prototypes not supported in %s", C_LANG_NAME()
        );
        return false;
    } // switch
  } // for
  return true;
}

/**
 * Checks an overloaded operator AST for errors.
 *
 * @param ast The overloaded operator `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_oper( c_ast_t const *ast ) {
  assert( ast != NULL );

  c_operator_t const *const op = op_get( ast->as.oper.oper_id );

  if ( (opt_lang & op->lang_ids) == LANG_NONE ) {
    print_error( &ast->loc,
      "overloading operator \"%s\" not supported in %s",
      op->name,
      C_LANG_NAME()
    );
    return false;
  }

  if ( (op->flags & OP_MASK_OVERLOAD) == OP_NOT_OVERLOADABLE ) {
    print_error( &ast->loc,
      "%s %s can not be overloaded",
      L_OPERATOR, op->name
    );
    return false;
  }

  switch ( ast->as.oper.oper_id ) {
    case OP_ARROW: {
      //
      // Special case for operator-> that must return a pointer to a struct,
      // union, or class.
      //
      c_ast_t const *const ret_ast = ast->as.oper.ret_ast;
      if ( !c_ast_is_ptr_to_type( ret_ast, T_CLASS_STRUCT_UNION ) ) {
        print_error( &ret_ast->loc,
          "%s -> must return a pointer to %s, %s, or %s",
          L_OPERATOR, L_STRUCT, L_UNION, L_CLASS
        );
        return false;
      }
      break;
    }
    default:
      /* suppress warning */;
  } // switch

  return true;
}

/**
 * Checks all overloaded operator arguments for semantic errors.
 *
 * @param ast The overloaded operator `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_oper_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  unsigned user_overload_flags = ast->as.oper.flags & OP_MASK_OVERLOAD;
  c_operator_t const *const op = op_get( ast->as.oper.oper_id );
  unsigned const op_overload_flags = op->flags & OP_MASK_OVERLOAD;
  size_t const n_args = c_ast_args_count( ast );

  char const *const op_type =
    op_overload_flags == OP_MEMBER     ? L_MEMBER     :
    op_overload_flags == OP_NON_MEMBER ? L_NON_MEMBER :
    "";
  char const *const user_type =
    user_overload_flags == OP_MEMBER     ? L_MEMBER     :
    user_overload_flags == OP_NON_MEMBER ? L_NON_MEMBER :
    op_type;

  if ( user_overload_flags == OP_UNSPECIFIED ) {
    //
    // If the user didn't specify either member or non-member explicitly...
    //
    switch ( op_overload_flags ) {
      //
      // ...and the operator can not be both, then assume the user meant the
      // one the operator can only be.
      //
      case OP_MEMBER:
      case OP_NON_MEMBER:
        user_overload_flags = op_overload_flags;
        break;
      //
      // ...and the operator can be either one, then infer which one based on
      // the number of arguments given.
      //
      case OP_MEMBER | OP_NON_MEMBER:
        if ( n_args == op->args_min )
          user_overload_flags = OP_MEMBER;
        else if ( n_args == op->args_max )
          user_overload_flags = OP_NON_MEMBER;
        break;
    } // switch
  }
  else if ( (user_overload_flags & op_overload_flags) == 0 ) {
    //
    // The user specified either member or non-member, but the operator can't
    // be that.
    //
    print_error( &ast->loc,
      "%s %s can only be a %s",
      L_OPERATOR, op->name, op_type
    );
    return false;
  }

  //
  // Determine the minimum and maximum number of arguments the operator can
  // have based on whether it's a member, non-member, or unspecified.
  //
  bool const is_ambiguous = op_is_ambiguous( op );
  unsigned req_args_min = 0, req_args_max = 0;
  switch ( user_overload_flags ) {
    case OP_NON_MEMBER:
      // Non-member operators must always take at least one argument (the enum,
      // class, struct, or union for which it's overloaded).
      req_args_min = is_ambiguous ? 1 : op->args_max;
      req_args_max = op->args_max;
      break;
    case OP_MEMBER:
      if ( op->args_max != OP_ARGS_UNLIMITED ) {
        req_args_min = op->args_min;
        req_args_max = is_ambiguous ? 1 : op->args_min;
        break;
      }
      // FALLTHROUGH
    case OP_UNSPECIFIED:
      req_args_min = op->args_min;
      req_args_max = op->args_max;
      break;
  } // switch

  //
  // Ensure the operator has the required number of arguments.
  //
  if ( n_args < req_args_min ) {
    if ( req_args_min == req_args_max )
same: print_error( &ast->loc,
        "%s%s%s %s must have exactly %u argument%s",
        SP_AFTER( user_type ), L_OPERATOR, op->name,
        req_args_min, plural_s( req_args_min )
      );
    else
      print_error( &ast->loc,
        "%s%s%s %s must have at least %u argument%s",
        SP_AFTER( user_type ), L_OPERATOR, op->name,
        req_args_min, plural_s( req_args_min )
      );
    return false;
  }
  if ( n_args > req_args_max ) {
    if ( op->args_min == req_args_max )
      goto same;
    print_error( &ast->loc,
      "%s%s%s %s can have at most %u argument%s",
      SP_AFTER( user_type ), L_OPERATOR, op->name,
      op->args_max, plural_s( op->args_max )
    );
    return false;
  }

  bool const is_user_non_member = user_overload_flags == OP_NON_MEMBER;
  if ( is_user_non_member ) {
    //
    // Ensure non-member operators are not const, defaulted, deleted,
    // overridden, final, reference, rvalue reference, nor virtual.
    //
    c_type_id_t const member_only_types = ast->type_id & T_MEMBER_ONLY;
    if ( member_only_types != T_NONE ) {
      print_error( &ast->loc,
        "%s operators can not be %s",
        L_NON_MEMBER,
        c_type_name_error( member_only_types )
      );
      return false;
    }

    //
    // Ensure non-member operators have at least one enum, class, struct, or
    // union argument.
    //
    bool has_ecsu_arg = false;
    for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
      c_ast_t const *const arg_ast = c_ast_arg_ast( arg );
      if ( c_ast_is_ecsu( arg_ast ) ) {
        has_ecsu_arg = true;
        break;
      }
    } // for
    if ( !has_ecsu_arg ) {
      print_error( &ast->loc,
        "at least 1 argument of a %s %s must be an %s"
        "; or a %s or %s %s thereto",
        L_NON_MEMBER, L_OPERATOR, c_kind_name( K_ENUM_CLASS_STRUCT_UNION ),
        L_REFERENCE, L_RVALUE, L_REFERENCE
      );
      return false;
    }
  }
  else if ( user_overload_flags == OP_MEMBER ) {
    //
    // Ensure member operators are not friend.
    //
    c_type_id_t const non_member_only_types =
      ast->type_id & T_NON_MEMBER_ONLY;
    if ( non_member_only_types != T_NONE ) {
      print_error( &ast->loc,
        "%s operators can not be %s",
        L_MEMBER,
        c_type_name_error( non_member_only_types )
      );
      return false;
    }
  }

  switch ( ast->as.oper.oper_id ) {
    case OP_MINUS2:
    case OP_PLUS2: {
      //
      // Ensure that the dummy argument for postfix -- or ++ is type int (or is
      // a typedef of int).
      //
      c_ast_arg_t const *arg = c_ast_args( ast );
      if ( arg == NULL )              // member prefix
        break;
      if ( is_user_non_member ) {
        arg = arg->next;
        if ( arg == NULL )            // non-member prefix
          break;
      }
      // At this point, it's either member or non-member postfix:
      // operator++(int) or operator++(S&,int).
      c_ast_t const *const arg_ast = c_ast_arg_ast( arg );
      if ( !c_ast_is_builtin( arg_ast, T_INT ) ) {
        print_error( &arg_ast->loc,
          "argument of postfix %s%s%s %s must be %s",
          SP_AFTER( op_type ), L_OPERATOR, op->name,
          c_type_name_error( T_INT )
        );
        return false;
      }
      break;
    }

    default:
      /* suppress warning */;
  } // switch
  return true;
}

/**
 * Checks a pointer or pointer-to-member AST for errors.
 *
 * @param ast The pointer or pointer-to-member `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_pointer( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & K_ANY_POINTER) != K_NONE );

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind ) {
    case K_NAME:
      error_unknown_type( to_ast );
      return false;
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      print_error( &ast->loc,
        "%s to %s", c_kind_name( ast->kind ), c_kind_name( to_ast->kind )
      );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( (to_ast->type_id & T_REGISTER) != T_NONE ) {
    error_kind_to_type( ast, T_REGISTER );
    return false;
  }

  return true;
}

/**
 * Checks a pointer or pointer-to-member AST for errors.
 *
 * @param ast The pointer or pointer-to-member `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_reference( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & K_ANY_REFERENCE) != K_NONE );

  if ( !C_LANG_IS_CPP() ) {
    error_kind_not_supported( ast );
    return false;
  }

  if ( (ast->type_id & (T_CONST | T_VOLATILE)) != T_NONE ) {
    print_error( &ast->loc,
      "references can not be %s",
      c_type_name_error( ast->type_id & T_MASK_QUALIFIER )
    );
    print_hint(
      "reference to %s",
      c_type_name_error( ast->type_id & T_MASK_QUALIFIER )
    );
    return false;
  }

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind ) {
    case K_NAME:
      return error_unknown_type( to_ast );
    default:
      /* suppress warning */;
  } // switch

  if ( (to_ast->type_id & T_REGISTER) != T_NONE ) {
    error_kind_to_type( ast, T_REGISTER );
    return false;
  }

  if ( (to_ast->type_id & T_VOID) != T_NONE ) {
    error_kind_to_type( ast, T_VOID );
    print_hint( "pointer to void" );
    return false;
  }

  return true;
}

/**
 * Checks the return type of a function-like AST for errors.
 *
 * @param ast The function-like `c_ast` to check the return type of.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_ret_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind & K_FUNCTION_LIKE) != K_NONE );

  char const *const kind_name = c_kind_name( ast->kind );
  c_ast_t const *const ret_ast = ast->as.func.ret_ast;

  switch ( ret_ast->kind ) {
    case K_ARRAY:
      print_error( &ret_ast->loc, "%s returning array", kind_name );
      print_hint( "%s returning pointer", kind_name );
      return false;
    case K_BUILTIN:
      if ( opt_lang < LANG_CPP_14 ) {
        if ( (ret_ast->type_id & T_AUTO_CPP_11) != T_NONE ) {
          print_error( &ret_ast->loc,
            "\"%s\" return type not supported in %s",
            L_AUTO, C_LANG_NAME()
          );
          return false;
        }
      }
      break;
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      print_error( &ret_ast->loc,
        "%s returning %s",
        kind_name,
        c_kind_name( ret_ast->kind )
      );
      print_hint( "%s returning pointer to function", kind_name );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( (ast->type_id & T_EXPLICIT) != T_NONE ) {
    error_kind_not_type( ast, T_EXPLICIT );
    return false;
  }

  return true;
}

/**
 * Checks all user-defined literal arguments for semantic errors.
 *
 * @param ast The user-defined literal `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_user_def_lit_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_USER_DEF_LITERAL );

  size_t const args_count = c_ast_args_count( ast );
  if ( args_count == 0 ) {
    print_error( &ast->loc,
      "%s %s must have an argument", L_USER_DEFINED, L_LITERAL
    );
    return false;
  }

  c_ast_arg_t const *arg = c_ast_args( ast );
  c_ast_t const *arg_ast = c_ast_untypedef( c_ast_arg_ast( arg ) );
  c_ast_t const *tmp_ast = NULL;

  switch ( args_count ) {
    case 1: {
      c_type_id_t const type_id = arg_ast->type_id & ~T_MASK_QUALIFIER;
      switch ( type_id ) {
        case T_CHAR:
        case T_CHAR8_T:
        case T_CHAR16_T:
        case T_CHAR32_T:
        case T_WCHAR_T:
        case T_UNSIGNED | T_LONG | T_LONG_LONG:
        case T_LONG | T_DOUBLE:
          break;
        default:                        // check for: char const*
          tmp_ast = c_ast_unpointer( arg_ast );
          if ( tmp_ast == NULL || tmp_ast->type_id != (T_CONST | T_CHAR) ) {
            print_error( &arg_ast->loc,
              "\"%s\": invalid argument type for %s %s; must be one of: "
              "unsigned long long, long double, "
              "char, const char*, char8_t, char16_t, char32_t, wchar_t",
              c_type_name_error( arg_ast->type_id ), L_USER_DEFINED, L_LITERAL
            );
            return false;
          }
      } // switch
      break;
    }

    case 2:
      tmp_ast = c_ast_unpointer( arg_ast );
      if ( tmp_ast == NULL ||
           !((tmp_ast->type_id & T_CONST) != 0 &&
             (tmp_ast->type_id & T_ANY_CHAR) != 0) ) {
        print_error( &arg_ast->loc,
          "\"%s\": invalid argument type for %s %s; must be one of: "
          "const (char|wchar_t|char8_t|char16_t|char32_t)*",
          c_type_name_error( arg_ast->type_id ), L_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      arg = arg->next;
      tmp_ast = c_ast_untypedef( c_ast_arg_ast( arg ) );
      if ( tmp_ast == NULL || tmp_ast->type_id != (T_UNSIGNED | T_LONG) ) {
        print_error( &arg_ast->loc,
          "\"%s\": invalid argument type for %s %s; must be one of: "
          "unsigned long, size_t",
          c_type_name_error( arg_ast->type_id ), L_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      break;

    default:
      arg = arg->next->next;
      arg_ast = c_ast_untypedef( c_ast_arg_ast( arg ) );
      print_error( &arg_ast->loc,
        "%s %s may have at most 2 arguments", L_USER_DEFINED, L_LITERAL
      );
      return false;
  } // switch

  return true;
}

/**
 * Visitor function that checks an AST for semantic errors.
 *
 * @param ast The `c_ast` to check.
 * @param data Cast to `bool`, indicates if \a ast is a function argument.
 * @return Returns `VISITOR_ERROR_FOUND` if an error was found;
 * `VISITOR_ERROR_NOT_FOUND` if not.
 */
static bool c_ast_visitor_error( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  bool const is_func_arg = REINTERPRET_CAST( bool, data );

  if ( ast->align.kind != ALIGNAS_NONE && !c_ast_check_alignas( ast ) )
    return VISITOR_ERROR_FOUND;

  switch ( ast->kind ) {
    case K_ARRAY:
      if ( !c_ast_check_array( ast, is_func_arg ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_BUILTIN:
      if ( !c_ast_check_builtin( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( !c_ast_check_ecsu( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_OPERATOR:
      if ( !c_ast_check_oper( ast ) )
        return VISITOR_ERROR_FOUND;
      // FALLTHROUGH

    case K_FUNCTION:
      if ( !c_ast_check_func( ast ) )
        return VISITOR_ERROR_FOUND;
      // FALLTHROUGH

    case K_BLOCK:                       // Apple extension
    case K_USER_DEF_LITERAL:
      if ( !c_ast_check_ret_type( ast ) )
        return VISITOR_ERROR_FOUND;
      // FALLTHROUGH

    case K_CONSTRUCTOR: {
      if ( ast->kind == K_CONSTRUCTOR ) {
        c_type_id_t const t = ast->type_id & ~T_CONSTRUCTOR;
        if ( t != T_NONE )
          return error_kind_not_type( ast, t );
      }
      bool const args_ok =
        ast->kind == K_USER_DEF_LITERAL ?
          c_ast_check_user_def_lit_args( ast ) :
          opt_lang == LANG_C_KNR ?
            c_ast_check_func_args_knr( ast ) :
            c_ast_check_func_args( ast );
      if ( !args_ok )
        return VISITOR_ERROR_FOUND;
    }
      // FALLTHROUGH

    case K_DESTRUCTOR: {
      if ( (ast->kind & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE &&
           !c_ast_check_ctor_dtor( ast ) ) {
        return VISITOR_ERROR_FOUND;
      }

      c_type_id_t const t = ast->type_id & T_NOT_FUNC_LIKE;
      if ( t != T_NONE )
        return error_kind_not_type( ast, t );
      break;
    }

    case K_NAME:
    case K_TYPEDEF:
    case K_USER_DEF_CONVERSION:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
    case K_PLACEHOLDER:
      assert( ast->kind != K_PLACEHOLDER );

    case K_POINTER_TO_MEMBER:
      if ( !C_LANG_IS_CPP() )
        return error_kind_not_supported( ast );
      // FALLTHROUGH
    case K_POINTER:
      if ( !c_ast_check_pointer( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_lang < LANG_CPP_11 )
        return error_kind_not_supported( ast );
      // FALLTHROUGH
    case K_REFERENCE:
      if ( !c_ast_check_reference( ast ) )
        return VISITOR_ERROR_FOUND;
      break;
  } // switch

  if ( ast->kind != K_FUNCTION && (ast->type_id & T_CONSTEVAL) != T_NONE ) {
    print_error( &ast->loc, "only functions can be %s", L_CONSTEVAL );
    return VISITOR_ERROR_FOUND;
  }

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for type errors.
 *
 * @param ast The `c_ast` to visit.
 * @param data Cast to `bool`, indicates if \a ast is a function argument.
 * @return Returns `VISITOR_ERROR_FOUND` if an error was found;
 * `VISITOR_ERROR_NOT_FOUND` if not.
 */
static bool c_ast_visitor_type( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  bool const is_func_arg = REINTERPRET_CAST( bool, data );

  c_lang_id_t const lang_ids = c_type_check( ast->type_id );
  if ( lang_ids != LANG_ALL ) {
    if ( lang_ids == LANG_NONE )
      print_error( &ast->loc,
        "\"%s\" is illegal", c_type_name_error( ast->type_id )
      );
    else
      print_error( &ast->loc,
        "\"%s\" is illegal in %s",
        c_type_name_error( ast->type_id ), C_LANG_NAME()
      );
    return VISITOR_ERROR_FOUND;
  }

  switch ( ast->kind ) {
    case K_USER_DEF_CONVERSION: {
      if ( (ast->type_id & ~T_USER_DEF_CONV) != T_NONE ) {
        print_error( &ast->loc,
          "%s %s %ss can only be: %s",
          L_USER_DEFINED, L_CONVERSION, L_OPERATOR,
          c_type_name_error( T_USER_DEF_CONV )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( (ast->type_id & T_FRIEND) != T_NONE && c_ast_sname_empty( ast ) ) {
        print_error( &ast->loc,
          "%s %s %s %s must use qualified name",
          L_FRIEND, L_USER_DEFINED, L_CONVERSION, L_OPERATOR
        );
        return VISITOR_ERROR_FOUND;
      }
      c_ast_t const *const conv_ast =
        c_ast_untypedef( ast->as.user_def_conv.conv_ast );
      if ( conv_ast->kind == K_ARRAY ) {
        print_error( &conv_ast->loc,
          "%s %s %s can not convert to an %s",
          L_USER_DEFINED, L_CONVERSION, L_OPERATOR, L_ARRAY
        );
        print_hint( "pointer to array" );
        return VISITOR_ERROR_FOUND;
      }
      break;
    }

    case K_BLOCK:                       // Apple extension
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      data = REINTERPRET_CAST( void*, true );
      for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
        if ( !c_ast_check_visitor( c_ast_arg_ast( arg ), c_ast_visitor_type,
                                   data ) ) {
          return VISITOR_ERROR_FOUND;
        }
      } // for
      if ( (ast->kind & (K_FUNCTION | K_OPERATOR)) != K_NONE )
        break;
      // FALLTHROUGH

    default:
      if ( !is_func_arg && (ast->type_id & T_CARRIES_DEPENDENCY) != T_NONE ) {
        print_error( &ast->loc,
          "\"%s\" can only appear on functions or function arguments",
          c_type_name_error( T_CARRIES_DEPENDENCY )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( (ast->type_id & T_NORETURN) != T_NONE ) {
        print_error( &ast->loc,
          "\"%s\" can only appear on functions",
          c_type_name_error( T_NORETURN )
        );
        return VISITOR_ERROR_FOUND;
      }
  } // switch

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for semantic warnings.
 *
 * @param ast The `c_ast` to check.
 * @param data Not used.
 * @return Always returns `false`.
 */
static bool c_ast_visitor_warning( c_ast_t *ast, void *data ) {
  assert( ast != NULL );

  switch ( ast->kind ) {
    case K_ARRAY:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
    case K_USER_DEF_CONVERSION:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_USER_DEF_LITERAL:
      if ( c_sname_local_name( &ast->sname )[0] != '_' )
        print_warning( &ast->loc,
          "%s %s not starting with '_' are reserved",
          L_USER_DEFINED, L_LITERAL
        );
      // FALLTHROUGH
    case K_BLOCK:
    case K_FUNCTION:
    case K_OPERATOR: {
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      if ( (ast->type_id & T_NODISCARD) != T_NONE &&
           (ret_ast->type_id & T_VOID) != T_NONE ) {
        print_warning( &ast->loc,
          "[[%s]] %ss can not return %s",
          L_NODISCARD, c_kind_name( ast->kind ), L_VOID
        );
      }
      for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
        (void)c_ast_check_visitor(
          c_ast_arg_ast( arg ), c_ast_visitor_warning, data
        );
      } // for
      break;
    }

    case K_BUILTIN:
      if ( (ast->type_id & T_REGISTER) != T_NONE && opt_lang >= LANG_CPP_11 ) {
        print_warning( &ast->loc,
          "\"%s\" is deprecated in %s", L_REGISTER, C_LANG_NAME()
        );
      }
      break;

    case K_NAME:
      if ( opt_lang > LANG_C_KNR )
        print_warning( &ast->loc, "missing type specifier" );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
    case K_PLACEHOLDER:
      assert( ast->kind != K_PLACEHOLDER );
  } // switch

  for ( c_scope_t const *scope = c_ast_scope( ast ); scope != NULL;
        scope = scope->next ) {
    char const *const name = c_scope_name( scope );
    c_keyword_t const *const keyword = c_keyword_find( name, LANG_ALL );
    if ( keyword != NULL ) {
      print_warning( &ast->loc,
        "\"%s\" is a keyword in %s",
        name, c_lang_name( c_lang_oldest( keyword->lang_ids ) )
      );
    }
  } // for

  return false;
}

/**
 * Print an error: `can not cast into <kind>`.
 *
 * @param ast The `c_ast` .
 * @param hint The hint, if any.
 * @return Always returns `false`.
 */
static bool error_kind_not_cast_into( c_ast_t const *ast, char const *hint ) {
  assert( ast != NULL );
  print_error( &ast->loc, "can not cast into %s", c_kind_name( ast->kind ) );
  if ( hint != NULL )
    print_hint( "cast into %s", hint );
  return false;
}

/**
 * Prints an error: `<kind> can not be <type>`.
 *
 * @param ast The `c_ast` .
 * @param type_id The bad type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
static bool error_kind_not_type( c_ast_t const *ast, c_type_id_t type_id ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s can not be %s", c_kind_name( ast->kind ), c_type_name_error( type_id )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: `<kind> not supported in <lang>`.
 *
 * @param ast The `c_ast` having the bad kind.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
static bool error_kind_not_supported( c_ast_t const *ast ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s not supported in %s", c_kind_name( ast->kind ), C_LANG_NAME()
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: `<kind> to <type>`.
 *
 * @param ast The `c_ast` having the bad kind.
 * @param type_id The bad type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
static bool error_kind_to_type( c_ast_t const *ast, c_type_id_t type_id ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s to %s", c_kind_name( ast->kind ), c_type_name_error( type_id )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: `"<identifier>": unknown type`.
 *
 * @param ast The `c_ast` of the unknown type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
static bool error_unknown_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "\"%s\": unknown type", c_ast_sname_full_name( ast )
  );
  return VISITOR_ERROR_FOUND;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check( c_ast_t const *ast, c_check_t check ) {
  assert( ast != NULL );
  if ( check == CHECK_CAST && !c_ast_check_cast( ast ) )
    return false;
  if ( !c_ast_check_errors( ast, false ) )
    return false;
  (void)c_ast_check_visitor( ast, c_ast_visitor_warning, NULL );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
