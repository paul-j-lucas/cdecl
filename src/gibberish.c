/*
**      cdecl -- C gibberish translator
**      src/gibberish.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
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
  unsigned        func_depth;           // within "function () returning ..."
  c_ast_t const  *leaf_ast;             // leaf of AST
  c_ast_t const  *root_ast;             // root of AST
  bool            postfix;              // doing postfix gibberish?
  bool            space;                // printed a space yet?
};
typedef struct g_param g_param_t;

// local functions
static bool       c_ast_vistor_kind( c_ast_t*, void* );
static void       c_ast_gibberish_impl( c_ast_t const*, g_param_t* );
static void       c_ast_gibberish_postfix( c_ast_t const*, g_param_t* );
static void       c_ast_gibberish_qual_name( c_ast_t const*, g_param_t const* );
static void       c_ast_gibberish_space_name( c_ast_t const*, g_param_t* );
static void       g_param_init( g_param_t*, c_ast_t const*, g_kind_t, FILE* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets the kind of AST node the child node.
 *
 * @param The c_ast to get the child node's kind of.
 * @return Returns the child node's kind.
 */
static inline c_kind_t c_ast_child_kind( c_ast_t const *ast ) {
  return ast->as.parent.of_ast->kind;
}

/**
 * Gets the kind of AST node the parent node, if any, is.
 *
 * @param ast The c_ast to get the parent node's kind of.
 * @return Returns the parent node's kind or K_NONE if none.
 */
