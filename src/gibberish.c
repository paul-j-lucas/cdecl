/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
#include "pjl_config.h"                 /* must go first */
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
 * State maintained by c_ast_gibberish() (because there'd be too many function
 * arguments otherwise).
 */
struct g_state {
  c_gib_kind_t  gib_kind;               ///< Kind of gibberish to print.
  FILE         *gout;                   ///< Where to write the gibberish.
  bool          postfix;                ///< Doing postfix gibberish?
  bool          printed_space;          ///< Printed a space yet?
  bool          printing_typedef;       ///< Printing a `typedef`?
  bool          skip_name_for_using;    ///< Skip type name for `using`?
};
typedef struct g_state g_state_t;

// local functions
static void g_init( g_state_t*, c_gib_kind_t, bool, FILE* );
static void g_print_ast( g_state_t*, c_ast_t const* );
static void g_print_ast_name( g_state_t*, c_ast_t const* );
static void g_print_postfix( g_state_t*, c_ast_t const* );
static void g_print_qual_name( g_state_t*, c_ast_t const* );
static void g_print_space_ast_name( g_state_t*, c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Prints a space only if we haven't printed one yet.
 *
 * @param g The `g_state` to use.
 */
static inline void g_print_space_once( g_state_t *g ) {
  if ( false_set( &g->printed_space ) )
    FPUTC( ' ', g->gout );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_ast_gibberish() that prints \a ast as a C/C++
 * declaration or cast.
 *
 * @param ast The AST to print.
 * @param gib_kind The kind of gibberish to print as.
 * @param printing_typedef Printing a `typedef`?
 * @param gout The `FILE` to print to.
 */
static void c_ast_gibberish_impl( c_ast_t const *ast, c_gib_kind_t gib_kind,
                                  bool printing_typedef, FILE *gout ) {
  assert( ast != NULL );
  assert( gout != NULL );

  g_state_t g;
  g_init( &g, gib_kind, printing_typedef, gout );
  g_print_ast( &g, ast );
}

/**
 * Initializes a `g_state`.
 *
 * @param g The `g_state` to initialize.
 * @param gib_kind The kind of gibberish to print.
 * @param printing_typedef Printing a `typedef`?
 * @param gout The `FILE` to print it to.
 */
static void g_init( g_state_t *g, c_gib_kind_t gib_kind, bool printing_typedef,
                    FILE *gout ) {
  assert( g != NULL );
  assert( gout != NULL );

  MEM_ZERO( g );
  g->gib_kind = gib_kind;
  g->gout = gout;
  g->printing_typedef = printing_typedef;
  if ( gib_kind == C_GIB_USING )
    g->skip_name_for_using = true;
}

/**
 * Prints \a ast as gibberish, aka, a C/C++ declaration.
 *
 * @param g The `g_state` to use.
 * @param ast The AST to print.
 */
static void g_print_ast( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  c_type_t type = ast->type;

  c_tid_t cv_qual_stid    = TS_NONE;
  bool    is_default      = false;
  bool    is_delete       = false;
  bool    is_final        = false;
  bool    is_noexcept     = false;
  bool    is_override     = false;
  bool    is_pure_virtual = false;
  bool    is_throw        = false;
  c_tid_t ref_qual_stid   = TS_NONE;

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
      PJL_FALLTHROUGH;

    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      //
      // These things aren't printed as part of the type beforehand, so strip
      // them out of the type here, but print them after the parameters.
      //
      cv_qual_stid    = (type.stid & TS_MASK_QUALIFIER);
      is_default      = (type.stid & TS_DEFAULT) != TS_NONE;
      is_delete       = (type.stid & TS_DELETE) != TS_NONE;
      is_final        = (type.stid & TS_FINAL) != TS_NONE;
      is_noexcept     = (type.stid & TS_NOEXCEPT) != TS_NONE;
      is_override     = (type.stid & TS_OVERRIDE) != TS_NONE;
      is_pure_virtual = (type.stid & TS_PURE_VIRTUAL) != TS_NONE;
      is_throw        = (type.stid & TS_THROW) != TS_NONE;
      ref_qual_stid   = (type.stid & TS_MASK_REF_QUALIFIER);

      type.stid &= c_tid_compl(
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
        if ( true_clear( &is_noexcept ) )
          is_throw = true;
      } else {
        if ( true_clear( &is_throw ) )
          is_noexcept = true;
      }
      PJL_FALLTHROUGH;

    case K_ARRAY:
    case K_APPLE_BLOCK:
      if ( !c_type_is_none( &type ) )
        FPRINTF( g->gout, "%s ", c_type_name_c( &type ) );
      if ( ast->kind_id == K_USER_DEF_CONVERSION ) {
        if ( !c_ast_empty_name( ast ) )
          FPRINTF( g->gout, "%s::", c_ast_full_name( ast ) );
        FPRINTF( g->gout, "%s ", L_OPERATOR );
      }
      if ( ast->as.parent.of_ast != NULL )
        g_print_ast( g, ast->as.parent.of_ast );
      if ( false_set( &g->postfix ) ) {
        if ( !g->skip_name_for_using && g->gib_kind != C_GIB_CAST )
          g_print_space_once( g );
        g_print_postfix( g, ast );
      }
      if ( cv_qual_stid != TS_NONE )
        FPRINTF( g->gout, " %s", c_tid_name_c( cv_qual_stid ) );
      if ( ref_qual_stid != TS_NONE ) {
        FPUTS(
          (ref_qual_stid & TS_REFERENCE) != TS_NONE ?  " &" : " &&",
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
      FPUTS( c_type_name_c( &ast->type ), g->gout );
      g_print_space_ast_name( g, ast );
      if ( ast->as.builtin.bit_width > 0 )
        FPRINTF( g->gout, " : %u", ast->as.builtin.bit_width );
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
        type.btid &= c_tid_compl( TB_STRUCT | TB_CLASS );
      }

      if ( opt_east_const ) {
        cv_qual_stid = type.stid & (TS_CONST | TS_VOLATILE);
        type.stid &= c_tid_compl( TS_CONST | TS_VOLATILE );
      }

      FPUTS( c_type_name_c( &type ), g->gout );

      if ( g->gib_kind != C_GIB_TYPEDEF || g->printing_typedef ) {
        //
        // For enum, class, struct, or union (ECSU) types, we need to print the
        // ECSU name when either:
        //
        //  + The AST is not a typedef, e.g.:
        //
        //          cdecl> declare x as struct S
        //          struct S x;         // ast->sname = "x"; escu_name = "S"
        //
        //    (See the printing_typedef comment in c_typedef_gibberish()
        //    first.)  Or:
        //
        //  + We're printing an ECSU type in C only, e.g.:
        //
        //          typedef struct S T; // ast->sname ="T"; escu_name = "S"
        //
        FPRINTF( g->gout,
          " %s", c_sname_full_name( &ast->as.ecsu.ecsu_sname )
        );
      }

      if ( ast->as.ecsu.of_ast != NULL ) {
        FPUTS( " : ", g->gout );
        g_print_ast( g, ast->as.ecsu.of_ast );
      }

      if ( cv_qual_stid != TS_NONE )
        FPRINTF( g->gout, " %s", c_tid_name_c( cv_qual_stid ) );

      g_print_space_ast_name( g, ast );
      break;

    case K_NAME:
      if ( !c_ast_empty_name( ast ) && g->gib_kind != C_GIB_CAST )
        g_print_ast_name( g, ast );
      break;

    case K_NONE:                        // should not occur in completed AST
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:                 // should not occur in completed AST
      assert( ast->kind_id != K_PLACEHOLDER );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE: {
      c_tid_t stid = type.stid & TS_MASK_STORAGE;
      if ( stid != TS_NONE )
        FPRINTF( g->gout, "%s ", c_tid_name_c( stid ) );
      g_print_ast( g, ast->as.ptr_ref.to_ast );
      if ( !g->skip_name_for_using && g->gib_kind != C_GIB_CAST &&
           c_ast_find_name( ast, C_VISIT_UP ) != NULL &&
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
        g_print_space_once( g );
      }
      if ( !g->postfix )
        g_print_qual_name( g, ast );
      break;
    }

    case K_POINTER_TO_MEMBER:
      g_print_ast( g, ast->as.ptr_mbr.of_ast );
      if ( !g->postfix ) {
        FPUTC( ' ', g->gout );
        g_print_qual_name( g, ast );
      }
      break;

    case K_TYPEDEF: {
      //
      // Of course a K_TYPEDEF AST also has a type comprising TB_TYPEDEF, but
      // we need to see whether there's any more to the type, e.g., "const".
      //
      bool const is_more_than_plain_typedef =
        !c_type_equal( &ast->type, &C_TYPE_LIT_B( TB_TYPEDEF ) );
      if ( is_more_than_plain_typedef && !opt_east_const )
        FPRINTF( g->gout, "%s ", c_type_name_c( &ast->type ) );

      //
      // Temporarily set skip_name_for_using to false to force printing of the
      // type's name.  This is necessary for when printing the name of a
      // typedef of a typedef as a "using" declaration:
      //
      //      c++decl> typedef int32_t foo_t
      //      c++decl> show foo_t as using
      //      using foo_t = int32_t;
      //
      bool const orig_skip_name_for_using = g->skip_name_for_using;
      g->skip_name_for_using = false;
      g_print_ast_name( g, ast->as.tdef.for_ast );
      g->skip_name_for_using = orig_skip_name_for_using;

      if ( is_more_than_plain_typedef && opt_east_const )
        FPRINTF( g->gout, " %s", c_type_name_c( &ast->type ) );
      g_print_space_ast_name( g, ast );
      if ( ast->as.tdef.bit_width > 0 )
        FPRINTF( g->gout, " : %u", ast->as.tdef.bit_width );
      break;
    }

    case K_VARIADIC:
      FPUTS( L_ELLIPSIS, g->gout );
      break;
  } // switch
}

/**
 * Helper function for g_print_ast() that prints an array's size.
 *
 * @param g The `g_state` to use.
 * @param ast The AST that is a <code>\ref K_ARRAY</code> whose size to print.
 */
static void g_print_ast_array_size( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( ast->kind_id == K_ARRAY );

  FPUTS( graph_token_c( "[" ), g->gout );
  if ( ast->as.array.stid != TS_NONE )
    FPRINTF( g->gout, "%s ", c_tid_name_c( ast->as.array.stid ) );
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
 * Helper function for g_print_ast() that prints a function-like AST's
 * parameters, if any.
 *
 * @param g The `g_state` to use.
 * @param ast The AST that is <code>\ref K_ANY_FUNCTION_LIKE</code> whose
 * parameters to print.
 */
static void g_print_ast_func_params( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );

  bool comma = false;
  FPUTC( '(', g->gout );
  FOREACH_PARAM( param, ast ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", g->gout );
    g_state_t params_g;
    g_init( &params_g, g->gib_kind, /*printing_typedef=*/false, g->gout );
    g_print_ast( &params_g, c_param_ast( param ) );
  } // for
  FPUTC( ')', g->gout );
}

/**
 * Prints either the full or local name of \a ast based on whether we're
 * emitting the gibberish for a `typedef` since it can't have a scoped name.
 *
 * @param g The `g_state` to use.
 * @param ast The AST to get the name of.
 */
static void g_print_ast_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  if ( g->skip_name_for_using ) {
    //
    // If we're printing a type as a "using" declaration, we have to skip
    // printing the type name since it's already been printed immediately after
    // the "using".  For example, the type:
    //
    //      typedef int (*PF)(char c);
    //
    // when printed as a "using":
    //
    //      using PF = int(*)(char c);
    //
    // had the "using PF =" part already printed in c_typedef_gibberish(), so
    // we don't print it again after the '*'; but we still need to print all
    // subsequent names, if any.  Hence, reset the flags and print nothing.
    //
    g->skip_name_for_using = false;
    g->printed_space = true;
    return;
  }

  FPUTS(
    //
    // For typedefs, the scope names (if any) were already printed in
    // c_typedef_gibberish() so now we just print the local name.
    //
    g->gib_kind == C_GIB_TYPEDEF ?
      c_ast_local_name( ast ) : c_ast_full_name( ast ),
    g->gout
  );
}

/**
 * Helper function for g_print_ast() that handles the printing of "postfix"
 * cases:
 *
 *  + Array of pointer to function.
 *  + Pointer to array.
 *  + Reference to array.
 *
 * @param g The `g_state` to use.
 * @param ast The AST.
 */
static void g_print_postfix( g_state_t *g, c_ast_t const *ast ) {
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
        g_print_postfix( g, parent_ast );
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

        g_print_qual_name( g, parent_ast );
        if ( c_ast_is_parent( parent_ast->parent_ast ) )
          g_print_postfix( g, parent_ast );

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
    g_print_space_ast_name( g, ast );
    if ( ast->kind_id == K_APPLE_BLOCK )
      FPUTC( ')', g->gout );
  }

  //
  // We're now unwinding the recursion: print the "postfix" things (size for
  // arrays, parameters for functions) in root-to-leaf order.
  //
  switch ( ast->kind_id ) {
    case K_ARRAY:
      g_print_ast_array_size( g, ast );
      break;
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      g_print_ast_func_params( g, ast );
      break;
    case K_USER_DEF_CONVERSION:
      FPUTS( "()", g->gout );
      break;
    default:
      /* suppress warning */;
  } // switch
}

