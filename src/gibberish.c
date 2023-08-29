/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
#include "c_operator.h"
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

/**
 * @addtogroup printing-gibberish-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * State maintained by c_ast_gibberish() (because there'd be too many function
 * arguments otherwise).
 */
struct g_state {
  unsigned  gib_flags;                  ///< Gibberish printing flags.
  FILE     *gout;                       ///< Where to print the gibberish.
  bool      postfix;                    ///< Doing postfix gibberish?
  bool      printed_space;              ///< Printed a space yet?
  bool      printed_typedef;            ///< Printed `typedef`?
};
typedef struct g_state g_state_t;

// local functions
static void g_init( g_state_t*, unsigned, FILE* );
static void g_print_ast( g_state_t*, c_ast_t const* );
static void g_print_ast_bit_width( g_state_t const*, c_ast_t const* );
static void g_print_ast_list( g_state_t const*, c_ast_list_t const* );
static void g_print_ast_name( g_state_t*, c_ast_t const* );
static void g_print_postfix( g_state_t*, c_ast_t const* );
static void g_print_qual_name( g_state_t*, c_ast_t const* );
static void g_print_space_ast_name( g_state_t*, c_ast_t const* );

NODISCARD
static bool g_space_before_ptr_ref( g_state_t const*, c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Convenience function for finding an ancestor of \a ast that is of kind
 * #K_ANY_FUNCTION_LIKE, if any.
 *
 * @param ast The AST node whose parent to start from.
 * @return Returns a pointer to an ancestor of \a ast that is of kind
 * #K_ANY_FUNCTION_LIKE or NULL for none.
 */
NODISCARD
static inline c_ast_t const* c_ast_find_parent_func( c_ast_t const *ast ) {
  c_ast_t *const parent_ast = ast->parent_ast;
  return parent_ast != NULL ?
    c_ast_find_kind_any( parent_ast, C_VISIT_UP, K_ANY_FUNCTION_LIKE ) : NULL;
}

/**
 * Prints a space only if we haven't printed one yet.
 *
 * @param g The g_state to use.
 */
static inline void g_print_space_once( g_state_t *g ) {
  if ( false_set( &g->printed_space ) )
    FPUTC( ' ', g->gout );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Initializes a g_state.
 *
 * @param g The g_state to initialize.
 * @param gib_flags The gibberish flags to use.
 * @param gout The `FILE` to print to.
 */
static void g_init( g_state_t *g, unsigned gib_flags, FILE *gout ) {
  assert( g != NULL );
  assert( gib_flags != C_GIB_NONE );
  assert( gout != NULL );

  MEM_ZERO( g );
  g->gib_flags = gib_flags;
  g->gout = gout;
  g->printed_space = (gib_flags & C_GIB_OMIT_TYPE) != 0;
}

/**
 * Prints \a ast as gibberish, aka, a C/C++ declaration.
 *
 * @param g The g_state to use.
 * @param ast The AST to print.
 */
static void g_print_ast( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  g_state_t child_g;
  c_type_t type = ast->type;

  if ( (g->gib_flags & C_GIB_USING) != 0 ) {
    //
    // If we're printing a "using" declaration, don't print either "typedef" or
    // attributes since they will have been printed in c_typedef_gibberish().
    //
    type.stids &= c_tid_compl( TS_TYPEDEF );
    type.atids = TA_NONE;
  }

  c_tid_t cv_qual_stids   = TS_NONE;
  bool    is_default      = false;
  bool    is_delete       = false;
  bool    is_final        = false;
  bool    is_fixed_enum   = false;
  bool    is_noexcept     = false;
  bool    is_override     = false;
  bool    is_pure_virtual = false;
  bool    is_throw        = false;
  bool    is_trailing_ret = false;
  c_tid_t msc_call_atids  = TA_NONE;
  c_tid_t ref_qual_stids  = TS_NONE;

  //
  // This isn't implemented using a visitor due to the complicated way the
  // nodes need to be visited in order to print gibberish.
  //
  switch ( ast->kind ) {
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_USER_DEF_CONVERSION:
      //
      // Since none of these have a return type, no space needs to be printed
      // before the name, so lie and set the "space" flag.
      //
      g->printed_space = true;
      FALLTHROUGH;

    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      //
      // These things aren't printed as part of the type beforehand, so strip
      // them out of the type here, but print them after the parameters.
      //
      cv_qual_stids   = (type.stids & TS_ANY_QUALIFIER);
      is_default      = (type.stids & TS_DEFAULT) != TS_NONE;
      is_delete       = (type.stids & TS_DELETE) != TS_NONE;
      is_final        = (type.stids & TS_FINAL) != TS_NONE;
      is_noexcept     = (type.stids & TS_NOEXCEPT) != TS_NONE;
      is_pure_virtual = (type.stids & TS_PURE_VIRTUAL) != TS_NONE;
      is_throw        = (type.stids & TS_THROW) != TS_NONE;
      ref_qual_stids  = (type.stids & TS_ANY_REFERENCE);

      // In C++, "override" should be printed only if "final" isn't.
      is_override     = !is_final && (type.stids & TS_OVERRIDE) != TS_NONE;

      type.stids &= c_tid_compl(
                       TS_ANY_QUALIFIER
                     | TS_ANY_REFERENCE
                     | TS_DEFAULT
                     | TS_DELETE
                     | TS_FINAL
                     | TS_NOEXCEPT
                     | TS_OVERRIDE
                     | TS_PURE_VIRTUAL
                     | TS_THROW
                     // In C++, if either "override" or "final" is printed,
                     // "virtual" shouldn't be.
                     | (is_override || is_final ? TS_VIRTUAL : TS_NONE)
                   );

      //
      // Microsoft calling conventions are printed specially.
      //
      msc_call_atids = type.atids & TA_ANY_MSC_CALL;
      type.atids &= c_tid_compl( TA_ANY_MSC_CALL );

      //
      // Depending on the C++ language version, change noexcept to throw() or
      // vice versa.
      //
      if ( OPT_LANG_IS( noexcept ) ) {
        if ( true_clear( &is_throw ) )
          is_noexcept = true;
      } else {
        if ( true_clear( &is_noexcept ) )
          is_throw = true;
      }
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_ARRAY:
      if ( ast->kind != K_ARRAY ||
           ( !c_tid_is_any( ast->type.stids, TS_ANY_ARRAY_QUALIFIER ) ) ) {
        fputs_sp( c_type_name_c( &type ), g->gout );
      }
      if ( ast->kind == K_USER_DEF_CONVERSION ) {
        if ( !c_sname_empty( &ast->sname ) )
          FPRINTF( g->gout, "%s::", c_sname_full_name( &ast->sname ) );
        FPUTS( "operator ", g->gout );
      }
      if ( ast->parent.of_ast != NULL ) {
        is_trailing_ret = (ast->kind & K_ANY_TRAILING_RETURN) != 0 &&
          opt_trailing_ret && OPT_LANG_IS( TRAILING_RETURN_TYPE );
        if ( is_trailing_ret )
          FPUTS( L_auto, g->gout );
        else
          g_print_ast( g, ast->parent.of_ast );
      }
      if ( msc_call_atids != TA_NONE &&
           !c_ast_parent_is_kind( ast, K_POINTER ) ) {
        //
        // If ast is a function having a Microsoft calling convention, but not
        // a pointer to such a function, print the calling convention.
        // (Pointers to such functions are handled in g_print_postfix().)
        //
        FPRINTF( g->gout, " %s", c_tid_name_c( msc_call_atids ) );
      }
      if ( false_set( &g->postfix ) ) {
        if ( (g->gib_flags & (C_GIB_CAST | C_GIB_USING)) == 0 )
          g_print_space_once( g );
        g_print_postfix( g, ast );
      }
      if ( cv_qual_stids != TS_NONE )
        FPRINTF( g->gout, " %s", c_tid_name_c( cv_qual_stids ) );
      if ( ref_qual_stids != TS_NONE ) {
        FPRINTF( g->gout, " %s",
          alt_token_c(
            c_tid_is_any( ref_qual_stids, TS_REFERENCE ) ? "&" : "&&"
          )
        );
      }
      if ( is_noexcept )
        FPUTS( " noexcept", g->gout );
      else if ( is_throw )
        FPUTS( " throw()", g->gout );
      if ( is_override )
        FPUTS( " override", g->gout );
      else if ( is_final )
        FPUTS( " final", g->gout );
      if ( is_trailing_ret ) {
        FPUTS( " -> ", g->gout );
        //
        // Temporarily orphan the return type's AST in order to print it as a
        // stand-alone trailing type.
        //
        c_ast_t *const ret_ast = ast->func.ret_ast;
        c_ast_t *const orig_ret_ast_parent_ast = ret_ast->parent_ast;
        ret_ast->parent_ast = NULL;

        g_init( &child_g, C_GIB_DECL, g->gout );
        g_print_ast( &child_g, ret_ast );
        ret_ast->parent_ast = orig_ret_ast_parent_ast;
      }
      if ( is_pure_virtual )
        FPUTS( " = 0", g->gout );
      else if ( is_default )
        FPUTS( " = default", g->gout );
      else if ( is_delete )
        FPUTS( " = delete", g->gout );
      break;

    case K_BUILTIN:
      if ( (g->gib_flags & C_GIB_OMIT_TYPE) == 0 )
        FPUTS( c_type_name_c( &type ), g->gout );
      if ( c_ast_is_tid_any( ast, TB_BITINT ) && ast->builtin.BitInt.width > 0 )
        FPRINTF( g->gout, "(%u)", ast->builtin.BitInt.width );
      g_print_space_ast_name( g, ast );
      g_print_ast_bit_width( g, ast );
      break;

    case K_CAPTURE:
      switch ( ast->capture.kind ) {
        case C_CAPTURE_COPY:
          FPUTC( '=', g->gout );
          break;
        case C_CAPTURE_REFERENCE:
          FPUTS( alt_token_c( "&" ), g->gout );
          if ( c_sname_empty( &ast->sname ) )
            break;
          if ( opt_alt_tokens )
            FPUTC( ' ', g->gout );
          FALLTHROUGH;
        case C_CAPTURE_VARIABLE:
          FPUTS( c_sname_full_name( &ast->sname ), g->gout );
          break;
        case C_CAPTURE_STAR_THIS:
          FPUTC( '*', g->gout );
          FALLTHROUGH;
        case C_CAPTURE_THIS:
          FPUTS( L_this, g->gout );
          break;
      } // switch
      break;

    case K_CAST:
      assert( g->gib_flags == C_GIB_CAST );
      g_init( &child_g, C_GIB_CAST, g->gout );
      if ( ast->cast.kind == C_CAST_C ) {
        FPUTC( '(', g->gout );
        g_print_ast( &child_g, ast->cast.to_ast );
        FPRINTF( g->gout, ")%s\n", c_sname_full_name( &ast->sname ) );
      } else {
        FPRINTF( g->gout, "%s<", c_cast_gibberish( ast->cast.kind ) );
        g_print_ast( &child_g, ast->cast.to_ast );
        FPRINTF( g->gout, ">(%s)\n", c_sname_full_name( &ast->sname ) );
      }
      break;

    case K_ENUM:
      is_fixed_enum = ast->enum_.of_ast != NULL;
      //
      // Special case: an enum class must be written as just "enum" when doing
      // an elaborated-type-specifier:
      //
      //      c++decl> declare e as enum class C
      //      enum C e;                 // not: enum class C e;
      //
      type.btids &= c_tid_compl( TB_STRUCT | TB_CLASS );
      FALLTHROUGH;

    case K_CLASS_STRUCT_UNION: {
      if ( opt_east_const ) {
        cv_qual_stids = type.stids & TS_CV;
        type.stids &= c_tid_compl( TS_CV );
      }

      char const *const type_name =
        (g->gib_flags & (C_GIB_CAST | C_GIB_DECL)) != 0 &&
        //
        // Special case: a fixed type enum must always have "enum" printed, so
        // we don't call c_type_name_ecsu() that may omit it by applying
        // opt_explicit_ecsu_btids.
        //
        !is_fixed_enum ?
          c_type_name_ecsu( &type ) :
          c_type_name_c( &type );

      FPUTS( type_name, g->gout );

      if ( (g->gib_flags & C_GIB_TYPEDEF) == 0 || g->printed_typedef ) {
        //
        // For enum, class, struct, or union (ECSU) types, we need to print the
        // ECSU name when either:
        //
        //  + The AST is not a typedef, e.g.:
        //
        //          cdecl> declare x as struct S
        //          struct S x;         // ast->sname = "x"; escu_name = "S"
        //
        //    (See the printed_typedef comment in c_typedef_gibberish() first.)
        //    Or:
        //
        //  + We're printing an ECSU type in C only, e.g.:
        //
        //          typedef struct S T; // ast->sname ="T"; escu_name = "S"
        //
        FPRINTF( g->gout,
          "%s%s",
          type_name[0] != '\0' ? " " : "",
          c_sname_full_name( &ast->csu.csu_sname )
        );
      }

      bool printed_name = false;

      if ( is_fixed_enum ) {
        if ( (g->gib_flags & C_GIB_TYPEDEF) != 0 ) {
          //
          // Special case: a fixed type enum needs to have its underlying type
          // printed before east-const qualifiers (if any); but, if it's a type
          // declaration, then we also need to print its name before the
          // qualifiers:
          //
          //      c++decl> define E as enum E of type int
          //      c++decl> show typedef
          //      enum E : int;
          //
          // But if we print its name now, we can't print it again later, so
          // set a flag.
          //
          g_print_space_ast_name( g, ast );
          printed_name = true;
        }
        FPUTS( " : ", g->gout );
        g_print_ast( g, ast->enum_.of_ast );
      }

      if ( cv_qual_stids != TS_NONE )
        FPRINTF( g->gout, " %s", c_tid_name_c( cv_qual_stids ) );

      if ( !printed_name )
        g_print_space_ast_name( g, ast );

      if ( ast->kind == K_ENUM )
        g_print_ast_bit_width( g, ast );
      break;
    }

    case K_LAMBDA:
      FPUTS( graph_token_c( "[" ), g->gout );
      g_print_ast_list( g, &ast->lambda.capture_ast_list );
      FPUTS( graph_token_c( "]" ), g->gout );
      if ( c_ast_params_count( ast ) > 0 ) {
        FPUTC( '(', g->gout );
        g_print_ast_list( g, &ast->lambda.param_ast_list );
        FPUTC( ')', g->gout );
      }
      if ( !c_tid_is_none( ast->type.stids ) )
        FPRINTF( g->gout, " %s", c_tid_name_c( ast->type.stids ) );
      if ( !c_tid_is_none( ast->type.atids ) )
        FPRINTF( g->gout, " %s", c_tid_name_c( ast->type.atids ) );
      if ( ast->lambda.ret_ast != NULL &&
           !c_ast_is_builtin_any( ast->lambda.ret_ast, TB_AUTO | TB_VOID ) ) {
        FPUTS( " -> ", g->gout );
        g_print_ast( g, ast->lambda.ret_ast );
      }
      break;

    case K_NAME:
      if ( OPT_LANG_IS( PROTOTYPES ) ) {
        //
        // In C89-C17, just a name for a function parameter is implicitly int:
        //
        //      cdecl> declare f as function (x) returning double
        //      double f(int x)
        //
        FPUTS( L_int, g->gout );
      }
      if ( (g->gib_flags & C_GIB_CAST) == 0 ) {
        if ( OPT_LANG_IS( PROTOTYPES ) )
          FPUTC( ' ', g->gout );
        g_print_ast_name( g, ast );
      }
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      if ( (g->gib_flags & C_GIB_OMIT_TYPE) == 0 )
        fputs_sp( c_tid_name_c( type.stids & TS_ANY_STORAGE ), g->gout );
      g_print_ast( g, ast->ptr_ref.to_ast );
      if ( g_space_before_ptr_ref( g, ast ) )
        g_print_space_once( g );
      if ( !g->postfix )
        g_print_qual_name( g, ast );
      break;

    case K_POINTER_TO_MEMBER:
      g_print_ast( g, ast->ptr_mbr.to_ast );
      if ( !g->printed_space )
        FPUTC( ' ', g->gout );
      if ( !g->postfix )
        g_print_qual_name( g, ast );
      break;

    case K_TYPEDEF:
      if ( (g->gib_flags & C_GIB_OMIT_TYPE) == 0 ) {
        //
        // Of course a K_TYPEDEF AST also has a type comprising TB_TYPEDEF, but
        // we need to see whether there's any more to the type, e.g., "const".
        //
        bool const is_more_than_plain_typedef =
          !c_type_equiv( &ast->type, &C_TYPE_LIT_B( TB_TYPEDEF ) );

        if ( is_more_than_plain_typedef && !opt_east_const )
          FPUTS( c_type_name_c( &ast->type ), g->gout );

        //
        // Special case: C++23 adds an _Atomic(T) macro for compatibility with
        // C11, but while _Atomic can be printed without () in C, they're
        // required in C++:
        //
        //      _Atomic size_t x;       // C11 only
        //      _Atomic(size_t) y;      // C11 or C++23
        //
        // Note that this handles printing () only for typedef types; for non-
        // typedef types, see the similar special case in c_type_name_impl().
        //
        bool const print_parens_for_Atomic =
          OPT_LANG_IS( CPP_MIN(23) ) &&
          c_tid_is_any( type.stids, TS_ATOMIC );

        if ( print_parens_for_Atomic )
          FPUTC( '(', g->gout );
        else if ( is_more_than_plain_typedef && !opt_east_const )
          FPUTC( ' ', g->gout );

        //
        // Temporarily turn off C_GIB_USING to force printing of the type's
        // name.  This is necessary for when printing the name of a typedef of
        // a typedef as a "using" declaration:
        //
        //      c++decl> typedef int32_t foo_t
        //      c++decl> show foo_t as using
        //      using foo_t = int32_t;
        //
        unsigned const orig_flags = g->gib_flags;
        g->gib_flags &= ~C_GIB_USING;
        g_print_ast_name( g, ast->tdef.for_ast );
        g->gib_flags = orig_flags;
        if ( print_parens_for_Atomic )
          FPUTC( ')', g->gout );
        if ( is_more_than_plain_typedef && opt_east_const )
          FPRINTF( g->gout, " %s", c_type_name_c( &ast->type ) );
      }

      g_print_space_ast_name( g, ast );
      g_print_ast_bit_width( g, ast );
      break;

    case K_VARIADIC:
      FPUTS( L_ellipsis, g->gout );
      break;

    CASE_K_PLACEHOLDER;
  } // switch
}

/**
 * Helper function for g_print_ast() that prints an array's size.
 *
 * @param g The g_state to use.
 * @param ast The AST that is a \ref K_ARRAY whose size to print.
 */
static void g_print_ast_array_size( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );

  FPUTS( graph_token_c( "[" ), g->gout );

  bool const is_qual = c_tid_is_any( ast->type.stids, TS_ANY_ARRAY_QUALIFIER );
  if ( is_qual )
    FPUTS( c_type_name_c( &ast->type ), g->gout );

  switch ( ast->array.kind ) {
    case C_ARRAY_EMPTY_SIZE:
      break;
    case C_ARRAY_INT_SIZE:
      if ( is_qual )
        FPUTC( ' ', g->gout );
      FPRINTF( g->gout, "%u", ast->array.size_int );
      break;
    case C_ARRAY_NAMED_SIZE:
      if ( is_qual )
        FPUTC( ' ', g->gout );
      FPUTS( ast->array.size_name, g->gout );
      break;
    case C_ARRAY_VLA_STAR:
      FPUTC( '*', g->gout );
      break;
  } // switch

  FPUTS( graph_token_c( "]" ), g->gout );
}

/**
 * Helper function for c_ast_visitor_english() that prints a bit-field width,
 * if any.
 *
 * @param g The g_state to use.
 * @param ast The AST to print the bit-field width of.
 */
static void g_print_ast_bit_width( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_BIT_FIELD ) );

  if ( ast->bit_field.bit_width > 0 )
    FPRINTF( g->gout, " : %u", ast->bit_field.bit_width );
}

