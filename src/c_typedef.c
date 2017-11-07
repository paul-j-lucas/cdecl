/*
**      cdecl -- C gibberish translator
**      src/c_typedef.c
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
#include "c_ast.h"
#include "c_typedef.h"
#include "options.h"
#include "red_black.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct rb_visitor_data {
  c_typedef_visitor_t visitor;          ///< Caller's visitor function.
  void *data;                           ///< Caller's optional data.
};
typedef struct rb_visitor_data rb_visitor_data_t;

// local variable definitions
static rb_tree_t *typedefs;             ///< Global set of `typedef`s.
static bool       user_defined;         ///< Are new `typedef`s used-defined?

///////////////////////////////////////////////////////////////////////////////

/**
 * Types from `stdint.h`.
 *
 * The underlying types used here are merely typical and do not necessarily
 * match the underlying type on any particular platform.
 *
 * @hideinitializer
 */
static char const *const TYPEDEFS_STDINT_H[] = {
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

/**
 * Types from `stdatomic.h`.
 *
 * @hideinitializer
 */
static char const *const TYPEDEFS_STDATOMIC_H[] = {
  "typedef _Atomic          _Bool     atomic_bool",
  "typedef _Atomic          char      atomic_char",
  "typedef _Atomic   signed char      atomic_schar",
  "typedef _Atomic          char16_t  atomic_char16_t",
  "typedef _Atomic          char32_t  atomic_char32_t",
  "typedef _Atomic          wchar_t   atomic_wchar_t",
  "typedef _Atomic          short     atomic_short",
  "typedef _Atomic          int       atomic_int",
  "typedef _Atomic          long      atomic_long",
  "typedef _Atomic          long long atomic_llong",
  "typedef _Atomic unsigned char      atomic_uchar",
  "typedef _Atomic unsigned short     atomic_ushort",
  "typedef _Atomic unsigned int       atomic_uint",
  "typedef _Atomic unsigned long      atomic_ulong",
  "typedef _Atomic unsigned long long atomic_ullong",

  "typedef _Atomic  ptrdiff_t         atomic_ptrdiff_t",
  "typedef _Atomic  size_t            atomic_size_t",

  "typedef _Atomic  intmax_t          atomic_intmax_t",
  "typedef _Atomic  intptr_t          atomic_intptr_t",
  "typedef _Atomic uintptr_t          atomic_uintptr_t",
  "typedef _Atomic uintmax_t          atomic_uintmax_t",

  "typedef _Atomic  int_fast8_t       atomic_int_fast8_t",
  "typedef _Atomic  int_fast16_t      atomic_int_fast16_t",
  "typedef _Atomic  int_fast32_t      atomic_int_fast32_t",
  "typedef _Atomic  int_fast64_t      atomic_int_fast64_t",
  "typedef _Atomic uint_fast8_t       atomic_uint_fast8_t",
  "typedef _Atomic uint_fast16_t      atomic_uint_fast16_t",
  "typedef _Atomic uint_fast32_t      atomic_uint_fast32_t",
  "typedef _Atomic uint_fast64_t      atomic_uint_fast64_t",

  "typedef _Atomic  int_least8_t      atomic_int_least8_t",
  "typedef _Atomic  int_least16_t     atomic_int_least16_t",
  "typedef _Atomic  int_least32_t     atomic_int_least32_t",
  "typedef _Atomic  int_least64_t     atomic_int_least64_t",
  "typedef _Atomic uint_least8_t      atomic_uint_least8_t",
  "typedef _Atomic uint_least16_t     atomic_uint_least16_t",
  "typedef _Atomic uint_least32_t     atomic_uint_least32_t",
  "typedef _Atomic uint_least64_t     atomic_uint_least64_t",

  NULL
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Comparison function for `c_typedef` data used by the red-black tree.
 *
 * @param data_i A pointer to data.
 * @param data_j A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the `typedef` name pointed to by \a data_i is less than, equal
 * to, or greater than the `typedef` name pointed to by \a data_j.
 */
static int c_typedef_cmp( void const *data_i, void const *data_j ) {
  c_typedef_t const *const ti = REINTERPRET_CAST( c_typedef_t const*, data_i );
  c_typedef_t const *const tj = REINTERPRET_CAST( c_typedef_t const*, data_j );
  return strcmp( ti->type_name, tj->type_name );
}

/**
 * Red-black tree node free function for `c_typedef` data used by the red-black
 * tree.
 *
 * @param data A pointer to the data to free.
 */
static void c_typedef_free( void *data ) {
  c_typedef_t *const t = REINTERPRET_CAST( c_typedef_t*, data );
  FREE( t->type_name );
}

/**
 * Creates a new `c_typedef`.
 *
 * @param type_name The type name.
 * @param ast The `c_ast` of the type.
 * @return Returns said `c_typedef`.
 */
static c_typedef_t* c_typedef_new( char const *type_name, c_ast_t const *ast ) {
  c_typedef_t *const t = MALLOC( c_typedef_t, 1 );
  t->type_name = type_name;
  t->ast = ast;
  t->user_defined = user_defined;
  return t;
}

/**
 * Parses an array of built-in `typedef` declarations.
 *
 * @param types An array of pointers to `typedef` strings.  The last element
 * must be null.
 */
static void c_typedef_parse_builtins( char const *const types[] ) {
  extern bool parse_string( char const*, size_t );
  for ( char const *const *ptype = types; *ptype != NULL; ++ptype ) {
    bool const ok = parse_string( *ptype, 0 );
    assert( ok );
  } // for
}

/**
 * Red-black tree visitor function that forwards to the `c_typedef_visitor_t`
 * function.
 *
 * @param node_data A pointer to the node's data.
 * @param aux_data Optional data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of `rb_tree_visit()`.
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
    //
    // Temporarily turn off debug output for built-in typedefs.
    //
    bool const prev_debug = opt_debug;
    opt_debug = false;
#endif /* ENABLE_CDECL_DEBUG */
    //
    // Temporarily set the language to the latest C version to allow all built-
    // in typedefs.
    //
    c_lang_t const prev_lang = opt_lang;
    opt_lang = LANG_C_MAX;

    c_typedef_parse_builtins( TYPEDEFS_STDINT_H );    // must go first
    c_typedef_parse_builtins( TYPEDEFS_STDATOMIC_H );

#ifdef ENABLE_CDECL_DEBUG
    opt_debug = prev_debug;
#endif /* ENABLE_CDECL_DEBUG */
    opt_lang = prev_lang;
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
