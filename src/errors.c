/*
**      cdecl -- C gibberish translator
**      src/errors.c
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
 * Defines functions for performing semantic checks on an AST.
 */

// local
#include "config.h"                     /* must go first */
#include "c_ast_util.h"
#include "c_type.h"
#include "diagnostics.h"
#include "literals.h"
#include "options.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>

// local constants
static bool const VISITOR_ERROR_FOUND     = true;
static bool const VISITOR_ERROR_NOT_FOUND = false;

/// @endcond

// local functions
static bool c_ast_visitor_error( c_ast_t*, void* );
static bool c_ast_visitor_type( c_ast_t*, void* );
static bool error_kind_not_supported( c_ast_t const* );
static bool error_kind_not_type( c_ast_t const*, c_type_t );
static bool error_kind_to_type( c_ast_t const*, c_type_t );

////////// inline functions ///////////////////////////////////////////////////

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

////////// local functions ////////////////////////////////////////////////////

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
    c_type_t const storage = storage_ast->type & T_MASK_STORAGE;
    print_error( &ast->loc,
      "can not cast into %s", c_type_name_error( storage )
    );
    return false;
  }

  switch ( ast->kind ) {
    case K_ARRAY:
      print_error( &ast->loc, "can not cast into array" );
      print_hint( "cast into pointer" );
      return false;
    case K_FUNCTION:
      print_error( &ast->loc, "can not cast into function" );
      print_hint( "cast into pointer to function" );
      return false;
    default:
      return true;
  } // switch
}

/**
 * Checks an entire AST for semantic errors.
 *
 * @param ast The function (or block) `c_ast` to check.
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
 * Checks all function (or block) arguments for semantic errors.
 *
 * @param ast The function (or block) `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_func_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind & (K_BLOCK | K_FUNCTION) );
  assert( opt_lang != LANG_C_KNR );

  c_ast_t const *variadic_ast = NULL, *void_ast = NULL;
  unsigned n_args = 0;

  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( ++n_args > 1 && void_ast != NULL )
      goto only_void;

    c_ast_t const *const arg_ast = C_AST_DATA( arg );
    switch ( arg_ast->kind ) {
      case K_BUILTIN:
        if ( (arg_ast->type & T_AUTO_CPP_11) != T_NONE ) {
          print_error( &arg_ast->loc, "arguments can not be auto" );
          return false;
        }
        if ( (arg_ast->type & T_VOID) != T_NONE ) {
          //
          // Ordinarily, void arguments are invalid; but a single void function
          // "argument" is valid (as long as it doesn't have a name).
          //
          if ( arg_ast->name != NULL ) {
            print_error( &arg_ast->loc, "arguments can not be void" );
            return false;
          }
          void_ast = arg_ast;
          if ( n_args > 1 )
            goto only_void;
          continue;
        }
        break;

      case K_NAME:
        if ( opt_lang >= LANG_CPP_MIN ) {
          print_error( &arg_ast->loc, "C++ requires type specifier" );
          return false;
        }
        break;

      case K_VARIADIC:
        if ( arg->next != NULL ) {
          print_error( &arg_ast->loc, "variadic specifier must be last" );
          return false;
        }
        variadic_ast = arg_ast;
        continue;

      default:
        /* suppress warning */;
    } // switch

    c_type_t const storage = arg_ast->type & (T_MASK_STORAGE & ~T_REGISTER);
    if ( storage != T_NONE ) {
      print_error( &arg_ast->loc,
        "function arguments can not be %s", c_type_name_error( storage )
      );
      return false;
    }

    if ( !c_ast_check_errors( arg_ast, true ) )
      return false;
  } // for

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
 * Checks all function (or block) arguments for semantic errors in K&R C.
 *
 * @param ast The function (or block) `c_ast` to check.
 * @return Returns `true` only if all checks passed.
 */
