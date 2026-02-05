/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "gibberish.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_kind.h"
#include "c_lang.h"
#include "c_operator.h"
#include "c_sname.h"
#include "c_type.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "strbuf.h"
#include "type_traits.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>                     /* for unreachable(3) */

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
  decl_flags_t  gib_flags;              ///< Gibberish printing flags.
  FILE         *fout;                   ///< Where to print the gibberish.
  bool          is_nested_scope;        ///< Within `{` ... `}`?
  bool          is_postfix;             ///< Doing postfix gibberish?
  bool          printed_space;          ///< Printed a space yet?
  bool          printed_typedef;        ///< Printed `typedef`?
};
typedef struct gib_state gib_state_t;

// local functions
static void c_ast_list_gibberish( c_ast_list_t const*, gib_state_t const* );
static void c_ast_name_gibberish( c_ast_t const*, gib_state_t* );
static void c_ast_postfix_gibberish( c_ast_t const*, gib_state_t* );
static void c_ast_qual_name_gibberish( c_ast_t const*, gib_state_t* );
static void c_ast_space_name_gibberish( c_ast_t const*, gib_state_t* );
static void c_builtin_ast_gibberish( c_ast_t const*, c_type_t const*,
                                     gib_state_t* );
static void c_capture_ast_gibberish( c_ast_t const*, gib_state_t* );
static void c_cast_ast_gibberish( c_ast_t const*, gib_state_t* );
static void c_concept_ast_gibberish( c_ast_t const*, c_type_t*, gib_state_t* );
static void c_ecsu_ast_gibberish( c_ast_t const*, c_type_t*, gib_state_t* );
static void c_lambda_ast_gibberish( c_ast_t const*, c_type_t const*,
                                    gib_state_t* );
static void c_name_ast_gibberish( c_ast_t const*, gib_state_t* );
static void c_ptr_mbr_ast_gibberish( c_ast_t const*, gib_state_t* );
static void c_ptr_ref_ast_gibberish( c_ast_t const*, c_type_t const*,
                                     gib_state_t* );
static void c_struct_bind_ast_gibberish( c_ast_t const*, gib_state_t* );
static void c_typedef_ast_gibberish( c_ast_t const*, c_type_t const*,
                                     gib_state_t* );
static void gib_init( gib_state_t*, decl_flags_t, FILE* );

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
  c_ast_t const *const parent_ast = ast->parent_ast;
  return parent_ast != NULL ?
    c_ast_find_kind_any( parent_ast, C_VISIT_UP, K_ANY_FUNCTION_LIKE ) : NULL;
}

/**
 * Checks whether \a ast or any child AST thereof is one of \ref
 * opt_west_decl_kinds.
 *
 * @param ast The AST node to start checking at.
 * @return Returns `true` only if it is.
 */
NODISCARD
static inline bool c_ast_is_west_decl_kind( c_ast_t const *ast ) {
  return c_ast_find_kind_any( ast, C_VISIT_DOWN, opt_west_decl_kinds ) != NULL;
}

/**
 * Prints a space only if we haven't printed one yet.
 *
 * @param gib The gib_state to use.
 */
static inline void gib_print_space_once( gib_state_t *gib ) {
  if ( false_set( &gib->printed_space ) )
    FPUTC( ' ', gib->fout );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * If \ref opt_alt_tokens is `true`, gets the alternative token of a C++
 * operator \a token.
 *
 * @param token The C++ operator token to get the alternative token for.
 * @return If \ref opt_alt_tokens is `true` and if \a token is a token that has
 * an alternative token, returns said token; otherwise returns \a token as-is.
 *
 * @sa c_op_token_c()
 * @sa graph_token_c()
 * @sa opt_alt_tokens
 */
NODISCARD
static char const* alt_token_c( char const *token ) {
  assert( token != NULL );

  if ( !opt_alt_tokens )
    return token;

  switch ( token[0] ) {
    case '!': switch ( token[1] ) {
                case '=' : return L_not_eq;
                case '\0': return L_not;
              }
              break;
    case '&': switch ( token[1] ) {
                case '&' : return L_and;
                case '=' : return L_and_eq;
                case '\0': return L_bitand;
              } // switch
              break;
    case '|': switch ( token[1] ) {
                case '|' : return L_or;
                case '=' : return L_or_eq;
                case '\0': return L_bitor;
              } // switch
              break;
    case '~': switch ( token[1] ) {
                case '\0': return L_compl;
              } // switch
              break;
    case '^': switch ( token[1] ) {
                case '=' : return L_xor_eq;
                case '\0': return L_xor;
              } // switch
              break;
    default : return token;
  } // switch

  UNEXPECTED_INT_VALUE( token[1] );
}

/**
 * Prints the alignment of \a ast in C/C++.
 *
 * @param ast The AST to print the alignment of.
 * @param fout The `FILE` to print to.
 */
static void c_ast_alignas_gibberish( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( fout != NULL );

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_BYTES:
      FPRINTF( fout, "%s(%u) ", alignas_name(), ast->align.bytes );
      break;
    case C_ALIGNAS_SNAME:
      FPRINTF( fout,
        "%s(%s) ", alignas_name(), c_sname_gibberish( &ast->align.sname )
      );
      break;
    case C_ALIGNAS_TYPE:
      FPRINTF( fout, "%s(", alignas_name() );
      c_ast_gibberish( ast->align.type_ast, C_GIB_PRINT_DECL, fout );
      FPUTS( ") ", fout );
      break;
  } // switch
}

/**
 * Prints an array's size as part of a C/C++ declaration.
 *
 * @param ast The AST that is a #K_ARRAY whose size to print.
 * @param gib The gib_state to use.
 */
static void c_ast_array_size_gibberish( c_ast_t const *ast,
                                        gib_state_t const *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );
  assert( gib != NULL );

  FPUTS( other_token_c( "[" ), gib->fout );

  bool const is_qual = c_tid_is_any( ast->type.stids, TS_ANY_ARRAY_QUALIFIER );
  if ( is_qual )
    FPUTS( c_type_gibberish( &ast->type ), gib->fout );

  switch ( ast->array.kind ) {
    case C_ARRAY_SIZE_NONE:
      break;
    case C_ARRAY_SIZE_INT:
      FPRINTF( gib->fout, "%s%u", is_qual ? " " : "", ast->array.size_int );
      break;
    case C_ARRAY_SIZE_NAME:
      FPRINTF( gib->fout, "%s%s", is_qual ? " " : "", ast->array.size_name );
      break;
    case C_ARRAY_SIZE_VLA:
      FPUTC( '*', gib->fout );
      break;
  } // switch

  FPUTS( other_token_c( "]" ), gib->fout );
}

