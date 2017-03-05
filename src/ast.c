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

// local variable definitions
static unsigned   c_ast_count;          // alloc'd but not yet freed
static c_ast_t   *c_ast_head;           // linked list of alloc'd objects

////////// local functions ////////////////////////////////////////////////////

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

static void print_indent( unsigned indent, FILE *out ) {
  FPRINTF( out, "%*s", (int)(indent * JSON_INDENT), "" );
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check( c_ast_t const *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_NONE:
      break;

    case K_ARRAY: {
      c_ast_t const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast->kind ) {
        case K_BUILTIN:
          if ( of_ast->as.builtin.type & T_VOID ) {
            c_error( "array of void", "array of pointer to void" );
            return false;
          }
          if ( of_ast->as.builtin.type & T_REGISTER ) {
            c_error( "register array", NULL );
            return false;
          }
          break;
        case K_FUNCTION:
          c_error( "array of function", "array of pointer to function" );
          return false;
        default:
          /* suppress warning */;
      } // switch
      break;
    }

    case K_BLOCK:
      // TODO
      break;

    case K_BUILTIN: {
      c_type_t const type = ast->as.builtin.type;
      if ( type & T_VOID ) {
        c_error( "variable of void", "pointer to void" );
        return false;
      }
      break;
    }

    case K_ENUM_CLASS_STRUCT_UNION:
      // nothing to do
      break;

    case K_FUNCTION: {
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      switch ( ret_ast->kind ) {
        case K_BUILTIN:
          if ( ret_ast->as.builtin.type & T_REGISTER ) {
            c_error( "register function", NULL );
            return false;
          }
        default:
          /* suppress warning */;
      } // switch
      break;
    }

    case K_POINTER_TO_MEMBER:
      break;
    case K_NAME:
      break;
    case K_POINTER:
      break;

    case K_REFERENCE: {
      c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
      switch ( to_ast->kind ) {
        case K_BUILTIN:
          if ( to_ast->as.builtin.type & T_VOID ) {
            c_error( "referece to void", "pointer to void" );
            return false;
          }
          break;
        default:
          /* suppress warning */;
      } // switch
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
  c_ast_t *const clone = c_ast_new( ast->kind );
  clone->name = check_strdup( ast->name );
  clone->next = c_ast_clone( ast->next );

  switch ( ast->kind ) {
    case K_NONE:
    case K_NAME:
      break;
    case K_ARRAY:
      clone->as.array.of_ast = c_ast_clone( ast->as.array.of_ast );
      clone->as.array.size = ast->as.array.size;
      break;
    case K_BLOCK:
      clone->as.block.args = c_ast_list_clone( &ast->as.block.args );
      clone->as.block.type = ast->as.block.type;
      // no break;
    case K_FUNCTION:
      clone->as.func.ret_ast = c_ast_clone( ast->as.func.ret_ast );
      break;
    case K_ENUM_CLASS_STRUCT_UNION:
      clone->as.ecsu.ecsu_name = check_strdup( ast->as.ecsu.ecsu_name );
      // no break;
    case K_BUILTIN:
      clone->as.builtin.type = ast->as.builtin.type;
      break;
    case K_POINTER_TO_MEMBER:
      clone->as.ptr_mbr.type = ast->as.ptr_mbr.type;
      clone->as.ptr_mbr.class_name = check_strdup( ast->as.ptr_mbr.class_name );
      // no break;
    case K_POINTER:
    case K_REFERENCE:
      clone->as.ptr_ref.qualifier = ast->as.ptr_ref.qualifier;
      clone->as.ptr_ref.to_ast = c_ast_clone( ast->as.ptr_ref.to_ast );
      break;
  } // switch

  return clone;
}

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  if ( ast == NULL )
    return;

  bool comma = false;

  switch ( ast->kind ) {
    case K_NONE:
      break;

    case K_ARRAY:
      FPRINTF( eout, "%s ", L_ARRAY );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( eout, "%d ", ast->as.array.size );
      FPRINTF( eout, "%s ", L_OF );
      c_ast_english( ast->as.array.of_ast, eout );
      break;

    case K_BLOCK:
    case K_FUNCTION:
      if ( ast->as.func.type )
        FPRINTF( eout, "%s ", c_type_name( ast->as.func.type ) );
      FPRINTF( eout, "%s (", c_kind_name( ast->kind ) );
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; arg = arg->next ) {
        if ( true_or_set( &comma ) )
          FPUTS( ", ", eout );
        c_ast_english( arg, eout );
      }
      FPRINTF( eout, ") %s ", L_RETURNING );
      c_ast_english( ast->as.func.ret_ast, eout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->as.builtin.type ), eout );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout,
        "%s %s",
        c_type_name( ast->as.builtin.type ), ast->as.ecsu.ecsu_name
      );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_NAME:
      if ( ast->name )
        FPUTS( ast->name, eout );
      break;

    case K_POINTER:
    case K_REFERENCE:
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind ), L_TO );
      c_ast_english( ast->as.ptr_ref.to_ast, eout );
      break;

    case K_POINTER_TO_MEMBER:
      if ( ast->as.ptr_mbr.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_mbr.qualifier ) );
      FPRINTF( eout,
        "%s %s %s %s %s %s ",
        L_POINTER, L_TO, L_MEMBER, L_OF, c_type_name( ast->as.ptr_mbr.type ),
        ast->as.ptr_mbr.class_name
      );
      c_ast_english( ast->as.ptr_mbr.of_ast, eout );
      break;
  } // switch
}

