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
#include "decl_flags.h"
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */
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
struct gib_state {
  unsigned  gib_flags;                  ///< Gibberish printing flags.
  FILE     *gout;                       ///< Where to print the gibberish.
  bool      postfix;                    ///< Doing postfix gibberish?
  bool      printed_space;              ///< Printed a space yet?
  bool      printed_typedef;            ///< Printed `typedef`?
};
typedef struct gib_state gib_state_t;

// local functions
static void c_ast_list_gibberish( c_ast_list_t const*, gib_state_t const* );
static void c_ast_name_gibberish( c_ast_t const*, gib_state_t* );
static void c_ast_postfix_gibberish( c_ast_t const*, gib_state_t* );
static void c_ast_qual_name_gibberish( c_ast_t const*, gib_state_t* );
static void c_ast_space_name_gibberish( c_ast_t const*, gib_state_t* );
static void gib_init( gib_state_t*, unsigned, FILE* );

NODISCARD
static bool c_ast_space_before_ptr_ref( c_ast_t const*, gib_state_t const* );

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
 * @param gib The gib_state to use.
 */
static inline void gib_print_space_once( gib_state_t *gib ) {
  if ( false_set( &gib->printed_space ) )
    FPUTC( ' ', gib->gout );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_ast_gibberish_impl() that prints an array's size.
 *
 * @param ast The AST that is a \ref K_ARRAY whose size to print.
 * @param gib The gib_state to use.
 */
static void c_ast_array_size_gibberish( c_ast_t const *ast,
                                        gib_state_t const *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );
  assert( gib != NULL );

  FPUTS( graph_token_c( "[" ), gib->gout );

  bool const is_qual = c_tid_is_any( ast->type.stids, TS_ANY_ARRAY_QUALIFIER );
  if ( is_qual )
    FPUTS( c_type_name_c( &ast->type ), gib->gout );

  switch ( ast->array.kind ) {
    case C_ARRAY_EMPTY_SIZE:
      break;
    case C_ARRAY_INT_SIZE:
      FPRINTF( gib->gout, "%s%u", is_qual ? " " : "", ast->array.size_int );
      break;
    case C_ARRAY_NAMED_SIZE:
      FPRINTF( gib->gout, "%s%s", is_qual ? " " : "", ast->array.size_name );
      break;
    case C_ARRAY_VLA_STAR:
      FPUTC( '*', gib->gout );
      break;
  } // switch

  FPUTS( graph_token_c( "]" ), gib->gout );
}

/**
 * Helper function for c_ast_gibberish_impl() that prints a bit-field width,
 * if any.
 *
 * @param ast The AST to print the bit-field width of.
 * @param gib The gib_state to use.
 */
static void c_ast_bit_width_gibberish( c_ast_t const *ast,
                                       gib_state_t const *gib ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_BIT_FIELD ) );
  assert( gib != NULL );

  if ( ast->bit_field.bit_width > 0 )
    FPRINTF( gib->gout, " : %u", ast->bit_field.bit_width );
}

/**
 * Prints \a ast as gibberish, aka, a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param gib The gib_state to use.
 */
