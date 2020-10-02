/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
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
 * Defines functions for printing in gibberish, aka, a C/C++ declaration.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "gibberish.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Type of gibberish.  The gibberish emitted varies slightly depending on the
 * type.
 */
enum g_type {
  G_DECL,                               ///< Gibberish is a declaration
  G_CAST,                               ///< Gibberish is a cast.
  G_TYPEDEF                             ///< Gibberish is a `typedef`.
};
typedef enum g_type g_type_t;

/**
 * State maintained by `c_ast_gibberish_cast()` and `c_ast_gibberish_declare()`
 * (because there'd be too many function arguments otherwise).
 */
struct g_state {
  g_type_t        gtype;                ///< Type of gibberish.
  FILE           *gout;                 ///< Where to write the gibberish.
  c_ast_t const  *leaf_ast;             ///< Leaf of AST.
  c_ast_t const  *root_ast;             ///< Root of AST.
  bool            postfix;              ///< Doing postfix gibberish?
  bool            printed_space;        ///< Printed a space yet?
};
typedef struct g_state g_state_t;

// local functions
static void         g_impl( g_state_t*, c_ast_t const* );
static void         g_init( g_state_t*, c_ast_t const*, unsigned, FILE* );
static void         g_postfix( g_state_t*, c_ast_t const* );
static void         g_qual_name( g_state_t*, c_ast_t const* );
C_WARN_UNUSED_RESULT
static char const*  g_sname_full_or_local( g_state_t*, c_ast_t const* );
static void         g_space_name( g_state_t*, c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Prints a space only if we haven't printed one yet.
 *
 * @param g The `g_state` to use.
 */
static inline void g_print_space( g_state_t *g ) {
  if ( false_set( &g->printed_space ) )
    FPUTC( ' ', g->gout );
}

/**
 * Sets the leaf AST.
 *
 * @param g The `g_state` to use.
 * @param ast The AST to set the leaf to.
 */
static inline void g_set_leaf( g_state_t *g, c_ast_t const *ast ) {
  assert( g->leaf_ast == NULL );
  assert( ast != NULL );
  g->leaf_ast = ast;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints \a ast as a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param gtype The type of gibberish.
 * @param gout The `FILE` to print to.
 */
static void c_ast_gibberish( c_ast_t const *ast, g_type_t gtype, FILE *gout ) {
  assert( ast != NULL );
  assert( gout != NULL );

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_EXPR:
      FPRINTF( gout, "%s(%u) ", alignas_lang(), ast->align.as.expr );
      break;
    case C_ALIGNAS_TYPE:
      FPRINTF( gout, "%s(", alignas_lang() );
      c_ast_gibberish_declare( ast->align.as.type_ast, gout );
      FPUTS( ") ", gout );
      break;
  } // switch

  g_state_t g;
  g_init( &g, ast, gtype, gout );
  g_impl( &g, ast );
}

/**
 * Helper function for `g_impl()` that prints an array's size as well as the
 * size for all child arrays, if any.
 *
 * @param g The `g_state` to use.
 * @param ast The AST that is a <code>\ref K_ARRAY</code> whose size to
 * print.
 */
static void g_array_size( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( ast->kind_id == K_ARRAY );

  FPUTS( graph_token_c( "[" ), g->gout );
  if ( ast->as.array.store_tid != TS_NONE )
    FPRINTF( g->gout, "%s ", c_type_id_name( ast->as.array.store_tid ) );
  switch ( ast->as.array.size ) {
    case C_ARRAY_SIZE_NONE:
      break;
    case C_ARRAY_SIZE_VARIABLE:
      FPUTC( '*', g->gout );
      break;
    default:
      FPRINTF( g->gout, "%d", ast->as.array.size );
  } // switch
  FPUTS( graph_token_c( "]" ), g->gout );
}

/**
 * Helper function for `g_impl()` that prints a function-like AST's parameters,
 * if any.
 *
 * @param g The `g_state` to use.
 * @param ast The AST that is <code>\ref K_ANY_FUNCTION_LIKE</code> whose
 * parameters to print.
 */
static void g_func_params( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );

  bool comma = false;
  FPUTC( '(', g->gout );
  FOREACH_PARAM( param, ast ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", g->gout );
    c_ast_t const *const param_ast = c_param_ast( param );
    g_state_t params_g;
    g_init( &params_g, param_ast, g->gtype, g->gout );
    g_impl( &params_g, param_ast );
  } // for
  FPUTC( ')', g->gout );
}