/**
 * Helper function for g_print_ast() that prints a pointer, pointer-to-member,
 * reference, or rvalue reference, its qualifier, if any, and the name, if any.
 *
 * @param g The `g_state` to use.
 * @param ast The AST that is one of <code>\ref K_POINTER</code>,
 * <code>\ref K_POINTER_TO_MEMBER</code>, <code>\ref K_REFERENCE</code>, or
 * <code>\ref K_RVALUE_REFERENCE</code> whose qualifier, if any, and name, if
 * any, to print.
 */
static void g_print_qual_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  c_tid_t const qual_stid = ast->type.stid & TS_MASK_QUALIFIER;

  switch ( ast->kind_id ) {
    case K_POINTER:
      if ( qual_stid != TS_NONE && g->gib_kind != C_GIB_CAST &&
           ast->as.ptr_ref.to_ast->kind_id != K_FUNCTION ) {
        //
        // If we're printing a type as a "using" declaration and there's a
        // qualifier for the pointer, print a space before it.  For example:
        //
        //      typedef int *const PI;
        //
        // when printed as a "using":
        //
        //      using PI = int *const;
        //
        // However, if it's a pointer-to-function, don't.  For example:
        //
        //      typedef int (*const PF)(char c);
        //
        // when printed as a "using":
        //
        //      using PF = int(*const)(char c);
        //
        g_print_space_once( g );
      }
      FPUTC( '*', g->gout );
      break;
    case K_POINTER_TO_MEMBER:
      FPRINTF( g->gout,
        "%s::*", c_sname_full_name( &ast->as.ptr_mbr.class_sname )
      );
      break;
    case K_REFERENCE:
      if ( opt_alt_tokens ) {
        g_print_space_once( g );
        FPRINTF( g->gout, "%s ", L_BITAND );
      } else {
        FPUTC( '&', g->gout );
      }
      break;
    case K_RVALUE_REFERENCE:
      if ( opt_alt_tokens ) {
        g_print_space_once( g );
        FPRINTF( g->gout, "%s ", L_AND );
      } else {
        FPUTS( "&&", g->gout );
      }
      break;
    default:
      /* suppress warning */;
  } // switch

  if ( qual_stid != TS_NONE ) {
    FPUTS( c_tid_name_c( qual_stid ), g->gout );

    if ( (g->gib_kind & (C_GIB_DECL | C_GIB_TYPEDEF)) != C_GIB_NONE &&
         c_ast_find_name( ast, C_VISIT_UP ) != NULL ) {
      //
      // For declarations and typedefs, if there is a qualifier and if a name
      // has yet to be printed, we always need to print a space after the
      // qualifier, e.g.:
      //
      //      char *const p;
      //                 ^
      FPUTC( ' ', g->gout );
    }
  }

  g_print_ast_name( g, ast );
}

