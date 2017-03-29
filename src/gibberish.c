/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
**
**      Paul J. Lucas
*/

/**
 * @file
 * Contains the code for printing an AST as gibberish, aka, a C/C++
 * declaration.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "ast_util.h"
#include "literals.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <string.h>                     /* for memset(3) */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * The kind of gibberish to create.
 */
enum g_kind {
  G_CAST,                               // omits names and unneeded whitespace
  G_DECLARE
};
typedef enum g_kind g_kind_t;

/**
 * Parameters used by c_ast_gibberish() (because there'd be too many function
 * arguments otherwise).
 */
struct g_param {
  g_kind_t        gkind;                // the kind of gibberish to create
  FILE           *gout;                 // where to write the gibberish
  c_ast_t const  *leaf_ast;             // leaf of AST
  c_ast_t const  *root_ast;             // root of AST
  bool            postfix;              // doing postfix gibberish?
  bool            space;                // printed a space yet?
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
 * Prints a space only if we haven't printed one yet.
 *
 * @param param The g_param to use.
 */
static inline void g_param_space( g_param_t *param ) {
  if ( false_set( &param->space ) )
    FPUTC( ' ', param->gout );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_ast_gibberish_impl() that prints an array's size as
 * well as the size for all child arrays, if any.
 *
 * @param ast the The c_ast that is a K_ARRAY whose size to print.
 * @param param The g_param to use.
 */
static void c_ast_gibberish_array_size( c_ast_t const *ast, g_param_t *param ) {
  assert( ast );
  assert( ast->kind == K_ARRAY );

  FPUTC( '[', param->gout );
  if ( ast->as.array.size != C_ARRAY_NO_SIZE )
    FPRINTF( param->gout, "%d", ast->as.array.size );
  FPUTC( ']', param->gout );
}

/**
 * Helper function for c_ast_gibberish_impl() that prints a block's or
 * function's arguments, if any.
 *
 * @param ast The c_ast that is either a K_BLOCK or a K_FUNCTION whose
 * arguments to print.
 * @param param The g_param to use.
 */
static void c_ast_gibberish_func_args( c_ast_t const *ast, g_param_t *param ) {
  assert( ast );
  assert( ast->kind & (K_BLOCK | K_FUNCTION) );

  bool comma = false;
  FPUTC( '(', param->gout );
  for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", param->gout );
    g_param_t args_param;
    g_param_init( &args_param, arg, param->gkind, param->gout );
    c_ast_gibberish_impl( arg, &args_param );
  } // for
  FPUTC( ')', param->gout );
}

/**
 * Prints the given AST as gibberish, aka, a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param param The g_param to use.
 */
static void c_ast_gibberish_impl( c_ast_t const *ast, g_param_t *param ) {
  assert( ast );
  assert( param );

  switch ( ast->kind ) {
    case K_ARRAY:
    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      c_ast_gibberish_impl( ast->as.parent.of_ast, param );
      if ( false_set( &param->postfix ) ) {
        if ( param->gkind != G_CAST )
          g_param_space( param );
        if ( ast == param->root_ast )
          c_ast_gibberish_postfix( param->leaf_ast->parent, param );
        else
          c_ast_gibberish_postfix( ast, param );
      }
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type ), param->gout );
      c_ast_gibberish_space_name( ast, param );
      param->leaf_ast = ast;
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( param->gout,
        "%s %s", c_kind_name( ast->kind ), ast->as.ecsu.ecsu_name
      );
      param->leaf_ast = ast;
      break;

