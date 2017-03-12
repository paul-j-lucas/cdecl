/*
**      cdecl -- C gibberish translator
**      src/ast.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "common.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <string.h>                     /* for memset(3) */
#include <sysexits.h>

#define PRINT_COMMA \
  BLOCK( if ( !comma ) { FPUTS( ",\n", jout ); comma = true; } )

#define PRINT_JSON(...) \
  BLOCK( print_indent( indent, jout ); FPRINTF( jout, __VA_ARGS__ ); )

#define PRINT_JSON_KV(KEY,VALUE) \
  BLOCK( print_indent( indent, jout ); json_print_kv( (KEY), (VALUE), jout ); )

///////////////////////////////////////////////////////////////////////////////

/**
 * The kind of gibberish to create.
 */
enum gibberish {
  G_CAST,                               // omits names and unneeded whitespace
  G_DECLARE
};
typedef enum gibberish gibberish_t;

// local variable definitions
static unsigned   c_ast_count;          // alloc'd but not yet freed
static c_ast_t   *c_ast_head;           // linked list of alloc'd objects

// local functions
static void       c_ast_gibberish_impl( c_ast_t const*, c_ast_t const*,
                                        gibberish_t, bool*, FILE* );
static void       c_ast_gibberish_qual_name( c_ast_t const*, gibberish_t,
                                             FILE* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Convenience function for getting block/function arguments.
 *
 * @param ast The c_ast to get the arguments of.
 * @return Returns a pointe to the first argument or null if none.
 */
static inline c_ast_t const* c_ast_args( c_ast_t const *ast ) {
  return ast->as.func.args.head_ast;
}

/**
 * Frees all the memory used by the given c_ast.
 *
 * @param ast The c_ast to free.  May be null.
 */
static void c_ast_free( c_ast_t *ast ) {
  if ( ast ) {
    assert( c_ast_count > 0 );
    --c_ast_count;

    FREE( ast->name );
    switch ( ast->kind ) {
      case K_ENUM_CLASS_STRUCT_UNION:
        FREE( ast->as.ecsu.ecsu_name );
        break;
      case K_POINTER_TO_MEMBER:
        FREE( ast->as.ptr_mbr.class_name );
        break;
      default:
        break;
    } // switch
    FREE( ast );
  }
}

/**
 * Helper function for c_ast_gibberish_impl() that prints a block's or
 * function's arguments, if any.
 *
 * @param ast The c_ast that is either a K_BLOCK or a K_FUNCTION whose
 * arguments to print.
 * @param g_kind The kind of gibberish to print.
 * @param gout The FILE to print to.
 */
static void c_ast_gibberish_args( c_ast_t const *ast, gibberish_t g_kind,
                                  FILE *gout ) {
  assert( ast );
  assert( ast->kind == K_BLOCK || ast->kind == K_FUNCTION );

  bool comma = false;
  FPUTC( '(', gout );
  for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", gout );
    bool space = false;
    c_ast_gibberish_impl( arg, ast, g_kind, &space, gout );
  } // for
  FPUTC( ')', gout );
}

/**
 * Helper function for c_ast_gibberish_impl() that prints an array's size as
 * well as the size for all child arrays, if any.
 *
 * @param ast the The c_ast that is a K_ARRAY whose size to print.
 * @param gout The FILE to print to.
 */
static void c_ast_gibberish_array_size( c_ast_t const *ast, FILE *gout ) {
  assert( ast );
  assert( ast->kind == K_ARRAY );

  FPUTC( '[', gout );
  if ( ast->as.array.size != C_ARRAY_NO_SIZE )
    FPRINTF( gout, "%d", ast->as.array.size );
  FPUTC( ']', gout );

  if ( ast->as.array.of_ast->kind == K_ARRAY )
    c_ast_gibberish_array_size( ast->as.array.of_ast, gout );
}

/**
 * Prints the given AST as gibberish, aka, a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param parent The parent c_ast.  Must \c not be null.
 * @param g_kind The kind of gibberish to print.
 * @param space A pointer to a \c bool: if \c false, it means we haven't
 * printed a space yet, so print one and set the \c bool to \c true; if \c
 * true, do nothing.
 * @param gout The FILE to print to.
 */