/**
 * Helper function for g_print_ast() that prints a space (if it hasn't printed
 * one before) and an AST node's name, if any; but only if we're printing a
 * declaration (not a cast).
 *
 * @param g The `g_state` to use.
 * @param ast The AST to print the name (if any) of.
 */
static void g_print_space_ast_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  if ( g->gib_kind == C_GIB_CAST )
    return;                             // for casts, print nothing

  switch ( ast->kind_id ) {
    case K_CONSTRUCTOR:
      FPUTS( c_ast_full_name( ast ), g->gout );
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
      g_print_space_once( g );
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
      g_print_space_once( g );
      if ( c_ast_count_name( ast ) > 1 )
        FPRINTF( g->gout, "%s::", c_ast_scope_name( ast ) );
      FPRINTF( g->gout, "%s\"\" %s", L_OPERATOR, c_ast_local_name( ast ) );
      break;
    default:
      if ( !c_ast_empty_name( ast ) ) {
        if ( !g->skip_name_for_using )
          g_print_space_once( g );
        g_print_ast_name( g, ast );
      }
      break;
  } // switch
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_gibberish( c_ast_t const *ast, c_gib_kind_t gib_kind, FILE *gout ) {
  assert( ast != NULL );
  assert( (gib_kind & (C_GIB_CAST | C_GIB_DECL)) != C_GIB_NONE );
  assert( gout != NULL );

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_EXPR:
      FPRINTF( gout, "%s(%u) ", alignas_lang(), ast->align.as.expr );
      break;
    case C_ALIGNAS_TYPE:
      FPRINTF( gout, "%s(", alignas_lang() );
      c_ast_gibberish( ast->align.as.type_ast, C_GIB_DECL, gout );
      FPUTS( ") ", gout );
      break;
  } // switch

  c_ast_gibberish_impl( ast, gib_kind, /*printing_typedef=*/false, gout );
}