c_ast_t* c_ast_find_name( c_ast_t *ast ) {
  if ( ast == NULL )
    return NULL;
  if ( ast->name )
    return ast;

  switch ( ast->kind ) {
    case K_ARRAY:
    case K_BLOCK:
    case K_FUNCTION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
      return c_ast_find_name( ast->as.array.of_ast );
    case K_BUILTIN:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_NAME:
    case K_NONE:
      return NULL;
  } // switch
}

void c_ast_gibberish_impl( c_ast_t const *ast, c_ast_t const *parent,
                           FILE *gout ) {
  if ( ast == NULL )
    return;

  bool comma = false;

  switch ( ast->kind ) {
    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_ARRAY:
      c_ast_gibberish( ast->as.array.of_ast, gout );
      FPUTC( '[', gout );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( gout, "%d", ast->as.array.size );
      FPUTC( ']', gout );
      break;

    case K_BLOCK:
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->as.builtin.type ), gout );
      if ( ast->name )
        FPRINTF( gout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( gout,
        "%s %s", c_kind_name( ast->kind ), ast->as.ecsu.ecsu_name
      );
      break;

    case K_NAME:
      break;

    case K_FUNCTION:
      c_ast_gibberish( ast->as.func.ret_ast, gout );
      if ( parent && parent->kind == K_POINTER ) {
        FPRINTF( gout, " (*" );
        if ( parent->as.ptr_ref.qualifier )
          FPRINTF( gout, "%s ", c_type_name( parent->as.ptr_ref.qualifier ) );
        FPUTS( parent->name, gout );
      } else {
        FPUTS( ast->name, gout );
      }
      if ( parent && parent->kind == K_POINTER ) {
        FPUTC( ')', gout );
      }
      FPUTC( '(', gout );
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; arg = arg->next ) {
        if ( true_or_set( &comma ) )
          FPUTS( ", ", gout );
          c_ast_gibberish( arg, gout );
      } // for
      FPUTC( ')', gout );
      break;

    case K_POINTER_TO_MEMBER:
      break;

    case K_POINTER: {
      c_ast_t const *const to = ast->as.ptr_ref.to_ast;
      if ( to->kind == K_FUNCTION ) {
        c_ast_gibberish_impl( to, ast, gout );
      } else {
        c_ast_gibberish( to, gout );
        FPUTS( " *", gout );
        if ( ast->as.ptr_ref.qualifier )
          FPRINTF( gout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
        if ( ast->name )
          FPUTS( ast->name, gout );
      }
      break;
    }

    case K_REFERENCE:
      c_ast_gibberish( ast->as.ptr_ref.to_ast, gout );
      FPUTS( " *", gout );
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( gout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      if ( ast->name )
        FPUTS( ast->name, gout );
      break;
  } // switch
}