/**
 * Prints a bit-field width, if any, as part of a C/C++ declaration.
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
    FPRINTF( gib->fout, " : %u", ast->bit_field.bit_width );
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
    case K_USER_DEFINED_CONV:
      //
      // Since none of these have a return type, no space needs to be printed
      // before the name, so lie and set the "space" flag.
      //
      gib->printed_space = true;
      FALLTHROUGH;

    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEFINED_LIT:
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
      // If noexcept is supported, change throw() to noexcept.
      //
      if ( OPT_LANG_IS( noexcept ) && true_clear( &is_throw ) )
        is_noexcept = true;
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_ARRAY:
      if ( ast->kind != K_ARRAY ||
           ( !c_tid_is_any( type.stids, TS_ANY_ARRAY_QUALIFIER ) ) ) {
        fputs_sp( c_type_gibberish( &type ), gib->fout );
      }
      if ( ast->kind == K_USER_DEFINED_CONV ) {
        if ( !c_sname_empty( &ast->sname ) )
          FPRINTF( gib->fout, "%s::", c_sname_gibberish( &ast->sname ) );
        FPUTS( "operator ", gib->fout );
      }
      if ( ast->parent.of_ast != NULL ) {
        is_trailing_ret = (ast->kind & K_ANY_TRAILING_RETURN) != 0 &&
          opt_trailing_ret && OPT_LANG_IS( TRAILING_RETURN_TYPES );
        if ( is_trailing_ret )
          FPUTS( L_auto, gib->fout );
        else
          c_ast_gibberish_impl( ast->parent.of_ast, gib );
      }
      if ( msc_call_atids != TA_NONE &&
           !c_ast_parent_is_kind_any( ast, K_POINTER ) ) {
        //
        // If ast is a function having a Microsoft calling convention, but not
        // a pointer to such a function, print the calling convention.
        // (Pointers to such functions are handled in
        // c_ast_postfix_gibberish().)
        //
        FPRINTF( gib->fout, " %s", c_tid_gibberish( msc_call_atids ) );
      }

      if ( false_set( &gib->is_postfix ) ) {
        if ( (gib->gib_flags & (C_GIB_PRINT_CAST | C_GIB_USING)) == 0 )
          gib_print_space_once( gib );
        c_ast_postfix_gibberish( ast, gib );
      }

      fputsp_s( c_tid_gibberish( cv_qual_stids ), gib->fout );

      if ( ref_qual_stids != TS_NONE ) {
        FPRINTF( gib->fout, " %s",
          other_token_c(
            c_tid_is_any( ref_qual_stids, TS_REFERENCE ) ? "&" : "&&"
          )
        );
      }

      if ( is_noexcept )
        FPUTS( " noexcept", gib->fout );
      else if ( is_throw )
        FPUTS( " throw()", gib->fout );
      if ( is_override )
        FPUTS( " override", gib->fout );
      else if ( is_final )
        FPUTS( " final", gib->fout );

      if ( is_trailing_ret ) {
        FPUTS( " -> ", gib->fout );
        //
        // Temporarily orphan the return type's AST in order to print it as a
        // stand-alone trailing type.
        //
        c_ast_t *const ret_ast = ast->func.ret_ast;
        c_ast_t *const orig_ret_ast_parent_ast = ret_ast->parent_ast;
        ret_ast->parent_ast = NULL;

        gib_init( &child_gib, C_GIB_PRINT_DECL, gib->fout );
        c_ast_gibberish_impl( ret_ast, &child_gib );
        ret_ast->parent_ast = orig_ret_ast_parent_ast;
      }

      if ( is_pure_virtual )
        FPUTS( " = 0", gib->fout );
      else if ( is_default )
        FPUTS( " = default", gib->fout );
      else if ( is_delete )
        FPUTS( " = delete", gib->fout );
      break;

    case K_BUILTIN:
      c_builtin_ast_gibberish( ast, &type, gib );
      break;

    case K_CAPTURE:
      c_capture_ast_gibberish( ast, gib );
      break;

    case K_CAST:
      c_cast_ast_gibberish( ast, gib );
      break;

    case K_ENUM:
      //
      // Special case: an enum class must be written as just "enum" when doing
      // an elaborated-type-specifier:
      //
      //      c++decl> declare e as enum class C
      //      enum C e;                 // not: enum class C e;
      //
      type.btids &= c_tid_compl( TB_struct | TB_class );
      FALLTHROUGH;

    case K_CLASS_STRUCT_UNION:
      c_ecsu_ast_gibberish( ast, &type, gib );
      break;

    case K_CONCEPT:
      c_concept_ast_gibberish( ast, &type, gib );
      break;

    case K_LAMBDA:
      c_lambda_ast_gibberish( ast, &type, gib );
      break;

    case K_NAME:
      c_name_ast_gibberish( ast, gib );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      c_ptr_ref_ast_gibberish( ast, &type, gib );
      break;

    case K_POINTER_TO_MEMBER:
      c_ptr_mbr_ast_gibberish( ast, gib );
      break;

    case K_STRUCTURED_BINDING:
      c_struct_bind_ast_gibberish( ast, gib );
      break;

    case K_TYPEDEF:
      c_typedef_ast_gibberish( ast, &type, gib );
      break;

    case K_VARIADIC:
      FPUTS( L_ELLIPSIS, gib->fout );
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

  decl_flags_t const node_gib_flags =
    gib->gib_flags & ~TO_UNSIGNED_EXPR( C_GIB_OPT_OMIT_TYPE );

  FOREACH_SLIST_NODE( ast_node, ast_list ) {
    gib_state_t node_gib;
    gib_init( &node_gib, node_gib_flags, gib->fout );
    node_gib.is_nested_scope = gib->is_nested_scope;
    c_ast_gibberish_impl( ast_node->data, &node_gib );
    if ( ast_node->next != NULL )
      FPUTS( ", ", gib->fout );
  } // for
}

/**
 * Prints either the full or local name of \a ast based on whether we're
 * emitting the gibberish for nested scope.
 *
 * @param ast The AST to get the name of.
 * @param gib The gib_state to use.
 *
 * @sa c_ast_space_name_gibberish()
 */
