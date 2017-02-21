/*
**      cdecl -- C gibberish translator
**      src/ast.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <string.h>                     /* for memset(3) */
#include <sysexits.h>

#define INDENT_SPACING            "  "

#define PRINT_COMMA \
  BLOCK( if ( !comma ) { FPUTS( ",\n", fout ); comma = true; } )

#define PRINT_JSON(...) \
  BLOCK( print_indent( fout, indent ); FPRINTF( fout, __VA_ARGS__ ); )

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static unsigned c_ast_count;            // allocated but not freed

////////// local functions ////////////////////////////////////////////////////

static void print_indent( FILE *fout, unsigned indent ) {
  for ( ; indent > 0; --indent )
    FPUTS( INDENT_SPACING, fout );
}

static void c_ast_json_impl( c_ast_t const *ast, char const *key0,
                             unsigned indent, FILE *fout ) {
  if ( ast == NULL )
    return;

  if ( key0 && *key0 ) {
    PRINT_JSON( "\"%s\": ", key0 );
    FPRINTF( fout, "{\n" );
  } else {
    PRINT_JSON( "{\n" );
  }
  ++indent;

  PRINT_JSON( "\"kind\": \"%s\",\n", c_kind_name( ast->kind ) );
  PRINT_JSON( "\"name\": \"%s\"", ast->name ? ast->name : ""  );

  bool comma = false;

  switch ( ast->kind ) {
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_NAME:
    case K_NONE:
      break;

    case K_ARRAY:
      PRINT_COMMA;
      PRINT_JSON( "\"size\": %d,\n", ast->as.array.size );
      c_ast_json_impl( ast->as.array.of_ast, "of_ast", indent + 1, fout );
      break;

    case K_BUILTIN:
      PRINT_COMMA;
      PRINT_JSON( "\"type\": \"%s\"", c_type_name( ast->as.type ) );
      break;

    case K_BLOCK:
      PRINT_COMMA;
      PRINT_JSON( "\"type\": \"%s\",\n", c_type_name( ast->as.block.type ) );
      // no break;
    case K_FUNCTION:
      PRINT_COMMA;
      PRINT_JSON( "\"args\": [\n" );
      comma = false;
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; arg = arg->next ) {
        if ( comma )
          FPRINTF( fout, ", " );
        else
          comma = true;
        c_ast_json_impl( arg, NULL, indent + 1, fout );
      } // for
      FPUTC( '\n', fout );
      PRINT_JSON( "],\n" );
      c_ast_json_impl( ast->as.func.ret_ast, "ret_ast", indent + 1, fout );
      break;

    case K_PTR_TO_MEMBER:
      PRINT_COMMA;
      PRINT_JSON( "\"class_name\": \"%s\"\n", ast->as.ptr_mbr.class_name );
      // no break;
    case K_POINTER:
    case K_REFERENCE:
      PRINT_COMMA;
      PRINT_JSON(
        "\"qualifier\": \"%s\",\n", c_type_name( ast->as.ptr_ref.qualifier )
      );
      c_ast_json_impl( ast->as.ptr_ref.to_ast, "to_ast", indent + 1, fout );
      break;
  } // switch

  FPUTC( '\n', fout );
  --indent;
  PRINT_JSON( "}" );
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
    case K_PTR_TO_MEMBER:
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