void c_ast_gibberish( c_ast_t const *ast, FILE *gout ) {
  return c_ast_gibberish_impl( ast, NULL, gout );
}

#if 0
void c_ast_give_name( c_ast_t *ast, char const *name ) {
  assert( ast );
  assert( name );

  switch ( ast->kind ) {
    case K_NONE:
      assert( ast->kind != K_NONE );
      
  } // switch
}
#endif

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

    bool comma = false;

    switch ( ast->kind ) {
      case K_NAME:
      case K_NONE:
        break;

      case K_ARRAY:
        PRINT_COMMA;
        PRINT_JSON( "\"size\": %d,\n", ast->as.array.size );
        c_ast_json( ast->as.array.of_ast, indent, "of_ast", jout );
        break;

      case K_BUILTIN:
        PRINT_COMMA;
        PRINT_JSON_KV( "type", c_type_name( ast->as.builtin.type ) );
        break;

      case K_BLOCK:
      case K_FUNCTION:
        PRINT_COMMA;
        PRINT_JSON_KV( "type", c_type_name( ast->as.func.type ) );
        FPRINTF( jout, ",\n" );
        if ( ast->as.func.args.head_ast != NULL ) {
          PRINT_JSON( "\"args\": [\n" );
          comma = false;
          for ( c_ast_t *arg = ast->as.func.args.head_ast; arg;
                arg = arg->next ) {
            if ( true_or_set( &comma ) )
              FPUTS( ", ", jout );
            c_ast_json( arg, indent + 1, NULL, jout );
          } // for
          FPUTC( '\n', jout );
          PRINT_JSON( "],\n" );
        } else {
          PRINT_JSON( "\"args\": [],\n" );
        }
        c_ast_json( ast->as.func.ret_ast, indent, "ret_ast", jout );
        break;

      case K_ENUM_CLASS_STRUCT_UNION:
        PRINT_COMMA;
        PRINT_JSON_KV( "ecsu_name", ast->as.ecsu.ecsu_name );
        break;

      case K_POINTER_TO_MEMBER:
        PRINT_COMMA;
        PRINT_JSON_KV( "class_name", ast->as.ptr_mbr.class_name );
        FPRINTF( jout, ",\n" );
        PRINT_JSON_KV( "type", c_type_name( ast->as.ptr_mbr.type ) );
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

char const* c_ast_name( c_ast_t *ast ) {
  c_ast_t *const found = c_ast_find_name( ast );
  return found ? found->name : NULL;
}

c_ast_t* c_ast_new( c_kind_t kind ) {
  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  memset( ast, 0, sizeof( c_ast_t ) );
  ast->kind = kind;
  ast->gc_next = c_ast_head;
  c_ast_head = ast;
  ++c_ast_count;
  return ast;
}

c_ast_t* c_ast_pop( c_ast_t **phead ) {
  assert( phead );
  if ( *phead ) {
    c_ast_t *const popped = (*phead);
    (*phead) = popped->next;
    popped->next = NULL;
    return popped;
  }
  return NULL;
}

void c_ast_push( c_ast_t **phead, c_ast_t *new_ast ) {
  assert( phead );
  assert( new_ast );
  assert( new_ast->next == NULL );
  new_ast->next = (*phead);
  (*phead) = new_ast;
}

char const* c_ast_take_name( c_ast_t *ast ) {
  c_ast_t *const found = c_ast_find_name( ast );
  if ( !found )
    return NULL;
  char const *const name = found->name;
  found->name = NULL;
  return name;
}

c_type_t c_ast_take_storage( c_ast_t *ast ) {
  c_type_t storage = T_NONE;
  switch ( ast->kind ) {
    case K_BUILTIN:
      storage = ast->as.builtin.type & T_MASK_STORAGE;
      ast->as.builtin.type &= ~T_MASK_STORAGE;
      break;
    case K_POINTER:
    case K_REFERENCE:
      return c_ast_take_storage( ast->as.ptr_ref.to_ast );
    default:
      /* suppress warning */;
  } // switch
  return storage;
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