static void c_ast_gibberish_impl( c_ast_t const *ast, c_ast_t const *parent,
                                  gibberish_t g_kind, bool *space,
                                  FILE *gout ) {
  assert( ast );
  assert( parent );

  switch ( ast->kind ) {
    case K_ARRAY:
      c_ast_gibberish_impl( ast->as.array.of_ast, ast, g_kind, space, gout );
      if ( parent->kind == K_POINTER ) {
        //
        // If the parent node is a pointer, it's a pointer to array: print it
        // like:
        //
        //      type (*name)[size]
        //
        // rather than:
        //
        //      type *name[size]
        //
        // which is an array [size] of pointer to type.
        //
        FPUTS( " (*", gout );
        c_ast_gibberish_qual_name( parent, g_kind, gout );
        FPUTC( ')', gout );
      }
      else if ( ast->name && g_kind != G_CAST ) {
        if ( false_set( space ) )
          FPUTC( ' ', gout );
        FPUTS( ast->name, gout );
      }

      if ( parent->kind != K_ARRAY ) {
        //
        // We have to defer printing the array's size until we've fully unwound
        // nested arrays, if any, so we print:
        //
        //      type name[3][5]
        //
        // rather than:
        //
        //      type[5] name[3]
        //
        c_ast_gibberish_array_size( ast, gout );
      }
      break;

    case K_BLOCK:                       // Apple extension
      c_ast_gibberish_impl( ast->as.block.ret_ast, ast, g_kind, space, gout );
      FPUTS( "(^", gout );
      if ( parent->kind == K_POINTER ) {
        //
        // If the parent node is a pointer, it's a pointer to block.
        //
        FPUTC( '*', gout );
        c_ast_gibberish_qual_name( parent, g_kind, gout );
      }
      else if ( ast->name && g_kind != G_CAST ) {
        FPUTS( ast->name, gout );
      }
      FPUTC( ')', gout );
      c_ast_gibberish_args( ast, g_kind, gout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type ), gout );
      if ( ast->name && g_kind != G_CAST )
        FPRINTF( gout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( gout,
        "%s %s", c_kind_name( ast->kind ), ast->as.ecsu.ecsu_name
      );
      break;

    case K_FUNCTION:
      c_ast_gibberish_impl( ast->as.func.ret_ast, ast, g_kind, space, gout );
      FPUTC( ' ', gout );
      if ( parent->kind == K_POINTER ) {
        //
        // If the parent node is a pointer, it's a pointer to function: print
        // it like:
        //
        //      ret_type (*name)(arg_list)
        //
        // rather than:
        //
        //      ret_type *name(arg_list)
        //
        // which is a function returning pointer to ret_type.
        //
        FPUTS( "(*", gout );
        c_ast_gibberish_qual_name( parent, g_kind, gout );
        FPUTC( ')', gout );
      }
      else if ( ast->name && g_kind != G_CAST ) {
        FPUTS( ast->name, gout );
      }
      c_ast_gibberish_args( ast, g_kind, gout );
      break;

    case K_NAME:
      if ( ast->name && g_kind != G_CAST )
        FPUTS( ast->name, gout );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, ast, g_kind, space, gout );
      switch ( ast->as.ptr_ref.to_ast->kind ) {
        case K_ARRAY:
        case K_BLOCK:                   // Apple extension
        case K_FUNCTION:
          //
          // We have to handle pointers to these kinds in those kinds
          // themselves due to the extra parentheses needed in the output.
          //
          break;
        default:
          if ( parent->kind != K_FUNCTION && g_kind != G_CAST ) {
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
            if ( false_set( space ) )
              FPUTC( ' ', gout );
          }
          FPUTC( '*', gout );
          c_ast_gibberish_qual_name( ast, g_kind, gout );
      } // switch
      break;

    case K_POINTER_TO_MEMBER:
      c_ast_gibberish_impl( ast->as.ptr_mbr.of_ast, ast, g_kind, space, gout );
      FPRINTF( gout, " %s::*", ast->as.ptr_mbr.class_name );
      c_ast_gibberish_qual_name( ast, g_kind, gout );
      break;

    case K_REFERENCE:
      c_ast_gibberish_impl( ast->as.ptr_ref.to_ast, ast, g_kind, space, gout );
      if ( false_set( space ) )
        FPUTC( ' ', gout );
      FPUTC( '&', gout );
      c_ast_gibberish_qual_name( ast, g_kind, gout );
      break;
  } // switch
}

/**
 * Helper function for c_ast_gibberish_impl() that prints a pointer, pointer-
 * to-member, or reference qualifier, if any, and the name, if any.
 *
 * @param ast The c_ast that is one of K_POINTER, K_POINTER_TO_MEMBER, or
 * K_REFERENCE whose qualifier, if any, and name, if any, to print.
 * @param gout The FILE to print to.
 */
