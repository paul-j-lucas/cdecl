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
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_typedef.h"
#include "gibberish.h"
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
  unsigned        flags;                ///< API flags to tweak output.
  FILE           *gout;                 ///< Where to write the gibberish.
  c_ast_t const  *leaf_ast;             ///< Leaf of AST.
  c_ast_t const  *root_ast;             ///< Root of AST.
  bool            postfix;              ///< Doing postfix gibberish?
  bool            space;                ///< Printed a space yet?
};
typedef struct g_param g_param_t;

// local functions
static void         c_ast_gibberish_impl( c_ast_t const*, g_param_t* );
static void         c_ast_gibberish_postfix( c_ast_t const*, g_param_t* );
static void         c_ast_gibberish_qual_name( c_ast_t const*, g_param_t* );
static void         c_ast_gibberish_space_name( c_ast_t const*, g_param_t* );
C_WARN_UNUSED_RESULT
static char const*  c_ast_sname_full_or_local( c_ast_t const*, g_param_t* );
static void         g_param_init( g_param_t*, c_ast_t const*, g_kind_t, FILE* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets the alignas literal for the current language.
 *
 * @return Returns either `_Alignas` (for C) or `alignas` (for C++).
 */
C_WARN_UNUSED_RESULT
static inline char const* alignas_lang( void ) {
  return C_LANG_IS_CPP() ? L_ALIGNAS : L__ALIGNAS;
}

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

  FPUTS( graph_token_c( "[" ), param->gout );
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
  FPUTS( graph_token_c( "]" ), param->gout );
}

/**
 * Helper function for `c_ast_gibberish_impl()` that prints a function-like
 * AST's arguments, if any.
 *
 * @param ast The `c_ast` that is a <code>\ref K_FUNCTION_LIKE</code> whose
 * arguments to print.
 * @param param The `g_param` to use.
 */
