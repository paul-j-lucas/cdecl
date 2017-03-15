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

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static unsigned   c_ast_count;          // alloc'd but not yet freed
static c_ast_t   *c_ast_head;           // linked list of alloc'd objects

// local functions
static bool       c_ast_vistor_kind( c_ast_t*, void* );

////////// inline functions ///////////////////////////////////////////////////

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
 * Checks whether the given AST node is a parent node.
 *
 * @param ast The \c c_ast to check.
 * @return Returns \c true only if it is.
 */
static inline bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast->kind > 10;
}

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
 * A c_ast_visitor function used to find a name.
 *
 * @param ast The c_ast to check.
 * @param data Not used.
 * @return Returns \c true only if \a ast has a name.
 */
static bool c_ast_visitor_name( c_ast_t *ast, void *data ) {
  (void)data;
  return ast->name != NULL;
}

////////// extern functions ///////////////////////////////////////////////////

c_ast_t* c_ast_append_array( c_ast_t *ast, c_ast_t *array ) {
  assert( ast );
  assert( array );

  if ( ast->kind != K_ARRAY ) {
    assert( array->kind == K_ARRAY );
    assert( array->as.array.of_ast == NULL );
    //
    // We've reached the end of the array chain: make the new array be an array
    // of this AST node and return the array so the parent will now point to it
    // instead.
    //
    array->as.array.of_ast = ast;
    return array;
  }

  //
  // On the next-to-last recursive call, this sets this array to be an array of
  // the new array; for all prior recursive calls, it's a no-op.
  //
  ast->as.array.of_ast = c_ast_append_array( ast->as.array.of_ast, array );
  return ast;
}

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
      clone->as.parent.of_ast = c_ast_clone( ast->as.parent.of_ast );
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
  c_ast_t *const found = c_ast_visit( ast, c_ast_visitor_name, NULL );
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

void c_ast_set_parent( c_ast_t *child, c_ast_t *parent ) {
  assert( child );
  assert( parent );
  assert( c_ast_is_parent( parent ) );
  parent->as.parent.of_ast = child;
  child->parent = parent;
}

char const* c_ast_take_name( c_ast_t *ast ) {
  c_ast_t *const found = c_ast_visit( ast, c_ast_visitor_name, NULL );
  if ( !found )
    return NULL;
  char const *const name = found->name;
  found->name = NULL;
  return name;
}

c_type_t c_ast_take_storage( c_ast_t *ast ) {
  c_type_t storage = T_NONE;
  c_ast_t *const found =
    c_ast_visit( ast, c_ast_vistor_kind, (void*)K_BUILTIN );
  if ( found ) {
    storage = found->type & T_MASK_STORAGE;
    found->type &= ~T_MASK_STORAGE;
  }
  return storage;
}

bool c_ast_take_typedef( c_ast_t *ast ) {
  c_ast_t *const found =
    c_ast_visit( ast, c_ast_vistor_kind, (void*)K_BUILTIN );
  if ( found && (found->type & T_TYPEDEF) ) {
    found->type &= ~T_TYPEDEF;
    return true;
  }
  return false;
}

c_ast_t* c_ast_visit( c_ast_t *ast, c_ast_visitor visitor, void *data ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast, data ) )
    return ast;
  return c_ast_is_parent( ast ) ?
    c_ast_visit( ast->as.parent.of_ast, visitor, data ) : NULL;
}

c_ast_t* c_ast_visit_up( c_ast_t *ast, c_ast_visitor visitor, void *data ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast, data ) )
    return ast;
  return ast->parent ? c_ast_visit_up( ast->parent, visitor, data ) : NULL;
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