static inline c_kind_t c_ast_parent_kind( c_ast_t const *ast ) {
  return ast->parent ? ast->parent->kind : K_NONE;
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
  assert( ast->kind == K_BLOCK || ast->kind == K_FUNCTION );

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
      c_ast_gibberish_impl( ast->as.array.of_ast, param );
      if ( ast->as.array.of_ast == param->leaf_ast ) {
        //
        // We've reached the leaf on the current branch of the tree and printed
        // the type that the array is of: now recurse back up to the root so we
        // can print the AST nodes in root-to-leaf order as the recursion
        // unwinds.
        //
        if ( false_set( &param->space ) )
          FPUTC( ' ', param->gout );
        param->postfix = true;
        c_ast_gibberish_postfix( ast, param );
      }
      else if ( !param->postfix && param->func_depth == 0 ) {
        //
        // We have to defer printing the array's size until we've fully
        // unwound nested arrays, if any, so we print:
        //
        //      type name[3][5]
        //
        // rather than:
        //
        //      type[5] name[3]
        //
        c_ast_gibberish_space_name( ast, param );
        c_ast_gibberish_array_size( ast, param );
      }
      break;

    case K_BLOCK:                       // Apple extension
      c_ast_gibberish_impl( ast->as.block.ret_ast, param );
      FPUTS( "(^", param->gout );
      if ( c_ast_parent_kind( ast ) == K_POINTER ) {
        //
        // If the parent node is a pointer, it's a pointer to block.
        //
        c_ast_gibberish_qual_name( ast->parent, param );
      }
      else if ( ast->name && param->gkind != G_CAST ) {
        FPUTS( ast->name, param->gout );
      }
      FPUTC( ')', param->gout );
      c_ast_gibberish_func_args( ast, param );
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

    case K_FUNCTION:
      ++param->func_depth;
      c_ast_gibberish_impl( ast->as.func.ret_ast, param );
      --param->func_depth;
      if ( false_set( &param->postfix ) ) {
        if ( false_set( &param->space ) )
          FPUTC( ' ', param->gout );
        if ( ast == param->root_ast )
          c_ast_gibberish_postfix( param->leaf_ast->parent, param );
        else
          c_ast_gibberish_postfix( ast, param );
      }
      break;

    case K_NAME:
      if ( ast->name && param->gkind != G_CAST )
        FPUTS( ast->name, param->gout );
      param->leaf_ast = ast;
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, param );
      bool const has_function_ancestor =
        !!c_ast_visit_up( ast->parent, c_ast_vistor_kind, (void*)K_FUNCTION );

      if ( !has_function_ancestor && param->gkind != G_CAST ) {
        //
        // For all kinds except functions, we want the output to be like:
        //
        //      type *var
        //
        // i.e., the '*' adjacent to the variable; for functions, or when
        // we're casting, we want the output to be like:
        //
        //      type* func()        // function
        //      (type*)             // cast
        //
        // i.e., the '*' adjacent to the type.
        //
        if ( false_set( &param->space ) )
          FPUTC( ' ', param->gout );
      }
      if ( !param->postfix && c_ast_child_kind( ast ) != K_ARRAY )
        c_ast_gibberish_qual_name( ast, param );
      break;

    case K_POINTER_TO_MEMBER:
      c_ast_gibberish_impl( ast->as.ptr_mbr.of_ast, param );
      FPRINTF( param->gout, " %s::", ast->as.ptr_mbr.class_name );
      c_ast_gibberish_qual_name( ast, param );
      break;

    case K_REFERENCE:
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, param );
      if ( false_set( &param->space ) )
        FPUTC( ' ', param->gout );
      c_ast_gibberish_qual_name( ast, param );
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
  assert( ast->kind == K_ARRAY ||
          ast->kind == K_BLOCK ||
          ast->kind == K_FUNCTION ||
          ast->kind == K_POINTER );
  assert( param );

  c_ast_t const *const parent = ast->parent;

  if ( parent ) {
    switch ( parent->kind ) {
      case K_ARRAY:
      case K_BLOCK:                     // Apple extension
      case K_FUNCTION:
        c_ast_gibberish_postfix( parent, param );
        break;

      case K_POINTER: {
        //
        // Pointers are written in gibberish like:
        //
        //      type (*a)[size]         // pointer to array
        //      type (*f)(args)         // pointer to function
        //      type (*a[size])(args)   // array of pointer to function
        //
        // However, if there are consecutive pointers, omit the extra '('
        // characters:
        //
        //      type (**a)[size]        // pointer to pointer to array[size]
        //
        // rather than:
        //
        //      type (*(*a))[size]      // extra () unnecessary
        //
        if ( ast->kind != K_POINTER )
          FPUTC( '(', param->gout );
        c_ast_gibberish_qual_name( parent, param );

        c_ast_t const *const grandparent = parent->parent;
        if ( grandparent ) {
          switch ( grandparent->kind ) {
            case K_ARRAY:
            case K_BLOCK:               // Apple extension
            case K_FUNCTION:
            case K_POINTER:
              c_ast_gibberish_postfix( parent, param );
              break;
            default:
              /* suppress warning */;
          } // switch
        }

        if ( ast->kind != K_POINTER )
          FPUTC( ')', param->gout );
        break;
      }

      default:
        /* suppress warning */;
    } // switch
  } else {
    //
    // We've reached the root of the AST that has the name of the thing we're
    // printing the gibberish for.
    //
    c_ast_gibberish_space_name( ast, param );
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
    case K_POINTER_TO_MEMBER:
      FPUTC( '*', param->gout );
      break;
    case K_REFERENCE:
      FPUTC( '&', param->gout );
      break;
    default:
      assert( ast->kind == K_POINTER ||
              ast->kind == K_POINTER_TO_MEMBER ||
              ast->kind == K_REFERENCE );
  } // switch

  if ( ast->as.ptr_ref.qualifier )
    FPRINTF( param->gout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
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
    if ( false_set( &param->space ) )
      FPUTC( ' ', param->gout );
    FPUTS( ast->name, param->gout );
  }
}

/**
 * A c_ast_visitor function used to find an AST node of a particular kind.
 *
 * @param ast The c_ast to check.
 * @param data The c_kind (cast to <code>void*</code>) to find.
 * @return Returns \c true only if the kind of \a ast is \a data.
 */
static bool c_ast_vistor_kind( c_ast_t *ast, void *data ) {
  return ast->kind == (c_kind_t)data;
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