static void c_ast_name_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( gib != NULL );

  FPUTS(
    //
    // If we're in a nested scope, just print the local name.
    //
    gib->is_nested_scope ?
      c_sname_local_name( &ast->sname ) : c_sname_gibberish( &ast->sname ),
    gib->fout
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
      case K_USER_DEFINED_CONV:
      case K_USER_DEFINED_LIT:
        c_ast_postfix_gibberish( parent_ast, gib );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        switch ( ast->kind ) {
          case K_APPLE_BLOCK:
            FPRINTF( gib->fout, "(%s", c_op_token_c( C_OP_CARET ) );
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
            FPUTC( '(', gib->fout );

            if ( c_tid_is_any( ast->type.atids, TA_ANY_MSC_CALL ) ) {
              //
              // A pointer to a function having a Microsoft calling convention
              // has the convention printed just inside the '(':
              //
              //      void (__stdcall *pf)(int, int)
              //
              c_tid_t const msc_call_atids = ast->type.atids & TA_ANY_MSC_CALL;
              FPRINTF( gib->fout, "%s ", c_tid_gibberish( msc_call_atids ) );
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
          FPUTC( ')', gib->fout );
        break;

      case K_CLASS_STRUCT_UNION:
      case K_TYPEDEF:
        // nothing to do
        break;                          // LCOV_EXCL_LINE

      case K_BUILTIN:
      case K_CAPTURE:
      case K_CAST:
      case K_CONCEPT:
      case K_ENUM:
      case K_NAME:
      case K_STRUCTURED_BINDING:
      case K_VARIADIC:
        UNEXPECTED_INT_VALUE( parent_ast->kind );

      case K_PLACEHOLDER:
        unreachable();
    } // switch
  } else {
    //
    // We've reached the root of the AST that has the name of the thing we're
    // printing the gibberish for.
    //
    if ( ast->kind == K_APPLE_BLOCK ) {
      FPRINTF( gib->fout, "(%s", c_op_token_c( C_OP_CARET ) );
      if ( opt_alt_tokens && !c_sname_empty( &ast->sname ) )
        FPUTC( ' ', gib->fout );
    }
    c_ast_space_name_gibberish( ast, gib );
    if ( ast->kind == K_APPLE_BLOCK )
      FPUTC( ')', gib->fout );
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
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEFINED_CONV:
    case K_USER_DEFINED_LIT:
      FPUTC( '(', gib->fout );
      c_ast_list_gibberish( &ast->func.param_ast_list, gib );
      FPUTC( ')', gib->fout );
      break;
    case K_BUILTIN:
    case K_CAPTURE:
    case K_CAST:
    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_ENUM:
    case K_LAMBDA:                      // handled in c_ast_gibberish_impl()
    case K_NAME:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_STRUCTURED_BINDING:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to do
      break;
    case K_PLACEHOLDER:
      unreachable();
  } // switch
}

/**
 * Prints a pointer, pointer-to-member, reference, or rvalue reference, its
 * qualifier, if any, and the name, if any, as part of a C/C++ declaration.
 *
 * @param ast The AST that is one of #K_POINTER, #K_POINTER_TO_MEMBER,
 * #K_REFERENCE, or #K_RVALUE_REFERENCE whose qualifier, if any, and name, if
 * any, to print.
 * @param gib The gib_state to use.
 */
static void c_ast_qual_name_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER_OR_REFERENCE ) );
  assert( gib != NULL );

  bool defer_space = false;
  bool const is_west_decl_kind = c_ast_is_west_decl_kind( ast );
  c_tid_t const qual_stids = ast->type.stids & TS_ANY_QUALIFIER;

  switch ( ast->kind ) {
    case K_POINTER:
      if ( ((gib->gib_flags & C_GIB_PRINT_CAST) == 0 &&
            qual_stids != TS_NONE &&
            !c_ast_is_ptr_to_kind_any( ast, K_FUNCTION )) ||
           ast->is_param_pack ) {
        //
        // 1. If we're not printing a cast and there's a qualifier for the
        //    pointer, print a space before it.  For example:
        //
        //          int const *PI;
        //          typedef int *const PI;
        //
        //    However, for a pointer-to-function:
        //
        //          typedef int (*const PF)(char c);
        //
        //    when printed as a "using", don't print the space:
        //
        //          using PF = int(*const)(char c);
        //
        // 2. If we're printing a parameter pack, print a space before it.  For
        //    exmaple:
        //
        //          auto *...
        //
        // However, if the AST is one of opt_west_decl_kinds, defer printing
        // the space until after the '*':
        //
        if ( is_west_decl_kind )
          defer_space = true;
        else
          gib_print_space_once( gib );
      }
      FPUTC( '*', gib->fout );
      if ( defer_space )
        gib_print_space_once( gib );
      break;

    case K_POINTER_TO_MEMBER:
      FPRINTF( gib->fout,
        "%s::*", c_sname_gibberish( &ast->ptr_mbr.class_sname )
      );
      c_ast_t const *const func_ast = c_ast_find_parent_func( ast );
      gib->printed_space =
        func_ast == NULL || (func_ast->kind & opt_west_decl_kinds) == 0;
      break;

    case K_REFERENCE:
      if ( opt_alt_tokens ) {
        gib_print_space_once( gib );
        FPUTS( "bitand ", gib->fout );
      } else {
        if ( ast->is_param_pack )
          gib_print_space_once( gib );
        FPUTC( '&', gib->fout );
      }
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_alt_tokens ) {
        gib_print_space_once( gib );
        FPUTS( "and ", gib->fout );
      } else {
        FPUTS( "&&", gib->fout );
      }
      break;

    default:
      /* suppress warning */;
  } // switch

  if ( qual_stids != TS_NONE ) {
    FPUTS( c_tid_gibberish( qual_stids ), gib->fout );

    if ( (gib->gib_flags & (C_GIB_PRINT_DECL | C_GIB_TYPEDEF)) != 0 ) {
      //
      // For declarations and typedefs, if there's a qualifier and if a name
      // has yet to be printed, we always need to print a space after the
      // qualifier, e.g.:
      //
      //      char *const p;
      //                 ^
      //
      // However, similar to the above case, if the AST is one of
      // opt_west_decl_kinds, defer printing the space:
      //
      if ( is_west_decl_kind && !ast->is_param_pack ) {
        defer_space = true;
      }
      else if ( c_ast_find_name( ast, C_VISIT_UP ) != NULL ) {
        //
        // Don't use gib_print_space_once(): we must always print a space
        // between the qualifier and the name.
        //
        FPUTC( ' ', gib->fout );
        gib->printed_space = true;
      }
    }
  }

  if ( defer_space )
    gib->printed_space = false;

  c_ast_space_name_gibberish( ast, gib );
}

