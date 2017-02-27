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
static unsigned c_ast_count;            // allocated but not freed

////////// local functions ////////////////////////////////////////////////////

static void print_indent( unsigned indent, FILE *jout ) {
  FPRINTF( jout, "%*s", (int)(indent * JSON_INDENT), "" );
}

////////// extern functions ///////////////////////////////////////////////////

#if 0
void c_ast_check( c_ast_t *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_NONE:
      break;

    case K_ARRAY: {
      c_ast const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast.kind ) {
        case K_BUILTIN:
          if ( of_ast.type & T_VOID )
            /* complain */;
          break;
        case K_FUNCTION:
          /* complain */;
          break;
      } // switch
      break;
    }

    case K_BLOCK:
      // TODO
      break;
    case K_BUILTIN:
      // nothing to do
      break;
    case K_ENUM_CLASS_STRUCT_UNION:
      // TODO
      break;
    case K_FUNCTION:
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; arg = arg->next )
        /* TODO */;
      break;
    case K_POINTER_TO_MEMBER:
      break;
    case K_NAME:
      break;
    case K_POINTER:
    case K_REFERENCE:
      break;
  } // switch
}
#endif

void c_ast_cleanup( void ) {
  if ( c_ast_count > 0 )
    INTERNAL_ERR( "number of c_ast objects (%u) > 0\n", c_ast_count );
}

c_ast_t* c_ast_clone( c_ast_t const *ast ) {
  if ( ast == NULL )
    return NULL;
  c_ast_t *const clone = c_ast_new( ast->kind );
  clone->name = ast->name ? check_strdup( ast->name ) : NULL;
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
    case K_BUILTIN:
    case K_ENUM_CLASS_STRUCT_UNION:
      clone->as.builtin.type = ast->as.builtin.type;
      break;
    case K_POINTER_TO_MEMBER:
      clone->as.ptr_mbr.type = ast->as.ptr_mbr.type;
      clone->as.ptr_mbr.class_name = ast->as.ptr_mbr.class_name ?
        check_strdup( ast->as.ptr_mbr.class_name ) : NULL;
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
      FPRINTF( eout, "%s (", L_BLOCK );
      for ( c_ast_t *arg = ast->as.block.args.head_ast; arg; arg = arg->next )
        c_ast_english( arg, eout );
      FPRINTF( eout, ") %s ", L_RETURNING );
      c_ast_english( ast->as.block.ret_ast, eout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->as.builtin.type ), eout );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPUTS( c_type_name( ast->as.ecsu.type ), eout );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_NAME:
      if ( ast->name )
        FPUTS( ast->name, eout );
      break;

    case K_FUNCTION:
      FPRINTF( eout, "%s (", L_FUNCTION );
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; arg = arg->next )
        c_ast_english( arg, eout );
      FPRINTF( eout, ") %s ", L_RETURNING );
      c_ast_english( ast->as.func.ret_ast, eout );
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

void c_ast_free( c_ast_t *ast ) {
  if ( ast == NULL )
    return;
  assert( c_ast_count > 0 );
  --c_ast_count;

  FREE( ast->name );

  switch ( ast->kind ) {
    case K_BLOCK:
    case K_FUNCTION:
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; ) {
        c_ast_t *const next = arg->next;
        c_ast_free( arg );
        arg = next;
      } // for
      // no break;
    case K_ARRAY:
    case K_POINTER:
    case K_REFERENCE:
      c_ast_free( ast->as.array.of_ast );
      break;

    case K_POINTER_TO_MEMBER:
      FREE( ast->as.ptr_mbr.class_name );
      c_ast_free( ast->as.ptr_mbr.of_ast );
      break;

    case K_BUILTIN:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_NAME:
    case K_NONE:
      // nothing to do
      break;
  } // switch
}

void c_ast_gibberish( c_ast_t const *ast, FILE *gout ) {
  if ( ast == NULL )
    return;

  switch ( ast->kind ) {
    case K_NONE:
      break;
    case K_ARRAY:
      // TODO
      break;
    case K_BLOCK:
      // TODO
      break;
    case K_BUILTIN:
      // TODO
      break;
    case K_ENUM_CLASS_STRUCT_UNION:
      // TODO
      break;
    case K_NAME:
      // TODO
      break;
    case K_FUNCTION:
      // TODO
      break;
    case K_POINTER_TO_MEMBER:
      // TODO
      break;
    case K_POINTER:
      // TODO
      break;
    case K_REFERENCE:
      // TODO
      break;
  } // switch
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

    bool comma = false;

    switch ( ast->kind ) {
      case K_ENUM_CLASS_STRUCT_UNION:
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
        PRINT_COMMA;
        PRINT_JSON_KV( "type", c_type_name( ast->as.block.type ) );
        FPUTC( '\n', jout );
        // no break;
      case K_FUNCTION:
        PRINT_COMMA;
        if ( ast->as.func.args.head_ast != NULL ) {
          PRINT_JSON( "\"args\": [\n" );
          comma = false;
          for ( c_ast_t *arg = ast->as.func.args.head_ast; arg;
                arg = arg->next ) {
            if ( comma )
              FPUTS( ", ", jout );
            else
              comma = true;
            c_ast_json( arg, indent + 1, NULL, jout );
          } // for
          FPUTC( '\n', jout );
          PRINT_JSON( "],\n" );
        } else {
          PRINT_JSON( "\"args\": []\n" );
        }
        c_ast_json( ast->as.func.ret_ast, indent, "ret_ast", jout );
        break;

      case K_POINTER_TO_MEMBER:
        PRINT_COMMA;
        PRINT_JSON_KV( "class_name", ast->as.ptr_mbr.class_name );
        FPUTC( '\n', jout );
        PRINT_JSON_KV( "type", c_type_name( ast->as.ptr_mbr.type ) );
        FPUTC( '\n', jout );
        // no break;
      case K_POINTER:
      case K_REFERENCE:
        PRINT_COMMA;
        PRINT_JSON_KV( "qualifier", c_type_name( ast->as.ptr_ref.qualifier ) );
        FPUTC( '\n', jout );
        c_ast_json( ast->as.ptr_ref.to_ast, indent, "to_ast", jout );
        break;
    } // switch

    FPUTC( '\n', jout );
    --indent;
  }

  PRINT_JSON( "}" );
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
  ++c_ast_count;
  return ast;
}

c_ast_t* c_ast_pop( c_ast_t **phead ) {
  assert( phead );
  assert( *phead );
  c_ast_t *const popped = (*phead);
  (*phead) = popped->next;
  popped->next = NULL;
  return popped;
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
  c_ast_free( ast );
  return name;
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
