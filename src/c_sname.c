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
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_SNAME_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "c_sname.h"
#include "util.h"

// standard
#include <assert.h>

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for `c_sname_full_name()` and `c_sname_scope_name()` that
 * writes the scope names from outermost to innermost separated by `::` into a
 * buffer.
 *
 * @param name_buf The buffer to write into.
 * @param sname The scoped name to write.
 * @param end_scope The scope to stop before or null for all scopes.
 * @return Returns \a name_buf if \a sname is not empty or null otherwise.
 */
static char const* c_sname_scope_name_impl( char *name_buf,
                                            c_sname_t const *sname,
                                            c_scope_t const *end_scope ) {
  assert( name_buf != NULL );
  assert( sname != NULL );

  char *name = name_buf;
  name[0] = '\0';
  bool colon2 = false;

  for ( c_scope_t const *scope = sname->head; scope != end_scope;
        scope = scope->next ) {
    if ( true_or_set( &colon2 ) )
      STRCAT( name, "::" );
    STRCAT( name, c_scope_name( scope ) );
  } // for

  return name_buf;
}

////////// extern functions ///////////////////////////////////////////////////

char const* c_sname_full_name( c_sname_t const *sname ) {
  static char name_buf[ 256 ];
  assert( sname != NULL );
  return c_sname_scope_name_impl( name_buf, sname, NULL );
}

bool c_sname_is_ctor( c_sname_t const *sname ) {
  assert( sname != NULL );
  if ( c_sname_count( sname ) < 2 )
    return false;
  char const *const class_name = SLIST_PEEK_ATR( char const*, sname, 1 );
  char const *const local_name = SLIST_TAIL( char const*, sname );
  return strcmp( local_name, class_name ) == 0;
}

char const* c_sname_scope_name( c_sname_t const *sname ) {
  static char name_buf[ 256 ];
  assert( sname != NULL );
  return c_sname_scope_name_impl( name_buf, sname, sname->tail );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