/**
 * Determine whether we should print a space before the `*`, `&`, or `&&` in a
 * declaration.
 *
 * @remarks
 * @parblock
 * By default, for all kinds _except_ function-like ASTs, we want
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
 * @endparblock
 *
 * @param ast The current AST node.
 * @param gib The gib_state to use.
 * @return Returns `true` only if we should print a space after type type.
 */
NODISCARD
static bool c_ast_space_before_ptr_ref( c_ast_t const *ast,
                                        gib_state_t const *gib ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER_OR_REFERENCE ) );
  assert( gib != NULL );

  if ( (gib->gib_flags & (C_GIB_PRINT_CAST | C_GIB_USING)) != 0 )
    return false;
  if ( (gib->gib_flags & C_GIB_OPT_MULTI_DECL) != 0 )
    return true;

  c_ast_t const *const func_ast = c_ast_find_parent_func( ast );
  if ( func_ast != NULL )               // function returning pointer to ...
    return (func_ast->kind & opt_west_decl_kinds) == 0;

  if ( c_ast_find_name( ast, C_VISIT_UP ) == NULL )
    return false;

  if ( c_ast_is_west_decl_kind( ast ) )
    return false;

  return true;
}

/**
 * Prints a space (if one hasn't printed one before) and an AST node's name, if
 * any; but only if we're printing a declaration (not a cast).
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
    case K_APPLE_BLOCK:
    case K_ARRAY:
    case K_BUILTIN:
    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_ENUM:
    case K_FUNCTION:
    case K_NAME:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
      if ( (gib->gib_flags & C_GIB_USING) != 0 )
        break;
      if ( !c_sname_empty( &ast->sname ) || ast->is_param_pack )
        gib_print_space_once( gib );
      if ( ast->is_param_pack )
        FPUTS( L_ELLIPSIS, gib->fout );
      if ( !c_sname_empty( &ast->sname ) )
        c_ast_name_gibberish( ast, gib );
      break;

    case K_CAST:
    case K_USER_DEFINED_CONV:
    case K_STRUCTURED_BINDING:
    case K_VARIADIC:
      // Do nothing since these don't have names.
      break;

    case K_CONSTRUCTOR:
      FPUTS( c_sname_gibberish( &ast->sname ), gib->fout );
      break;

    case K_DESTRUCTOR:
      if ( c_sname_count( &ast->sname ) > 1 )
        FPRINTF( gib->fout, "%s::", c_sname_scope_gibberish( &ast->sname ) );
      if ( opt_alt_tokens )
        FPUTS( "compl ", gib->fout );
      else
        FPUTC( '~', gib->fout );
      FPUTS( c_sname_local_name( &ast->sname ), gib->fout );
      break;

    case K_OPERATOR:
      gib_print_space_once( gib );
      if ( !c_sname_empty( &ast->sname ) )
        FPRINTF( gib->fout, "%s::", c_sname_gibberish( &ast->sname ) );
      char const *const token = c_op_token_c( ast->oper.operator->op_id );
      FPRINTF( gib->fout,
        "operator%s%s", isalpha( token[0] ) ? " " : "", token
      );
      break;

    case K_USER_DEFINED_LIT:
      gib_print_space_once( gib );
      if ( c_sname_count( &ast->sname ) > 1 )
        FPRINTF( gib->fout, "%s::", c_sname_scope_gibberish( &ast->sname ) );
      FPRINTF( gib->fout,
        "operator\"\" %s", c_sname_local_name( &ast->sname )
      );
      break;

    case K_CAPTURE:
    case K_LAMBDA:
      UNEXPECTED_INT_VALUE( ast->kind );

    case K_PLACEHOLDER:
      unreachable();
  } // switch
}

/**
 * Prints a #K_BUILTIN AST as a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param type The \ref c_type to use instead of \ref c_ast::type.
 * @param gib The gib_state to use.
 */
static void c_builtin_ast_gibberish( c_ast_t const *ast, c_type_t const *type,
                                      gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );
  assert( type != NULL );
  assert( gib != NULL );

  if ( (gib->gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 ) {
    FPUTS( c_type_gibberish( type ), gib->fout );
    if ( c_ast_is_tid_any( ast, TB__BitInt ) )
      FPRINTF( gib->fout, "(%u)", ast->builtin.BitInt.width );
  }
  c_ast_space_name_gibberish( ast, gib );
  c_ast_bit_width_gibberish( ast, gib );
}

/**
 * Prints a #K_CAPTURE AST as part of a #K_LAMBDA declaration.
 *
 * @param ast The AST to print.
 * @param gib The gib_state to use.
 */
static void c_capture_ast_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_CAPTURE );
  assert( gib != NULL );

  switch ( ast->capture.kind ) {
    case C_CAPTURE_COPY:
      FPUTC( '=', gib->fout );
      break;
    case C_CAPTURE_REFERENCE:
      FPUTS( other_token_c( "&" ), gib->fout );
      if ( c_sname_empty( &ast->sname ) )
        break;
      if ( opt_alt_tokens )
        FPUTC( ' ', gib->fout );
      FALLTHROUGH;
    case C_CAPTURE_VARIABLE:
      FPUTS( c_sname_local_name( &ast->sname ), gib->fout );
      break;
    case C_CAPTURE_STAR_THIS:
      FPUTC( '*', gib->fout );
      FALLTHROUGH;
    case C_CAPTURE_THIS:
      FPUTS( L_this, gib->fout );
      break;
  } // switch
}