static void c_ast_gibberish_qual_name( c_ast_t const *ast, gibberish_t g_kind,
                                       FILE *gout ) {
  assert( ast );
  assert( ast->kind == K_POINTER ||
          ast->kind == K_POINTER_TO_MEMBER ||
          ast->kind == K_REFERENCE );

  if ( ast->as.ptr_ref.qualifier )
    FPRINTF( gout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
  if ( ast->name && g_kind == G_DECLARE )
    FPUTS( ast->name, gout );
}

/**
 * Initializes a c_ast.
 *
 * @param ast The c_ast to initialize.
 * @param kind The kind of c_ast to initialize.
 */
static inline void c_ast_init( c_ast_t *ast, c_kind_t kind ) {
  memset( ast, 0, sizeof( c_ast_t ) );
  ast->kind = kind;
}

/**
 * A c_ast_visitor function used to find a K_BUILTIN.
 *
 * @param ast The c_ast to check.
 * @return Returns \c true only if the kind of \a ast is K_BUILTIN.
 */
static bool c_ast_vistor_builtin( c_ast_t *ast ) {
  return ast->kind == K_BUILTIN;
}

/**
 * A c_ast_visitor function used to find a name.
 *
 * @param ast The c_ast to check.
 * @return Returns \c true only if \a ast has a name.
 */
static bool c_ast_visitor_name( c_ast_t *ast ) {
  return ast->name != NULL;
}

/**
 * Prints a multiple of \a indent spaces.
 *
 * @param indent How much to indent.
 * @param out The FILE to print to.
 */
static void print_indent( unsigned indent, FILE *out ) {
  FPRINTF( out, "%*s", (int)(indent * JSON_INDENT), "" );
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check( c_ast_t const *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_ARRAY: {
      c_ast_t const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast->kind ) {
        case K_BUILTIN:
          if ( of_ast->type & T_VOID ) {
            print_error( &ast->loc, "array of void" );
            print_hint( "array of pointer to void" );
            return false;
          }
          if ( of_ast->type & T_REGISTER ) {
            print_error( &ast->loc, "register array" );
            return false;
          }
          break;
        case K_FUNCTION:
          print_error( &ast->loc, "array of function" );
          print_hint( "array of pointer to function" );
          return false;
        default:
          /* suppress warning */;
      } // switch
      break;
    }

    case K_BLOCK:                       // Apple extension
      // TODO
      break;

    case K_BUILTIN:
      if ( ast->type & T_VOID ) {
        print_error( &ast->loc, "variable of void" );
        print_hint( "pointer to void" );
        return false;
      }
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      // nothing to do
      break;

    case K_FUNCTION:
      if ( ast->type & T_REGISTER ) {
        print_error( &ast->loc, "register function" );
        return false;
      }
      break;

    case K_NAME:
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
      break;

    case K_POINTER_TO_MEMBER:
      break;

    case K_REFERENCE: {
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      if ( to_ast->type & T_VOID ) {
        print_error( &ast->loc, "referece to void" );
        print_hint( "pointer to void" );
        return false;
      }
      break;
    }
  } // switch

  return true;
}

void c_ast_cleanup( void ) {
  if ( c_ast_count > 0 )
    INTERNAL_ERR( "number of c_ast objects (%u) > 0\n", c_ast_count );
}

void c_ast_gc( void ) {
  for ( c_ast_t *p = c_ast_head; p; ) {
    c_ast_t *const next = p->gc_next;
    c_ast_free( p );
    p = next;
  } // for
  c_ast_head = NULL;
}

