/*
**      cdecl -- C gibberish translator
**      src/ast_check.c
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
#include "ast_util.h"
#include "diagnostics.h"
#include "options.h"
#include "types.h"

// standard
#include <assert.h>
#include <stdbool.h>

// local constants
static bool const VISITOR_ERROR_FOUND     = true;
static bool const VISITOR_ERROR_NOT_FOUND = false;

// local functions
static bool c_ast_visitor_error( c_ast_t*, void* );
static bool c_ast_visitor_type( c_ast_t*, void* );
static bool error_kind_not_supported( c_ast_t const* );
static bool error_kind_not_type( c_ast_t const*, c_type_t );
static bool error_kind_to_type( c_ast_t const*, c_type_t );
static bool error_type_not_supported( c_type_t, c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Simple wrapper around \c c_ast_found().
 *
 * @param ast The AST to check.
 * @param visitor The visitor to use.
 * @return Returns \c true only if all checks passed.
 */
static inline bool c_ast_check_visitor( c_ast_t const *ast,
                                        c_ast_visitor visitor ) {
  return !c_ast_found( ast, V_DOWN, visitor, NULL );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Performs additional checks on an entire AST for semantic errors when
 * casting.
 *
 * @param ast The AST to check.
 * @return Returns \c true only if all checks passed.
 */
static bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );

  c_ast_t const *const storage_ast =
    c_ast_find_type( nonconst_ast, V_DOWN, T_MASK_STORAGE );

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
 * Checks an entire AST for semantic errors.
 *
 * @param ast The function (or block) AST to check.
 * @return Returns \c true only if all checks passed.
 */
static bool c_ast_check_errors( c_ast_t const *ast ) {
  if ( !c_ast_check_visitor( ast, c_ast_visitor_error ) )
    return false;
  if ( !c_ast_check_visitor( ast, c_ast_visitor_type ) )
    return false;
  return true;
}

/**
 * Checks all function (or block) arguments for semantic errors.
 *
 * @param ast The function (or block) AST to check.
 * @return Returns \c true only if all checks passed.
 */
static bool c_ast_check_func_args( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind & (K_BLOCK | K_FUNCTION) );
  assert( opt_lang != LANG_C_KNR );

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
          print_error( &arg->loc, "C++ requires type specifier" );
          return false;
        }
        break;

      case K_VARIADIC:
        if ( arg->next ) {
          print_error( &arg->loc, "variadic specifier must be last" );
          return false;
        }
        variadic_arg = arg;
        continue;

      default:
        /* suppress warning */;
    } // switch

    c_type_t const storage = arg->type & (T_MASK_STORAGE & ~T_REGISTER);
    if ( storage ) {
      print_error( &arg->loc,
        "function argument can not be %s", c_type_name( storage )
      );
      return false;
    }

    if ( !c_ast_check_errors( arg ) )
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
 * Checks all function (or block) arguments for semantic errors in K&R C.
 *
 * @param ast The function (or block) AST to check.
 * @return Returns \c true only if all checks passed.
 */
static bool c_ast_check_func_args_knr( c_ast_t const *ast ) {
  assert( ast );
  assert( ast->kind & (K_BLOCK | K_FUNCTION) );
  assert( opt_lang == LANG_C_KNR );

  for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    switch ( arg->kind ) {
      case K_NAME:
        break;
      case K_NONE:
        assert( arg->kind != K_NONE );
      default:
        print_error( &ast->loc, "function prototypes" );
				return false;
    } // switch
  } // for
  return true;
}

/**
 * Vistor function that checks an AST for semantic errors.
 *
 * @param ast The AST to check.
 * @param data Not used.
 * @return Returns \c VISITOR_ERROR_FOUND if an error was found;
 * \c VISITOR_ERROR_NOT_FOUND if not.
 */