/**
 * Prints \a ast as gibberish, aka, a C/C++ declaration.
 *
 * @param g The `g_state` to use.
 * @param ast The AST to print.
 */
static void g_impl( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  c_type_t type = ast->type;

  c_type_id_t cv_qual_tid     = TS_NONE;
  bool        is_default      = false;
  bool        is_delete       = false;
  bool        is_final        = false;
  bool        is_noexcept     = false;
  bool        is_override     = false;
  bool        is_pure_virtual = false;
  bool        is_throw        = false;
  c_type_id_t ref_qual_tid    = TS_NONE;

  //
  // This isn't implemented using a visitor because c_ast_visit() visits in
  // post-order and, in order to print gibberish, the AST has to be visited in
  // pre-order.  Since this is the only case where a pre-order traversal has to
  // be done, it's not worth having a pre-order version of c_ast_visit().
  //
  switch ( ast->kind_id ) {
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_USER_DEF_CONVERSION:
      //
      // Since none of these have a return type, no space needs to be printed
      // before the name, so lie and set the "space" flag.
      //
      g->printed_space = true;
      C_FALLTHROUGH;

    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      //
      // These things aren't printed as part of the type beforehand, so strip
      // them out of the type here, but print them after the parameters.
      //
      cv_qual_tid     = (type.store_tid & TS_MASK_QUALIFIER);
      is_final        = (type.store_tid & TS_FINAL) != TS_NONE;
      is_noexcept     = (type.store_tid & TS_NOEXCEPT) != TS_NONE;
      is_override     = (type.store_tid & TS_OVERRIDE) != TS_NONE;
      is_pure_virtual = (type.store_tid & TS_PURE_VIRTUAL) != TS_NONE;
      is_default      = (type.store_tid & TS_DEFAULT) != TS_NONE;
      is_delete       = (type.store_tid & TS_DELETE) != TS_NONE;
      is_throw        = (type.store_tid & TS_THROW) != TS_NONE;
      ref_qual_tid    = (type.store_tid & TS_MASK_REF_QUALIFIER);

      type.store_tid &= c_type_id_compl(
                            TS_MASK_QUALIFIER
                          | TS_DEFAULT
                          | TS_DELETE
                          | TS_FINAL
                          | TS_NOEXCEPT
                          | TS_OVERRIDE
                          | TS_PURE_VIRTUAL
                          | TS_THROW
                          | TS_MASK_REF_QUALIFIER
                        );

      //
      // Depending on the C++ language version, change noexcept to throw() or
      // vice versa.
      //
      if ( opt_lang < LANG_CPP_11 ) {
        if ( is_noexcept ) {
          is_noexcept = false;
          is_throw = true;
        }
      } else {
        if ( is_throw ) {
          is_noexcept = true;
          is_throw = false;
        }
      }
      C_FALLTHROUGH;

    case K_ARRAY:
    case K_APPLE_BLOCK:
      if ( !c_type_is_none( &type ) )
        FPRINTF( g->gout, "%s ", c_type_name( &type ) );
      if ( ast->kind_id == K_USER_DEF_CONVERSION ) {
        if ( !c_ast_empty_name( ast ) )
          FPRINTF( g->gout, "%s::", c_ast_full_name( ast ) );
        FPRINTF( g->gout, "%s ", L_OPERATOR );
      }
      if ( ast->as.parent.of_ast != NULL )
        g_impl( g, ast->as.parent.of_ast );
      if ( false_set( &g->postfix ) ) {
        if ( g->gtype != G_CAST )
          g_print_space( g );
        if ( ast == g->root_ast && g->leaf_ast != NULL )
          g_postfix( g, g->leaf_ast->parent_ast );
        else
          g_postfix( g, ast );
      }
      if ( cv_qual_tid != TS_NONE )
        FPRINTF( g->gout, " %s", c_type_id_name( cv_qual_tid ) );
      if ( ref_qual_tid != TS_NONE ) {
        FPUTS(
          (ref_qual_tid & TS_REFERENCE) != TS_NONE ?  " &" : " &&",
          g->gout
        );
      }
      if ( is_noexcept )
        FPRINTF( g->gout, " %s", L_NOEXCEPT );
      if ( is_throw )
        FPRINTF( g->gout, " %s()", L_THROW );
      if ( is_override )
        FPRINTF( g->gout, " %s", L_OVERRIDE );
      if ( is_final )
        FPRINTF( g->gout, " %s", L_FINAL );
      if ( is_default )
        FPRINTF( g->gout, " = %s", L_DEFAULT );
      if ( is_delete )
        FPRINTF( g->gout, " = %s", L_DELETE );
      if ( is_pure_virtual )
        FPUTS( " = 0", g->gout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( &ast->type ), g->gout );
      g_space_name( g, ast );
      g_set_leaf( g, ast );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( c_type_is_tid_any( &type, TB_ENUM ) ) {
        //
        // Special case: an enum class must be written as just "enum" when
        // doing an elaborated-type-specifier:
        //
        //      c++decl> declare e as enum class C
        //      enum C e;                   // not: enum class C e;
        //
        type.base_tid &= c_type_id_compl( TB_STRUCT | TB_CLASS );
      }

      if ( opt_east_const ) {
        cv_qual_tid = type.store_tid & (TS_CONST | TS_VOLATILE);
        type.store_tid &= c_type_id_compl( TS_CONST | TS_VOLATILE );
      }

      FPRINTF( g->gout,
        "%s %s", c_type_name( &type ),
        c_sname_full_name( &ast->as.ecsu.ecsu_sname )
      );

      if ( cv_qual_tid != TS_NONE )
        FPRINTF( g->gout, " %s", c_type_id_name( cv_qual_tid ) );

      g_space_name( g, ast );
      g_set_leaf( g, ast );
      break;

    case K_NAME:
      if ( !c_ast_empty_name( ast ) && g->gtype != G_CAST )
        FPUTS( g_sname_full_or_local( g, ast ), g->gout );
      g_set_leaf( g, ast );
      break;

    case K_NONE:
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:
      assert( ast->kind_id != K_PLACEHOLDER );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE: {
      c_type_id_t store_tid = type.store_tid & TS_MASK_STORAGE;
      if ( store_tid != TS_NONE )
        FPRINTF( g->gout, "%s ", c_type_id_name( store_tid ) );
      g_impl( g, ast->as.ptr_ref.to_ast );
      if ( g->gtype != G_CAST && c_ast_find_name( ast, C_VISIT_UP ) != NULL &&
           !c_ast_find_kind_any( ast->parent_ast, C_VISIT_UP,
                                 K_ANY_FUNCTION_LIKE ) ) {
        //
        // For all kinds except function-like ASTs, we want the output to be
        // like:
        //
        //      type *var
        //
        // i.e., the '*', '&', or "&&" adjacent to the variable; for function-
        // like ASTs, when there's no name for a parameter, or when we're
        // casting, we want the output to be like:
        //
        //      type* func()            // function
        //      type* (^block)()        // block
        //      func(type*)             // nameless function parameter
        //      (type*)expr             // cast
        //
        // i.e., the '*', '&', or "&&" adjacent to the type.
        //
        g_print_space( g );
      }
      if ( !g->postfix )
        g_qual_name( g, ast );
      break;
    }

    case K_POINTER_TO_MEMBER:
      g_impl( g, ast->as.ptr_mbr.of_ast );
      if ( !g->postfix ) {
        FPUTC( ' ', g->gout );
        g_qual_name( g, ast );
      }
      break;

    case K_TYPEDEF: {
      bool const is_typedef =
        c_type_equal( &ast->type, &C_TYPE_LIT_B( TB_TYPEDEF ) );
      if ( !opt_east_const && !is_typedef )
        FPRINTF( g->gout, "%s ", c_type_name( &ast->type ) );
      FPRINTF( g->gout,
        "%s", g_sname_full_or_local( g, ast->as.c_typedef.of_ast )
      );
      if ( opt_east_const && !is_typedef )
        FPRINTF( g->gout, " %s", c_type_name( &ast->type ) );
      g_space_name( g, ast );
      g_set_leaf( g, ast );
      break;
    }

    case K_VARIADIC:
      FPUTS( L_ELLIPSIS, g->gout );
      g_set_leaf( g, ast );
      break;
  } // switch
}

