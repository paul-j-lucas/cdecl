/*
**      cdecl -- C gibberish translator
**      src/typedefs.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
 * Defines functions for looking up C/C++ typedef declarations.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "options.h"
#include "red_black.h"
#include "typedefs.h"
#include "util.h"

// standard
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct rb_visitor_data {
  c_typedef_visitor_t visitor;          // caller's visitor function
  void *data;                           // caller's optional data
};
typedef struct rb_visitor_data rb_visitor_data_t;

// local variable definitions
static rb_tree_t *typedefs;             // global set of typedef declarations
static bool       user_defined;         // are new typedefs used-defined?

///////////////////////////////////////////////////////////////////////////////

static char const *const BUILTIN_TYPEDEFS[] = {
  "typedef          long   ptrdiff_t",
  "typedef          long  ssize_t",
  "typedef unsigned long   size_t",

  "typedef          long   intmax_t",
  "typedef          long   intptr_t",
  "typedef unsigned long  uintmax_t",
  "typedef unsigned long  uintptr_t",

  "typedef          char   int8_t",
  "typedef          short  int16_t",
  "typedef          int    int32_t",
  "typedef          long   int64_t",
  "typedef unsigned char  uint8_t",
  "typedef unsigned short uint16_t",
  "typedef unsigned int   uint32_t",
  "typedef unsigned long  uint64_t",

  "typedef          char   int_fast8_t",
  "typedef          short  int_fast16_t",
  "typedef          int    int_fast32_t",
  "typedef          long   int_fast64_t",
  "typedef unsigned char  uint_fast8_t",
  "typedef unsigned short uint_fast16_t",
  "typedef unsigned int   uint_fast32_t",
  "typedef unsigned long  uint_fast64_t",

  "typedef          char   int_least8_t",
  "typedef          short  int_least16_t",
  "typedef          int    int_least32_t",
  "typedef          long   int_least64_t",
  "typedef unsigned char  uint_least8_t",
  "typedef unsigned short uint_least16_t",
  "typedef unsigned int   uint_least32_t",
  "typedef unsigned long  uint_least64_t",

  NULL
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Comparison function for c_typedef data used by the red-black tree.
 *
 * @param data_i A pointer to data.
 * @param data_j A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the \c typedef name pointed to by \a data_i is less than, equal
 * to, or greater than the \c typedef name pointed to by \a data_j.
 */
static int c_typedef_cmp( void const *data_i, void const *data_j ) {
  c_typedef_t const *const ti = REINTERPRET_CAST( c_typedef_t const*, data_i );
  c_typedef_t const *const tj = REINTERPRET_CAST( c_typedef_t const*, data_j );
  return strcmp( ti->type_name, tj->type_name );
}

/**
 * Red-black tree node free function for c_typedef data used by the red-black
 * tree.
 *
 * @param data A pointer to the data to free.
 */
static void c_typedef_free( void *data ) {
  c_typedef_t *const t = REINTERPRET_CAST( c_typedef_t*, data );
  FREE( t->type_name );
  c_ast_free( CONST_CAST( c_ast_t*, t->ast ) );
}

/**
 * Creates a new c_typedef.
 *
 * @param type_name The type name.
 * @param ast The c_ast of the type.
 * @return Returns said c_typedef.
 */
static c_typedef_t* c_typedef_new( char const *type_name, c_ast_t const *ast ) {
  c_typedef_t *const t = MALLOC( c_typedef_t, 1 );
  t->type_name = type_name;
  t->ast = ast;
  t->user_defined = user_defined;
  return t;
}

/**
 * Parses a built-in \c typedef declaration.
 *
 * @param types An array of pointers to typedef strings.  The last element must
 * be NULL.
 */
static void c_typedef_parse_builtins( char const *const types[] ) {
  extern bool parse_string( char const*, size_t );
  for ( char const *const *ptype = types; *ptype; ++ptype ) {
    bool const ok = parse_string( *ptype, 0 );
    assert( ok );
  } // for
}

/**
 * Red-black tree visitor function that forwards to the c_typedef_visitor_t
 * function.
 *
 * @param node_data A pointer to the node's data.
 * @param aux_data Optional data passed to to the visitor.
 * @return Returning \c true will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
static bool rb_visitor( void *node_data, void *aux_data ) {
  c_typedef_t const *const t =
    REINTERPRET_CAST( c_typedef_t const*, node_data );

  rb_visitor_data_t const *const vd =
    REINTERPRET_CAST( rb_visitor_data_t*, aux_data );

  return vd->visitor( t, vd->data );
}

////////// extern functions ///////////////////////////////////////////////////

bool c_typedef_add( char const *type_name, c_ast_t const *ast ) {
  assert( type_name != NULL );
  assert( ast != NULL );

  c_typedef_t *const new_t = c_typedef_new( type_name, ast );
  rb_node_t const *const old_rb = rb_tree_insert( typedefs, new_t );
  if ( old_rb == NULL )                 // type_name didn't exist
    return true;

  //
  // A typedef having the same name already exists.  In C, multiple typedef
  // declarations having the same name are allowed only if the types are
  // equivalent:
  //
  //      typedef int T;
  //      typedef int T;              // OK
  //      typedef double T;           // error: types aren't equivalent
  //
  c_typedef_t const *const old_t = rb_type_data( c_typedef_t const*, old_rb );
  bool const is_equiv = c_ast_equiv( ast, old_t->ast );
  if ( is_equiv ) {
    //
    // If the types are equivalent, take ownership of type_name and AST, but we
    // don't need the duplicate c_typedef, so free it via c_typedef_free() that
    // will free type_name and AST.
    //
    c_typedef_free( new_t );
  }
  else {
    //
    // If the types are not equivalent, don't take ownership, but we still need
    // to free the c_typedef struct itself but not either type_name or AST,
    // hence a plain FREE().
    //
    FREE( new_t );
  }
  return is_equiv;
}

void c_typedef_cleanup( void ) {
  rb_tree_free( typedefs, &c_typedef_free );
  typedefs = NULL;
}

c_typedef_t const* c_typedef_find( char const *type_name ) {
  c_typedef_t rb_find;
  rb_find.type_name = type_name;
  rb_node_t const *const rb_found = rb_tree_find( typedefs, &rb_find );
  return rb_found ? rb_type_data( c_typedef_t const*, rb_found ) : NULL;
}

void c_typedef_init( void ) {
  assert( typedefs == NULL );
  typedefs = rb_tree_new( &c_typedef_cmp );

  if ( opt_typedefs ) {
#ifdef ENABLE_CDECL_DEBUG
    // Temporarily turn off debug output for built-in typedefs.
    bool const prev_debug = opt_debug;
    opt_debug = false;
#endif /* ENABLE_CDECL_DEBUG */

    user_defined = false;
    c_typedef_parse_builtins( BUILTIN_TYPEDEFS );

#ifdef ENABLE_CDECL_DEBUG
    opt_debug = prev_debug;
#endif /* ENABLE_CDECL_DEBUG */
  }

  user_defined = true;
}

c_typedef_t const* c_typedef_visit( c_typedef_visitor_t visitor, void *data ) {
  assert( visitor != NULL );

  rb_visitor_data_t vd = { visitor, data };

  rb_node_t const *const rb = rb_tree_visit( typedefs, &rb_visitor, &vd );
  return rb ? rb_type_data( c_typedef_t const*, rb ) : NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
