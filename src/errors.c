/*
**      cdecl -- C gibberish translator
**      src/errors.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_type.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

// local constants
static bool const VISITOR_ERROR_FOUND     = true;
static bool const VISITOR_ERROR_NOT_FOUND = false;

/// @endcond

// local functions
C_WARN_UNUSED_RESULT
static bool c_ast_check_oper_args( c_ast_t const* );

C_WARN_UNUSED_RESULT
static bool c_ast_check_func_c( c_ast_t const* );

C_WARN_UNUSED_RESULT
static bool c_ast_check_func_cpp( c_ast_t const* );

C_WARN_UNUSED_RESULT
static bool c_ast_check_func_main( c_ast_t const* );

C_WARN_UNUSED_RESULT
static bool c_ast_check_oper_new_args( c_ast_t const* );

C_WARN_UNUSED_RESULT
static bool c_ast_check_oper_delete_args( c_ast_t const* );

C_WARN_UNUSED_RESULT
static bool c_ast_visitor_error( c_ast_t*, void* );

C_WARN_UNUSED_RESULT
static bool c_ast_visitor_type( c_ast_t*, void* );

C_NOWARN_UNUSED_RESULT
static bool error_kind_not_cast_into( c_ast_t const*, char const* );

C_NOWARN_UNUSED_RESULT
static bool error_kind_not_supported( c_ast_t const* );

C_NOWARN_UNUSED_RESULT
static bool error_kind_not_type( c_ast_t const*, c_type_id_t );

C_NOWARN_UNUSED_RESULT
static bool error_kind_to_kind( c_ast_t const*, c_kind_id_t );

C_NOWARN_UNUSED_RESULT
static bool error_kind_to_type( c_ast_t const*, c_type_id_t );

C_NOWARN_UNUSED_RESULT
static bool error_unknown_type( c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets the alignas literal for the current language.
 *
 * @return Returns either `_Alignas` (for C) or `alignas` (for C++).
 */
C_WARN_UNUSED_RESULT
static inline char const* alignas_lang( void ) {
  return C_LANG_IS_CPP() ? L_ALIGNAS : L__ALIGNAS;
}

/**
 * Simple wrapper around `c_ast_find()`.
 *
 * @param ast The `c_ast` to check.
 * @param visitor The visitor to use.
 * @param data Optional data passed to `c_ast_visit()`.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static inline bool c_ast_check_visitor( c_ast_t const *ast,
                                        c_ast_visitor_t visitor, void *data ) {
  return !c_ast_find( ast, C_VISIT_DOWN, visitor, data );
}

/**
 * Returns an "s" or not based on \a n to pluralize a word.
 *
 * @param n A quantity.
 * @return Returns the empty string only if \a n == 1; otherwise returns "s".
 */
C_WARN_UNUSED_RESULT
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
C_WARN_UNUSED_RESULT
static bool c_ast_check_alignas( c_ast_t *ast ) {
  assert( ast != NULL );
  assert( ast->align.kind != C_ALIGNAS_NONE );

  if ( (ast->type_id & T_REGISTER) != T_NONE ) {
    print_error( &ast->loc,
      "\"%s\" can not be combined with \"%s\"", alignas_lang(), L_REGISTER
    );
    return false;
  }

  if ( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE ) {
    print_error( &ast->loc,
      "%s can not be %s", c_kind_name( ast->kind_id ), L_ALIGNED
    );
    return false;
  }

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_EXPR: {
      unsigned const alignment = ast->align.as.expr;
      if ( !at_most_one_bit_set( alignment ) ) {
        print_error( &ast->loc,
          "\"%u\": alignment is not a power of 2", alignment
        );
        return false;
      }
      break;
    }
    case C_ALIGNAS_TYPE:
      if ( !c_ast_check_declaration( ast->align.as.type_ast ) )
        return false;
      break;
  } // switch

  return true;
}