static void c_ast_gibberish_impl( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( gib != NULL );

  gib_state_t child_gib;
  c_type_t type = ast->type;

  if ( (gib->gib_flags & C_GIB_USING) != 0 ) {
    //
    // If we're printing a "using" declaration, don't print either "typedef" or
    // attributes since they will have been printed in c_typedef_gibberish().
    //
    type.stids &= c_tid_compl( TS_typedef );
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
    case K_UDEF_CONV:
      //
      // Since none of these have a return type, no space needs to be printed
      // before the name, so lie and set the "space" flag.
      //
      gib->printed_space = true;
      FALLTHROUGH;

    case K_FUNCTION:
    case K_OPERATOR:
    case K_UDEF_LIT:
      //
      // These things aren't printed as part of the type beforehand, so strip
      // them out of the type here, but print them after the parameters.
      //
      cv_qual_stids   = (type.stids & TS_ANY_QUALIFIER);
      is_default      = (type.stids & TS_default) != TS_NONE;
      is_delete       = (type.stids & TS_delete) != TS_NONE;
      is_final        = (type.stids & TS_final) != TS_NONE;
      is_noexcept     = (type.stids & TS_noexcept) != TS_NONE;
      is_pure_virtual = (type.stids & TS_PURE_virtual) != TS_NONE;
      is_throw        = (type.stids & TS_throw) != TS_NONE;
      ref_qual_stids  = (type.stids & TS_ANY_REFERENCE);

      // In C++, "override" should be printed only if "final" isn't.
      is_override     = !is_final && (type.stids & TS_override) != TS_NONE;

      type.stids &= c_tid_compl(
                       TS_ANY_QUALIFIER
                     | TS_ANY_REFERENCE
                     | TS_default
                     | TS_delete
                     | TS_final
                     | TS_noexcept
                     | TS_override
                     | TS_PURE_virtual
                     | TS_throw
                     // In C++, if either "override" or "final" is printed,
                     // "virtual" shouldn't be.
                     | (is_override || is_final ? TS_virtual : TS_NONE)
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
        fputs_sp( c_type_name_c( &type ), gib->gout );
      }
      if ( ast->kind == K_UDEF_CONV ) {
        if ( !c_sname_empty( &ast->sname ) )
          FPRINTF( gib->gout, "%s::", c_sname_full_name( &ast->sname ) );
        FPUTS( "operator ", gib->gout );
      }
      if ( ast->parent.of_ast != NULL ) {
        is_trailing_ret = (ast->kind & K_ANY_TRAILING_RETURN) != 0 &&
          opt_trailing_ret && OPT_LANG_IS( TRAILING_RETURN_TYPES );
        if ( is_trailing_ret )
          FPUTS( L_auto, gib->gout );
        else
          c_ast_gibberish_impl( ast->parent.of_ast, gib );
      }
      if ( msc_call_atids != TA_NONE &&
           !c_ast_parent_is_kind( ast, K_POINTER ) ) {
        //
        // If ast is a function having a Microsoft calling convention, but not
        // a pointer to such a function, print the calling convention.
        // (Pointers to such functions are handled in
        // c_ast_postfix_gibberish().)
        //
        FPRINTF( gib->gout, " %s", c_tid_name_c( msc_call_atids ) );
      }
      if ( false_set( &gib->postfix ) ) {
        if ( (gib->gib_flags & (C_GIB_PRINT_CAST | C_GIB_USING)) == 0 )
          gib_print_space_once( gib );
        c_ast_postfix_gibberish( ast, gib );
      }
      if ( cv_qual_stids != TS_NONE )
        FPRINTF( gib->gout, " %s", c_tid_name_c( cv_qual_stids ) );
      if ( ref_qual_stids != TS_NONE ) {
        FPRINTF( gib->gout, " %s",
          alt_token_c(
            c_tid_is_any( ref_qual_stids, TS_REFERENCE ) ? "&" : "&&"
          )
        );
      }
      if ( is_noexcept )
        FPUTS( " noexcept", gib->gout );
      else if ( is_throw )
        FPUTS( " throw()", gib->gout );
      if ( is_override )
        FPUTS( " override", gib->gout );
      else if ( is_final )
        FPUTS( " final", gib->gout );
      if ( is_trailing_ret ) {
        FPUTS( " -> ", gib->gout );
        //
        // Temporarily orphan the return type's AST in order to print it as a
        // stand-alone trailing type.
        //
        c_ast_t *const ret_ast = ast->func.ret_ast;
        c_ast_t *const orig_ret_ast_parent_ast = ret_ast->parent_ast;
        ret_ast->parent_ast = NULL;

        gib_init( &child_gib, C_GIB_PRINT_DECL, gib->gout );
        c_ast_gibberish_impl( ret_ast, &child_gib );
        ret_ast->parent_ast = orig_ret_ast_parent_ast;
      }
      if ( is_pure_virtual )
        FPUTS( " = 0", gib->gout );
      else if ( is_default )
        FPUTS( " = default", gib->gout );
      else if ( is_delete )
        FPUTS( " = delete", gib->gout );
      break;

    case K_BUILTIN:
      if ( (gib->gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 )
        FPUTS( c_type_name_c( &type ), gib->gout );
      if ( c_ast_is_tid_any( ast, TB__BitInt ) )
        FPRINTF( gib->gout, "(%u)", ast->builtin.BitInt.width );
      c_ast_space_name_gibberish( ast, gib );
      c_ast_bit_width_gibberish( ast, gib );
      break;

    case K_CAPTURE:
      switch ( ast->capture.kind ) {
        case C_CAPTURE_COPY:
          FPUTC( '=', gib->gout );
          break;
        case C_CAPTURE_REFERENCE:
          FPUTS( alt_token_c( "&" ), gib->gout );
          if ( c_sname_empty( &ast->sname ) )
            break;
          if ( opt_alt_tokens )
            FPUTC( ' ', gib->gout );
          FALLTHROUGH;
        case C_CAPTURE_VARIABLE:
          FPUTS( c_sname_full_name( &ast->sname ), gib->gout );
          break;
        case C_CAPTURE_STAR_THIS:
          FPUTC( '*', gib->gout );
          FALLTHROUGH;
        case C_CAPTURE_THIS:
          FPUTS( L_this, gib->gout );
          break;
      } // switch
      break;

    case K_CAST:
      assert( gib->gib_flags == C_GIB_PRINT_CAST );
      gib_init( &child_gib, C_GIB_PRINT_CAST, gib->gout );
      if ( ast->cast.kind == C_CAST_C ) {
        FPUTC( '(', gib->gout );
        c_ast_gibberish_impl( ast->cast.to_ast, &child_gib );
        FPRINTF( gib->gout, ")%s\n", c_sname_full_name( &ast->sname ) );
      } else {
        FPRINTF( gib->gout, "%s<", c_cast_gibberish( ast->cast.kind ) );
        c_ast_gibberish_impl( ast->cast.to_ast, &child_gib );
        FPRINTF( gib->gout, ">(%s)\n", c_sname_full_name( &ast->sname ) );
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
      type.btids &= c_tid_compl( TB_struct | TB_class );
      FALLTHROUGH;

    case K_CLASS_STRUCT_UNION: {
      if ( opt_east_const ) {
        cv_qual_stids = type.stids & TS_CV;
        type.stids &= c_tid_compl( TS_CV );
      }

      char const *const type_name =
        (gib->gib_flags & (C_GIB_PRINT_CAST | C_GIB_PRINT_DECL)) != 0 &&
        //
        // Special case: a fixed type enum must always have "enum" printed, so
        // we don't call c_type_name_ecsu() that may omit it by applying
        // opt_explicit_ecsu_btids.
        //
        !is_fixed_enum ?
          c_type_name_ecsu( &type ) :
          c_type_name_c( &type );

      FPUTS( type_name, gib->gout );

      if ( (gib->gib_flags & C_GIB_TYPEDEF) == 0 || gib->printed_typedef ) {
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
        FPRINTF( gib->gout,
          "%s%s",
          type_name[0] != '\0' ? " " : "",
          c_sname_full_name( &ast->csu.csu_sname )
        );
      }

      bool printed_name = false;

      if ( is_fixed_enum ) {
        if ( (gib->gib_flags & C_GIB_TYPEDEF) != 0 ) {
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
          c_ast_space_name_gibberish( ast, gib );
          printed_name = true;
        }
        FPUTS( " : ", gib->gout );
        c_ast_gibberish_impl( ast->enum_.of_ast, gib );
      }

      if ( cv_qual_stids != TS_NONE )
        FPRINTF( gib->gout, " %s", c_tid_name_c( cv_qual_stids ) );

      if ( !printed_name )
        c_ast_space_name_gibberish( ast, gib );

      if ( ast->kind == K_ENUM )
        c_ast_bit_width_gibberish( ast, gib );
      break;
    }

    case K_LAMBDA:
      FPUTS( graph_token_c( "[" ), gib->gout );
      c_ast_list_gibberish( &ast->lambda.capture_ast_list, gib );
      FPUTS( graph_token_c( "]" ), gib->gout );
      if ( c_ast_params_count( ast ) > 0 ) {
        FPUTC( '(', gib->gout );
        c_ast_list_gibberish( &ast->lambda.param_ast_list, gib );
        FPUTC( ')', gib->gout );
      }
      if ( !c_tid_is_none( ast->type.stids ) )
        FPRINTF( gib->gout, " %s", c_tid_name_c( ast->type.stids ) );
      if ( !c_tid_is_none( ast->type.atids ) )
        FPRINTF( gib->gout, " %s", c_tid_name_c( ast->type.atids ) );
      if ( ast->lambda.ret_ast != NULL &&
           !c_ast_is_builtin_any( ast->lambda.ret_ast, TB_auto | TB_void ) ) {
        FPUTS( " -> ", gib->gout );
        c_ast_gibberish_impl( ast->lambda.ret_ast, gib );
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
        FPUTS( L_int, gib->gout );
      }
      if ( (gib->gib_flags & C_GIB_PRINT_CAST) == 0 ) {
        if ( OPT_LANG_IS( PROTOTYPES ) )
          FPUTC( ' ', gib->gout );
        c_ast_name_gibberish( ast, gib );
      }
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      if ( (gib->gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 )
        fputs_sp( c_tid_name_c( type.stids & TS_ANY_STORAGE ), gib->gout );
      c_ast_gibberish_impl( ast->ptr_ref.to_ast, gib );
      if ( c_ast_space_before_ptr_ref( ast, gib ) )
        gib_print_space_once( gib );
      if ( !gib->postfix )
        c_ast_qual_name_gibberish( ast, gib );
      break;

    case K_POINTER_TO_MEMBER:
      c_ast_gibberish_impl( ast->ptr_mbr.to_ast, gib );
      gib_print_space_once( gib );
      if ( !gib->postfix )
        c_ast_qual_name_gibberish( ast, gib );
      break;

    case K_TYPEDEF:
      if ( (gib->gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 ) {
        //
        // Of course a K_TYPEDEF AST also has a type comprising TB_typedef, but
        // we need to see whether there's any more to the type, e.g., "const".
        //
        bool const is_more_than_plain_typedef =
          !c_type_equiv( &ast->type, &C_TYPE_LIT_B( TB_typedef ) );

        if ( is_more_than_plain_typedef && !opt_east_const )
          FPUTS( c_type_name_c( &ast->type ), gib->gout );

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
          c_tid_is_any( type.stids, TS__Atomic );

        if ( print_parens_for_Atomic )
          FPUTC( '(', gib->gout );
        else if ( is_more_than_plain_typedef && !opt_east_const )
          FPUTC( ' ', gib->gout );

        //
        // Temporarily turn off C_GIB_USING to force printing of the type's
        // name.  This is necessary for when printing the name of a typedef of
        // a typedef as a "using" declaration:
        //
        //      c++decl> typedef int32_t foo_t
        //      c++decl> show foo_t as using
        //      using foo_t = int32_t;
        //
        unsigned const orig_flags = gib->gib_flags;
        gib->gib_flags &= ~C_GIB_USING;
        c_ast_name_gibberish( ast->tdef.for_ast, gib );
        gib->gib_flags = orig_flags;
        if ( print_parens_for_Atomic )
          FPUTC( ')', gib->gout );
        if ( is_more_than_plain_typedef && opt_east_const )
          FPRINTF( gib->gout, " %s", c_type_name_c( &ast->type ) );
      }

      c_ast_space_name_gibberish( ast, gib );
      c_ast_bit_width_gibberish( ast, gib );
      break;

    case K_VARIADIC:
      FPUTS( L_ellipsis, gib->gout );
      break;

    case K_PLACEHOLDER:
      unreachable();
  } // switch
}

/**
 * Prints a list of AST nodes separated by commas.
 *
 * @param ast_list The list of AST nodes to print.
 * @param gib The gib_state to use.
 *
 * @sa c_ast_gibberish_impl()
 */
static void c_ast_list_gibberish( c_ast_list_t const *ast_list,
                                  gib_state_t const *gib ) {
  assert( ast_list != NULL );
  assert( gib != NULL );

  bool comma = false;
  FOREACH_SLIST_NODE( ast_node, ast_list ) {
    gib_state_t node_gib;
    gib_init( &node_gib, gib->gib_flags & ~C_GIB_OPT_OMIT_TYPE, gib->gout );
    fput_sep( ", ", &comma, gib->gout );
    c_ast_gibberish_impl( c_param_ast( ast_node ), &node_gib );
  } // for
}

/**
 * Prints either the full or local name of \a ast based on whether we're
 * emitting the gibberish for a `typedef` since it can't have a scoped name.
 *
 * @param ast The AST to get the name of.
 * @param gib The gib_state to use.
 *
 * @sa c_ast_space_name_gibberish()
 */
static void c_ast_name_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( gib != NULL );

  if ( (gib->gib_flags & C_GIB_PRINT_CAST) != 0 ) {
    //
    // When printing a cast, the cast itself and the AST's name (the thing
    // that's being cast) is printed in c_ast_gibberish_impl(), so we mustn't
    // print it here and print only the type T:
    //
    //      (T)name
    //      static_cast<T>(name)
    //
    return;
  }

  if ( (gib->gib_flags & C_GIB_USING) != 0 ) {
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
    gib->printed_space = true;
    return;
  }

  FPUTS(
    //
    // For typedefs, the scope names (if any) were already printed in
    // c_typedef_gibberish() so now we just print the local name.
    //
    (gib->gib_flags & C_GIB_TYPEDEF) != 0 ?
      c_sname_local_name( &ast->sname ) : c_sname_full_name( &ast->sname ),
    gib->gout
  );
}

/**
 * Helper function for c_ast_gibberish_impl() that handles the printing of
 * "postfix" cases:
 *
 *  + Array of pointer to function, e.g., `void (*a[4])(int)`.
 *  + Pointer to array, e.g., `int (*p)[4]`.
 *  + Reference to array, e.g., `int (&r)[4]`.
 *
 * @param ast The AST.
 * @param gib The gib_state to use.
 */
static void c_ast_postfix_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( gib != NULL );

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
      case K_UDEF_CONV:
      case K_UDEF_LIT:
        c_ast_postfix_gibberish( parent_ast, gib );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        switch ( ast->kind ) {
          case K_APPLE_BLOCK:
            FPRINTF( gib->gout, "(%s", c_oper_token_c( C_OP_CARET ) );
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
            FPUTC( '(', gib->gout );

            if ( c_tid_is_any( ast->type.atids, TA_ANY_MSC_CALL ) ) {
              //
              // A pointer to a function having a Microsoft calling convention
              // has the convention printed just inside the '(':
              //
              //      void (__stdcall *pf)(int, int)
              //
              c_tid_t const msc_call_atids = ast->type.atids & TA_ANY_MSC_CALL;
              FPRINTF( gib->gout, "%s ", c_tid_name_c( msc_call_atids ) );
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

        c_ast_qual_name_gibberish( parent_ast, gib );
        if ( c_ast_is_parent( parent_ast->parent_ast ) )
          c_ast_postfix_gibberish( parent_ast, gib );

        if ( (ast->kind & K_ANY_POINTER) == 0 )
          FPUTC( ')', gib->gout );
        break;

      case K_CLASS_STRUCT_UNION:
      case K_TYPEDEF:
        // nothing to do
        break;                          // LCOV_EXCL_LINE

      case K_BUILTIN:
      case K_CAPTURE:
      case K_CAST:
      case K_ENUM:
      case K_NAME:
      case K_PLACEHOLDER:
      case K_VARIADIC:
        UNEXPECTED_INT_VALUE( parent_ast->kind );
    } // switch
  } else {
    //
    // We've reached the root of the AST that has the name of the thing we're
    // printing the gibberish for.
    //
    if ( ast->kind == K_APPLE_BLOCK ) {
      FPRINTF( gib->gout, "(%s", c_oper_token_c( C_OP_CARET ) );
      if ( opt_alt_tokens && !c_sname_empty( &ast->sname ) )
        FPUTC( ' ', gib->gout );
    }
    c_ast_space_name_gibberish( ast, gib );
    if ( ast->kind == K_APPLE_BLOCK )
      FPUTC( ')', gib->gout );
  }

  //
  // We're now unwinding the recursion: print the "postfix" things (size for
  // arrays, parameters for functions) in root-to-leaf order.
  //
  switch ( ast->kind ) {
    case K_ARRAY:
      c_ast_array_size_gibberish( ast, gib );
      break;
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_UDEF_LIT:
      FPUTC( '(', gib->gout );
      c_ast_list_gibberish( &ast->func.param_ast_list, gib );
      FPUTC( ')', gib->gout );
      break;
    case K_DESTRUCTOR:
    case K_UDEF_CONV:
      FPUTS( "()", gib->gout );
      break;
    case K_BUILTIN:
    case K_CAPTURE:
    case K_CAST:
    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_LAMBDA:                      // handled in c_ast_gibberish_impl()
    case K_NAME:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to do
      break;
    case K_PLACEHOLDER:
      unreachable();
  } // switch
}

/**
 * Helper function for c_ast_gibberish_impl() that prints a pointer, pointer-
 * to-member, reference, or rvalue reference, its qualifier, if any, and the
 * name, if any.
 *
 * @param ast The AST that is one of \ref K_POINTER, \ref K_POINTER_TO_MEMBER,
 * \ref K_REFERENCE, or \ref K_RVALUE_REFERENCE whose qualifier, if any, and
 * name, if any, to print.
 * @param gib The gib_state to use.
 */
static void c_ast_qual_name_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER | K_ANY_REFERENCE ) );
  assert( gib != NULL );

  c_tid_t const qual_stids = ast->type.stids & TS_ANY_QUALIFIER;

  switch ( ast->kind ) {
    case K_POINTER:
      if ( qual_stids != TS_NONE && (gib->gib_flags & C_GIB_PRINT_CAST) == 0 &&
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
        gib_print_space_once( gib );
      }
      FPUTC( '*', gib->gout );
      break;

    case K_POINTER_TO_MEMBER: {
      FPRINTF( gib->gout,
        "%s::*", c_sname_full_name( &ast->ptr_mbr.class_sname )
      );
      c_ast_t const *const func_ast = c_ast_find_parent_func( ast );
      gib->printed_space =
        func_ast == NULL || (func_ast->kind & opt_west_pointer_kinds) == 0;
      break;
    }

    case K_REFERENCE:
      if ( opt_alt_tokens ) {
        gib_print_space_once( gib );
        FPUTS( "bitand ", gib->gout );
      } else {
        FPUTC( '&', gib->gout );
      }
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_alt_tokens ) {
        gib_print_space_once( gib );
        FPUTS( "and ", gib->gout );
      } else {
        FPUTS( "&&", gib->gout );
      }
      break;

    default:
      /* suppress warning */;
  } // switch

  if ( qual_stids != TS_NONE ) {
    FPUTS( c_tid_name_c( qual_stids ), gib->gout );

    if ( (gib->gib_flags & (C_GIB_PRINT_DECL | C_GIB_TYPEDEF)) != 0 &&
         c_ast_find_name( ast, C_VISIT_UP ) != NULL ) {
      //
      // For declarations and typedefs, if there is a qualifier and if a name
      // has yet to be printed, we always need to print a space after the
      // qualifier, e.g.:
      //
      //      char *const p;
      //                 ^
      FPUTC( ' ', gib->gout );
      gib->printed_space = true;
    }
  }

  c_ast_space_name_gibberish( ast, gib );
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
 * @param ast The current AST node.
 * @param gib The gib_state to use.
 * @return Returns `true` only if we should print a space after type type.
 */
