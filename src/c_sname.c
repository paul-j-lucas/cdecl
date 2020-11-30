/*
**      cdecl -- C gibberish translator
**      src/c_sname.c
**
**      Copyright (C) 2019-2020  Paul J. Lucas, et al.
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
 * Defines functions for dealing with "sname" (C++ scoped name) objects, e.g.,
 * `S::T::x`.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_SNAME_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "c_sname.h"
#include "util.h"

// standard
#include <assert.h>

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for `c_sname_local_type()` that returns the scope type of the
 * innermost scope (that has a type).
 *
 * @param scope A pointer to a scope.
 * @return Returns the scope type.
 */
PJL_WARN_UNUSED_RESULT
static c_type_t const* c_sname_local_type_impl( c_scope_t const *scope ) {
  assert( scope != NULL );
  c_scope_t const *const next = scope->next;
  return next != NULL && !c_type_is_none( &c_scope_data( next )->type ) ?
    c_sname_local_type_impl( next ) : &c_scope_data( scope )->type;
}

/**
 * Helper function for `c_sname_full_name()` and `c_sname_scope_name()` that
 * writes the scope names from outermost to innermost separated by `::` into a
 * buffer.
 *
 * @param name_buf The buffer to write into.
 * @param name_end One past the end of \a name_buf.
 * @param sname The scoped name to write.
 * @param end_scope The scope to stop before or null for all scopes.
 * @return Returns \a name_buf.
 */
PJL_WARN_UNUSED_RESULT
static char const* scope_name_impl( char *name_buf, char const *name_end,
                                    c_sname_t const *sname,
                                    c_scope_t const *end_scope ) {
  assert( name_buf != NULL );
  assert( name_end != NULL );
  assert( sname != NULL );

  char *name = name_buf;
  name[0] = '\0';
  bool colon2 = false;

  for ( c_scope_t const *scope = sname->head; scope != end_scope;
        scope = scope->next ) {
    if ( true_or_set( &colon2 ) )
      STRCAT( name, "::" );
    STRCAT( name, c_scope_data( scope )->name );
    assert( name < name_end );
  } // for

  return name_buf;
}

////////// extern functions ///////////////////////////////////////////////////

int c_scope_data_cmp( c_scope_data_t *i_data, c_scope_data_t *j_data ) {
  assert( i_data != NULL );
  assert( j_data != NULL );
  return strcmp( i_data->name, j_data->name );
}

c_scope_data_t* c_scope_data_dup( c_scope_data_t const *src ) {
  assert( src != NULL );
  c_scope_data_t *const dst = MALLOC( c_scope_data_t, 1 );
  dst->name = check_strdup( src->name );
  dst->type = src->type;
  return dst;
}

void c_scope_data_free( c_scope_data_t *data ) {
  if ( data != NULL ) {
    FREE( data->name );
    free( data );
  }
}

void c_sname_append_name( c_sname_t *sname, char *name ) {
  assert( sname != NULL );
  assert( name != NULL );
  c_scope_data_t *const data = MALLOC( c_scope_data_t, 1 );
  data->name = name;
  data->type = T_NONE;
  slist_push_tail( sname, data );
}

char const* c_sname_full_name( c_sname_t const *sname ) {
  static char name_buf[ 256 ];
  assert( sname != NULL );
  return scope_name_impl( name_buf, BUF_END( name_buf ), sname, NULL );
}

bool c_sname_is_ctor( c_sname_t const *sname ) {
  assert( sname != NULL );
  if ( c_sname_count( sname ) < 2 )
    return false;
  char const *const class_name = c_sname_name_atr( sname, 1 );
  char const *const local_name = SLIST_PEEK_TAIL(c_scope_data_t*, sname)->name;
  return strcmp( local_name, class_name ) == 0;
}

char const* c_sname_scope_name( c_sname_t const *sname ) {
  static char name_buf[ 256 ];
  assert( sname != NULL );
  return scope_name_impl( name_buf, BUF_END( name_buf ), sname, sname->tail );
}

c_type_t const* c_sname_local_type( c_sname_t const *sname ) {
  assert( sname != NULL );
  return c_sname_empty( sname ) ?
    &T_NONE : c_sname_local_type_impl( sname->head );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