/**
 * Checks an array AST for errors.
 *
 * @param ast The array `c_ast` to check.
 * @param is_func_arg If `true`, \a ast is an AST for a function-like argument.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_array( c_ast_t const *ast, bool is_func_arg ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_ARRAY );

  if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE ) {
    if ( (opt_lang & LANG_C_MIN(99)) == LANG_NONE ) {
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
    if ( (opt_lang & LANG_C_MIN(99)) == LANG_NONE ) {
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
  switch ( of_ast->kind_id ) {
    case K_BUILTIN:
      if ( (of_ast->type_id & T_VOID) != T_NONE ) {
        print_error( &ast->loc, "%s of %s", L_ARRAY, L_VOID );
        print_hint( "%s of %s to %s", L_ARRAY, L_POINTER, L_VOID );
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
      print_error( &ast->loc,
        "%s of %s", L_ARRAY, c_kind_name( of_ast->kind_id )
      );
      print_hint( "%s of %s to %s", L_ARRAY, L_POINTER, L_FUNCTION );
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
C_WARN_UNUSED_RESULT
static bool c_ast_check_builtin( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_BUILTIN );

  if ( (ast->parent_ast == NULL ||
        ast->parent_ast->kind_id != K_USER_DEF_CONVERSION) &&
        (ast->type_id & T_MASK_TYPE) == T_NONE ) {
    print_error( &ast->loc,
      "implicit \"%s\" is illegal in %s", L_INT, C_LANG_NAME()
    );
    return false;
  }

  if ( (ast->type_id & T_VOID) != T_NONE && ast->parent_ast == NULL ) {
    print_error( &ast->loc, "variable of %s", L_VOID );
    print_hint( "%s to %s", L_POINTER, L_VOID );
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
 * Checks a constructor or destructor AST for errors.
 *
 * @param ast The constructor or destructor `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_ctor_dtor( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE );

  if ( c_ast_sname_count( ast ) > 1 && !c_ast_sname_is_ctor( ast ) ) {
    print_error( &ast->loc,
      "\"%s\", \"%s\": %s and %s names don't match",
      c_ast_sname_name_atr( ast, 1 ), c_ast_sname_local_name( ast ),
      c_type_name_error( c_ast_sname_local_type( ast ) ),
      c_kind_name( ast->kind_id )
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
C_WARN_UNUSED_RESULT
static bool c_ast_check_ecsu( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_ENUM_CLASS_STRUCT_UNION );

  if ( (ast->type_id & T_ANY_CLASS) != T_NONE &&
       (ast->type_id & T_REGISTER) != T_NONE ) {
    error_kind_not_type( ast, T_REGISTER );
    return false;
  }

  if ( c_mode == C_GIBBERISH_TO_ENGLISH &&
       (ast->type_id & T_ENUM) != T_NONE &&
       (ast->type_id & (T_STRUCT | T_CLASS)) != T_NONE &&
       (ast->type_id & T_TYPEDEF) == T_NONE ) {
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
C_WARN_UNUSED_RESULT
static bool c_ast_check_errors( c_ast_t const *ast, bool is_func_arg ) {
  assert( ast != NULL );
  // check in major-to-minor error order
  void *const data = REINTERPRET_CAST( void*, is_func_arg );
  return  c_ast_check_visitor( ast, c_ast_visitor_error, data ) &&
          c_ast_check_visitor( ast, c_ast_visitor_type, data );
}

/**
 * Checks a function-like AST for errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_func( c_ast_t const *ast ) {
  assert( ast != NULL );

  if ( ast->kind_id == K_FUNCTION ) {
    SNAME_VAR_INIT( main_sname, "main" );
    if ( c_sname_cmp( &ast->sname, &main_sname ) == 0 &&
         !c_ast_check_func_main( ast ) ) {
      return false;
    }
  }

  return C_LANG_IS_C() ?
    c_ast_check_func_c( ast ) :
    c_ast_check_func_cpp( ast );
}

/**
 * Checks all function-like arguments for semantic errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_func_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );
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

    c_type_id_t const arg_storage_type =
      arg_ast->type_id & (T_MASK_STORAGE & ~T_REGISTER);
    if ( arg_storage_type != T_NONE ) {
      print_error( &arg_ast->loc,
        "%s arguments can not be %s",
        c_kind_name( ast->kind_id ),
        c_type_name_error( arg_storage_type )
      );
      return false;
    }

    switch ( arg_ast->kind_id ) {
      case K_BUILTIN:
        if ( (arg_ast->type_id & T_AUTO_TYPE) != T_NONE ) {
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
        if ( ast->kind_id == K_OPERATOR &&
             ast->as.oper.oper_id != C_OP_PARENS ) {
          print_error( &arg_ast->loc,
            "%s %s can not have a %s argument",
            L_OPERATOR, c_oper_get( ast->as.oper.oper_id )->name, L_VARIADIC
          );
          return false;
        }
        if ( arg->next != NULL ) {
          print_error( &arg_ast->loc, "%s specifier must be last", L_VARIADIC );
          return false;
        }
        variadic_ast = arg_ast;
        continue;

      default:
        /* suppress warning */;
    } // switch

    if ( !c_ast_check_errors( arg_ast, true ) )
      return false;
  } // for

  if ( ast->kind_id == K_OPERATOR && !c_ast_check_oper_args( ast ) )
    return false;

  if ( variadic_ast != NULL && n_args == 1 ) {
    print_error( &variadic_ast->loc,
      "%s specifier can not be only argument", L_VARIADIC
    );
    return false;
  }

  return true;