/**
 * Prints a list of AST nodes separated by commas.
 *
 * @param g The g_state to use.
 * @param ast_list The list of AST nodes to print.
 *
 * @sa g_print_ast()
 */
static void g_print_ast_list( g_state_t const *g,
                              c_ast_list_t const *ast_list ) {
  assert( g != NULL );
  assert( ast_list != NULL );

  bool comma = false;
  FOREACH_SLIST_NODE( ast_node, ast_list ) {
    g_state_t node_g;
    g_init( &node_g, g->gib_flags & ~C_GIB_OMIT_TYPE, g->gout );
    fput_sep( ", ", &comma, g->gout );
    g_print_ast( &node_g, c_param_ast( ast_node ) );
  } // for
}

/**
 * Prints either the full or local name of \a ast based on whether we're
 * emitting the gibberish for a `typedef` since it can't have a scoped name.
 *
 * @param g The g_state to use.
 * @param ast The AST to get the name of.
 *
 * @sa g_print_space_ast_name()
 */
static void g_print_ast_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  if ( (g->gib_flags & C_GIB_CAST) != 0 ) {
    //
    // When printing a cast, the cast itself and the AST's name (the thing
    // that's being cast) is printed in g_print_ast(), so we mustn't print it
    // here and print only the type T:
    //
    //      (T)name
    //      static_cast<T>(name)
    //
    return;
  }

  if ( (g->gib_flags & C_GIB_USING) != 0 ) {
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
    // subsequent names, if any.  Hence, reset the flag and print nothing.
    //
    g->printed_space = true;
    return;
  }

  FPUTS(
    //
    // For typedefs, the scope names (if any) were already printed in
    // c_typedef_gibberish() so now we just print the local name.
    //
    (g->gib_flags & C_GIB_TYPEDEF) != 0 ?
      c_sname_local_name( &ast->sname ) : c_sname_full_name( &ast->sname ),
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
 * @param g The g_state to use.
 * @param ast The AST.
 */
static void g_print_postfix( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( c_ast_is_parent( ast ) );

  c_ast_t const *const parent_ast = ast->parent_ast;

  if ( parent_ast != NULL ) {
    switch ( parent_ast->kind ) {
      case K_ARRAY:
      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_DESTRUCTOR:
      case K_FUNCTION:
      case K_LAMBDA:
      case K_OPERATOR:
      case K_USER_DEF_CONVERSION:
      case K_USER_DEF_LITERAL:
        g_print_postfix( g, parent_ast );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        switch ( ast->kind ) {
          case K_APPLE_BLOCK:
            FPRINTF( g->gout, "(%s", c_oper_token_c( C_OP_CARET ) );
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

            if ( c_tid_is_any( ast->type.atids, TA_ANY_MSC_CALL ) ) {
              //
              // A pointer to a function having a Microsoft calling convention
              // has the convention printed just inside the '(':
              //
              //      void (__stdcall *pf)(int, int)
              //
              c_tid_t const msc_call_atids = ast->type.atids & TA_ANY_MSC_CALL;
              FPRINTF( g->gout, "%s ", c_tid_name_c( msc_call_atids ) );
            }
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

        if ( (ast->kind & K_ANY_POINTER) == 0 )
          FPUTC( ')', g->gout );
        break;

      case K_CLASS_STRUCT_UNION:
      case K_TYPEDEF:
        // nothing to do
        break;                          // LCOV_EXCL_LINE

      case K_BUILTIN:                   // impossible
      case K_CAPTURE:                   // impossible
      case K_CAST:                      // impossible
      case K_ENUM:                      // impossible
      case K_NAME:                      // impossible
      case K_VARIADIC:                  // impossible
        UNEXPECTED_INT_VALUE( parent_ast->kind );

      CASE_K_PLACEHOLDER;
    } // switch
  } else {
    //
    // We've reached the root of the AST that has the name of the thing we're
    // printing the gibberish for.
    //
    if ( ast->kind == K_APPLE_BLOCK ) {
      FPRINTF( g->gout, "(%s", c_oper_token_c( C_OP_CARET ) );
      if ( opt_alt_tokens && !c_sname_empty( &ast->sname ) )
        FPUTC( ' ', g->gout );
    }
    g_print_space_ast_name( g, ast );
    if ( ast->kind == K_APPLE_BLOCK )
      FPUTC( ')', g->gout );
  }

  //
  // We're now unwinding the recursion: print the "postfix" things (size for
  // arrays, parameters for functions) in root-to-leaf order.
  //
  switch ( ast->kind ) {
    case K_ARRAY:
      g_print_ast_array_size( g, ast );
      break;
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      FPUTC( '(', g->gout );
      g_print_ast_list( g, &ast->func.param_ast_list );
      FPUTC( ')', g->gout );
      break;
    case K_DESTRUCTOR:
    case K_USER_DEF_CONVERSION:
      FPUTS( "()", g->gout );
      break;
    case K_BUILTIN:
    case K_CAPTURE:
    case K_CAST:
    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_LAMBDA:                      // handled in g_print_ast()
    case K_NAME:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to do
      break;
    CASE_K_PLACEHOLDER;
  } // switch
}

/**
 * Helper function for g_print_ast() that prints a pointer, pointer-to-member,
 * reference, or rvalue reference, its qualifier, if any, and the name, if any.
 *
 * @param g The g_state to use.
 * @param ast The AST that is one of \ref K_POINTER, \ref K_POINTER_TO_MEMBER,
 * \ref K_REFERENCE, or \ref K_RVALUE_REFERENCE whose qualifier, if any, and
 * name, if any, to print.
 */
static void g_print_qual_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER | K_ANY_REFERENCE ) );

  c_tid_t const qual_stids = ast->type.stids & TS_ANY_QUALIFIER;

  switch ( ast->kind ) {
    case K_POINTER:
      if ( qual_stids != TS_NONE && (g->gib_flags & C_GIB_CAST) == 0 &&
           !c_ast_is_ptr_to_kind_any( ast, K_FUNCTION ) ) {
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

    case K_POINTER_TO_MEMBER: {
      FPRINTF( g->gout,
        "%s::*", c_sname_full_name( &ast->ptr_mbr.class_sname )
      );
      c_ast_t const *const func_ast = c_ast_find_parent_func( ast );
      g->printed_space =
        func_ast == NULL || (func_ast->kind & opt_west_pointer_kinds) == 0;
      break;
    }

    case K_REFERENCE:
      if ( opt_alt_tokens ) {
        g_print_space_once( g );
        FPUTS( "bitand ", g->gout );
      } else {
        FPUTC( '&', g->gout );
      }
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_alt_tokens ) {
        g_print_space_once( g );
        FPUTS( "and ", g->gout );
      } else {
        FPUTS( "&&", g->gout );
      }
      break;

    default:
      /* suppress warning */;
  } // switch

  if ( qual_stids != TS_NONE ) {
    FPUTS( c_tid_name_c( qual_stids ), g->gout );

    if ( (g->gib_flags & (C_GIB_DECL | C_GIB_TYPEDEF)) != 0 &&
         c_ast_find_name( ast, C_VISIT_UP ) != NULL ) {
      //
      // For declarations and typedefs, if there is a qualifier and if a name
      // has yet to be printed, we always need to print a space after the
      // qualifier, e.g.:
      //
      //      char *const p;
      //                 ^
      FPUTC( ' ', g->gout );
      g->printed_space = true;
    }
  }

  g_print_space_ast_name( g, ast );
}