static bool c_ast_visitor_error( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  (void)data;

  switch ( ast->kind ) {
    case K_ARRAY: {
      c_ast_t const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast->kind ) {
        case K_BUILTIN:
          if ( (of_ast->type & T_VOID) ) {
            print_error( &ast->loc, "array of void" );
            print_hint( "array of pointer to void" );
            return VISITOR_ERROR_FOUND;
          }
          if ( (of_ast->type & T_REGISTER) )
            return error_kind_not_type( ast, T_REGISTER );
          break;
        case K_FUNCTION:
          print_error( &ast->loc, "array of function" );
          print_hint( "array of pointer to function" );
          return VISITOR_ERROR_FOUND;
        default:
          return VISITOR_ERROR_NOT_FOUND;
      } // switch
      break;
    }

    case K_BUILTIN:
      if ( (ast->type & T_VOID) && !ast->parent ) {
        print_error( &ast->loc, "variable of void" );
        print_hint( "pointer to void" );
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( (ast->type & (T_STRUCT | T_UNION | T_CLASS)) &&
           (ast->type & T_REGISTER) ) {
        return error_kind_not_type( ast, T_REGISTER );
      }
      break;

    case K_FUNCTION:
      if ( (ast->type & T_REGISTER) )
        return error_kind_not_type( ast, T_REGISTER );
      if ( opt_lang < LANG_CPP_MIN ) {
        if ( (ast->type & T_CONST) )
          return error_type_not_supported( T_CONST, ast );
        if ( (ast->type & (T_VIRTUAL | T_PURE_VIRTUAL)) )
          return error_type_not_supported( T_VIRTUAL, ast );
      } else {
        if ( (ast->type & T_PURE_VIRTUAL) && !(ast->type & T_VIRTUAL) ) {
          print_error( &ast->loc, "non-virtual function can not be pure" );
          return VISITOR_ERROR_FOUND;
        }
        // TODO
      }
      // no break;
    case K_BLOCK: {                     // Apple extension
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      char const *const kind_name = c_kind_name( ast->kind );
      switch ( ret_ast->kind ) {
        case K_ARRAY:
          print_error( &ret_ast->loc, "%s returning array", kind_name );
          print_hint( "%s returning pointer", kind_name );
          return VISITOR_ERROR_FOUND;
        case K_FUNCTION:
          print_error( &ret_ast->loc, "%s returning function", kind_name );
          print_hint( "%s returning pointer to function", kind_name );
          return VISITOR_ERROR_FOUND;
        default: {
          bool const args_ok = opt_lang == LANG_C_KNR ?
            c_ast_check_func_args_knr( ast ) :
            c_ast_check_func_args( ast );
          return args_ok ? VISITOR_ERROR_NOT_FOUND : VISITOR_ERROR_FOUND;
        }
      } // switch
      break;
    }

    case K_NAME:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER_TO_MEMBER:
      if ( opt_lang < LANG_CPP_MIN )
        return error_kind_not_supported( ast );
      // no break;
    case K_POINTER: {
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( (to_ast->kind & (K_REFERENCE | K_RVALUE_REFERENCE)) ) {
        print_error( &ast->loc,
          "%s to %s", c_kind_name( ast->kind ), c_kind_name( to_ast->kind )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( (to_ast->type & T_REGISTER) )
        return error_kind_to_type( ast, T_REGISTER );
      return VISITOR_ERROR_NOT_FOUND;
    }

    case K_RVALUE_REFERENCE:
      if ( opt_lang < LANG_CPP_11 )
        return error_kind_not_supported( ast );
      // no break;
    case K_REFERENCE: {
      if ( opt_lang < LANG_CPP_MIN )
        return error_kind_not_supported( ast );
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( (to_ast->type & T_REGISTER) )
        return error_kind_to_type( ast, T_REGISTER );
      if ( (to_ast->type & T_VOID) ) {
        error_kind_to_type( ast, T_VOID );
        print_hint( "pointer to void" );
        return VISITOR_ERROR_FOUND;
      }
      return VISITOR_ERROR_NOT_FOUND;
    }
  } // switch

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Vistor function that checks an AST for type errors.
 *
 * @param data Not used.
 * @return Returns \c VISITOR_ERROR_FOUND if an error was found;
 * \c VISITOR_ERROR_NOT_FOUND if not.
 */
static bool c_ast_visitor_type( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  (void)data;

  c_lang_t const bad_langs = c_type_check( ast->type );
  if ( bad_langs == LANG_NONE ) {
    if ( ast->kind & (K_BLOCK | K_FUNCTION) ) {
      for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
        if ( !c_ast_check_visitor( arg, c_ast_visitor_type ) )
          return VISITOR_ERROR_FOUND;
      } // for
    }
    return VISITOR_ERROR_NOT_FOUND;
  }

  if ( bad_langs == LANG_ALL )
    print_error( &ast->loc, "\"%s\" is illegal", c_type_name( ast->type ) );
  else
    print_error( &ast->loc,
      "\"%s\" is illegal in %s",
      c_type_name( ast->type ), c_lang_name( opt_lang )
    );
  return VISITOR_ERROR_FOUND;
}

/**
 * Vistor function that checks an AST for semantic warnings.
 *
 * @param ast The AST to check.
 * @param data Not used.
 * @return Always returns \c false.
 */
static bool c_ast_visitor_warning( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  (void)data;

  switch ( ast->kind ) {
    case K_ARRAY:
    case K_BUILTIN:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_BLOCK:
    case K_FUNCTION:
      for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next )
        (void)c_ast_check_visitor( arg, c_ast_visitor_warning );
      break;

    case K_NAME:
      if ( opt_lang > LANG_C_KNR )
        print_warning( &ast->loc, "missing type specifier" );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
  } // switch
  return false;
}

/**
 * Prints an error: <kind> can not be <type>.
 *
 * @param ast The AST.
 * @param type The bad type.
 * @return Always returns \c VISITOR_ERROR_FOUND.
 */
static bool error_kind_not_type( c_ast_t const *ast, c_type_t type ) {
  print_error( &ast->loc,
    "%s can not be %s", c_kind_name( ast->kind ), c_type_name( type )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: <kind> not supported in <lang>.
 *
 * @param ast The AST having the bad kind.
 * @return Always returns \c VISITOR_ERROR_FOUND.
 */
static bool error_kind_not_supported( c_ast_t const *ast ) {
  print_error( &ast->loc,
    "%s not supported in %s", c_kind_name( ast->kind ), c_lang_name( opt_lang )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: <kind> to <type>.
 *
 * @param ast The AST having the bad kind.
 * @param type The bad type.
 * @return Always returns \c VISITOR_ERROR_FOUND.
 */
static bool error_kind_to_type( c_ast_t const *ast, c_type_t type ) {
  print_error( &ast->loc,
    "%s to %s", c_kind_name( ast->kind ), c_type_name( type )
  );
  return VISITOR_ERROR_FOUND;
}

/**
 * Prints an error: <type> not supported in <lang>.
 *
 * @param type The bad type.
 * @param ast The AST having the location of the error.
 * @return Always returns \c VISITOR_ERROR_FOUND.
 */
static bool error_type_not_supported( c_type_t type, c_ast_t const *ast ) {
  print_error( &ast->loc,
    "%s not supported in %s", c_type_name( type ), c_lang_name( opt_lang )
  );
  return VISITOR_ERROR_FOUND;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check( c_ast_t const *ast, c_check_t check ) {
  if ( check == CHECK_CAST && !c_ast_check_cast( ast ) )
    return false;
  if ( !c_ast_check_errors( ast ) )
    return false;
  (void)c_ast_check_visitor( ast, c_ast_visitor_warning );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
