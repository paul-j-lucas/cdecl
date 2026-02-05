/*
**      cdecl -- C gibberish translator
**      src/c_sglob.c
**
**      Copyright (C) 2021-2026  Paul J. Lucas
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
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "c_sglob.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdlib.h>                     /* for free(3) */

////////// extern functions ///////////////////////////////////////////////////

void c_sglob_cleanup( c_sglob_t *sglob ) {
  if ( sglob != NULL ) {
    if ( sglob->pattern != NULL ) {
      for ( size_t i = 0; i < sglob->count; ++i )
        free( sglob->pattern[i] );
      free( sglob->pattern );
    }
    c_sglob_init( sglob );
  }
}

void c_sglob_parse( char const *s, c_sglob_t *rv_sglob ) {
  assert( rv_sglob != NULL );

  if ( s == NULL )
    return;
  SKIP_WS( s );

  switch ( s[0] ) {
    case '\0':
      return;                           // LCOV_EXCL_LINE
    case '*':
      if ( s[1] == '*' ) {              // starts with "**": match in any scope
        s += STRLITLEN( "**" );
        SKIP_WS( s );
        assert( s[0] == ':' && s[1] == ':' );
        s += STRLITLEN( "::" );
        SKIP_WS( s );
        rv_sglob->match_in_any_scope = true;
      }
      break;
    case ':':
      assert( s[1] == ':' );            // starts with "::": global scope
      s += STRLITLEN( "::" );
      SKIP_WS( s );
      break;
  } // switch

  //
  // Scan through the scoped glob to count the number of scopes which is the
  // number of occurrences of `::` plus 1, e.g., `a::b::c` yields 3.
  //
  size_t scope_count = 1;
  for ( char const *t = s; *t != '\0'; ++t ) {
    if ( *t == ':' ) {
      ++t;
      assert( *t == ':' );
      ++scope_count;
    }
  } // for

  rv_sglob->pattern = MALLOC( char*, scope_count );

  //
  // Break up scoped glob into array of globs.
  //
  char const *glob_begin = s;
  for (;;) {
    if ( *s != ':' && *s != '\0' /* end of glob */ ) {
      ++s;
      continue;
    }
    size_t const glob_len = STATIC_CAST( size_t, s - glob_begin );
    assert( glob_len > 0 );
    assert( rv_sglob->count < scope_count );
    rv_sglob->pattern[ rv_sglob->count++ ] =
      check_strndup( glob_begin, glob_len );
    if ( *s == '\0' )
      break;
    s += STRLITLEN( "::" );
    glob_begin = SKIP_WS( s );
  } // for

  assert( rv_sglob->count == scope_count );
}

///////////////////////////////////////////////////////////////////////////////

extern inline bool c_sglob_empty( c_sglob_t const* );
extern inline void c_sglob_init( c_sglob_t* );

/* vim:set et sw=2 ts=2: */