    case K_NAME:
      if ( ast->name && param->gkind != G_CAST )
        FPUTS( ast->name, param->gout );
      param->leaf_ast = ast;
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
    case K_REFERENCE:
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, param );
      if ( param->gkind != G_CAST && c_ast_name( ast, V_UP ) != NULL &&
           !c_ast_find_kind( ast->parent, V_UP, K_BLOCK | K_FUNCTION ) ) {
        //
        // For all kinds except functions and blocks, we want the output to be
        // like:
        //
        //      type *var
        //
        // i.e., the '*' or '&' adjacent to the variable; for functions,
        // blocks, when there's no name for a function argument, or when we're
        // casting, we want the output to be like:
        //
        //      type* func()            // function
        //      type* (^block)()        // block
        //      func(type*)             // nameless function argument
        //      (type*)expr             // cast
        //
        // i.e., the '*' or '&' adjacent to the type.
        //
        g_param_space( param );
      }
      if ( !param->postfix )
        c_ast_gibberish_qual_name( ast, param );
      break;

    case K_POINTER_TO_MEMBER:
      c_ast_gibberish_impl( ast->as.ptr_mbr.of_ast, param );
      if ( !param->postfix ) {
        FPUTC( ' ', param->gout );
        c_ast_gibberish_qual_name( ast, param );
      }
      break;

    case K_VARIADIC:
      FPUTS( L_ELLIPSIS, param->gout );
      param->leaf_ast = ast;
      break;
  } // switch
}

/**
 * Helper function for c_ast_gibberish_impl() that handles the printing of
 * "postfix" cases:
 *  + array of pointer to function
 *  + pointer to array
 *
 * @param ast The c_ast
 * @param param The g_param to use.
 */
static void c_ast_gibberish_postfix( c_ast_t const *ast, g_param_t *param ) {
  assert( ast );
  assert( ast->kind &
          (K_ARRAY | K_BLOCK | K_FUNCTION | K_POINTER | K_REFERENCE) );
  assert( param );

  c_ast_t const *const parent = ast->parent;

  if ( parent ) {
    switch ( parent->kind ) {
      case K_ARRAY:
      case K_BLOCK:                     // Apple extension
      case K_FUNCTION:
        c_ast_gibberish_postfix( parent, param );
        break;

      case K_POINTER:
      case K_POINTER_TO_MEMBER:
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
        if ( parent->parent ) {
          switch ( parent->parent->kind ) {
            case K_ARRAY:
            case K_BLOCK:               // Apple extension
            case K_FUNCTION:
            case K_POINTER:
            case K_POINTER_TO_MEMBER:
            case K_REFERENCE:
              c_ast_gibberish_postfix( parent, param );
              break;
            default:
              /* suppress warning */;
          } // switch
        }

        if ( !(ast->kind & (K_POINTER | K_POINTER_TO_MEMBER)) )
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
 * Helper function for c_ast_gibberish_impl() that prints a pointer, pointer-
 * to-member, or reference, its qualifier, if any, and the name, if any.
 *
 * @param ast The c_ast that is one of K_POINTER, K_POINTER_TO_MEMBER, or
 * K_REFERENCE whose qualifier, if any, and name, if any, to print.
 * @param param The g_param to use.
 */
static void c_ast_gibberish_qual_name( c_ast_t const *ast,
                                       g_param_t const *param ) {
  assert( ast );

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
    default:
      assert( ast->kind & (K_POINTER | K_POINTER_TO_MEMBER | K_REFERENCE) );
  } // switch

  if ( ast->as.ptr_ref.qualifier ) {
    FPUTS( c_type_name( ast->as.ptr_ref.qualifier ), param->gout );
    if ( param->gkind != G_CAST )
      FPUTC( ' ', param->gout );
  }
  if ( ast->name && param->gkind != G_CAST )
    FPUTS( ast->name, param->gout );
}

/**
 * Helper function for c_ast_gibberish_impl() that prints a space (if it hasn't
 * printed one before) and an AST node's name, if any.
 *
 * @param ast The c_ast to print the name of, if any.
 * @param param The g_param to use.
 */
static void c_ast_gibberish_space_name( c_ast_t const *ast, g_param_t *param ) {
  assert( ast );
  assert( param );

  if ( ast->name && param->gkind != G_CAST ) {
    g_param_space( param );
    FPUTS( ast->name, param->gout );
  }
}

/**
 * Initializes a g_param.
 *
 * @param param The \c g_param to initialize.
 * @param gkind The kind of gibberish to print.
 * @param root The AST root.
 * @param gout The FILE to print it to.
 */
static void g_param_init( g_param_t *param, c_ast_t const *root,
                          g_kind_t gkind, FILE *gout ) {
  memset( param, 0, sizeof( g_param_t ) );
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