static void c_ast_gibberish_func_args( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( (ast->kind & K_FUNCTION_LIKE) != K_NONE );

  bool comma = false;
  FPUTC( '(', param->gout );
  for ( c_ast_arg_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", param->gout );
    c_ast_t const *const arg_ast = c_ast_arg_ast( arg );
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
  c_type_id_t cv_type         = T_NONE;
  bool        is_default      = false;
  bool        is_deleted      = false;
  bool        is_final        = false;
  bool        is_noexcept     = false;
  bool        is_override     = false;
  bool        is_pure_virtual = false;
  bool        is_throw        = false;
  c_type_id_t ref_qual_type   = T_NONE;

  //
  // This isn't implemented using a visitor because c_ast_visit() visits in
  // post-order and, in order to print gibberish, the AST has to be visited in
  // pre-order.  Since this is the only case where a pre-order traversal has to
  // be done, it's not worth having a pre-order version of c_ast_visit().
  //
  switch ( ast->kind ) {
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_USER_DEF_CONVERSION:
      //
      // Since none of these have a return type, no space needs to be printed
      // before the name, so lie and set the "space" flag.
      //
      param->space = true;
      // FALLTHROUGH

    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      //
      // These things aren't printed as part of the type beforehand, so strip
      // them out of the type here, but print them after the arguments.
      //
      cv_type         = (ast_type & T_MASK_QUALIFIER);
      is_final        = (ast_type & T_FINAL) != T_NONE;
      is_noexcept     = (ast_type & T_NOEXCEPT) != T_NONE;
      is_override     = (ast_type & T_OVERRIDE) != T_NONE;
      is_pure_virtual = (ast_type & T_PURE_VIRTUAL) != T_NONE;
      is_default      = (ast_type & T_DEFAULT) != T_NONE;
      is_deleted      = (ast_type & T_DELETE) != T_NONE;
      is_throw        = (ast_type & T_THROW) != T_NONE;
      ref_qual_type   = (ast_type & T_MASK_REF_QUALIFIER);

      ast_type &= ~(T_MASK_QUALIFIER
                  | T_DEFAULT
                  | T_DELETE
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
    case K_APPLE_BLOCK:
      if ( ast_type != T_NONE )         // storage class
        FPRINTF( param->gout, "%s ", c_type_name( ast_type ) );
      if ( ast->kind == K_USER_DEF_CONVERSION ) {
        if ( !c_ast_sname_empty( ast ) )
          FPRINTF( param->gout, "%s::", c_ast_sname_full_name( ast ) );
        FPRINTF( param->gout, "%s ", L_OPERATOR );
      }
      if ( ast->as.parent.of_ast != NULL )
        c_ast_gibberish_impl( ast->as.parent.of_ast, param );
      if ( false_set( &param->postfix ) ) {
        if ( param->gkind != G_CAST )
          g_param_space( param );
        if ( ast == param->root_ast && param->leaf_ast != NULL )
          c_ast_gibberish_postfix( param->leaf_ast->parent, param );
        else
          c_ast_gibberish_postfix( ast, param );
      }
      if ( cv_type != T_NONE )
        FPRINTF( param->gout, " %s", c_type_name( cv_type ) );
      if ( ref_qual_type != T_NONE )
        FPUTS( (ref_qual_type & T_REFERENCE) ? " &" : " &&", param->gout );
      if ( is_noexcept )
        FPRINTF( param->gout, " %s", L_NOEXCEPT );
      if ( is_throw )
        FPRINTF( param->gout, " %s()", L_THROW );
      if ( is_override )
        FPRINTF( param->gout, " %s", L_OVERRIDE );
      if ( is_final )
        FPRINTF( param->gout, " %s", L_FINAL );
      if ( is_default )
        FPRINTF( param->gout, " = %s", L_DEFAULT );
      if ( is_deleted )
        FPRINTF( param->gout, " = %s", L_DELETE );
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

      if ( opt_east_const ) {
        cv_type = ast_type & (T_CONST | T_VOLATILE);
        ast_type &= ~(T_CONST | T_VOLATILE);
      }

      FPRINTF( param->gout,
        "%s %s", c_type_name( ast_type ),
        c_sname_full_name( &ast->as.ecsu.ecsu_sname )
      );

      if ( cv_type != T_NONE )
        FPRINTF( param->gout, " %s", c_type_name( cv_type ) );

      c_ast_gibberish_space_name( ast, param );
      g_param_leaf( param, ast );
      break;

    case K_NAME:
      if ( !c_ast_sname_empty( ast ) && param->gkind != G_CAST )
        FPUTS( c_ast_sname_full_or_local( ast, param ), param->gout );
      g_param_leaf( param, ast );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
    case K_PLACEHOLDER:
      assert( ast->kind != K_PLACEHOLDER );

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE: {
      c_type_id_t const storage_type = (ast_type & T_MASK_STORAGE);
      if ( storage_type != T_NONE )
        FPRINTF( param->gout, "%s ", c_type_name( storage_type ) );
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, param );
      if ( param->gkind != G_CAST &&
           c_ast_find_name( ast, C_VISIT_UP ) != NULL &&
           !c_ast_find_kind( ast->parent, C_VISIT_UP, K_FUNCTION_LIKE ) ) {
        //
        // For all kinds except function-like ASTs, we want the output to be
        // like:
        //
        //      type *var
        //
        // i.e., the '*', '&', or "&&" adjacent to the variable; for function-
        // like ASTs, when there's no name for an argument, or when we're
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
      if ( !opt_east_const && ast_type != T_TYPEDEF_TYPE )
        FPRINTF( param->gout, "%s ", c_type_name( ast_type ) );
      FPRINTF( param->gout,
        "%s", c_ast_sname_full_or_local( ast->as.c_typedef->ast, param )
      );
      if ( opt_east_const && ast_type != T_TYPEDEF_TYPE )
        FPRINTF( param->gout, " %s", c_type_name( ast_type ) );
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
      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_DESTRUCTOR:
      case K_FUNCTION:
      case K_OPERATOR:
      case K_USER_DEF_CONVERSION:
      case K_USER_DEF_LITERAL:
        c_ast_gibberish_postfix( parent, param );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
        switch ( ast->kind ) {
          case K_APPLE_BLOCK:
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

        if ( (ast->kind & K_ANY_POINTER) == K_NONE )
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
    if ( ast->kind == K_APPLE_BLOCK )
      FPUTS( "(^", param->gout );
    c_ast_gibberish_space_name( ast, param );
    if ( ast->kind == K_APPLE_BLOCK )
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
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      c_ast_gibberish_func_args( ast, param );
      break;
    case K_USER_DEF_CONVERSION:
      FPUTS( "()", param->gout );
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
static void c_ast_gibberish_qual_name( c_ast_t const *ast, g_param_t *param ) {
  assert( ast != NULL );
  assert( param != NULL );

  switch ( ast->kind ) {
    case K_POINTER:
      FPUTC( '*', param->gout );
      break;
    case K_POINTER_TO_MEMBER:
      FPRINTF( param->gout,
        "%s::*", c_sname_full_name( &ast->as.ptr_mbr.class_sname )
      );
      break;
    case K_REFERENCE:
      if ( opt_alt_tokens ) {
        g_param_space( param );
        FPRINTF( param->gout, "%s ", L_BITAND );
      } else {
        FPUTC( '&', param->gout );
      }
      break;
    case K_RVALUE_REFERENCE:
      if ( opt_alt_tokens ) {
        g_param_space( param );
        FPRINTF( param->gout, "%s ", L_AND );
      } else {
        FPUTS( "&&", param->gout );
      }
      break;
    default:
      assert( (ast->kind & (K_ANY_POINTER | K_ANY_REFERENCE)) != K_NONE );
  } // switch

  c_type_id_t const qual_type = (ast->type_id & T_MASK_QUALIFIER);
  if ( qual_type != T_NONE ) {
    FPUTS( c_type_name( qual_type ), param->gout );
    if ( param->gkind != G_CAST )
      FPUTC( ' ', param->gout );
  }
  if ( !c_ast_sname_empty( ast ) && param->gkind != G_CAST )
    FPUTS( c_ast_sname_full_or_local( ast, param ), param->gout );
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

  if ( param->gkind != G_CAST ) {
    switch ( ast->kind ) {
      case K_CONSTRUCTOR:
        FPUTS( c_ast_sname_full_name( ast ), param->gout );
        if ( (ast->type_id & T_EXPLICIT) == T_NONE &&
             c_ast_sname_count( ast ) == 1 ) {
          //
          // For non-explicit constructors, we have to repeat the local name
          // since that's the name of the constructor at file-scope.
          //
          FPRINTF( param->gout, "::%s", c_ast_sname_local_name( ast ) );
        }
        break;
      case K_DESTRUCTOR:
        if ( c_ast_sname_count( ast ) > 1 )
          FPRINTF( param->gout, "%s::", c_ast_sname_scope_name( ast ) );
        if ( opt_alt_tokens )
          FPRINTF( param->gout, "%s ", L_COMPL );
        else
          FPUTC( '~', param->gout );
        FPUTS( c_ast_sname_local_name( ast ), param->gout );
        break;
      case K_OPERATOR: {
        g_param_space( param );
        if ( !c_ast_sname_empty( ast ) )
          FPRINTF( param->gout, "%s::", c_ast_sname_full_name( ast ) );
        c_operator_t const *const op = op_get( ast->as.oper.oper_id );
        char const *const token = graph_token_c( alt_token_c( op->name ) );
        FPRINTF( param->gout,
          "%s%s%s", L_OPERATOR, isalpha( token[0] ) ? " " : "", token
        );
        break;
      }
      case K_USER_DEF_CONVERSION:
        // Do nothing since these don't have names.
        break;
      case K_USER_DEF_LITERAL:
        g_param_space( param );
        if ( c_ast_sname_count( ast ) > 1 )
          FPRINTF( param->gout, "%s::", c_ast_sname_scope_name( ast ) );
        FPRINTF( param->gout,
          "%s\"\" %s",
          L_OPERATOR, c_ast_sname_local_name( ast )
        );
        break;
      default:
        if ( !c_ast_sname_empty( ast ) ) {
          g_param_space( param );
          FPUTS( c_ast_sname_full_or_local( ast, param ), param->gout );
        }
        break;
    } // switch
  }
}

/**
 * Gets either the full or local name of \a ast based on whether we're emitting
 * the gibberish for a `typedef` since it can't have a scoped name.
 *
 * @param ast The `c_ast` to get the name of.
 * @param param The `g_param` to use.
 * @return Returns said name.
 */
C_WARN_UNUSED_RESULT
static char const* c_ast_sname_full_or_local( c_ast_t const *ast,
                                              g_param_t *param ) {
  assert( ast != NULL );
  assert( param != NULL );

  if ( (param->flags & G_DECL_TYPEDEF) != 0 ) {
    param->flags &= ~G_DECL_TYPEDEF;
    return c_ast_sname_local_name( ast );
  }
  return c_ast_sname_full_name( ast );
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
  assert( gout != NULL );

  MEM_ZERO( param );
  param->gkind = gkind;
  param->gout = gout;
  param->root_ast = root;
}

////////// extern functions ///////////////////////////////////////////////////

char const* alt_token_c( char const *token ) {
  assert( token != NULL );

  if ( opt_alt_tokens ) {
    switch ( token[0] ) {
      case '!': switch ( token[1] ) {
                  case '=': return L_NOT_EQ;
                  default : return L_NOT;
                }
      case '&': switch ( token[1] ) {
                  case '&': return L_AND;
                  case '=': return L_AND_EQ;
                  default : return L_BITAND;
                } // switch
      case '|': switch ( token[1] ) {
                  case '|': return L_OR;
                  case '=': return L_OR_EQ;
                  default : return L_BITOR;
                } // switch
      case '~': return L_COMPL;
      case '^': switch ( token[1] ) {
                  case '=': return L_XOR_EQ;
                  default : return L_XOR;
                } // switch
    } // switch
  }
  return token;
}

void c_ast_gibberish_cast( c_ast_t const *ast, FILE *gout ) {
  g_param_t param;
  g_param_init( &param, ast, G_CAST, gout );
  c_ast_gibberish_impl( ast, &param );
}

void c_ast_gibberish_declare( c_ast_t const *ast, unsigned flags, FILE *gout ) {
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
      c_ast_gibberish_declare( ast->align.as.type_ast, G_DECL_NONE, gout );
      FPUTS( ") ", gout );
      break;
  } // switch

  g_param_t param;
  g_param_init( &param, ast, G_DECLARE, gout );
  param.flags = flags;
  c_ast_gibberish_impl( ast, &param );
}

char const* graph_token_c( char const *token ) {
  assert( token != NULL );

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
  return token;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