void c_typedef_gibberish( c_typedef_t const *tdef, c_gib_kind_t gib_kind,
                          FILE *gout ) {
  assert( tdef != NULL );
  assert( (gib_kind & (C_GIB_TYPEDEF | C_GIB_USING)) != C_GIB_NONE );
  assert( gout != NULL );

  size_t scope_close_braces_to_print = 0;
  c_type_t scope_type = T_NONE;

  c_sname_t const *const sname = c_ast_find_name( tdef->ast, C_VISIT_DOWN );
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
    if ( scope_type.btid != TB_NAMESPACE ||
         (opt_lang & (LANG_CPP_MIN(17) | LANG_C_ANY)) != LANG_NONE ) {
      //
      // All C++ versions support nested class/struct/union declarations, e.g.:
      //
      //      struct S::T { typedef int I; }
      //
      // However, only C++17 and later support nested namespace declarations:
      //
      //      namespace S::T { typedef int I; }
      //
      // If the current language is any version of C, also print in nested
      // namespace form.
      //
      scope_type = *c_sname_scope_type( sname );
      if ( scope_type.btid == TB_SCOPE )
        scope_type.btid = TB_NAMESPACE;
      FPRINTF( gout,
        "%s %s { ", c_type_name_c( &scope_type ), c_sname_scope_name( sname )
      );
      scope_close_braces_to_print = 1;
    }
    else {
      //
      // Namespaces in C++14 and earlier require distinct declarations:
      //
      //      namespace S { namespace T { typedef int I; } }
      //
      FOREACH_SCOPE( scope, sname, sname->tail ) {
        scope_type = c_scope_data( scope )->type;
        if ( scope_type.btid == TB_SCOPE )
          scope_type.btid = TB_NAMESPACE;
        FPRINTF( gout,
          "%s %s { ",
          c_type_name_c( &scope_type ), c_scope_data( scope )->name
        );
      } // for
      scope_close_braces_to_print = c_sname_count( sname ) - 1;
    }
  }

  bool const is_ecsu = tdef->ast->kind_id == K_ENUM_CLASS_STRUCT_UNION;

  //
  // When printing a type, all types except enum, class, struct, or union types
  // must be preceded by "typedef", e.g.:
  //
  //      typedef int int32_t;
  //
  // However, enum, class, struct, and union types are preceded by "typedef"
  // only when the type was declared in C since those types in C without a
  // typedef are merely in the tags namespace and not first-class types:
  //
  //      struct S;                     // In C, tag only -- not a type.
  //      typedef struct S S;           // Now it's a type.
  //
  // In C++, such typedefs are unnecessary since such types are first-class
  // types and not just tags, so we don't print "typedef".
  //
  bool const printing_typedef = gib_kind == C_GIB_TYPEDEF &&
    (!is_ecsu || c_lang_is_c( tdef->lang_ids ) ||
    (OPT_LANG_IS(C_ANY) && !c_lang_is_cpp( tdef->lang_ids )));

  bool const printing_using = gib_kind == C_GIB_USING && !is_ecsu;

  if ( printing_typedef )
    FPRINTF( gout, "%s ", L_TYPEDEF );
  else if ( printing_using )
    FPRINTF( gout, "%s %s = ", L_USING, c_sname_local_name( sname ) );

  c_ast_gibberish_impl(
    tdef->ast, printing_using ? C_GIB_USING : C_GIB_TYPEDEF,
    printing_typedef, gout
  );

  if ( scope_close_braces_to_print > 0 ) {
    FPUTC( ';', gout );
    while ( scope_close_braces_to_print-- > 0 )
      FPUTS( " }", gout );
  }

  if ( opt_semicolon && scope_type.btid != TB_NAMESPACE )
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