/**
 * Initializes a `g_state`.
 *
 * @param g The `g_state` to initialize.
 * @param gtype The type of gibberish.
 * @param root_ast The AST root.
 * @param gout The `FILE` to print it to.
 */
static void g_init( g_state_t *g, c_ast_t const *root_ast, g_type_t gtype,
                    FILE *gout ) {
  assert( g != NULL );
  assert( root_ast != NULL );
  assert( gout != NULL );

  MEM_ZERO( g );
  g->gtype = gtype;
  g->gout = gout;
  g->root_ast = root_ast;
}

/**
 * Helper function for `g_impl()` that handles the printing of "postfix" cases:
 *
 *  + Array of pointer to function.
 *  + Pointer to array.
 *  + Reference to array.
 *
 * @param g The `g_state` to use.
 * @param ast The AST.
 */
static void g_postfix( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( c_ast_is_parent( ast ) );

  c_ast_t const *const parent_ast = ast->parent_ast;

  if ( parent_ast != NULL ) {
    switch ( parent_ast->kind_id ) {
      case K_ARRAY:
      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_DESTRUCTOR:
      case K_FUNCTION:
      case K_OPERATOR:
      case K_USER_DEF_CONVERSION:
      case K_USER_DEF_LITERAL:
        g_postfix( g, parent_ast );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
        switch ( ast->kind_id ) {
          case K_APPLE_BLOCK:
            FPUTS( "(^", g->gout );
            break;
          default:
            //
            // Pointers are written in gibberish like:
            //
            //      type (*a)[size]     // pointer to array
            //      type (*f)()         // pointer to function
            //      type (*a[size])()   // array of pointer to function
            //
            // so we need to add parentheses.
            //
            FPUTC( '(', g->gout );
            break;
          case K_POINTER:
            //
            // However, if there are consecutive pointers, omit the extra '(':
            //
            //      type (**a)[size]    // pointer to pointer to array[size]
            //
            // rather than:
            //
            //      type (*(*a))[size]  // extra () unnecessary
            //
            break;
        } // switch

        g_qual_name( g, parent_ast );
        if ( c_ast_is_parent( parent_ast->parent_ast ) )
          g_postfix( g, parent_ast );

        if ( (ast->kind_id & K_ANY_POINTER) == K_NONE )
          FPUTC( ')', g->gout );
        break;

      default:
        /* suppress warning */;
    } // switch
  } else {
    //
    // We've reached the root of the AST that has the name of the thing we're
    // printing the gibberish for.
    //
    if ( ast->kind_id == K_APPLE_BLOCK )
      FPUTS( "(^", g->gout );
    g_space_name( g, ast );
    if ( ast->kind_id == K_APPLE_BLOCK )
      FPUTC( ')', g->gout );
  }

  //
  // We're now unwinding the recursion: print the "postfix" things (size for
  // arrays, parameters for functions) in root-to-leaf order.
  //
  switch ( ast->kind_id ) {
    case K_ARRAY:
      g_array_size( g, ast );
      break;
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      g_func_params( g, ast );
      break;
    case K_USER_DEF_CONVERSION:
      FPUTS( "()", g->gout );
      break;
    default:
      /* suppress warning */;
  } // switch
}