/**
 * Prints a #K_CAST AST.
 *
 * @param ast The AST to print.
 * @param gib The gib_state to use.
 */
static void c_cast_ast_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_CAST );
  assert( gib != NULL );
  assert( gib->gib_flags == C_GIB_PRINT_CAST );

  gib_state_t child_gib;
  gib_init( &child_gib, C_GIB_PRINT_CAST, gib->fout );

  if ( ast->cast.kind == C_CAST_C ) {
    FPUTC( '(', gib->fout );
    c_ast_gibberish_impl( ast->cast.to_ast, &child_gib );
    FPRINTF( gib->fout, ")%s\n", c_sname_gibberish( &ast->sname ) );
  } else {
    FPRINTF( gib->fout, "%s<", c_cast_gibberish( ast->cast.kind ) );
    c_ast_gibberish_impl( ast->cast.to_ast, &child_gib );
    FPRINTF( gib->fout, ">(%s)\n", c_sname_gibberish( &ast->sname ) );
  }
}

/**
 * Prints a #K_CONCEPT AST as part of a C++ declaration.
 *
 * @param ast The AST to print.
 * @param type The \ref c_type to use instead of \ref c_ast::type.
 * @param gib The gib_state to use.
 */
static void c_concept_ast_gibberish( c_ast_t const *ast, c_type_t *type,
                                     gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_CONCEPT );
  assert( type != NULL );
  assert( gib != NULL );

  c_tid_t cv_qual_stids = TS_NONE;

  if ( opt_east_const ) {
    cv_qual_stids = type->stids & TS_CONCEPT;
    type->stids &= c_tid_compl( TS_CONCEPT );
  }

  fputs_sp( c_type_gibberish( type ), gib->fout );
  FPRINTF( gib->fout,
    "%s %s",
    c_sname_gibberish( &ast->concept.concept_sname ), L_auto
  );
  fputsp_s( c_tid_gibberish( cv_qual_stids ), gib->fout );
  c_ast_space_name_gibberish( ast, gib );
}

/**
 * Prints a #K_ENUM or #K_CLASS_STRUCT_UNION AST as a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param type The \ref c_type to use instead of \ref c_ast::type.
 * @param gib The gib_state to use.
 */
static void c_ecsu_ast_gibberish( c_ast_t const *ast, c_type_t *type,
                                  gib_state_t *gib ) {
  assert( ast != NULL );
  assert( (ast->kind & K_ANY_ECSU) != 0 );
  assert( type != NULL );
  assert( gib != NULL );

  c_tid_t cv_qual_stids = TS_NONE;
  bool const is_fixed_enum = ast->kind == K_ENUM && ast->enum_.of_ast != NULL;

  if ( opt_east_const ) {
    cv_qual_stids = type->stids & TS_CVA;
    type->stids &= c_tid_compl( TS_CVA );
  }

  char const *const type_name =
    (gib->gib_flags & (C_GIB_PRINT_CAST | C_GIB_PRINT_DECL)) != 0 &&
    //
    // Special case: a fixed type enum must always have "enum" printed, so
    // we don't call c_type_name_ecsu() that may omit it by applying
    // opt_explicit_ecsu_btids.
    //
    !is_fixed_enum ? c_type_name_ecsu( type ) : c_type_gibberish( type );

  FPUTS( type_name, gib->fout );

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
    FPRINTF( gib->fout,
      "%s%s",
      type_name[0] != '\0' ? " " : "",
      c_sname_gibberish( &ast->csu.csu_sname )
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
      // But if we print its name now, we can't print it again later, so set a
      // flag.
      //
      c_ast_space_name_gibberish( ast, gib );
      printed_name = true;
    }
    FPUTS( " : ", gib->fout );
    c_ast_gibberish_impl( ast->enum_.of_ast, gib );
  }

  fputsp_s( c_tid_gibberish( cv_qual_stids ), gib->fout );

  if ( !printed_name )
    c_ast_space_name_gibberish( ast, gib );

  if ( ast->kind == K_ENUM )
    c_ast_bit_width_gibberish( ast, gib );
}

/**
 * Prints a #K_LAMBDA AST as a C++ declaration.
 *
 * @param ast The AST to print.
 * @param type The \ref c_type to use instead of \ref c_ast::type.
 * @param gib The gib_state to use.
 */
static void c_lambda_ast_gibberish( c_ast_t const *ast, c_type_t const *type,
                                    gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );
  assert( type != NULL );
  assert( gib != NULL );

  FPUTS( other_token_c( "[" ), gib->fout );
  c_ast_list_gibberish( &ast->lambda.capture_ast_list, gib );
  FPUTS( other_token_c( "]" ), gib->fout );

  if ( !slist_empty( &ast->lambda.param_ast_list ) ) {
    FPUTC( '(', gib->fout );
    c_ast_list_gibberish( &ast->lambda.param_ast_list, gib );
    FPUTC( ')', gib->fout );
  }

  fputsp_s( c_tid_gibberish( type->stids ), gib->fout );
  fputsp_s( c_tid_gibberish( type->atids ), gib->fout );

  if ( ast->lambda.ret_ast != NULL &&
        !c_ast_is_builtin_any( ast->lambda.ret_ast, TB_auto | TB_void ) ) {
    FPUTS( " -> ", gib->fout );
    c_ast_gibberish_impl( ast->lambda.ret_ast, gib );
  }
}

/**
 * Prints a #K_NAME AST as part of a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param gib The gib_state to use.
 */
static void c_name_ast_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_NAME );
  assert( gib != NULL );

  bool printed_type = false;

  if ( OPT_LANG_IS( PROTOTYPES ) ) {
    if ( c_ast_is_param( ast ) && c_ast_is_untyped( ast ) ) {
      //
      // A name can occur as an untyped K&R C function parameter.  In C89-C17,
      // it's implicitly int:
      //
      //      cdecl> declare f as function (x) returning char
      //      char f(int x)
      //
      FPUTS( L_int, gib->fout );
      printed_type = true;
    }
    else if ( ast->parent_ast == NULL ) {
      FPUTS( c_sname_gibberish( &ast->name.sname ), gib->fout );
      printed_type = true;
    }
  }

  if ( (gib->gib_flags & C_GIB_PRINT_CAST) == 0 ) {
    if ( printed_type )
      FPUTC( ' ', gib->fout );
    c_ast_name_gibberish( ast, gib );
  }
}