static bool c_ast_check_func_args_knr( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind & (K_BLOCK | K_FUNCTION) );
  assert( opt_lang == LANG_C_KNR );

  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    c_ast_t const *const arg_ast = C_AST_DATA( arg );
    switch ( arg_ast->kind ) {
      case K_NAME:
        break;
      case K_PLACEHOLDER:
        assert( arg_ast->kind != K_PLACEHOLDER );
      default:
        print_error( &ast->loc, "function prototypes" );
        return false;
    } // switch
  } // for
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

  c_type_t tmp_type;

  switch ( ast->kind ) {
    case K_ARRAY: {
      if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE ) {
        if ( (opt_lang & (LANG_MIN(C_99) & ~LANG_CPP_ALL)) == LANG_NONE ) {
          print_error( &ast->loc,
            "variable length arrays not supported in %s",
            C_LANG_NAME()
          );
          return VISITOR_ERROR_FOUND;
        }
        if ( !is_func_arg ) {
          print_error( &ast->loc,
            "variable length arrays are illegal outside of function arguments"
          );
          return VISITOR_ERROR_FOUND;
        }
      }

      if ( ast->as.array.type ) {
        if ( (opt_lang & (LANG_MIN(C_99) & ~LANG_CPP_ALL)) == LANG_NONE ) {
          print_error( &ast->loc,
            "\"%s\" arrays not supported in %s",
            c_type_name_error( ast->as.array.type ),
            C_LANG_NAME()
          );
          return VISITOR_ERROR_FOUND;
        }
        if ( !is_func_arg ) {
          print_error( &ast->loc,
            "\"%s\" arrays are illegal outside of function arguments",
            c_type_name_error( ast->as.array.type )
          );
          return VISITOR_ERROR_FOUND;
        }
      }

      c_ast_t const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast->kind ) {
        case K_BUILTIN:
          if ( (of_ast->type & T_VOID) != T_NONE ) {
            print_error( &ast->loc, "array of void" );
            print_hint( "array of pointer to void" );
            return VISITOR_ERROR_FOUND;
          }
          if ( (of_ast->type & T_REGISTER) != T_NONE )
            return error_kind_not_type( ast, T_REGISTER );
          break;
        case K_FUNCTION:
          print_error( &ast->loc, "array of function" );
          print_hint( "array of pointer to function" );
          return VISITOR_ERROR_FOUND;
        default:
          /* suppress warning */;
      } // switch

      break;
    }

    case K_BUILTIN:
      if ( (ast->type & T_MASK_TYPE) == T_NONE ) {
        print_error( &ast->loc,
          "implicit \"int\" is illegal in %s",
          C_LANG_NAME()
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( (ast->type & T_VOID) != T_NONE && ast->parent == NULL ) {
        print_error( &ast->loc, "variable of void" );
        print_hint( "pointer to void" );
        return VISITOR_ERROR_FOUND;
      }
      if ( (ast->type & T_INLINE) != T_NONE && opt_lang < LANG_CPP_17 ) {
        print_error( &ast->loc,
          "inline variables not supported in %s",
          C_LANG_NAME()
        );
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( (ast->type & (T_STRUCT | T_UNION | T_CLASS)) != T_NONE &&
           (ast->type & T_REGISTER) != T_NONE ) {
        return error_kind_not_type( ast, T_REGISTER );
      }
      if ( c_mode == MODE_GIBBERISH_TO_ENGLISH &&
           (ast->type & T_ENUM) && (ast->type & (T_STRUCT | T_CLASS)) ) {
        print_error( &ast->loc,
          "\"%s\": enum classes must just use \"enum\"",
          c_type_name_error( ast->type )
        );
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_FUNCTION:
      if ( (ast->type & (T_REFERENCE | T_RVALUE_REFERENCE)) != T_NONE ) {
        if ( opt_lang < LANG_CPP_11 ) {
          print_error( &ast->loc,
            "reference qualified functions not supported in %s",
            C_LANG_NAME()
          );
          return VISITOR_ERROR_FOUND;
        }
        if ( (ast->type & (T_EXTERN | T_STATIC)) != T_NONE ) {
          print_error( &ast->loc,
            "reference qualified functions can not be %s",
            c_type_name_error( ast->type & (T_EXTERN | T_STATIC) )
          );
          return VISITOR_ERROR_FOUND;
        }
      }
      if ( opt_lang >= LANG_CPP_MIN ) {
        if ( (ast->type & T_PURE_VIRTUAL) != T_NONE &&
             (ast->type & T_VIRTUAL) == T_NONE ) {
          print_error( &ast->loc, "non-virtual functions can not be pure" );
          return VISITOR_ERROR_FOUND;
        }
      }
      else if ( (ast->type & T_MASK_QUALIFIER) != T_NONE ) {
        print_error( &ast->loc,
          "\"%s\" functions not supported in %s",
          c_type_name_error( ast->type & T_MASK_QUALIFIER ),
          C_LANG_NAME()
        );
        return VISITOR_ERROR_FOUND;
      }
      // FALLTHROUGH

    case K_BLOCK: {                     // Apple extension
      tmp_type = (ast->type &
                  (T_AUTO_C | T_BLOCK | T_MUTABLE | T_REGISTER |
                   T_THREAD_LOCAL));
      if ( tmp_type != T_NONE )
        return error_kind_not_type( ast, tmp_type );

      char const *const kind_name = c_kind_name( ast->kind );
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;

      switch ( ret_ast->kind ) {
        case K_ARRAY:
          print_error( &ret_ast->loc, "%s returning array", kind_name );
          print_hint( "%s returning pointer", kind_name );
          return VISITOR_ERROR_FOUND;
        case K_BUILTIN:
          if ( opt_lang < LANG_CPP_14 ) {
            if ( (ret_ast->type & (/*T_AUTO_C |*/ T_AUTO_CPP_11)) != T_NONE ) {
              print_error( &ret_ast->loc,
                "\"auto\" return type not supported in %s",
               C_LANG_NAME()
              );
              return VISITOR_ERROR_FOUND;
            }
          }
          break;
        case K_FUNCTION:
          print_error( &ret_ast->loc, "%s returning function", kind_name );
          print_hint( "%s returning pointer to function", kind_name );
          return VISITOR_ERROR_FOUND;
        default:
          /* suppress warning */;
      } // switch

      bool const args_ok = opt_lang == LANG_C_KNR ?
        c_ast_check_func_args_knr( ast ) :
        c_ast_check_func_args( ast );
      return args_ok ? VISITOR_ERROR_NOT_FOUND : VISITOR_ERROR_FOUND;
    }

    case K_NAME:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
    case K_PLACEHOLDER:
      assert( ast->kind != K_PLACEHOLDER );

    case K_POINTER_TO_MEMBER:
      if ( opt_lang < LANG_CPP_MIN )
        return error_kind_not_supported( ast );
      // FALLTHROUGH
    case K_POINTER: {
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( (to_ast->kind & (K_REFERENCE | K_RVALUE_REFERENCE)) != K_NONE ) {
        print_error( &ast->loc,
          "%s to %s", c_kind_name( ast->kind ), c_kind_name( to_ast->kind )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( (to_ast->type & T_REGISTER) != T_NONE )
        return error_kind_to_type( ast, T_REGISTER );
      break;
    }

    case K_RVALUE_REFERENCE:
      if ( opt_lang < LANG_CPP_11 )
        return error_kind_not_supported( ast );
      // FALLTHROUGH
    case K_REFERENCE: {
      if ( opt_lang < LANG_CPP_MIN )
        return error_kind_not_supported( ast );
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( (to_ast->type & T_REGISTER) != T_NONE )
        return error_kind_to_type( ast, T_REGISTER );
      if ( (to_ast->type & T_VOID) != T_NONE ) {
        error_kind_to_type( ast, T_VOID );
        print_hint( "pointer to void" );
        return VISITOR_ERROR_FOUND;
      }
      break;
    }
  } // switch

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

  c_lang_t const ok_langs = c_type_check( ast->type );
  if ( ok_langs != LANG_ALL ) {
    if ( ok_langs == LANG_NONE )
      print_error( &ast->loc,
        "\"%s\" is illegal", c_type_name_error( ast->type )
      );
    else
      print_error( &ast->loc,
        "\"%s\" is illegal in %s",
        c_type_name_error( ast->type ), C_LANG_NAME()
      );
    return VISITOR_ERROR_FOUND;
  }

  switch ( ast->kind ) {
    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      data = REINTERPRET_CAST( void*, true );
      for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
        if ( !c_ast_check_visitor( C_AST_DATA( arg ), c_ast_visitor_type,
                                   data ) ) {
          return VISITOR_ERROR_FOUND;
        }
      } // for
      if ( ast->kind == K_FUNCTION )
        break;
      // FALLTHROUGH

    default:
      if ( !is_func_arg && (ast->type & T_CARRIES_DEPENDENCY) != T_NONE ) {
        print_error( &ast->loc,
          "\"%s\" can only appear on functions or function arguments",
          c_type_name_error( T_CARRIES_DEPENDENCY )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( (ast->type & T_NORETURN) != T_NONE ) {
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
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_BLOCK:
    case K_FUNCTION: {
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      if ( (ast->type & T_NODISCARD) != T_NONE &&
           (ret_ast->type & T_VOID) != T_NONE ) {
        print_warning( &ast->loc,
          "[[%s]] functions can not return void", L_NODISCARD
        );
      }
      for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
        (void)c_ast_check_visitor(
          C_AST_DATA( arg ), c_ast_visitor_warning, data
        );
      } // for
      break;
    }

    case K_BUILTIN:
      if ( (ast->type & T_REGISTER) != T_NONE && opt_lang >= LANG_CPP_11 ) {
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
  return false;
}

/**
 * Prints an error: `<kind> can not be <type>`.
 *
 * @param ast The `c_ast` .
 * @param type The bad type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
static bool error_kind_not_type( c_ast_t const *ast, c_type_t type ) {
  assert( ast != NULL );
  print_error( &ast->loc,
    "%s can not be %s", c_kind_name( ast->kind ), c_type_name_error( type )
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
 * @param type The bad type.
 * @return Always returns `VISITOR_ERROR_FOUND`.
 */
static bool error_kind_to_type( c_ast_t const *ast, c_type_t type ) {
  print_error( &ast->loc,
    "%s to %s", c_kind_name( ast->kind ), c_type_name_error( type )
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