/**
 * Helper function for `g_impl()` that prints a pointer, pointer-to-member,
 * reference, or rvalue reference, its qualifier, if any, and the name, if any.
 *
 * @param g The `g_state` to use.
 * @param ast The AST that is one of <code>\ref K_POINTER</code>,
 * <code>\ref K_POINTER_TO_MEMBER</code>, <code>\ref K_REFERENCE</code>, or
 * <code>\ref K_RVALUE_REFERENCE</code> whose qualifier, if any, and name, if
 * any, to print.
 */
static void g_qual_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  switch ( ast->kind_id ) {
    case K_POINTER:
      FPUTC( '*', g->gout );
      break;
    case K_POINTER_TO_MEMBER:
      FPRINTF( g->gout,
        "%s::*", c_sname_full_name( &ast->as.ptr_mbr.class_sname )
      );
      break;
    case K_REFERENCE:
      if ( opt_alt_tokens ) {
        g_print_space( g );
        FPRINTF( g->gout, "%s ", L_BITAND );
      } else {
        FPUTC( '&', g->gout );
      }
      break;
    case K_RVALUE_REFERENCE:
      if ( opt_alt_tokens ) {
        g_print_space( g );
        FPRINTF( g->gout, "%s ", L_AND );
      } else {
        FPUTS( "&&", g->gout );
      }
      break;
    default:
      assert( (ast->kind_id & (K_ANY_POINTER | K_ANY_REFERENCE)) != K_NONE );
  } // switch

  c_type_id_t const qual_tid = ast->type.store_tid & TS_MASK_QUALIFIER;
  if ( qual_tid != TS_NONE ) {
    FPUTS( c_type_id_name( qual_tid ), g->gout );
    if ( g->gtype != G_CAST )
      FPUTC( ' ', g->gout );
  }
  if ( !c_ast_empty_name( ast ) && g->gtype != G_CAST )
    FPUTS( g_sname_full_or_local( g, ast ), g->gout );
}