/**
 * Helper function for c_sname_gibberish() and c_sname_scope_gibberish() that
 * writes the scope names from outermost to innermost separated by `::` into a
 * buffer.
 *
 * @param sbuf The buffer to write into.
 * @param sname The scoped name to write.
 * @param end_scope The scope to stop before or NULL for all scopes.
 * @return If not NULL, returns \a sbuf&ndash;>str; otherwise returns the empty
 * string.
 */
NODISCARD
static char const* c_sname_name_impl( strbuf_t *sbuf, c_sname_t const *sname,
                                      c_scope_t const *end_scope ) {
  assert( sbuf != NULL );
  assert( sname != NULL );

  strbuf_reset( sbuf );

  FOREACH_SNAME_SCOPE_UNTIL( scope, sname, end_scope ) {
    c_scope_data_t const *const data = c_scope_data( scope );
    if ( data->type.stids != TS_NONE ) {
      // For nested inline namespaces, e.g., namespace A::inline B::C.
      strbuf_printf( sbuf, "%s ", c_tid_gibberish( data->type.stids ) );
    }
    strbuf_puts( sbuf, data->name );
    if ( scope->next != end_scope )
      strbuf_putsn( sbuf, "::", 2 );
  } // for

  return empty_if_null( sbuf->str );
}

/**
 * Prints a #K_POINTER_TO_MEMBER AST as a C++ declaration.
 *
 * @param ast The AST to print.
 * @param gib The gib_state to use.
 */
static void c_ptr_mbr_ast_gibberish( c_ast_t const *ast, gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_POINTER_TO_MEMBER );
  assert( gib != NULL );

  c_ast_gibberish_impl( ast->ptr_mbr.to_ast, gib );
  gib_print_space_once( gib );
  if ( !gib->is_postfix )
    c_ast_qual_name_gibberish( ast, gib );
}

/**
 * Prints a #K_POINTER or #K_ANY_REFERENCE AST as a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param type The \ref c_type to use instead of \ref c_ast::type.
 * @param gib The gib_state to use.
 */
static void c_ptr_ref_ast_gibberish( c_ast_t const *ast, c_type_t const *type,
                                     gib_state_t *gib ) {
  assert( ast != NULL );
  assert( (ast->kind & (K_POINTER | K_ANY_REFERENCE)) != 0 );
  assert( type != NULL );
  assert( gib != NULL );

  if ( (gib->gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 )
    fputs_sp( c_tid_gibberish( type->stids & TS_ANY_STORAGE ), gib->fout );
  c_ast_gibberish_impl( ast->ptr_ref.to_ast, gib );
  if ( c_ast_space_before_ptr_ref( ast, gib ) )
    gib_print_space_once( gib );
  if ( !gib->is_postfix )
    c_ast_qual_name_gibberish( ast, gib );
}

/**
 * Prints each name in \a sname_list as type \a ast in gibberish for a C++
 * structured binding.
 *
 * @param ast The AST of the type to print.
 * @param gib The gib_state to use.
 *
 * @sa c_ast_gibberish()
 * @sa c_ast_sname_list_gibberish()
 */
static void c_struct_bind_ast_gibberish( c_ast_t const *ast,
                                         gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_STRUCTURED_BINDING );
  assert( gib != NULL );

  c_tid_t cv_qual_stids = TS_NONE;
  c_tid_t const ref_qual_stid = ast->type.stids & TS_ANY_REFERENCE;

  c_type_t type = ast->type;
  c_type_and_eq_compl( &type, &C_TYPE_LIT_S( TS_ANY_REFERENCE ) );

  if ( opt_east_const ) {
    cv_qual_stids = type.stids & TS_CV;
    type.stids &= c_tid_compl( TS_CV );
  }

  fputs_sp( c_type_gibberish( &type ), gib->fout );
  if ( !opt_east_const )
    fputs_sp( c_tid_gibberish( cv_qual_stids ), gib->fout );
  FPUTS( L_auto, gib->fout );
  fputsp_s( c_tid_gibberish( cv_qual_stids ), gib->fout );

  if ( ref_qual_stid == TS_NONE ) {
    FPUTC( ' ', gib->fout );
  }
  else if ( opt_alt_tokens ) {
    FPRINTF( gib->fout,
      " %s ", ref_qual_stid == TS_REFERENCE ? L_bitand : L_and
    );
  }
  else {
    if ( (opt_west_decl_kinds & K_STRUCTURED_BINDING) == 0 )
      FPUTC( ' ', gib->fout );
    FPUTS( ref_qual_stid == TS_REFERENCE ? "&" : "&&", gib->fout );
    if ( (opt_west_decl_kinds & K_STRUCTURED_BINDING) != 0 )
      FPUTC( ' ', gib->fout );
  }

  FPUTC( '[', gib->fout );

  FOREACH_SLIST_NODE( sname_node, &ast->struct_bind.sname_list ) {
    FPUTS( c_sname_local_name( sname_node->data ), gib->fout );
    if ( sname_node->next != NULL )
      FPUTS( ", ", gib->fout );
  } // for

  FPUTC( ']', gib->fout );
}

/**
 * Prints a #K_TYPEDEF AST as a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param type The \ref c_type to use instead of \ref c_ast::type.
 * @param gib The gib_state to use.
 */
