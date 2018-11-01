/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
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
 * Defines functions for printing an AST in gibberish, aka, a C/C++
 * declaration.
 */

// local
#include "config.h"                     /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * The kind of gibberish to create.
 */
enum g_kind {
  G_CAST,                               ///< Omit names and unneeded whitespace.
  G_DECLARE                             ///< Regular C/C++ declarations.
};
typedef enum g_kind g_kind_t;

/**
 * Parameters used by `c_ast_gibberish_cast()` and `c_ast_gibberish_declare()`
 * (because there'd be too many function arguments otherwise).
 */
struct g_param {
  g_kind_t        gkind;                ///< The kind of gibberish to create.
  FILE           *gout;                 ///< Where to write the gibberish.
  c_ast_t const  *leaf_ast;             ///< Leaf of AST.
  c_ast_t const  *root_ast;             ///< Root of AST.
  bool            postfix;              ///< Doing postfix gibberish?
  bool            space;                ///< Printed a space yet?
};
typedef struct g_param g_param_t;

// local functions
static void       c_ast_gibberish_impl( c_ast_t const*, g_param_t* );
static void       c_ast_gibberish_postfix( c_ast_t const*, g_param_t* );
static void       c_ast_gibberish_qual_name( c_ast_t const*, g_param_t const* );
static void       c_ast_gibberish_space_name( c_ast_t const*, g_param_t* );
static void       g_param_init( g_param_t*, c_ast_t const*, g_kind_t, FILE* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Sets the leaf `c_ast`.
 *
 * @param param The `g_param` to use.
 * @param ast The `c_ast` to set the leaf to.
 */
static inline void g_param_leaf( g_param_t *param, c_ast_t const *ast ) {
  assert( param->leaf_ast == NULL );
  assert( ast != NULL );
  param->leaf_ast = ast;
}

/**
 * Prints a space only if we haven't printed one yet.
 *
 * @param param The `g_param` to use.
 */
static inline void g_param_space( g_param_t *param ) {
  if ( false_set( &param->space ) )
    FPUTC( ' ', param->gout );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for `c_ast_gibberish_impl()` that prints an array's size as
 * well as the size for all child arrays, if any.
 *
 * @param ast The `c_ast` that is a <code>\ref K_ARRAY</code> whose size to
 * print.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_array_size( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );

  FPUTC( '[', param->gout );
  if ( ast->as.array.type_id != T_NONE )
    FPRINTF( param->gout, "%s ", c_type_name( ast->as.array.type_id ) );
  switch ( ast->as.array.size ) {
    case C_ARRAY_SIZE_NONE:
      break;
    case C_ARRAY_SIZE_VARIABLE:
      FPUTC( '*', param->gout );
      break;
    default:
      FPRINTF( param->gout, "%d", ast->as.array.size );
  } // switch
  FPUTC( ']', param->gout );
}

/**
 * Helper function for `c_ast_gibberish_impl()` that prints a block's or
 * function's arguments, if any.
 *
 * @param ast The `c_ast` that is either a <code>\ref K_BLOCK</code> or a
 * <code>\ref K_FUNCTION</code> whose arguments to print.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_func_args( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( (ast->kind & (K_BLOCK | K_FUNCTION)) != K_NONE );

  bool comma = false;
  FPUTC( '(', param->gout );
  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", param->gout );
    c_ast_t const *const arg_ast = C_AST_DATA( arg );
    g_param_t args_param;
    g_param_init( &args_param, arg_ast, param->gkind, param->gout );
    c_ast_gibberish_impl( arg_ast, &args_param );
  } // for
  FPUTC( ')', param->gout );
}

/**
 * Prints \a ast as gibberish, aka, a C/C++ declaration.
 *
 * @param ast The `c_ast` to print.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_impl( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( param != NULL );

  c_type_id_t ast_type        = ast->type_id;
  c_type_id_t cv_qualifier    = T_NONE;
  bool        is_final        = false;
  bool        is_noexcept     = false;
  bool        is_override     = false;
  bool        is_pure_virtual = false;
  bool        is_throw        = false;
  c_type_id_t ref_qualifier   = T_NONE;

  //
  // This isn't implemented using a visitor because c_ast_visit() visits in
  // post-order and, in order to print gibberish, the AST has to be visited in
  // pre-order.  Since this is the only case where a pre-order traversal has to
  // be done, it's not worth having a pre-order version of c_ast_visit().
  //
  switch ( ast->kind ) {
    case K_FUNCTION:
      //
      // These things aren't printed as part of the type beforehand, so strip
      // them out of the type here, but print them after the arguments.
      //
      cv_qualifier    = (ast_type & T_MASK_QUALIFIER);
      is_final        = (ast_type & T_FINAL) != T_NONE;
      is_noexcept     = (ast_type & T_NOEXCEPT) != T_NONE;
      is_override     = (ast_type & T_OVERRIDE) != T_NONE;
      is_pure_virtual = (ast_type & T_PURE_VIRTUAL) != T_NONE;
      is_throw        = (ast_type & T_THROW) != T_NONE;
      ref_qualifier   = (ast_type & T_MASK_REF_QUALIFIER);

      ast_type &= ~(T_MASK_QUALIFIER
                  | T_FINAL
                  | T_NOEXCEPT
                  | T_OVERRIDE
                  | T_PURE_VIRTUAL
                  | T_THROW
                  | T_MASK_REF_QUALIFIER);

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
      // FALLTHROUGH

    case K_ARRAY:
    case K_BLOCK:                       // Apple extension
      if ( ast_type != T_NONE )         // storage class
        FPRINTF( param->gout, "%s ", c_type_name( ast_type ) );
      c_ast_gibberish_impl( ast->as.parent.of_ast, param );
      if ( false_set( &param->postfix ) ) {
        if ( param->gkind != G_CAST )
          g_param_space( param );
        if ( ast == param->root_ast )
          c_ast_gibberish_postfix( param->leaf_ast->parent, param );
        else
          c_ast_gibberish_postfix( ast, param );
      }
      if ( cv_qualifier != T_NONE )
        FPRINTF( param->gout, " %s", c_type_name( cv_qualifier ) );
      if ( ref_qualifier != T_NONE )
        FPUTS( (ref_qualifier & T_REFERENCE) ? " &" : " &&", param->gout );
      if ( is_noexcept )
        FPRINTF( param->gout, " %s", L_NOEXCEPT );
      if ( is_throw )
        FPRINTF( param->gout, " %s()", L_THROW );
      if ( is_override )
        FPRINTF( param->gout, " %s", L_OVERRIDE );
      if ( is_final )
        FPRINTF( param->gout, " %s", L_FINAL );
      if ( is_pure_virtual )
        FPUTS( " = 0", param->gout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast_type ), param->gout );
      c_ast_gibberish_space_name( ast, param );
      g_param_leaf( param, ast );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( (ast_type & T_ENUM) != T_NONE ) {
        //
        // Special case: an enum class must be written as just "enum" when
        // doing an elaborated-type-specifier:
        //
        //      c++decl> declare e as enum class C
        //      enum C e;                   // not: enum class C e;
        //
        ast_type &= ~(T_STRUCT | T_CLASS);
      }
      FPRINTF( param->gout,
        "%s %s", c_type_name( ast_type ), ast->as.ecsu.ecsu_name
      );
      c_ast_gibberish_space_name( ast, param );
      g_param_leaf( param, ast );
      break;

    case K_NAME:
      if ( ast->name != NULL && param->gkind != G_CAST )
        FPUTS( ast->name, param->gout );
      g_param_leaf( param, ast );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
    case K_PLACEHOLDER:
      assert( ast->kind != K_PLACEHOLDER );

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE: {
      c_type_id_t const storage = (ast_type & T_MASK_STORAGE);
      if ( storage != T_NONE )
        FPRINTF( param->gout, "%s ", c_type_name( storage ) );
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, param );
      if ( param->gkind != G_CAST && c_ast_name( ast, V_UP ) != NULL &&
           !c_ast_find_kind( ast->parent, V_UP, K_BLOCK | K_FUNCTION ) ) {
        //
        // For all kinds except functions and blocks, we want the output to be
        // like:
        //
        //      type *var
        //
        // i.e., the '*', '&', or "&&" adjacent to the variable; for functions,
        // blocks, when there's no name for a function argument, or when we're
        // casting, we want the output to be like:
        //
        //      type* func()            // function
        //      type* (^block)()        // block
        //      func(type*)             // nameless function argument
        //      (type*)expr             // cast
        //
        // i.e., the '*', '&', or "&&" adjacent to the type.
        //
        g_param_space( param );
      }
      if ( !param->postfix )
        c_ast_gibberish_qual_name( ast, param );
      break;
    }

    case K_POINTER_TO_MEMBER:
      c_ast_gibberish_impl( ast->as.ptr_mbr.of_ast, param );
      if ( !param->postfix ) {
        FPUTC( ' ', param->gout );
        c_ast_gibberish_qual_name( ast, param );
      }
      break;

    case K_TYPEDEF:
      if ( ast_type != T_TYPEDEF_TYPE )
        FPRINTF( param->gout, "%s ", c_type_name( ast_type ) );
      FPRINTF( param->gout, "%s", ast->as.c_typedef->ast->name );
      c_ast_gibberish_space_name( ast, param );
      g_param_leaf( param, ast );
      break;

    case K_VARIADIC:
      FPUTS( L_ELLIPSIS, param->gout );
      g_param_leaf( param, ast );
      break;
  } // switch
}

/**
 * Helper function for `c_ast_gibberish_impl()` that handles the printing of
 * "postfix" cases:
 *
 *  + Array of pointer to function.
 *  + Pointer to array.
 *  + Reference to array.
 *
 * @param ast The `c_ast`.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_postfix( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( c_ast_is_parent( ast ) );
  assert( param != NULL );

  c_ast_t const *const parent = ast->parent;

  if ( parent != NULL ) {
    switch ( parent->kind ) {
      case K_ARRAY:
      case K_BLOCK:                     // Apple extension
      case K_FUNCTION:
        c_ast_gibberish_postfix( parent, param );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
        switch ( ast->kind ) {
          case K_BLOCK:                 // Apple extension
            FPUTS( "(^", param->gout );
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
            FPUTC( '(', param->gout );
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

        c_ast_gibberish_qual_name( parent, param );
        if ( c_ast_is_parent( parent->parent ) )
          c_ast_gibberish_postfix( parent, param );

        if ( (ast->kind & (K_POINTER | K_POINTER_TO_MEMBER)) == K_NONE )
          FPUTC( ')', param->gout );
        break;

      default:
        /* suppress warning */;
    } // switch
  } else {
    //
    // We've reached the root of the AST that has the name of the thing we're
    // printing the gibberish for.
    //
    if ( ast->kind == K_BLOCK )
      FPUTS( "(^", param->gout );
    c_ast_gibberish_space_name( ast, param );
    if ( ast->kind == K_BLOCK )
      FPUTC( ')', param->gout );
  }

  //
  // We're now unwinding the recursion: print the "postfix" things (size for
  // arrays, arguments for functions) in root-to-leaf order.
  //
  switch ( ast->kind ) {
    case K_ARRAY:
      c_ast_gibberish_array_size( ast, param );
      break;
    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      c_ast_gibberish_func_args( ast, param );
      break;
    default:
      /* suppress warning */;
  } // switch
}

