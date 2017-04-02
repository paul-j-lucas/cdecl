/*
**      cdecl -- C gibberish translator
**      src/ast.c
**
**      Paul J. Lucas
*/

// local
#include "config.h"                     /* must go first */
#define CDECL_AST_INLINE _GL_EXTERN_INLINE
#include "ast.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <string.h>                     /* for memset(3) */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static unsigned   c_ast_count;          // alloc'd but not yet freed
static c_ast_t   *c_ast_gc_head;        // linked list of alloc'd objects

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

#ifndef NDEBUG
/**
 * Checks the AST for a cycle.
 *
 * @param ast The AST node to begin at.
 * @return Returns \c true only if there is a cycle.
 */
static bool c_ast_has_cycle( c_ast_t const *ast ) {
  assert( ast != NULL );
  for ( c_ast_t const *const start_ast = ast; ast->parent; ) {
    ast = ast->parent;
    if ( ast == start_ast )
      return true;
  } // for
  return false;
}
#endif /* NDEBUG */

////////// extern functions ///////////////////////////////////////////////////

void c_ast_cleanup( void ) {
  if ( c_ast_count > 0 )
    INTERNAL_ERR( "number of c_ast objects (%u) > 0\n", c_ast_count );
}

void c_ast_gc( void ) {
  for ( c_ast_t *p = c_ast_gc_head; p; ) {
    c_ast_t *const next = p->gc_next;
    c_ast_free( p );
    p = next;
  } // for
  c_ast_gc_head = NULL;
}

void c_ast_list_append( c_ast_list_t *list, c_ast_t *ast ) {
  assert( list != NULL );
  if ( ast ) {
    assert( ast->next == NULL );
    if ( !list->head_ast ) {
      assert( list->tail_ast == NULL );
      list->head_ast = list->tail_ast = ast;
    } else {
      assert( list->tail_ast != NULL );
      assert( list->tail_ast->next == NULL );
      list->tail_ast->next = ast;
      list->tail_ast = ast;
    }
  }
}

c_ast_t* c_ast_new( c_kind_t kind, unsigned depth, YYLTYPE const *loc ) {
  assert( loc != NULL );
  static unsigned next_id;

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  memset( ast, 0, sizeof( c_ast_t ) );

  ast->depth = depth;
  ast->id = ++next_id;
  ast->kind = kind;
  ast->loc = *loc;
  ast->gc_next = c_ast_gc_head;

  c_ast_gc_head = ast;
  ++c_ast_count;
  return ast;
}

c_ast_t* c_ast_root( c_ast_t *ast ) {
  assert( ast != NULL );
  while ( ast->parent )
    ast = ast->parent;
  return ast;
}

void c_ast_set_parent( c_ast_t *child, c_ast_t *parent ) {
  assert( child != NULL );
  assert( parent != NULL );
  assert( c_ast_is_parent( parent ) );

  child->parent = parent;
  parent->as.parent.of_ast = child;

  assert( !c_ast_has_cycle( child ) );
}

c_ast_t* c_ast_visit_down( c_ast_t *ast, c_ast_visitor visitor, void *data ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast, data ) )
    return ast;
  if ( !c_ast_is_parent( ast ) )
    return NULL;
  return c_ast_visit_down( ast->as.parent.of_ast, visitor, data );
}

c_ast_t* c_ast_visit_up( c_ast_t *ast, c_ast_visitor visitor, void *data ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast, data ) )
    return ast;
  if ( !ast->parent )
    return NULL;
  return c_ast_visit_up( ast->parent, visitor, data );
}

bool c_ast_vistor_kind( c_ast_t *ast, void *data ) {
  c_kind_t const kind = REINTERPRET_CAST( c_kind_t, data );
  return (ast->kind & kind) != 0;
}

bool c_ast_visitor_name( c_ast_t *ast, void *data ) {
  (void)data;
  return ast->name != NULL;
}

bool c_ast_vistor_type( c_ast_t *ast, void *data ) {
  c_type_t const type = REINTERPRET_CAST( c_type_t, data );
  return (ast->type & type) != 0;
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
    case K_RVALUE_REFERENCE : return "rvalue reference";
    case K_VARIADIC         : return "variadic";

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
