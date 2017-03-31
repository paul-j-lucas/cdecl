/*
**      cdecl -- C gibberish translator
**      src/ast_check.c
**
**      Paul J. Lucas
*/

// local
#include "config.h"                     /* must go first */
#include "ast_util.h"
#include "diagnostics.h"
#include "options.h"

// standard
#include <assert.h>

// local functions
static bool c_ast_check_impl( c_ast_t const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Performs additional checks on an entire AST for semantic validity when
 * casting.
 *
 * @param ast The AST to check.
 * @return Returns \c true only if the AST does not violate any cast checks.
 */
static bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast );
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );

  c_ast_t const *const storage_ast =
    c_ast_find_type( nonconst_ast, V_DOWN, T_STORAGE );

  if ( storage_ast ) {
    c_type_t const storage = storage_ast->type & T_MASK_STORAGE;
    print_error( &ast->loc, "can not cast into %s", c_type_name( storage ) );
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
 * Checks all function (or block) arguments for semantic validity.
 *
 * @param ast The function (or block) AST to check.
 * @return Returns \c true only if all function arguments are valid.
 */
static bool c_ast_check_func_args( c_ast_t const *ast ) {
  assert( ast );
  assert( ast->kind & (K_BLOCK | K_FUNCTION) );

  c_ast_t const *arg, *variadic_arg = NULL, *void_arg = NULL;
  unsigned n_args = 0;

  for ( arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( ++n_args > 1 && void_arg )
      goto only_void;

    switch ( arg->kind ) {
      case K_BUILTIN:
        if ( (arg->type & T_VOID) ) {
          //
          // Ordinarily, void variables are invalid; but a single void function
          // "argument" is valid (as long as it doesn't have a name).
          //
          if ( arg->name ) {
            print_error( &arg->loc, "argument of void" );
            return false;
          }
          void_arg = arg;
          if ( n_args > 1 )
            goto only_void;
          continue;
        }
        break;

      case K_NAME:
        if ( opt_lang >= LANG_CPP_MIN ) {
          print_error( &ast->loc, "C++ requires type specifier" );
          return false;
        }
        break;

      case K_VARIADIC:
        if ( !void_arg ) {
          if ( variadic_arg ) {
            print_error( &arg->loc, "more than one variadic specifier" );
            return false;
          }
          variadic_arg = arg;
          continue;
        }
        break;

      default:
        /* suppress warning */;
    } // switch

    if ( variadic_arg ) {
      print_error( &variadic_arg->loc, "variadic specifier must be last" );
      return false;
    }
    if ( !c_ast_check_impl( arg ) )
      return false;
  } // for

  if ( variadic_arg && n_args == 1 ) {
    print_error( &variadic_arg->loc,
      "variadic specifier can not be only argument"
    );
    return false;
  }

  return true;

only_void:
  print_error( &void_arg->loc, "\"void\" must be only parameter if specified" );
  return false;
}

/**
 * Checks an entire AST for semantic validity.
 *
 * @param ast The AST to check.
 * @return Returns \c true only if the entire AST is valid.
 */
static bool c_ast_check_impl( c_ast_t const *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_ARRAY: {
      c_ast_t const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast->kind ) {
        case K_BUILTIN:
          if ( of_ast->type & T_VOID ) {
            print_error( &ast->loc, "array of void" );
            print_hint( "array of pointer to void" );
            return false;
          }
          if ( of_ast->type & T_REGISTER ) {
            print_error( &ast->loc, "array can not be register" );
            return false;
          }
          break;
        case K_FUNCTION:
          print_error( &ast->loc, "array of function" );
          print_hint( "array of pointer to function" );
          return false;
        default:
          return c_ast_check_impl( of_ast );
      } // switch
      break;
    }

    case K_BUILTIN:
      if ( (ast->type & T_VOID) && !ast->parent ) {
        print_error( &ast->loc, "variable of void" );
        print_hint( "pointer to void" );
        return false;
      }
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( ast->type & T_REGISTER ) {
        print_error( &ast->loc,
          "%s can not be register", c_kind_name( ast->kind )
        );
        return false;
      }
      break;

    case K_FUNCTION:
      if ( ast->type & T_REGISTER ) {
        print_error( &ast->loc, "function can not be register" );
        return false;
      }
      // no break;
    case K_BLOCK: {                     // Apple extension
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      char const *const kind_name = c_kind_name( ast->kind );
      switch ( ret_ast->kind ) {
        case K_ARRAY:
          print_error( &ret_ast->loc, "%s returning array", kind_name );
          print_hint( "%s returning pointer", kind_name );
          return false;
        case K_FUNCTION:
          print_error( &ret_ast->loc, "%s returning function", kind_name );
          print_hint( "%s returning pointer to function", kind_name );
          return false;
        default:
          return c_ast_check_func_args( ast ) && c_ast_check_impl( ret_ast );
      } // switch
      break;
    }

    case K_NAME:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
    case K_POINTER_TO_MEMBER: {
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( to_ast->kind & K_REFERENCE ) {
        print_error( &ast->loc, "%s to reference", c_kind_name( ast->kind ) );
        return false;
      }
      if ( to_ast->type & T_REGISTER ) {
        print_error( &ast->loc, "%s to register", c_kind_name( ast->kind ) );
        return false;
      }
      return c_ast_check_impl( ast->as.ptr_ref.to_ast );
    }

    case K_REFERENCE: {
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( to_ast->type & T_REGISTER ) {
        print_error( &ast->loc, "reference to register" );
        return false;
      }
      if ( to_ast->type & T_VOID ) {
        print_error( &ast->loc, "reference to void" );
        print_hint( "pointer to void" );
        return false;
      }
      return c_ast_check_impl( to_ast );
    }
  } // switch

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check_errors( c_ast_t const *ast, c_check_t check ) {
  if ( check == CHECK_CAST && !c_ast_check_cast( ast ) )
    return false;
  return c_ast_check_impl( ast );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
