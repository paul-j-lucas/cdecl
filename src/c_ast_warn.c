/*
**      cdecl -- C gibberish translator
**      src/c_ast_warn.c
**
**      Copyright (C) 2024  Paul J. Lucas
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
 * Defines functions for checking an AST for semantic warnings.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast_warn.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_lang.h"
#include "c_sname.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local functions
static void c_ast_warn_name( c_ast_t const* );
static void c_ast_warn_ret_type( c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks an entire AST for semantic warnings using \a check_fn.
 *
 * @param ast The AST to check.
 * @param check_fn The check function to use.
 */
static inline void c_ast_warn_visitor( c_ast_t const *ast,
                                       c_ast_visit_fn_t check_fn ) {
  c_ast_visit( ast, C_VISIT_DOWN, check_fn, USER_DATA_ZERO );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Visitor function that checks an AST for semantic warnings.
 *
 * @param ast The AST to check.
 * @param user_data Not used.
 * @return Always returns `false`.
 */
NODISCARD
static bool c_ast_visitor_warning( c_ast_t const *ast, user_data_t user_data ) {
  assert( ast != NULL );
  (void)user_data;

  c_tid_t qual_stids;
  c_ast_t const *const raw_ast = c_ast_untypedef_qual( ast, &qual_stids );

  switch ( raw_ast->kind ) {
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      if ( c_tid_is_any( qual_stids, TS_CV ) ) {
        //
        // Either const or volatile applied to a reference directly is an error
        // and checked for in c_ast_check_reference(); so if we get here, the
        // const or volatile must be applied to a typedef of a reference type,
        // e.g.:
        //
        //      using rint = int&
        //      const rint x            // warning: no effect
        //
        assert( ast->kind == K_TYPEDEF );

        print_warning( &ast->loc,
          "\"%s\" on reference type ",
          c_tid_error( qual_stids )
        );
        print_ast_type_aka( ast, stderr );
        EPUTS( " has no effect\n" );
        break;
      }
      FALLTHROUGH;

    case K_ARRAY:
    case K_BUILTIN:
    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
      if ( c_ast_is_register( ast ) && OPT_LANG_IS( MIN(CPP_11) ) ) {
        print_warning( &ast->loc,
          "\"%s\" is deprecated%s\n",
          c_tid_error( TS_register ),
          C_LANG_WHICH( MAX(CPP_03) )
        );
      }
      break;

    case K_UDEF_LIT:
      if ( c_sname_local_name( &ast->sname )[0] != '_' )
        print_warning( &ast->loc,
          "%ss not starting with '_' are reserved\n",
          c_kind_name( K_UDEF_LIT )
        );
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
    case K_LAMBDA:
    case K_OPERATOR:
      c_ast_warn_ret_type( raw_ast );
      FALLTHROUGH;

    case K_CONSTRUCTOR:
      FOREACH_AST_FUNC_PARAM( param, ast ) {
        c_ast_t const *const param_ast = c_param_ast( param );
        c_ast_warn_visitor( param_ast, &c_ast_visitor_warning );
        if ( c_tid_is_any( param_ast->type.stids, TS_volatile ) &&
             !OPT_LANG_IS( volatile_PARAMS_NOT_DEPRECATED ) ) {
          print_warning( &param_ast->loc,
            "\"%s\" parameter types are deprecated%s\n",
            c_tid_error( TS_volatile ),
            C_LANG_WHICH( volatile_PARAMS_NOT_DEPRECATED )
          );
        }
      } // for
      FALLTHROUGH;

    case K_DESTRUCTOR:
      if ( c_tid_is_any( ast->type.stids, TS_throw ) &&
           OPT_LANG_IS( noexcept ) ) {
        print_warning( &ast->loc,
          "\"throw\" is deprecated%s",
          C_LANG_WHICH( CPP_MAX(03) )
        );
        print_hint( "\"%s\"", c_tid_error( TS_noexcept ) );
      }
      break;

    case K_NAME:
      if ( OPT_LANG_IS( PROTOTYPES ) && ast->param_of_ast != NULL &&
           c_ast_is_untyped( ast ) ) {
        //
        // A name can occur as an untyped K&R C function parameter.  In
        // C89-C17, it's implicitly int:
        //
        //      cdecl> declare f as function (x) returning char
        //      char f(int x)
        //
        print_warning( &ast->loc,
          "missing type specifier; \"%s\" assumed\n",
          c_tid_error( TB_int )
        );
      }
      break;

    case K_STRUCTURED_BINDING:
      if ( c_tid_is_any( ast->type.stids, TS_volatile ) &&
           !OPT_LANG_IS( volatile_STRUCTURED_BINDINGS_NOT_DEPRECATED ) ) {
        print_warning( &ast->loc,
          "\"%s\" structured bindings are deprecated%s\n",
          c_tid_error( TS_volatile ),
          C_LANG_WHICH( volatile_STRUCTURED_BINDINGS_NOT_DEPRECATED )
        );
      }
      break;

    case K_CAPTURE:
    case K_CAST:
    case K_CONCEPT:
    case K_UDEF_CONV:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_PLACEHOLDER:
    case K_TYPEDEF:                     // impossible after c_ast_untypedef()
      unreachable();
  } // switch

  c_ast_warn_name( ast );

  return /*stop=*/false;
}

/**
 * Checks an AST's name(s) for warnings.
 *
 * @param ast The AST to check.
 *
 * @sa c_ast_check_name()
 */
static void c_ast_warn_name( c_ast_t const *ast ) {
  assert( ast != NULL );

  c_sname_warn( &ast->sname, &ast->loc );

  if ( ast->align.kind == C_ALIGNAS_SNAME )
    c_sname_warn( &ast->align.sname, &ast->align.loc );

  if ( (ast->kind & K_ANY_NAME) != 0 &&
       c_sname_cmp( &ast->sname, &ast->name.sname ) != 0 ) {
    c_sname_warn( &ast->name.sname, &ast->loc );
  }
}

/**
 * Checks the return type of a function-like AST for warnings.
 *
 * @param ast The function-like AST to check.
 */
static void c_ast_warn_ret_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_RETURN ) );

  c_ast_t const *const ret_ast = ast->func.ret_ast;
  if ( ret_ast == NULL )
    return;

  c_tid_t ret_qual_stids;
  PJL_DISCARD_RV( c_ast_untypedef_qual( ret_ast, &ret_qual_stids ) );

  if ( c_tid_is_any( ret_qual_stids, TS_volatile ) &&
       !OPT_LANG_IS( volatile_RETURN_TYPES_NOT_DEPRECATED ) ) {
    print_warning( &ret_ast->loc,
      "\"%s\" return types are deprecated%s\n",
      c_tid_error( TS_volatile ),
      C_LANG_WHICH( volatile_RETURN_TYPES_NOT_DEPRECATED )
    );
  }

  if ( c_tid_is_any( ast->type.atids, TA_nodiscard ) &&
       c_ast_is_builtin_any( ret_ast, TB_void ) ) {
    print_warning( &ret_ast->loc,
      "\"%s\" %ss must return a value\n",
      c_tid_error( TA_nodiscard ),
      c_kind_name( ast->kind )
    );
  }
}

/**
 * Performs additional checks on an AST for a type.
 *
 * @param ast The AST of a type to check.
 * @param user_data Not used.
 * @return Always returns `false`.
 *
 * @sa c_ast_visitor_warning()
 * @sa c_type_ast_check()
 * @sa c_type_ast_visitor_error()
 */
NODISCARD
static bool c_type_ast_visitor_warning( c_ast_t const *ast,
                                        user_data_t user_data ) {
  assert( ast != NULL );
  (void)user_data;

  if ( (ast->kind & K_ANY_NAME) != 0 )
    c_sname_warn( &ast->name.sname, &ast->loc );

  return /*stop=*/false;
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_warn( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_ast_warn_visitor( ast, &c_ast_visitor_warning );
}

void c_type_ast_warn( c_ast_t const *type_ast ) {
  assert( type_ast != NULL );
  c_ast_warn_visitor( type_ast, &c_type_ast_visitor_warning );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