/**
 * Gets either the full or local name of \a ast based on whether we're emitting
 * the gibberish for a `typedef` since it can't have a scoped name.
 *
 * @param g The `g_state` to use.
 * @param ast The AST to get the name of.
 * @return Returns said name.
 */
C_WARN_UNUSED_RESULT
static char const* g_sname_full_or_local( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  if ( g->gtype == G_TYPEDEF ) {
    g->gtype = G_DECL;
    return c_ast_local_name( ast );
  }
  return c_ast_full_name( ast );
}

/**
 * Helper function for `g_impl()` that prints a space (if it hasn't printed one
 * before) and an AST node's name, if any; but only if we're printing a
 * declaration (not a cast).
 *
 * @param g The `g_state` to use.
 * @param ast The AST to print the name (if any) of.
 */
static void g_space_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  if ( g->gtype == G_CAST )
    return;                             // for casts, print nothing

  switch ( ast->kind_id ) {
    case K_CONSTRUCTOR:
      FPUTS( c_ast_full_name( ast ), g->gout );

      if ( !c_type_is_tid_any( &ast->type, TS_EXPLICIT ) &&
           c_ast_count_name( ast ) == 1 ) {
        //
        // For non-explicit constructors, we have to repeat the local name
        // since that's the name of the constructor at file-scope.
        //
        FPRINTF( g->gout, "::%s", c_ast_local_name( ast ) );
      }
      break;
    case K_DESTRUCTOR:
      if ( c_ast_count_name( ast ) > 1 )
        FPRINTF( g->gout, "%s::", c_ast_scope_name( ast ) );
      if ( opt_alt_tokens )
        FPRINTF( g->gout, "%s ", L_COMPL );
      else
        FPUTC( '~', g->gout );
      FPUTS( c_ast_local_name( ast ), g->gout );
      break;
    case K_OPERATOR: {
      g_print_space( g );
      if ( !c_ast_empty_name( ast ) )
        FPRINTF( g->gout, "%s::", c_ast_full_name( ast ) );
      char const *const token = c_oper_token_c( ast->as.oper.oper_id );
      FPRINTF( g->gout,
        "%s%s%s", L_OPERATOR, isalpha( token[0] ) ? " " : "", token
      );
      break;
    }
    case K_USER_DEF_CONVERSION:
      // Do nothing since these don't have names.
      break;
    case K_USER_DEF_LITERAL:
      g_print_space( g );
      if ( c_ast_count_name( ast ) > 1 )
        FPRINTF( g->gout, "%s::", c_ast_scope_name( ast ) );
      FPRINTF( g->gout, "%s\"\" %s", L_OPERATOR, c_ast_local_name( ast ) );
      break;
    default:
      if ( !c_ast_empty_name( ast ) ) {
        g_print_space( g );
        FPUTS( g_sname_full_or_local( g, ast ), g->gout );
      }
      break;
  } // switch
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_gibberish_cast( c_ast_t const *ast, FILE *gout ) {
  c_ast_gibberish( ast, G_CAST, gout );
}