static void c_typedef_ast_gibberish( c_ast_t const *ast, c_type_t const *type,
                                     gib_state_t *gib ) {
  assert( ast != NULL );
  assert( ast->kind == K_TYPEDEF );
  assert( type != NULL );
  assert( gib != NULL );

  if ( (gib->gib_flags & C_GIB_OPT_OMIT_TYPE) == 0 ) {
    //
    // Of course a K_TYPEDEF AST also has a type comprising TB_typedef, but we
    // need to see whether there's any more to the type, e.g., "const".
    //
    bool const is_more_than_plain_typedef = type->stids != TS_NONE;

    if ( is_more_than_plain_typedef && !opt_east_const )
      FPUTS( c_type_gibberish( type ), gib->fout );

    //
    // Special case: C++23 adds an _Atomic(T) macro for compatibility with C11,
    // but while _Atomic can be printed without () in C, they're required in
    // C++:
    //
    //      _Atomic size_t x;       // C11 only
    //      _Atomic(size_t) y;      // C11 or C++23
    //
    // Note that this handles printing () only for typedef types; for non-
    // typedef types, see the similar special case in c_type_name_impl().
    //
    bool const print_parens_for_Atomic =
      OPT_LANG_IS( CPP_MIN(23) ) &&
      c_tid_is_any( type->stids, TS__Atomic );

    if ( print_parens_for_Atomic )
      FPUTC( '(', gib->fout );
    else if ( is_more_than_plain_typedef && !opt_east_const )
      FPUTC( ' ', gib->fout );

    //
    // Temporarily turn off C_GIB_USING to force printing of the type's name.
    // This is necessary for when printing the name of a typedef of a typedef
    // as a "using" declaration:
    //
    //      c++decl> typedef int32_t foo_t
    //      c++decl> show foo_t as using
    //      using foo_t = int32_t;
    //
    decl_flags_t const orig_flags = gib->gib_flags;
    gib->gib_flags &= ~TO_UNSIGNED_EXPR( C_GIB_USING );
    c_ast_name_gibberish( ast->tdef.for_ast, gib );
    gib->gib_flags = orig_flags;
    if ( print_parens_for_Atomic )
      FPUTC( ')', gib->fout );
    if ( is_more_than_plain_typedef && opt_east_const )
      FPRINTF( gib->fout, " %s", c_type_gibberish( type ) );
  }

  c_ast_space_name_gibberish( ast, gib );
  c_ast_bit_width_gibberish( ast, gib );
}

/**
 * Initializes a gib_state.
 *
 * @param gib The gib_state to initialize.
 * @param gib_flags The gibberish flags to use.
 * @param fout The `FILE` to print to.
 */
static void gib_init( gib_state_t *gib, decl_flags_t gib_flags, FILE *fout ) {
  assert( gib != NULL );
  assert( is_1n_bit_only_in_set( gib_flags, C_GIB_ANY ) );
  assert( fout != NULL );

  *gib = (gib_state_t){
    .gib_flags = gib_flags,
    .fout = fout,
    .printed_space = (gib_flags & C_GIB_OPT_OMIT_TYPE) != 0
  };
}

/**
 * Gets the digraph or trigraph (collectively, "graph") equivalent of \a token.
 *
 * @param token The token to get the graph token for.
 * @return If we're \ref opt_graph "emitting graphs" and \a token contains one
 * or more characters that have a graph equivalent, returns \a token with said
 * characters replaced by their graphs; otherwise returns \a token as-is.
 *
 * @sa alt_token_c()
 * @sa c_op_token_c()
 * @sa opt_graph
 */
NODISCARD
static char const* graph_token_c( char const *token ) {
  assert( token != NULL );

  switch ( opt_graph ) {
    case C_GRAPH_NONE:
      return token;

    // Even though this could be done character-by-character, it's easier for
    // the calling code if multi-character tokens containing graph characters
    // are returned as a single string.

    case C_GRAPH_DI:
      if ( !OPT_LANG_IS( DIGRAPHS ) )
        return token;

      switch ( token[0] ) {
        case '#'  : switch ( token[1] ) {
                      case '#' : return "%:%:";
                      case '\0': return "%:";
                    } // switch
                    break;
        case '['  : switch ( token[1] ) {
                      case '[' : return "<:<:";
                      case ']' : return "<::>";
                      case '\0': return "<:";
                    } // switch
                    break;
        case ']'  : switch ( token[1] ) {
                      case ']' : return ":>:>";
                      case '\0': return ":>";
                    } // switch
                    break;
        case '{'  : switch ( token[1] ) {
                      case '\0': return "<%";
                    } // switch
                    break;
        case '}'  : switch ( token[1] ) {
                      case '\0': return "%>";
                    } // switch
                    break;
        default   : return token;
      } // switch
      break;

    case C_GRAPH_TRI:
      if ( !OPT_LANG_IS( TRIGRAPHS ) )
        return token;

      switch ( token[0] ) {
        case '#'  : switch ( token[1] ) {
                      case '#' : return "?\?=?\?=";
                      case '\0': return "?\?=";
                    } // switch
                    break;
        case '['  : switch ( token[1] ) {
                      case '[' : return "?\?(?\?(";
                      case ']' : return "?\?(?\?)";
                      case '\0': return "?\?(";
                    } // switch
                    break;
        case '\\' : // LCOV_EXCL_START
                    switch ( token[1] ) {
                      case '\0': return "?\?/";
                    } // switch
                    break;
                    // LCOV_EXCL_STOP
        case ']'  : switch ( token[1] ) {
                      case ']' : return "?\?)?\?)";
                      case '\0': return "?\?)";
                    } // switch
                    break;
        case '^'  : switch ( token[1] ) {
                      case '=' : return "?\?'=";
                      case '\0': return "?\?'";
                    } // switch
                    break;
        case '{'  : switch ( token[1] ) {
                      case '\0': return "?\?<";
                    } // switch
                    break;
        case '|'  : switch ( token[1] ) {
                      case '=' : return "?\?!=";
                      case '|' : return "?\?!?\?!";
                      case '\0': return "?\?!";
                    } // switch
                    break;
        case '}'  : switch ( token[1] ) {
                      case '\0': return "?\?>";
                    } // switch
                    break;
        case '~'  : switch ( token[1] ) {
                      case '\0': return "?\?-";
                    } // switch
                    break;
        default   : return token;
      } // switch
      break;
  } // switch

  UNEXPECTED_INT_VALUE( token[1] );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_gibberish( c_ast_t const *ast, decl_flags_t gib_flags, FILE *fout ) {
  assert( ast != NULL );
  assert( is_1n_bit_only_in_set( gib_flags, C_GIB_ANY ) );
  assert(
    is_1_bit_in_set(
      gib_flags, C_GIB_DECL_ANY | C_GIB_PRINT_DECL | C_GIB_PRINT_CAST
    )
  );
  assert( fout != NULL );

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
    c_typedef_gibberish( &tdef, C_GIB_USING, fout );
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
      c_ast_alignas_gibberish( ast, fout );
    }

    gib_state_t gib;
    gib_init( &gib, gib_flags, fout );
    c_ast_gibberish_impl( ast, &gib );
  }

  if ( (gib_flags & C_GIB_OPT_SEMICOLON) != 0 )
    FPUTC( ';', fout );
}