/**
 * Helper function for `c_ast_gibberish_impl()` that prints a pointer, pointer-
 * to-member, reference, or rvalue reference, its qualifier, if any, and the
 * name, if any.
 *
 * @param ast The `c_ast` that is one of <code>\ref K_POINTER</code>,
 * <code>\ref K_POINTER_TO_MEMBER</code>, <code>\ref K_REFERENCE</code>, or
 * <code>\ref K_RVALUE_REFERENCE</code> whose qualifier, if any, and name, if
 * any, to print.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_qual_name( c_ast_t const *ast,
                                       g_param_t const *param ) {
  assert( ast != NULL );

  switch ( ast->kind ) {
    case K_POINTER:
      FPUTC( '*', param->gout );
      break;
    case K_POINTER_TO_MEMBER:
      FPRINTF( param->gout, "%s::*", ast->as.ptr_mbr.class_name );
      break;
    case K_REFERENCE:
      FPUTC( '&', param->gout );
      break;
    case K_RVALUE_REFERENCE:
      FPUTS( "&&", param->gout );
      break;
    default:
      assert(
        ast->kind &
          (K_POINTER | K_POINTER_TO_MEMBER | K_REFERENCE | K_RVALUE_REFERENCE)
      );
  } // switch

  c_type_id_t const qualifier = (ast->type_id & T_MASK_QUALIFIER);
  if ( qualifier != T_NONE ) {
    FPUTS( c_type_name( qualifier ), param->gout );
    if ( param->gkind != G_CAST )
      FPUTC( ' ', param->gout );
  }
  if ( ast->name != NULL && param->gkind != G_CAST )
    FPUTS( ast->name, param->gout );
}

/**
 * Helper function for `c_ast_gibberish_impl()` that prints a space (if it
 * hasn't printed one before) and an AST node's name, if any.
 *
 * @param ast The `c_ast` to print the name of, if any.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_space_name( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( param != NULL );

  if ( ast->name != NULL && param->gkind != G_CAST ) {
    g_param_space( param );
    FPUTS( ast->name, param->gout );
  }
}

/**
 * Initializes a `g_param`.
 *
 * @param param The `g_param` to initialize.
 * @param gkind The <code>\ref g_kind</code> of gibberish to print.
 * @param root The `c_ast` root.
 * @param gout The `FILE` to print it to.
 */
static void g_param_init( g_param_t *param, c_ast_t const *root,
                          g_kind_t gkind, FILE *gout ) {
  assert( param != NULL );
  assert( root != NULL );

  STRUCT_ZERO( param );
  param->gkind = gkind;
  param->gout = gout;
  param->root_ast = root;
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_gibberish_cast( c_ast_t const *ast, FILE *gout ) {
  g_param_t param;
  g_param_init( &param, ast, G_CAST, gout );
  c_ast_gibberish_impl( ast, &param );
}

void c_ast_gibberish_declare( c_ast_t const *ast, FILE *gout ) {
  g_param_t param;
  g_param_init( &param, ast, G_DECLARE, gout );
  c_ast_gibberish_impl( ast, &param );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