/**
 * Helper function for g_print_ast() that prints a space (if it hasn't printed
 * one before) and an AST node's name, if any; but only if we're printing a
 * declaration (not a cast).
 *
 * @param g The g_state to use.
 * @param ast The AST to print the name of, if any.
 *
 * @sa g_print_ast_name()
 */
static void g_print_space_ast_name( g_state_t *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );

  if ( (g->gib_flags & C_GIB_CAST) != 0 )
    return;                             // for casts, print nothing

  switch ( ast->kind ) {
    case K_CONSTRUCTOR:
      FPUTS( c_sname_full_name( &ast->sname ), g->gout );
      break;
    case K_DESTRUCTOR:
      if ( c_sname_count( &ast->sname ) > 1 )
        FPRINTF( g->gout, "%s::", c_sname_scope_name( &ast->sname ) );
      if ( opt_alt_tokens )
        FPUTS( "compl ", g->gout );
      else
        FPUTC( '~', g->gout );
      FPUTS( c_sname_local_name( &ast->sname ), g->gout );
      break;
    case K_OPERATOR: {
      g_print_space_once( g );
      if ( !c_sname_empty( &ast->sname ) )
        FPRINTF( g->gout, "%s::", c_sname_full_name( &ast->sname ) );
      char const *const token = c_oper_token_c( ast->oper.operator->oper_id );
      FPRINTF( g->gout, "operator%s%s", isalpha( token[0] ) ? " " : "", token );
      break;
    }
    case K_USER_DEF_CONVERSION:
      // Do nothing since these don't have names.
      break;
    case K_USER_DEF_LITERAL:
      g_print_space_once( g );
      if ( c_sname_count( &ast->sname ) > 1 )
        FPRINTF( g->gout, "%s::", c_sname_scope_name( &ast->sname ) );
      FPRINTF( g->gout, "operator\"\" %s", c_sname_local_name( &ast->sname ) );
      break;
    default:
      if ( !c_sname_empty( &ast->sname ) ) {
        if ( (g->gib_flags & C_GIB_USING) == 0 )
          g_print_space_once( g );
        g_print_ast_name( g, ast );
      }
      break;
  } // switch
}

