/*
**      cdecl -- C gibberish translator
**      src/c_sglob.c
**
**      Copyright (C) 2021-2022  Paul J. Lucas
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
 * Defines functions for dealing with "sglob" (C++ scoped name glob) objects,
 * e.g., `S::T::*`, that are used to match snames (C++ scoped names).
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_SGLOB_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_sglob.h"

// standard
#include <assert.h>
#include <stdlib.h>                     /* for free(3) */

////////// extern functions ///////////////////////////////////////////////////

void c_sglob_cleanup( c_sglob_t *sglob ) {
  if ( sglob != NULL ) {
    for ( size_t i = 0; i < sglob->count; ++i )
      free( sglob->pattern[i] );
    free( sglob->pattern );
    c_sglob_init( sglob );
  }
}

void c_sglob_parse( char const *s, c_sglob_t *sglob ) {
  assert( sglob != NULL );

  if ( s == NULL )
    return;
  SKIP_WS( s );

  //
  // Scan through the scoped glob to count the number of scopes which is the
  // number of occurrences of `::` plus 1, e.g., `a::b::c` yields 3.
  //
  sglob->count = 1;
  for ( char const *t = s; *t != '\0'; ++t ) {
    if ( *t == ':' ) {
      ++t;
      assert( *t == ':' );
      ++sglob->count;
    }
  } // for

  //
  // Special case: if the scoped glob starts with `**`, match in any scope.
  // Skip past `**::` and decrement scope count.
  //
  sglob->match_in_any_scope = s[0] == '*' && s[1] == '*';
  if ( sglob->match_in_any_scope ) {
    s += 2 /* "**" */;
    SKIP_WS( s );
    assert( s[0] == ':' && s[1] == ':' );
    s += 2 /* "::" */;
    --sglob->count;
  }

  sglob->pattern = MALLOC( char*, sglob->count );

  //
  // Break up scoped glob into array of globs.
  //
  char const *glob_begin = SKIP_WS( s );
  size_t glob_index = 0;
  for (;;) {
    switch ( *s ) {
      case ':':
      case '\0': {                      // found end of glob
        size_t const glob_len = STATIC_CAST( size_t, s - glob_begin );
        assert( glob_len > 0 );
        assert( glob_index < sglob->count );
        sglob->pattern[ glob_index ] = check_strndup( glob_begin, glob_len );
        if ( *s == '\0' )
          return;
        assert( s[0] == ':' && s[1] == ':' );
        s += 2 /* "::" */;
        SKIP_WS( s );
        assert( is_ident( *s ) || *s == '*' );
        glob_begin = s;
        ++glob_index;
        break;
      }

      default:
        assert( is_ident( *s ) );
        FALLTHROUGH;
      case '*':
        ++s;
        break;
    } // switch
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