NODISCARD
static bool c_ast_space_before_ptr_ref( c_ast_t const *ast,
                                        gib_state_t const *gib ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER | K_ANY_REFERENCE ) );
  assert( gib != NULL );

  if ( (gib->gib_flags & (C_GIB_PRINT_CAST | C_GIB_USING)) != 0 )
    return false;
  if ( (gib->gib_flags & C_GIB_OPT_MULTI_DECL) != 0 )
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

/**
 * Helper function for c_ast_gibberish_impl() that prints a space (if it hasn't
 * printed one before) and an AST node's name, if any; but only if we're
 * printing a declaration (not a cast).
 *
 * @param ast The AST to print the name of, if any.
 * @param gib The gib_state to use.
 *
 * @sa c_ast_name_gibberish()
 */
static void c_ast_space_name_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( gib != NULL );

  if ( (gib->gib_flags & C_GIB_PRINT_CAST) != 0 )
    return;                             // for casts, print nothing

  switch ( ast->kind ) {
    case K_CONSTRUCTOR:
      FPUTS( c_sname_full_name( &ast->sname ), gib->gout );
      break;
    case K_DESTRUCTOR:
      if ( c_sname_count( &ast->sname ) > 1 )
        FPRINTF( gib->gout, "%s::", c_sname_scope_name( &ast->sname ) );
      if ( opt_alt_tokens )
        FPUTS( "compl ", gib->gout );
      else
        FPUTC( '~', gib->gout );
      FPUTS( c_sname_local_name( &ast->sname ), gib->gout );
      break;
    case K_OPERATOR: {
      gib_print_space_once( gib );
      if ( !c_sname_empty( &ast->sname ) )
        FPRINTF( gib->gout, "%s::", c_sname_full_name( &ast->sname ) );
      char const *const token = c_oper_token_c( ast->oper.operator->oper_id );
      FPRINTF( gib->gout,
        "operator%s%s", isalpha( token[0] ) ? " " : "", token
      );
      break;
    }
    case K_UDEF_CONV:
      // Do nothing since these don't have names.
      break;
    case K_UDEF_LIT:
      gib_print_space_once( gib );
      if ( c_sname_count( &ast->sname ) > 1 )
        FPRINTF( gib->gout, "%s::", c_sname_scope_name( &ast->sname ) );
      FPRINTF( gib->gout,
        "operator\"\" %s", c_sname_local_name( &ast->sname )
      );
      break;
    default:
      if ( !c_sname_empty( &ast->sname ) ) {
        if ( (gib->gib_flags & C_GIB_USING) == 0 )
          gib_print_space_once( gib );
        c_ast_name_gibberish( ast, gib );
      }
      break;
  } // switch
}