/**
 * Determine whether we should print a space before the `*`, `&`, or `&&` in a
 * declaration.  By default, for all kinds _except_ function-like ASTs, we want
 * the output to be like:
 *
 *      type *var
 *
 * i.e., the `*`, `&`, or `&&` adjacent to the variable ("east"); for function-
 * like ASTs, when there's no name for a parameter, or when we're casting, we
 * want the output to be like:
 *
 *      type* func()            // function
 *      type* (^block)()        // block
 *      func(type*)             // nameless function parameter
 *      (type*)expr             // cast
 *
 * i.e., the `*`, `&`, or `&&` adjacent to the type ("west").
 *
 * However, as an exception, if we're declaring more than one pointer to
 * function returning a pointer or reference in the same declaration, then keep
 * the `*`, `&`, or `&&` adjacent to the function like:
 *
 *      type &(*f)(), &(*g)()
 *
 * not:
 *
 *      type& (*f)(), &(*g)()
 *
 * because the latter looks inconsistent (even though it's correct).
 *
 * @param g The g_state to use.
 * @param ast The current AST node.
 * @return Returns `true` only if we should print a space after type type.
 */
NODISCARD
static bool g_space_before_ptr_ref( g_state_t const *g, c_ast_t const *ast ) {
  assert( g != NULL );
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER | K_ANY_REFERENCE ) );

  if ( (g->gib_flags & (C_GIB_CAST | C_GIB_USING)) != 0 )
    return false;
  if ( (g->gib_flags & C_GIB_MULTI_DECL) != 0 )
    return true;

  c_ast_t const *const func_ast = c_ast_find_parent_func( ast );
  if ( func_ast != NULL )               // function returning pointer to ...
    return (func_ast->kind & opt_west_pointer_kinds) == 0;

  if ( c_ast_find_name( ast, C_VISIT_UP ) == NULL )
    return false;

  c_ast_t const *const to_ast = c_ast_find_kind_any(
    ast->ptr_ref.to_ast, C_VISIT_DOWN, opt_west_pointer_kinds
  );
  if ( to_ast != NULL )
    return false;

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