only_void:
  print_error( &void_ast->loc,
    "\"%s\" must be only parameter if specified", L_VOID
  );
  return false;
}

/**
 * Checks all function arguments for semantic errors in K&R C.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_func_args_knr( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_APPLE_BLOCK | K_FUNCTION)) != K_NONE );
  assert( opt_lang == LANG_C_KNR );

  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    c_ast_t const *const arg_ast = c_ast_arg_ast( arg );
    switch ( arg_ast->kind_id ) {
      case K_NAME:
        break;
      case K_PLACEHOLDER:
        assert( arg_ast->kind_id != K_PLACEHOLDER );
        break;
      default:
        print_error( &arg_ast->loc,
          "%s prototypes not supported in %s", L_FUNCTION, C_LANG_NAME()
        );
        return false;
    } // switch
  } // for
  return true;
}

/**
 * Checks a C function (or block) AST for errors.
 *
 * @param ast The function (or block) AST to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_func_c( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_APPLE_BLOCK | K_FUNCTION)) != K_NONE );
  assert( C_LANG_IS_C() );

  c_type_id_t const qual_type = ast->type_id & T_MASK_QUALIFIER;
  if ( qual_type != T_NONE ) {
    print_error( &ast->loc,
      "\"%s\" %ss not supported in %s",
      c_type_name_error( qual_type ),
      c_kind_name( ast->kind_id ),
      C_LANG_NAME()
    );
    return false;
  }
  return true;
}

/**
 * Checks a C++ function-like AST for errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_func_cpp( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );
  assert( C_LANG_IS_CPP() );

  if ( (ast->type_id & T_ANY_REFERENCE) != T_NONE ) {
    if ( opt_lang < LANG_CPP_11 ) {
      print_error( &ast->loc,
        "%s qualified %ss not supported in %s",
        L_REFERENCE, c_kind_name( ast->kind_id ), C_LANG_NAME()
      );
      return false;
    }
    if ( (ast->type_id & (T_EXTERN | T_STATIC)) != T_NONE ) {
      print_error( &ast->loc,
        "%s qualified %ss can not be %s",
        L_REFERENCE, c_kind_name( ast->kind_id ),
        c_type_name_error( ast->type_id & (T_EXTERN | T_STATIC) )
      );
      return false;
    }
  }

  c_type_id_t const member_func_types = ast->type_id & T_MEMBER_FUNC_ONLY;
  c_type_id_t const nonmember_func_types = ast->type_id & T_NONMEMBER_FUNC_ONLY;

  if ( member_func_types != T_NONE &&
       (ast->type_id & (T_EXTERN | T_STATIC)) != T_NONE ) {
    // Need this since c_type_name_error() can't be called more than once in
    // the same expression.
    char const *const member_func_types_names =
      check_strdup( c_type_name_error( member_func_types ) );

    print_error( &ast->loc,
      "%s %ss can not be %s",
      c_type_name_error( ast->type_id & (T_EXTERN | T_STATIC) ),
      c_kind_name( ast->kind_id ),
      member_func_types_names
    );
    FREE( member_func_types_names );
    return false;
  }

  if ( member_func_types != T_NONE && nonmember_func_types != T_NONE ) {
    // Need this since c_type_name_error() can't be called more than once in
    // the same expression.
    char const *const member_func_types_names =
      check_strdup( c_type_name_error( member_func_types ) );

    print_error( &ast->loc,
      "%ss can not be %s and %s",
      c_kind_name( ast->kind_id ),
      member_func_types_names,
      c_type_name_error( nonmember_func_types )
    );
    FREE( member_func_types_names );
    return false;
  }

  unsigned const user_overload_flags = ast->as.func.flags & C_FUNC_MASK_MEMBER;
  switch ( user_overload_flags ) {
    case C_FUNC_MEMBER:
      if ( nonmember_func_types != T_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s",
          L_MEMBER, c_kind_name( ast->kind_id ),
          c_type_name_error( nonmember_func_types )
        );
        return false;
      }
      break;
    case C_FUNC_NON_MEMBER:
      if ( member_func_types != T_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s",
          L_NON_MEMBER, c_kind_name( ast->kind_id ),
          c_type_name_error( member_func_types )
        );
        return false;
      }
      break;
  } // switch

  if ( (ast->type_id & (T_DEFAULT | T_DELETE)) != T_NONE ) {
    c_ast_t const *ret_ast = NULL;
    switch ( ast->kind_id ) {
      case K_OPERATOR:                // C& operator=(C const&)
        if ( ast->as.oper.oper_id != C_OP_EQ )
          goto only_special;
        ret_ast = ast->as.oper.ret_ast;
        if ( !c_ast_is_ref_to_type_any( ret_ast, T_ANY_CLASS ) )
          goto only_special;
        C_FALLTHROUGH;
      case K_CONSTRUCTOR: {           // C(C const&)
        if ( c_ast_args_count( ast ) != 1 )
          goto only_special;
        c_ast_t const *arg_ast = c_ast_arg_ast( c_ast_args( ast ) );
        if ( !c_ast_is_ref_to_type_any( arg_ast, T_ANY_CLASS ) )
          goto only_special;
        if ( ast->kind_id == K_OPERATOR ) {
          assert( ret_ast != NULL );
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

  if ( (ast->type_id & T_NO_UNIQUE_ADDRESS) != T_NONE ) {
    error_kind_not_type( ast, T_NO_UNIQUE_ADDRESS );
    return false;
  }

  if ( (ast->type_id & T_VIRTUAL) != T_NONE ) {
    if ( c_ast_sname_count( ast ) > 1 ) {
      print_error( &ast->loc,
        "\"%s\": %s can not be used in file-scoped %ss",
        c_ast_sname_full_name( ast ), L_VIRTUAL, c_kind_name( ast->kind_id )
      );
      return false;
    }
  }
  else if ( (ast->type_id & T_PURE_VIRTUAL) != T_NONE ) {
    print_error( &ast->loc,
      "non-%s %ss can not be %s",
      L_VIRTUAL, c_kind_name( ast->kind_id ), L_PURE
    );
    return false;
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
 * Checks the return type and arguments for `main()`.
 *
 * @param ast The main function AST to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_func_main( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_FUNCTION );

  if ( ast->type_id != T_NONE ) {
    print_error( &ast->loc,
      "main() can not be %s",
      c_type_name_error( ast->type_id )
    );
    return false;
  }

  c_ast_t const *const ret_ast = ast->as.func.ret_ast;
  if ( !c_ast_is_builtin( ret_ast, T_INT ) ) {
    print_error( &ret_ast->loc, "main() must return int" );
    return false;
  }

  c_ast_t const *arg_ast;

  switch ( c_ast_args_count( ast ) ) {
    case 0:                             // main()
      break;

    case 1:                             // main(void) ?
      arg_ast = c_ast_arg_ast( c_ast_args( ast ) );
      if ( !c_ast_is_builtin( arg_ast, T_VOID ) ) {
        print_error( &arg_ast->loc,
          "a single argument for main() must be %s", L_VOID
        );
        return false;
      }
      break;

    case 2: {                           // main( int, char *argv[] ) ?
      c_ast_arg_t const *arg = c_ast_args( ast );
      arg_ast = c_ast_arg_ast( arg );
      if ( !c_ast_is_builtin( arg_ast, T_INT ) ) {
        print_error( &arg_ast->loc, "main()'s first argument must be int" );
        return false;
      }
      arg = arg->next;
      arg_ast = c_ast_untypedef( c_ast_arg_ast( arg ) );
      switch ( arg_ast->kind_id ) {
        case K_ARRAY:                   // main( int, char *argv[] )
        case K_POINTER:                 // main( int, char **argv )
          if ( !c_ast_is_ptr_to_type( arg_ast->as.parent.of_ast,
                                      ~T_CONST, T_CHAR ) ) {
            print_error( &arg_ast->loc,
              "main()'s second argument must be %s %s %s to [%s] %s",
              c_kind_name( arg_ast->kind_id ),
              arg_ast->kind_id == K_ARRAY ? "of" : "to",
              L_POINTER, L_CONST, L_CHAR
            );
            return false;
          }
          break;

        default:                        // main( int, ??? )
          print_error( &arg_ast->loc, "illegal signature for main()" );
          return false;
      } // switch
      break;
    }

    default:
      print_error( &ast->loc, "main() must have either zero or two arguments" );
      return false;
  } // switch

  return true;
}

/**
 * Checks an overloaded operator AST for errors.
 *
 * @param ast The overloaded operator `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_oper( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );

  if ( (opt_lang & op->lang_ids) == LANG_NONE ) {
    print_error( &ast->loc,
      "overloading %s \"%s\" not supported in %s",
      L_OPERATOR, op->name, C_LANG_NAME()
    );
    return false;
  }

  if ( (op->flags & C_OP_MASK_OVERLOAD) == C_OP_NOT_OVERLOADABLE ) {
    print_error( &ast->loc,
      "%s %s can not be overloaded",
      L_OPERATOR, op->name
    );
    return false;
  }

  switch ( ast->as.oper.oper_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for operators new, new[], delete, and delete[] that can
      // only have specific types.
      //
      if ( (ast->type_id & ~T_NEW_DELETE_OPER) != T_NONE ) {
        print_error( &ast->loc,
          "%s %s can not be %s",
          L_OPERATOR, op->name, c_type_name_error( ast->type_id )
        );
        return false;
      }
      break;
    default:
      /* suppress warning */;
  } // switch

  c_ast_t const *const ret_ast = ast->as.oper.ret_ast;

  switch ( ast->as.oper.oper_id ) {
    case C_OP_ARROW:
      //
      // Special case for operator-> that must return a pointer to a struct,
      // union, or class.
      //
      if ( !c_ast_is_ptr_to_type_any( ret_ast, T_ANY_CLASS ) ) {
        print_error( &ret_ast->loc,
          "%s %s must return a %s to %s, %s, or %s",
          L_OPERATOR, op->name, L_POINTER, L_STRUCT, L_UNION, L_CLASS
        );
        return false;
      }
      break;

    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for operators delete and delete[] that must return void.
      //
      if ( ret_ast->type_id != T_VOID ) {
        print_error( &ret_ast->loc,
          "%s %s must return %s",
          L_OPERATOR, op->name, L_VOID
        );
        return false;
      }
      break;

    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
      //
      // Special case for operators new and new[] that must return pointer to
      // void.
      //
      if ( !c_ast_is_ptr_to_type_any( ret_ast, T_VOID ) ) {
        print_error( &ret_ast->loc,
          "%s %s must return a %s to %s",
          L_OPERATOR, op->name, L_POINTER, L_VOID
        );
        return false;
      }
      break;

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
C_WARN_UNUSED_RESULT
static bool c_ast_check_oper_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );

  unsigned user_overload_flags = ast->as.oper.flags & C_OP_MASK_OVERLOAD;
  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );
  unsigned const op_overload_flags = op->flags & C_OP_MASK_OVERLOAD;
  size_t const n_args = c_ast_args_count( ast );

  char const *const op_type =
    op_overload_flags == C_OP_MEMBER     ? L_MEMBER     :
    op_overload_flags == C_OP_NON_MEMBER ? L_NON_MEMBER :
    "";
  char const *const user_type =
    user_overload_flags == C_OP_MEMBER     ? L_MEMBER     :
    user_overload_flags == C_OP_NON_MEMBER ? L_NON_MEMBER :
    op_type;

  if ( user_overload_flags == C_OP_UNSPECIFIED ) {
    //
    // If the user didn't specify either member or non-member explicitly...
    //
    switch ( op_overload_flags ) {
      //
      // ...and the operator can not be both, then assume the user meant the
      // one the operator can only be.
      //
      case C_OP_MEMBER:
      case C_OP_NON_MEMBER:
        user_overload_flags = op_overload_flags;
        break;
      //
      // ...and the operator can be either one, then infer which one based on
      // the number of arguments given.
      //
      case C_OP_MEMBER | C_OP_NON_MEMBER:
        if ( n_args == op->args_min )
          user_overload_flags = C_OP_MEMBER;
        else if ( n_args == op->args_max )
          user_overload_flags = C_OP_NON_MEMBER;
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
  bool const is_ambiguous = c_oper_is_ambiguous( op );
  unsigned req_args_min = 0, req_args_max = 0;
  switch ( user_overload_flags ) {
    case C_OP_NON_MEMBER:
      // Non-member operators must always take at least one argument (the enum,
      // class, struct, or union for which it's overloaded).
      req_args_min = is_ambiguous ? 1 : op->args_max;
      req_args_max = op->args_max;
      break;
    case C_OP_MEMBER:
      if ( op->args_max != C_OP_ARGS_UNLIMITED ) {
        req_args_min = op->args_min;
        req_args_max = is_ambiguous ? 1 : op->args_min;
        break;
      }
      C_FALLTHROUGH;
    case C_OP_UNSPECIFIED:
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

  bool const is_user_non_member = user_overload_flags == C_OP_NON_MEMBER;
  if ( is_user_non_member ) {
    //
    // Ensure non-member operators are not const, defaulted, deleted,
    // overridden, final, reference, rvalue reference, nor virtual.
    //
    c_type_id_t const member_only_types = ast->type_id & T_MEMBER_FUNC_ONLY;
    if ( member_only_types != T_NONE ) {
      print_error( &ast->loc,
        "%s %ss can not be %s",
        L_NON_MEMBER, L_OPERATOR,
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
      if ( c_ast_is_kind_any( arg_ast, K_ENUM_CLASS_STRUCT_UNION ) ) {
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
  else if ( user_overload_flags == C_OP_MEMBER ) {
    //
    // Ensure member operators are not friend.
    //
    c_type_id_t const non_member_only_types =
      ast->type_id & T_NONMEMBER_FUNC_ONLY;
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
    case C_OP_MINUS2:
    case C_OP_PLUS2: {
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

    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      return c_ast_check_oper_delete_args( ast );

    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
      return c_ast_check_oper_new_args( ast );

    default:
      /* suppress warning */;
  } // switch

  return true;
}

/**
 * Checks overloaded operator `delete` and `delete[]` arguments for semantic
 * errors.
 *
 * @param ast The user-defined operator `delete` `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_oper_delete_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );
  assert( ast->as.oper.oper_id == C_OP_DELETE ||
          ast->as.oper.oper_id == C_OP_DELETE_ARRAY );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );

  size_t const args_count = c_ast_args_count( ast );
  if ( args_count == 0 ) {
    print_error( &ast->loc,
      "%s %s must have at least one argument",
      L_OPERATOR, op->name
    );
    return false;
  }

  c_ast_arg_t const *const arg = c_ast_args( ast );
  c_ast_t const *const arg_ast = c_ast_arg_ast( arg );

  if ( !c_ast_is_ptr_to_type_any( arg_ast, T_VOID | T_ANY_CLASS ) ) {
    print_error( &arg_ast->loc,
      "invalid argument type for %s %s; must be a %s to %s, %s, %s, or %s",
      L_OPERATOR, op->name,
      L_POINTER, L_VOID, L_CLASS, L_STRUCT, L_UNION
    );
    return false;
  }

  return true;
}

/**
 * Checks overloaded operator `new` and `new[]` arguments for semantic errors.
 *
 * @param ast The user-defined operator `new` `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_oper_new_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );
  assert( ast->as.oper.oper_id == C_OP_NEW ||
          ast->as.oper.oper_id == C_OP_NEW_ARRAY );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );

  size_t const args_count = c_ast_args_count( ast );
  if ( args_count == 0 ) {
    print_error( &ast->loc,
      "%s %s must have at least one argument",
      L_OPERATOR, op->name
    );
    return false;
  }

  c_ast_arg_t const *const arg = c_ast_args( ast );
  c_ast_t const *const arg_ast = c_ast_untypedef( c_ast_arg_ast( arg ) );

  if ( !c_type_is_size_t( arg_ast->type_id ) ) {
    print_error( &arg_ast->loc,
      "invalid argument type for %s %s; must be std::size_t (or equivalent)",
      L_OPERATOR, op->name
    );
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
C_WARN_UNUSED_RESULT
static bool c_ast_check_pointer( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_POINTER) != K_NONE );

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind_id ) {
    case K_NAME:
      error_unknown_type( to_ast );
      return false;
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      print_error( &ast->loc,
        "%s to %s", c_kind_name( ast->kind_id ), c_kind_name( to_ast->kind_id )
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
 * Checks a reference or rvalue reference AST for errors.
 *
 * @param ast The pointer or pointer-to-member `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_reference( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_REFERENCE) != K_NONE );

  if ( (ast->type_id & (T_CONST | T_VOLATILE)) != T_NONE ) {
    c_type_id_t const t = ast->type_id & T_MASK_QUALIFIER;
    error_kind_not_type( ast, t );
    print_hint( "%s to %s", L_REFERENCE, c_type_name_error( t ) );
    return false;
  }

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind_id ) {
    case K_NAME:
      return error_unknown_type( to_ast );
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      error_kind_to_kind( ast, to_ast->kind_id );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( (to_ast->type_id & T_REGISTER) != T_NONE ) {
    error_kind_to_type( ast, T_REGISTER );
    return false;
  }

  if ( (to_ast->type_id & T_VOID) != T_NONE ) {
    error_kind_to_type( ast, T_VOID );
    print_hint( "%s to %s", L_POINTER, L_VOID );
    return false;
  }

  return true;
}

/**
 * Checks the return type of a function-like AST for errors.
 *
 * @param ast The function-like AST to check the return type of.
 * @return Returns `true` only if all checks passed.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_check_ret_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );

  char const *const kind_name = c_kind_name( ast->kind_id );
  c_ast_t const *const ret_ast = ast->as.func.ret_ast;

  switch ( ret_ast->kind_id ) {
    case K_ARRAY:
      print_error( &ret_ast->loc, "%s returning %s", kind_name, L_ARRAY );
      print_hint( "%s returning %s", kind_name, L_POINTER );
      return false;
    case K_BUILTIN:
      if ( opt_lang < LANG_CPP_14 ) {
        if ( (ret_ast->type_id & T_AUTO_TYPE) != T_NONE ) {
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
        kind_name, c_kind_name( ret_ast->kind_id )
      );
      print_hint( "%s returning %s to %s", kind_name, L_POINTER, L_FUNCTION );
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
C_WARN_UNUSED_RESULT
static bool c_ast_check_user_def_lit_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_USER_DEF_LITERAL );

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
        case T_UNSIGNED | T_LONG | T_LONG_LONG | T_INT:
        case T_LONG | T_DOUBLE:
          break;
        default:                        // check for: char const*
          if ( !c_ast_is_ptr_to_type( arg_ast, ~T_NONE, T_CONST | T_CHAR ) ) {
            print_error( &arg_ast->loc,
              "invalid argument type for %s %s; must be one of: "
              "unsigned long long, long double, "
              "char, const char*, char8_t, char16_t, char32_t, wchar_t",
              L_USER_DEFINED, L_LITERAL
            );
            return false;
          }
      } // switch
      break;
    }

    case 2:
      tmp_ast = c_ast_unpointer( arg_ast );
      if ( tmp_ast == NULL ||
           !((tmp_ast->type_id & T_CONST) != T_NONE &&
             (tmp_ast->type_id & T_ANY_CHAR) != T_NONE) ) {
        print_error( &arg_ast->loc,
          "invalid argument type for %s %s; must be one of: "
          "const (char|wchar_t|char8_t|char16_t|char32_t)*",
          L_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      arg = arg->next;
      arg_ast = c_ast_untypedef( c_ast_arg_ast( arg ) );
      if ( arg_ast == NULL || !c_type_is_size_t( arg_ast->type_id ) ) {
        print_error( &arg_ast->loc,
          "invalid argument type for %s %s; "
          "must be std::size_t (or equivalent)",
          L_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      break;

    default:
      arg = arg->next->next;
      arg_ast = c_ast_arg_ast( arg );
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
C_WARN_UNUSED_RESULT
static bool c_ast_visitor_error( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  bool const is_func_arg = REINTERPRET_CAST( bool, data );

  if ( ast->align.kind != C_ALIGNAS_NONE && !c_ast_check_alignas( ast ) )
    return VISITOR_ERROR_FOUND;

  switch ( ast->kind_id ) {
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
      C_FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
      if ( !c_ast_check_ret_type( ast ) )
        return VISITOR_ERROR_FOUND;
      C_FALLTHROUGH;

    case K_CONSTRUCTOR:
      if ( !c_ast_check_func( ast ) )
        return VISITOR_ERROR_FOUND;
      if ( ast->kind_id == K_CONSTRUCTOR ) {
        c_type_id_t const t = ast->type_id & ~T_CONSTRUCTOR;
        if ( t != T_NONE )
          return error_kind_not_type( ast, t );
      }
      {
        bool const args_ok =
          ast->kind_id == K_USER_DEF_LITERAL ?
            c_ast_check_user_def_lit_args( ast ) :
            opt_lang == LANG_C_KNR ?
              c_ast_check_func_args_knr( ast ) :
              c_ast_check_func_args( ast );
        if ( !args_ok )
          return VISITOR_ERROR_FOUND;
      }
      C_FALLTHROUGH;

    case K_DESTRUCTOR: {
      if ( (ast->kind_id & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE &&
           !c_ast_check_ctor_dtor( ast ) ) {
        return VISITOR_ERROR_FOUND;
      }

      c_type_id_t const t = ast->type_id & ~T_FUNC_LIKE;
      if ( t != T_NONE )
        return error_kind_not_type( ast, t );
      break;
    }

    case K_NAME:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_NONE:
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:
      assert( ast->kind_id != K_PLACEHOLDER );
      break;

    case K_POINTER_TO_MEMBER:
      if ( C_LANG_IS_C() )
        return error_kind_not_supported( ast );
      C_FALLTHROUGH;
    case K_POINTER:
      if ( !c_ast_check_pointer( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_lang < LANG_CPP_11 )
        return error_kind_not_supported( ast );
      C_FALLTHROUGH;
    case K_REFERENCE:
      if ( C_LANG_IS_C() )
        return error_kind_not_supported( ast );
      if ( !c_ast_check_reference( ast ) )
        return VISITOR_ERROR_FOUND;
      break;
  } // switch

  if ( ast->kind_id != K_FUNCTION && (ast->type_id & T_CONSTEVAL) != T_NONE ) {
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
C_WARN_UNUSED_RESULT
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

  switch ( ast->kind_id ) {
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
      if ( conv_ast->kind_id == K_ARRAY ) {
        print_error( &conv_ast->loc,
          "%s %s %s can not convert to an %s",
          L_USER_DEFINED, L_CONVERSION, L_OPERATOR, L_ARRAY
        );
        print_hint( "%s to %s", L_POINTER, L_ARRAY );
        return VISITOR_ERROR_FOUND;
      }
      break;
    }

    case K_APPLE_BLOCK:
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
      if ( (ast->kind_id & (K_FUNCTION | K_OPERATOR)) != K_NONE )
        break;
      C_FALLTHROUGH;

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

  if ( (ast->type_id & T_RESTRICT) != T_NONE ) {
    switch ( ast->kind_id ) {
      case K_FUNCTION:
      case K_OPERATOR:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_USER_DEF_CONVERSION:
        //
        // These being declared "restrict" in C is already made an error by
        // checks elsewhere.
        //
      case K_POINTER:
        break;
      default:
        return error_kind_not_type( ast, T_RESTRICT );
    } // switch
  }

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for semantic warnings.
 *
 * @param ast The `c_ast` to check.
 * @param data Not used.
 * @return Always returns `false`.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_visitor_warning( c_ast_t *ast, void *data ) {
  assert( ast != NULL );

  switch ( ast->kind_id ) {
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
      C_FALLTHROUGH;
    case K_APPLE_BLOCK:
    case K_FUNCTION:
    case K_OPERATOR: {
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      if ( (ast->type_id & T_NODISCARD) != T_NONE &&
           (ret_ast->type_id & T_VOID) != T_NONE ) {
        print_warning( &ast->loc,
          "[[%s]] %ss can not return %s",
          L_NODISCARD, c_kind_name( ast->kind_id ), L_VOID
        );
      }
      if ( (ast->type_id & T_THROW) != T_NONE && opt_lang >= LANG_CPP_11 )
        print_warning( &ast->loc,
          "\"%s\" is deprecated in %s", L_THROW, C_LANG_NAME()
        );

      for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
        C_IGNORE_RV(
          c_ast_check_visitor(
            c_ast_arg_ast( arg ), c_ast_visitor_warning, data
          )
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
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:
      assert( ast->kind_id != K_PLACEHOLDER );
      break;
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
C_NOWARN_UNUSED_RESULT
static bool error_kind_not_cast_into( c_ast_t const *ast, char const *hint ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "can not %s %s %s", L_CAST, L_INTO, c_kind_name( ast->kind_id )
  );
  if ( hint != NULL )
    print_hint( "%s %s %s", L_CAST, L_INTO, hint );
  return false;
}

/**
 * Prints an error: `<kind> can not be <type>`.
 *
 * @param ast The `c_ast` .
 * @param type_id The bad type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
C_NOWARN_UNUSED_RESULT
static bool error_kind_not_type( c_ast_t const *ast, c_type_id_t type_id ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s can not be %s",
    c_kind_name( ast->kind_id ), c_type_name_error( type_id )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: `<kind> not supported in <lang>`.
 *
 * @param ast The `c_ast` having the bad kind.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
C_NOWARN_UNUSED_RESULT
static bool error_kind_not_supported( c_ast_t const *ast ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s not supported in %s", c_kind_name( ast->kind_id ), C_LANG_NAME()
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: `<kind> to <kind>`.
 *
 * @param ast The `c_ast` having the bad kind.
 * @param kind_id The other kind.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
C_NOWARN_UNUSED_RESULT
static bool error_kind_to_kind( c_ast_t const *ast, c_kind_id_t kind_id ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s to %s", c_kind_name( ast->kind_id ), c_kind_name( kind_id )
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
C_NOWARN_UNUSED_RESULT
static bool error_kind_to_type( c_ast_t const *ast, c_type_id_t type_id ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s to %s", c_kind_name( ast->kind_id ), c_type_name_error( type_id )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: `"<identifier>": unknown type`.
 *
 * @param ast The `c_ast` of the unknown type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
C_NOWARN_UNUSED_RESULT
static bool error_unknown_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "\"%s\": unknown type", c_ast_sname_full_name( ast )
  );
  return VISITOR_ERROR_FOUND;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );

  c_ast_t const *const storage_ast =
    c_ast_find_type_any( nonconst_ast, C_VISIT_DOWN, T_MASK_STORAGE );

  if ( storage_ast != NULL ) {
    c_type_id_t const storage_type = storage_ast->type_id & T_MASK_STORAGE;
    print_error( &ast->loc,
      "can not %s %s %s", L_CAST, L_INTO, c_type_name_error( storage_type )
    );
    return false;
  }

  switch ( ast->kind_id ) {
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
      /* suppress warning */;
  } // switch

  return c_ast_check_declaration( ast );
}

bool c_ast_check_declaration( c_ast_t const *ast ) {
  assert( ast != NULL );
  if ( !c_ast_check_errors( ast, false ) )
    return false;
  C_IGNORE_RV( c_ast_check_visitor( ast, c_ast_visitor_warning, NULL ) );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