/**
 * Initializes a gib_state.
 *
 * @param gib The gib_state to initialize.
 * @param gib_flags The gibberish flags to use.
 * @param gout The `FILE` to print to.
 */
static void gib_init( gib_state_t *gib, unsigned gib_flags, FILE *gout ) {
  assert( gib != NULL );
  assert( is_1n_bit_only_in_set( gib_flags, C_GIB_ANY ) );
  assert( gout != NULL );

  MEM_ZERO( gib );
  gib->gib_flags = gib_flags;
  gib->gout = gout;
  gib->printed_space = (gib_flags & C_GIB_OPT_OMIT_TYPE) != 0;
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
  assert( is_1n_bit_only_in_set( gib_flags, C_GIB_ANY ) );
  assert(
    is_1_bit_in_set(
      gib_flags, C_GIB_DECL_ANY | C_GIB_PRINT_DECL | C_GIB_PRINT_CAST
    )
  );
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
    c_typedef_t const tdef = C_TYPEDEF_LIT( ast, C_GIB_USING );
    c_typedef_gibberish( &tdef, C_GIB_USING, gout );
  }
  else {
    if ( (gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 ) {
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
          c_ast_gibberish( ast->align.type_ast, C_GIB_PRINT_DECL, gout );
          FPUTS( ") ", gout );
          break;
      } // switch
    }

    gib_state_t gib;
    gib_init( &gib, gib_flags, gout );
    c_ast_gibberish_impl( ast, &gib );
  }

  if ( (gib_flags & C_GIB_OPT_SEMICOLON) != 0 )
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
  assert( is_1_bit_in_set( gib_flags, C_GIB_DECL_ANY ) );
  assert(
    is_1n_bit_only_in_set( gib_flags, C_GIB_DECL_ANY | C_GIB_OPT_SEMICOLON )
  );
  assert( gout != NULL );

  size_t scope_close_braces_to_print = 0;
  c_type_t scope_type = T_NONE;

  c_sname_t temp_sname;
  c_sname_init( &temp_sname );          // for unconditional c_sname_cleanup()

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
    if ( scope_type.btids != TB_namespace ||
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
      if ( c_tid_is_any( scope_type.stids, TS_inline ) ) {
        //
        // For an inline namespace, the "inline" is printed like:
        //
        //      inline namespace NS { // ...
        //
        // as opposed to:
        //
        //      namespace inline NS { // ...
        //
        // so we have to turn off TS_inline on the sname's scope type.
        //
        temp_sname = c_sname_dup( sname );
        c_scope_data( temp_sname.head )->type.stids &= c_tid_compl( TS_inline );
        sname = &temp_sname;
      }
      else {
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
          scope_type.btids = TB_namespace;

        //
        // Starting in C++20, non-inline namespace may still have nested inline
        // namespaces and they're printed like:
        //
        //      namespace A::inline B { // ...
        //
        // so we turn off "inline" on the scope's type so "inline" isn't
        // printed before "namespace" as well.
        //
        scope_type.stids &= c_tid_compl( TS_inline );
      }

      FPRINTF( gout,
        "%s %s %s ",
        c_type_name_c( &scope_type ), c_sname_scope_name( sname ),
        graph_token_c( "{" )
      );
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
    (OPT_LANG_IS( C_ANY ) && !c_lang_is_cpp( tdef->lang_ids ))) &&
    c_ast_find_type_any( CONST_CAST( c_ast_t*, tdef->ast ), C_VISIT_DOWN, &T_TS_typedef ) == NULL;

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

  c_sname_cleanup( &temp_sname );

  gib_state_t gib;
  gib_init( &gib, print_using ? C_GIB_USING : C_GIB_TYPEDEF, gout );
  gib.printed_typedef = print_typedef;
  c_ast_gibberish_impl( tdef->ast, &gib );

  if ( scope_close_braces_to_print > 0 ) {
    FPUTC( ';', gout );
    while ( scope_close_braces_to_print-- > 0 )
      FPRINTF( gout, " %s", graph_token_c( "}" ) );
  }

  if ( (gib_flags & C_GIB_OPT_SEMICOLON) != 0 &&
       scope_type.btids != TB_namespace ) {
    FPUTC( ';', gout );
  }
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