char const* alt_token_c( char const *token ) {
  assert( token != NULL );

  if ( opt_alt_tokens ) {
    switch ( token[0] ) {
      case '!': switch ( token[1] ) {
                  case '=': return L_not_eq;
                  default : return L_not;
                }
      case '&': switch ( token[1] ) {
                  case '&': return L_and;
                  case '=': return L_and_eq;
                  default : return L_bitand;
                } // switch
      case '|': switch ( token[1] ) {
                  case '|': return L_or;
                  case '=': return L_or_eq;
                  default : return L_bitor;
                } // switch
      case '~': return L_compl;
      case '^': switch ( token[1] ) {
                  case '=': return L_xor_eq;
                  default : return L_xor;
                } // switch
    } // switch
  }

  return token;
}

void c_ast_gibberish( c_ast_t const *ast, unsigned gib_flags, FILE *gout ) {
  assert( ast != NULL );
  assert( is_1_bit_in_set( gib_flags, C_GIB_DECL | C_GIB_CAST ) );
  assert( gout != NULL );

  if ( c_ast_print_as_using( ast ) ) {
    //
    // This is when declaring types in C++11 or later when opt_using is set:
    //
    //      c++decl> declare pint as type pointer to int
    //      using pint = int*;
    //
    // It's simpler just to create a temporary c_typedef_t and call
    // c_typedef_gibberish().
    //
    c_typedef_t const tdef = C_TYPEDEF_AST_LIT( ast );
    c_typedef_gibberish( &tdef, C_GIB_USING, gout );
  }
  else {
    if ( (gib_flags & C_GIB_OMIT_TYPE) == 0 ) {
      //
      // If we're declaring more than one variable in the same declaration,
      // print the alignment, if any, only when also printing the type for the
      // first variable.  For example, for:
      //
      //      _Alignas(64) int i, j;
      //
      // print the alignment (and "int") only for "i" and not again for "j".
      //
      switch ( ast->align.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_BYTES:
          FPRINTF( gout, "%s(%u) ", alignas_name(), ast->align.bytes );
          break;
        case C_ALIGNAS_TYPE:
          FPRINTF( gout, "%s(", alignas_name() );
          c_ast_gibberish( ast->align.type_ast, C_GIB_DECL, gout );
          FPUTS( ") ", gout );
          break;
      } // switch
    }

    g_state_t g;
    g_init( &g, gib_flags, gout );
    g_print_ast( &g, ast );
  }

  if ( (gib_flags & C_GIB_FINAL_SEMI) != 0 )
    FPUTC( ';', gout );
}