void c_ast_sname_list_gibberish( c_ast_t *ast, slist_t const *sname_list,
                                 FILE *fout ) {
  assert( ast != NULL );
  assert( sname_list != NULL );
  assert( fout != NULL );

  decl_flags_t decl_flags = C_GIB_PRINT_DECL;
  if ( slist_len( sname_list ) > 1 )
    decl_flags |= C_GIB_OPT_MULTI_DECL;
  bool const print_as_using = c_ast_print_as_using( ast );
  if ( print_as_using && opt_semicolon ) {
    //
    // When declaring multiple types via the same "declare" as "using"
    // declarations, each type needs its own "using" declaration and hence its
    // own semicolon:
    //
    //      c++decl> declare I, J as type int
    //      using I = int;
    //      using J = int;
    //
    decl_flags |= C_GIB_OPT_SEMICOLON;
  }

  FOREACH_SLIST_NODE( sname_node, sname_list ) {
    c_sname_t *const cur_sname = sname_node->data;
    c_sname_set( &ast->sname, cur_sname );
    bool const is_last_sname = sname_node->next == NULL;
    if ( is_last_sname && opt_semicolon )
      decl_flags |= C_GIB_OPT_SEMICOLON;
    c_ast_gibberish( ast, decl_flags, fout );
    if ( is_last_sname )
      continue;
    if ( print_as_using ) {
      //
      // When declaring multiple types via the same "declare" as "using"
      // declarations, they need to be separated by newlines.  (The final
      // newine is handled below.)
      //
      FPUTC( '\n', fout );
    }
    else {
      //
      // When declaring multiple types (not as "using" declarations) or objects
      // via the same "declare", the second and subsequent types or objects
      // must not have the type name printed -- and they also need to be
      // separated by commas.  For example, when printing:
      //
      //      cdecl> declare x, y as pointer to int
      //      int *x, *y;
      //
      // the gibberish for `y` must not print the `int` again.
      //
      decl_flags |= C_GIB_OPT_OMIT_TYPE;
      FPUTS( ", ", fout );
    }
  } // for
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

char const* c_sname_gibberish( c_sname_t const *sname ) {
  static strbuf_t sbuf;
  return sname != NULL ?
    c_sname_name_impl( &sbuf, sname, /*end_scope=*/NULL ) : "";
}

char const* c_sname_scope_gibberish( c_sname_t const *sname ) {
  static strbuf_t sbuf;
  return sname != NULL ? c_sname_name_impl( &sbuf, sname, sname->tail ) : "";
}

void c_typedef_gibberish( c_typedef_t const *tdef, decl_flags_t gib_flags,
                          FILE *fout ) {
  assert( tdef != NULL );
  assert( is_1_bit_in_set( gib_flags, C_GIB_DECL_ANY ) );
  assert(
    is_1n_bit_only_in_set( gib_flags, C_GIB_DECL_ANY | C_GIB_OPT_SEMICOLON )
  );
  assert( fout != NULL );

  size_t scope_close_braces_to_print = 0;
  c_type_t scope_type = T_NONE;

  c_sname_t temp_sname;
  c_sname_init( &temp_sname );          // for unconditional c_sname_cleanup()

  c_sname_t const *sname = c_ast_find_name( tdef->ast, C_VISIT_DOWN );
  if ( sname != NULL && c_sname_count( sname ) > 1 ) {
    scope_type = *c_sname_global_type( sname );
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
        c_sname_global_data( &temp_sname )->type.stids &=
          c_tid_compl( TS_inline );
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

      FPRINTF( fout,
        "%s %s %s ",
        c_type_gibberish( &scope_type ), c_sname_scope_gibberish( sname ),
        other_token_c( "{" )
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
        FPRINTF( fout,
          "%s %s %s ",
          c_type_gibberish( &scope_type ), c_scope_data( scope )->name,
          other_token_c( "{" )
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
  bool const print_typedef =
    (gib_flags & C_GIB_TYPEDEF) != 0 &&
    ( !is_ecsu || (tdef->lang_ids & LANG_C_ANY) != LANG_NONE ||
      ( !OPT_LANG_IS( ECSU_IS_IMPLICIT_TYPE ) &&
        (tdef->lang_ids & LANG_ECSU_IS_IMPLICIT_TYPE) == LANG_NONE
      )
    ) &&
    c_ast_find_type_any( tdef->ast, C_VISIT_DOWN, &T_TS_typedef ) == NULL;

  //
  // When printing a "using", we don't have to check languages since "using" is
  // available only in C++.
  //
  bool const print_using = (gib_flags & C_GIB_USING) != 0 && !is_ecsu;

  if ( print_typedef ) {
    FPUTS( "typedef ", fout );
  }
  else if ( print_using ) {
    FPRINTF( fout, "using %s ", c_sname_local_name( sname ) );
    if ( tdef->ast->type.atids != TA_NONE )
      FPRINTF( fout, "%s ", c_tid_gibberish( tdef->ast->type.atids ) );
    FPUTS( "= ", fout );
  }

  c_sname_cleanup( &temp_sname );

  gib_state_t gib;
  gib_init( &gib, print_using ? C_GIB_USING : C_GIB_TYPEDEF, fout );
  gib.printed_typedef = print_typedef;
  gib.is_nested_scope = scope_close_braces_to_print > 0;
  c_ast_gibberish_impl( tdef->ast, &gib );

  if ( scope_close_braces_to_print > 0 ) {
    FPUTC( ';', fout );
    while ( scope_close_braces_to_print-- > 0 )
      FPRINTF( fout, " %s", other_token_c( "}" ) );
  }

  if ( (gib_flags & C_GIB_OPT_SEMICOLON) != 0 &&
       scope_type.btids != TB_namespace ) {
    FPUTC( ';', fout );
  }
}

char const* other_token_c( char const *token ) {
  char const *const alt_token = alt_token_c( token );
  return alt_token != token ? alt_token : graph_token_c( token );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