void c_ast_gibberish_declare( c_ast_t const *ast, FILE *gout ) {
  c_ast_gibberish( ast, G_DECL, gout );
}

void c_typedef_gibberish( c_ast_t const *typedef_ast, FILE *gout ) {
  assert( typedef_ast != NULL );
  assert( gout != NULL );

  size_t scope_close_braces_to_print = 0;
  c_type_t scope_type = T_NONE;

  c_sname_t const *const sname = c_ast_find_name( typedef_ast, C_VISIT_DOWN );
  if ( sname != NULL && c_sname_count( sname ) > 1 ) {
    scope_type = c_scope_data( sname->head )->type;
    //
    // A type name can't be scoped in a typedef declaration, e.g.:
    //
    //      typedef int S::T::I;        // illegal
    //
    // so we have to wrap it in a scoped declaration, one of: class, namespace,
    // struct, or union.
    //
    if ( scope_type.base_tid != TB_NAMESPACE || opt_lang >= LANG_CPP_17 ) {
      //
      // All C++ versions support nested class/struct/union declarations, e.g.:
      //
      //      struct S::T { typedef int I; }
      //
      // However, only C++17 and later support nested namespace declarations:
      //
      //      namespace S::T { typedef int I; }
      //
      scope_type = *c_sname_scope_type( sname );
      if ( scope_type.base_tid == TB_SCOPE )
        scope_type.base_tid = TB_NAMESPACE;
      FPRINTF( gout,
        "%s %s { ", c_type_name( &scope_type ), c_sname_scope_name( sname )
      );
      scope_close_braces_to_print = 1;
    }
    else {
      //
      // Namespaces in C++14 and earlier require distinct declarations:
      //
      //      namespace S { namespace T { typedef int I; } }
      //
      for ( c_scope_t const *scope = sname->head; scope != sname->tail;
            scope = scope->next ) {
        scope_type = c_scope_data( scope )->type;
        if ( scope_type.base_tid == TB_SCOPE )
          scope_type.base_tid = TB_NAMESPACE;
        FPRINTF( gout,
          "%s %s { ",
          c_type_name( &scope_type ), c_scope_data( scope )->name
        );
      } // for
      scope_close_braces_to_print = c_sname_count( sname ) - 1;
    }
  }

  FPRINTF( gout, "%s ", L_TYPEDEF );
  c_ast_gibberish( typedef_ast, G_TYPEDEF, gout );

  if ( scope_close_braces_to_print > 0 ) {
    FPUTC( ';', gout );
    while ( scope_close_braces_to_print-- > 0 )
      FPUTS( " }", gout );
  }

  if ( opt_semicolon && scope_type.base_tid != TB_NAMESPACE )
    FPUTC( ';', gout );
  FPUTC( '\n', gout );
}

char const* graph_token_c( char const *token ) {
  assert( token != NULL );

  if ( !opt_alt_tokens ) {
    switch ( opt_graph ) {
      case C_GRAPH_NONE:
        break;
      //
      // Even though this could be done character-by-character, it's easier for
      // the calling code if multi-character tokens containing graph characters
      // are returned as a single string.
      //
      case C_GRAPH_DI:
        switch ( token[0] ) {
          case '[': return token[1] == '[' ? "<:<:" : "<:";
          case ']': return token[1] == ']' ? ":>:>" : ":>";
        } // switch
        break;
      case C_GRAPH_TRI:
        switch ( token[0] ) {
          case '[': return token[1] == '[' ? "?\?(?\?(" : "?\?(";
          case ']': return token[1] == ']' ? "?\?)?\?)" : "?\?)";
          case '^': return token[1] == '=' ? "?\?'=" : "?\?'";
          case '|': switch ( token[1] ) {
                      case '=': return "?\?!=";
                      case '|': return "?\?!?\?!";
                    } // switch
                    return "?\?!";
          case '~': return "?\?-";
        } // switch
        break;
    } // switch
  }

  return token;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