void c_ast_english( c_ast_t const *ast, FILE *fout ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_NONE:
      break;

    case K_ARRAY:
      FPRINTF( fout, "%s ", L_ARRAY );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( fout, "%d ", ast->as.array.size );
      FPRINTF( fout, "%s ", L_OF );
      c_ast_english( ast->as.array.of_ast, fout );
      break;

    case K_BLOCK:
      // TODO

    case K_BUILTIN:
      FPUTS( c_type_name( ast->as.type ), fout );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPUTS( c_type_name( ast->as.type ), fout );
      if ( ast->name )
        FPRINTF( fout, " %s", ast->name );
      break;

    case K_FUNCTION:
      FPRINTF( fout, "%s (", L_FUNCTION );
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; arg = arg->next )
        c_ast_english( arg, fout );
      FPRINTF( fout, ") %s ", L_RETURNING );
      c_ast_english( ast->as.func.ret_ast, fout );
      break;

    case K_PTR_TO_MEMBER:
      if ( ast->as.ptr_mbr.qualifier )
        FPRINTF( fout, "%s ", c_type_name( ast->as.ptr_mbr.qualifier ) );
      FPRINTF( fout,
        "%s %s %s %s %s %s ",
        L_POINTER, L_TO, L_MEMBER, L_OF, L_CLASS, ast->as.ptr_mbr.class_name
      );
      c_ast_english( ast->as.ptr_mbr.of_ast, fout );
      break;

    case K_NAME:
      if ( ast->name )
        FPUTS( ast->name, fout );
      break;

    case K_POINTER:
    case K_REFERENCE:
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( fout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      FPRINTF( fout,
        "%s %s ", (ast->kind == K_POINTER ? L_POINTER : L_REFERENCE), L_TO
      );
      c_ast_english( ast->as.ptr_ref.to_ast, fout );
      break;
  } // switch
}

void c_ast_json( c_ast_t const *ast, char const *key0, FILE *fout ) {
  c_ast_json_impl( ast, key0, 0, fout );
  FPUTC( '\n', fout );
}

void c_ast_free( c_ast_t *ast ) {
  if ( ast == NULL )
    return;
  //assert( c_ast_count > 0 );

  --c_ast_count;

  if ( ast->name )
    FREE( ast->name );

  switch ( ast->kind ) {
    case K_NONE:
      break;
    case K_ARRAY:
      if ( ast->as.array.of_ast )
        c_ast_free( ast->as.array.of_ast );
      break;
    case K_BUILTIN:
      // nothing to do
      break;
    case K_ENUM_CLASS_STRUCT_UNION:
      // nothing to do
      break;
    case K_BLOCK:
    case K_FUNCTION:
      for ( c_ast_t *arg = ast->as.func.args.head_ast; arg; ) {
        c_ast_t *const next = arg->next;
        c_ast_free( arg );
        arg = next;
      }
      break;
    case K_PTR_TO_MEMBER:
      FREE( ast->as.ptr_mbr.class_name );
      c_ast_free( ast->as.ptr_mbr.of_ast );
      break;
    case K_NAME:
      // nothing to do
      break;
    case K_POINTER:
    case K_REFERENCE:
      c_ast_free( ast->as.ptr_ref.to_ast );
      break;
  } // switch
}

char const* c_ast_name( c_ast_t const *ast ) {
  if ( ast == NULL )
    return NULL;

  if ( ast->name )
    return ast->name;

  switch ( ast->kind ) {
    case K_ARRAY:
      return c_ast_name( ast->as.array.of_ast );
    case K_PTR_TO_MEMBER:
      return NULL;
    case K_POINTER:
    case K_REFERENCE:
      return c_ast_name( ast->as.ptr_ref.to_ast );
    default:
      return NULL;
  } // switch
}

c_ast_t* c_ast_new( c_kind_t kind ) {
  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  memset( ast, 0, sizeof( c_ast_t ) );
  ast->kind = kind;
  ++c_ast_count;
  return ast;
}

char const* c_kind_name( c_kind_t kind ) {
  switch ( kind ) {
    case K_NONE         : return "none";
    case K_ARRAY        : return "array";
    case K_BLOCK        : return "block";
    case K_BUILTIN      : return "built-in type";
    case K_FUNCTION     : return "function";
    case K_NAME         : return "name";
    case K_POINTER      : return "pointer";
    case K_PTR_TO_MEMBER: return "pointer-to-member";
    case K_REFERENCE    : return "reference";

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( opt_lang >= LANG_CPP )
        return "enum, class, struct, or union";
      if ( opt_lang >= LANG_C_89 )
        return "enum, struct, or union";
      return "struct or union";

    default:
      INTERNAL_ERR( "unexpected value (%d) for c_kind\n", (int)kind );
      return NULL;                      // suppress warning
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