c_ast_t* c_ast_clone( c_ast_t const *ast ) {
  if ( ast == NULL )
    return NULL;

  c_ast_t *const clone = c_ast_new( ast->kind, &ast->loc );
  //
  // Even though it's slightly less efficient, it's safer to memcpy() the clone
  // so we can't forget to copy non-pointer struct members.
  //
  c_ast_t *const gc_next_copy = clone->gc_next;
  memcpy( clone, ast, sizeof( c_ast_t ) );
  clone->gc_next = gc_next_copy;

  clone->name = check_strdup( ast->name );
  clone->next = c_ast_clone( ast->next );

  switch ( ast->kind ) {
    case K_POINTER_TO_MEMBER:
      clone->as.ptr_mbr.class_name = check_strdup( ast->as.ptr_mbr.class_name );
      // no break;
    case K_ARRAY:
    case K_POINTER:
    case K_REFERENCE:
      clone->as.array.of_ast = c_ast_clone( ast->as.array.of_ast );
      break;

    case K_BUILTIN:
    case K_NAME:
    case K_NONE:
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      clone->as.ecsu.ecsu_name = check_strdup( ast->as.ecsu.ecsu_name );
      break;

    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      clone->as.func.ret_ast = c_ast_clone( ast->as.func.ret_ast );
      clone->as.func.args = c_ast_list_clone( &ast->as.func.args );
      break;
  } // switch

  return clone;
}

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  if ( ast == NULL )
    return;

  bool comma = false;

  switch ( ast->kind ) {
    case K_ARRAY:
      FPRINTF( eout, "%s ", L_ARRAY );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( eout, "%d ", ast->as.array.size );
      FPRINTF( eout, "%s ", L_OF );
      c_ast_english( ast->as.array.of_ast, eout );
      break;

    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      if ( ast->type )
        FPRINTF( eout, "%s ", c_type_name( ast->type ) );
      FPUTS( c_kind_name( ast->kind ), eout );
      if ( c_ast_args( ast ) ) {
        FPUTS( " (", eout );
        for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
          if ( true_or_set( &comma ) )
            FPUTS( ", ", eout );
          c_ast_english( arg, eout );
        } // for
        FPUTC( ')', eout );
      }
      FPRINTF( eout, " %s ", L_RETURNING );
      c_ast_english( ast->as.func.ret_ast, eout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type ), eout );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout,
        "%s %s",
        c_type_name( ast->type ), ast->as.ecsu.ecsu_name
      );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_NAME:
      if ( ast->name )
        FPUTS( ast->name, eout );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
    case K_REFERENCE:
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind ), L_TO );
      c_ast_english( ast->as.ptr_ref.to_ast, eout );
      break;

    case K_POINTER_TO_MEMBER: {
      if ( ast->as.ptr_mbr.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_mbr.qualifier ) );
      char const *const type_name = c_type_name( ast->type );
      FPRINTF( eout,
        "%s %s %s %s ",
        L_POINTER, L_TO, L_MEMBER, L_OF
      );
      if ( *type_name )
        FPRINTF( eout, "%s ", type_name );
      FPRINTF( eout, "%s ", ast->as.ptr_mbr.class_name );
      c_ast_english( ast->as.ptr_mbr.of_ast, eout );
      break;
    }
  } // switch
}

void c_ast_gibberish_cast( c_ast_t const *ast, FILE *gout ) {
  c_ast_t dummy;
  c_ast_init( &dummy, K_NONE );
  bool space = false;
  return c_ast_gibberish_impl( ast, &dummy, G_CAST, &space, gout );
}

void c_ast_gibberish_declare( c_ast_t const *ast, FILE *gout ) {
  c_ast_t dummy;
  c_ast_init( &dummy, K_NONE );
  bool space = false;
  return c_ast_gibberish_impl( ast, &dummy, G_DECLARE, &space, gout );
}

void c_ast_json( c_ast_t const *ast, unsigned indent, char const *key0,
                 FILE *jout ) {
  if ( key0 && *key0 )
    PRINT_JSON( "\"%s\": {\n", key0 );
  else
    PRINT_JSON( "{\n" );

  if ( ast != NULL ) {
    ++indent;

    PRINT_JSON_KV( "kind", c_kind_name( ast->kind ) );
    FPUTS( ",\n", jout );
    PRINT_JSON_KV( "name", ast->name );
    FPUTS( ",\n", jout );
    PRINT_JSON_KV( "type", c_type_name( ast->type ) );

    bool comma = false;

    switch ( ast->kind ) {
      case K_BUILTIN:
      case K_NAME:
      case K_NONE:
        // nothing to do
        break;

      case K_ARRAY:
        PRINT_COMMA;
        PRINT_JSON( "\"size\": %d,\n", ast->as.array.size );
        c_ast_json( ast->as.array.of_ast, indent, "of_ast", jout );
        break;

      case K_BLOCK:                     // Apple extension
      case K_FUNCTION:
        PRINT_COMMA;
        PRINT_JSON( "\"args\": " );
        c_ast_list_json( &ast->as.func.args, indent, jout );
        FPUTS( ",\n", jout );
        c_ast_json( ast->as.func.ret_ast, indent, "ret_ast", jout );
        break;

      case K_ENUM_CLASS_STRUCT_UNION:
        PRINT_COMMA;
        PRINT_JSON_KV( "ecsu_name", ast->as.ecsu.ecsu_name );
        break;

      case K_POINTER_TO_MEMBER:
        PRINT_COMMA;
        PRINT_JSON_KV( "class_name", ast->as.ptr_mbr.class_name );
        FPUTC( '\n', jout );
        // no break;

      case K_POINTER:
      case K_REFERENCE:
        PRINT_COMMA;
        PRINT_JSON_KV( "qualifier", c_type_name( ast->as.ptr_ref.qualifier ) );
        FPRINTF( jout, ",\n" );
        c_ast_json( ast->as.ptr_ref.to_ast, indent, "to_ast", jout );
        break;
    } // switch

    FPUTC( '\n', jout );
    --indent;
  }

  PRINT_JSON( "}" );
}