char const* c_cast_gibberish( c_cast_kind_t kind ) {
  switch ( kind ) {
    case C_CAST_C:
      break;                            // LCOV_EXCL_LINE
    case C_CAST_CONST       : return L_const_cast;
    case C_CAST_DYNAMIC     : return L_dynamic_cast;
    case C_CAST_REINTERPRET : return L_reinterpret_cast;
    case C_CAST_STATIC      : return L_static_cast;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

void c_typedef_gibberish( c_typedef_t const *tdef, unsigned gib_flags,
                          FILE *gout ) {
  assert( tdef != NULL );
  assert( is_1_bit_in_set( gib_flags, C_GIB_TYPEDEF | C_GIB_USING ) );
  assert(
    is_1n_bit_only_in_set(
      gib_flags, C_GIB_TYPEDEF | C_GIB_USING | C_GIB_FINAL_SEMI
    )
  );
  assert( gout != NULL );

  size_t scope_close_braces_to_print = 0;
  c_type_t scope_type = T_NONE;

  c_sname_t const *sname = c_ast_find_name( tdef->ast, C_VISIT_DOWN );
  if ( sname != NULL && c_sname_count( sname ) > 1 ) {
    scope_type = *c_sname_first_type( sname );
    //
    // A type name can't be scoped in a typedef declaration, e.g.:
    //
    //      typedef int S::T::I;        // illegal
    //
    // so we have to wrap it in a scoped declaration, one of: class, namespace,
    // struct, or union.
    //
    if ( scope_type.btids != TB_NAMESPACE ||
         opt_lang_is_any( LANG_NESTED_namespace | LANG_C_ANY ) ) {
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
      // namespace form even though C doesn't have any namespaces because we
      // might be being asked to print all types.
      //
      c_sname_t temp_sname;
      if ( c_tid_is_any( scope_type.stids, TS_INLINE ) ) {
        //
        // For an inline namespace, the "inline" is printed like:
        //
        //      inline namespace NS { // ...
        //
        // as opposed to:
        //
        //      namespace inline NS { // ...
        //
        // so we have to turn off TS_INLINE on the sname's scope type.
        //
        temp_sname = c_sname_dup( sname );
        c_scope_data( temp_sname.head )->type.stids &= c_tid_compl( TS_INLINE );
        sname = &temp_sname;
      }
      else {
        c_sname_init( &temp_sname );    // for unconditional c_sname_cleanup()
        //
        // For all other cases (non-inline namespaces, enum, class, struct, and
        // union), the type is the scope's type, not the first type used above.
        // For example, in:
        //
        //      struct S::T { typedef int I; }
        //                ^
        //
        // it's the T that's the struct; what S is doesn't matter, so we reset
        // scope_type to be the actual scope's type of S::T::I which is T.
        //
        scope_type = *c_sname_scope_type( sname );
        if ( scope_type.btids == TB_SCOPE )
          scope_type.btids = TB_NAMESPACE;

        //
        // Starting in C++20, non-inline namespace may still have nested inline
        // namespaces and they're printed like:
        //
        //      namespace A::inline B { // ...
        //
        // so we turn off "inline" on the scope's type so "inline" isn't
        // printed before "namespace" as well.
        //
        scope_type.stids &= c_tid_compl( TS_INLINE );
      }

      FPRINTF( gout,
        "%s %s %s ",
        c_type_name_c( &scope_type ), c_sname_scope_name( sname ),
        graph_token_c( "{" )
      );
      c_sname_cleanup( &temp_sname );
      scope_close_braces_to_print = 1;
    }
    else {
      //
      // Namespaces in C++14 and earlier require distinct declarations:
      //
      //      namespace S { namespace T { typedef int I; } }
      //
      FOREACH_SNAME_SCOPE_UNTIL( scope, sname, sname->tail ) {
        scope_type = c_scope_data( scope )->type;
        FPRINTF( gout,
          "%s %s %s ",
          c_type_name_c( &scope_type ), c_scope_data( scope )->name,
          graph_token_c( "{" )
        );
      } // for
      scope_close_braces_to_print = c_sname_count( sname ) - 1;
    }
  }

  bool const is_ecsu = (tdef->ast->kind & K_ANY_ECSU) != 0;

  //
  // When printing a type, all types except enum, class, struct, or union
  // (ECSU) types must be preceded by "typedef", e.g.:
  //
  //      typedef int int32_t;
  //
  // However, ECSU types are preceded by "typedef" only when the type was
  // declared in C since those types in C without a typedef are merely in the
  // tags namespace and not first-class types:
  //
  //      struct S;                     // In C, tag only -- not a type.
  //      typedef struct S S;           // Now it's a type.
  //
  // In C++, such typedefs are unnecessary since such types are first-class
  // types and not just tags, so we don't print "typedef".
  //
  bool const print_typedef = (gib_flags & C_GIB_TYPEDEF) != 0 &&
    (!is_ecsu || c_lang_is_c( tdef->lang_ids ) ||
    (OPT_LANG_IS( C_ANY ) && !c_lang_is_cpp( tdef->lang_ids )));

  //
  // When printing a "using", we don't have to check languages since "using" is
  // available only in C++.
  //
  bool const print_using = (gib_flags & C_GIB_USING) != 0 && !is_ecsu;

  if ( print_typedef ) {
    FPUTS( "typedef ", gout );
  }
  else if ( print_using ) {
    FPRINTF( gout, "using %s ", c_sname_local_name( sname ) );
    if ( tdef->ast->type.atids != TA_NONE )
      FPRINTF( gout, "%s ", c_tid_name_c( tdef->ast->type.atids ) );
    FPUTS( "= ", gout );
  }

  g_state_t g;
  g_init( &g, print_using ? C_GIB_USING : C_GIB_TYPEDEF, gout );
  g.printed_typedef = print_typedef;
  g_print_ast( &g, tdef->ast );

  if ( scope_close_braces_to_print > 0 ) {
    FPUTC( ';', gout );
    while ( scope_close_braces_to_print-- > 0 )
      FPRINTF( gout, " %s", graph_token_c( "}" ) );
  }

  if ( (gib_flags & C_GIB_FINAL_SEMI) != 0 && scope_type.btids != TB_NAMESPACE )
    FPUTC( ';', gout );
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
        if ( OPT_LANG_IS( DIGRAPHS ) ) {
          switch ( token[0] ) {
            case '#'  : return token[1] == '#' ? "%:%:" : "%:";
            case '['  : switch ( token[1] ) {
                          case '[': return "<:<:";
                          case ']': return "<::>";
                          default : return "<:";
                        } // switch
            case ']'  : return token[1] == ']' ? ":>:>" : ":>";
            case '{'  : return "<%";
            case '}'  : return "%>";
          } // switch
        }
        break;
      case C_GRAPH_TRI:
        if ( OPT_LANG_IS( TRIGRAPHS ) ) {
          switch ( token[0] ) {
            case '#'  : return "?\?=";
            case '['  : switch ( token[1] ) {
                          case '[': return "?\?(?\?(";
                          case ']': return "?\?(?\?)";
                          default : return "?\?(";
                        } // switch
            case ']'  : return token[1] == ']' ? "?\?)?\?)" : "?\?)";
            case '\\' : return "?\?/";
            case '^'  : return token[1] == '=' ? "?\?'=" : "?\?'";
            case '{'  : return "?\?<";
            case '}'  : return "?\?>";
            case '|'  : switch ( token[1] ) {
                          case '=': return "?\?!=";
                          case '|': return "?\?!?\?!";
                          default : return "?\?!";
                        } // switch
            case '~'  : return "?\?-";
          } // switch
        }
        break;
    } // switch
  }

  return token;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