void c_ast_list_append( c_ast_list_t *list, c_ast_t *ast ) {
  assert( list );
  if ( ast ) {
    if ( !list->head_ast )
      list->head_ast = list->tail_ast = ast;
    else {
      assert( list->tail_ast );
      assert( list->tail_ast->next == NULL );
      list->tail_ast->next = ast;
      list->tail_ast = ast;
    }
  }
}

c_ast_list_t c_ast_list_clone( c_ast_list_t const *list ) {
  c_ast_list_t clone;
  clone.head_ast = list ? c_ast_clone( list->head_ast ) : NULL;

  c_ast_t *tail = clone.head_ast;
  while ( tail && tail->next )
    tail = tail->next;
  clone.tail_ast = tail;

  return clone;
}

void c_ast_list_json( c_ast_list_t const *list, unsigned indent, FILE *jout ) {
  assert( list );
  if ( list->head_ast != NULL ) {
    FPUTS( "[\n", jout );
    bool comma = false;
    for ( c_ast_t const *arg = list->head_ast; arg; arg = arg->next ) {
      if ( true_or_set( &comma ) )
        FPUTS( ",\n", jout );
      c_ast_json( arg, indent + 1, NULL, jout );
    } // for
    FPUTC( '\n', jout );
    PRINT_JSON( "]" );
  } else {
    FPUTS( "[]", jout );
  }
}

char const* c_ast_name( c_ast_t *ast ) {
  c_ast_t *const found = c_ast_visit( ast, c_ast_visitor_name );
  return found ? found->name : NULL;
}

c_ast_t* c_ast_new( c_kind_t kind, YYLTYPE const *loc ) {
  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  c_ast_init( ast, kind );
  ast->loc = *loc;
  ast->gc_next = c_ast_head;
  c_ast_head = ast;
  ++c_ast_count;
  return ast;
}

char const* c_ast_take_name( c_ast_t *ast ) {
  c_ast_t *const found = c_ast_visit( ast, c_ast_visitor_name );
  if ( !found )
    return NULL;
  char const *const name = found->name;
  found->name = NULL;
  return name;
}

c_type_t c_ast_take_storage( c_ast_t *ast ) {
  c_type_t storage = T_NONE;
  c_ast_t *const found = c_ast_visit( ast, c_ast_vistor_builtin );
  if ( found ) {
    storage = found->type & T_MASK_STORAGE;
    found->type &= ~T_MASK_STORAGE;
  }
  return storage;
}

bool c_ast_take_typedef( c_ast_t *ast ) {
  c_ast_t *const found = c_ast_visit( ast, c_ast_vistor_builtin );
  if ( found && (found->type & T_TYPEDEF) ) {
    found->type &= ~T_TYPEDEF;
    return true;
  }
  return false;
}

c_ast_t* c_ast_visit( c_ast_t *ast, c_ast_visitor visitor ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast ) )
    return ast;

  switch ( ast->kind ) {
    case K_ARRAY:
    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
      // all c_ast_t* are at the same offset thanks to the union
      return c_ast_visit( ast->as.array.of_ast, visitor );
    case K_BUILTIN:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_NAME:
    case K_NONE:
      return NULL;
  } // switch
}

char const* c_kind_name( c_kind_t kind ) {
  switch ( kind ) {
    case K_NONE             : return "none";
    case K_ARRAY            : return "array";
    case K_BLOCK            : return "block";
    case K_BUILTIN          : return "built-in type";
    case K_FUNCTION         : return "function";
    case K_NAME             : return "name";
    case K_POINTER          : return "pointer";
    case K_POINTER_TO_MEMBER: return "pointer-to-member";
    case K_REFERENCE        : return "reference";

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( opt_lang >= LANG_CPP_MIN )
        return "enum, class, struct, or union";
      if ( opt_lang >= LANG_C_89 )
        return "enum, struct, or union";
      assert( opt_lang == LANG_C_KNR );
      return "struct or union";

    default:
      INTERNAL_ERR( "unexpected value (%d) for c_kind\n", (int)kind );
      return NULL;                      // suppress warning
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
