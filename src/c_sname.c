/*
**      cdecl -- C gibberish translator
**      src/c_sname.c
**
**      Copyright (C) 2019-2021  Paul J. Lucas, et al.
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
#include "c_sname.h"
#include "c_keyword.h"
#include "options.h"
#include "strbuf.h"
#include "util.h"

// standard
#include <assert.h>
#include <fnmatch.h>

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_sname_full_name() and c_sname_scope_name() that writes
 * the scope names from outermost to innermost separated by `::` into a buffer.
 *
 * @param sbuf The buffer to write into.
 * @param sname The scoped name to write.
 * @param end_scope The scope to stop before or NULL for all scopes.
 * @return If not NULL, returns \a sbuf->str; otherwise returns the empty
 * string.
 */
PJL_WARN_UNUSED_RESULT
static char const* scope_name_impl( strbuf_t *sbuf, c_sname_t const *sname,
                                    c_scope_t const *end_scope ) {
  assert( sbuf != NULL );
  assert( sname != NULL );

  strbuf_free( sbuf );
  bool colon2 = false;

  FOREACH_SCOPE( scope, sname, end_scope )
    strbuf_sepsn_cats( sbuf, "::", 2, &colon2, c_scope_data( scope )->name );

  return sbuf->str != NULL ? sbuf->str : "";
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

void c_sname_fill_in_namespaces( c_sname_t *sname ) {
  assert( sname != NULL );
  c_type_t const *const local_type = c_sname_local_type( sname );
  if ( !c_type_is_tid_any( local_type, TB_NAMESPACE ) )
    return;

  FOREACH_SCOPE( scope, sname, sname->tail ) {
    c_type_t *const type = &c_scope_data( scope )->type;
    if ( c_type_is_none( type ) || c_type_is_tid_any( type, TB_SCOPE ) ) {
      type->base_tid &= c_tid_compl( TB_SCOPE );
      type->base_tid |= TB_NAMESPACE;
    }
  } // for
}

char const* c_sname_full_name( c_sname_t const *sname ) {
  static strbuf_t sbuf;
  assert( sname != NULL );
  return scope_name_impl( &sbuf, sname, NULL );
}

bool c_sname_is_ctor( c_sname_t const *sname ) {
  assert( sname != NULL );
  if ( c_sname_count( sname ) < 2 )
    return false;
  char const *const class_name = c_sname_name_atr( sname, 1 );
  char const *const local_name = c_sname_local_name( sname );
  return strcmp( local_name, class_name ) == 0;
}

bool c_sname_match( c_sname_t const *sname, char const *glob ) {
  assert( sname != NULL );
  assert( glob != NULL );

  size_t const scope_count = c_sname_count( sname );
  if ( scope_count == 0 || glob[0] == '\0' )
    return false;

  //
  // Scan through glob to count the number of scope globs which is the number
  // of occurrences of `::` plus 1, e.g., `a::b::c` yields 3.
  //
  size_t glob_count = 1;
  for ( char const *s = glob; *s != '\0'; ++s ) {
    if ( *s == ':' ) {
      ++s;
      assert( *s == ':' );
      ++glob_count;
    }
  } // for

  //
  // Special case: if glob starts with `**`, match in any scope.  Decrement
  // glob_count and skip past `**::`.
  //
  bool const match_in_any_scope = glob[0] == '*' && glob[1] == '*';
  if ( match_in_any_scope ) {
    if ( --glob_count > scope_count ) {
      //
      // There are more scope globs to match than there are scopes in sname so
      // it can't possibly match.
      //
      return false;
    }
    glob += 2 /* "**" */;
    SKIP_WS( glob );
    assert( glob[0] == ':' && glob[1] == ':' );
    glob += 2 /* "::" */;
  }
  else if ( glob_count != scope_count ) {
    //
    // For non-any-scope matches, the number of scope globs must equal the
    // number of scopes in sname and it doesn't so it can't possibly match.
    //
    return false;
  }

  char const *scope_glob[ glob_count ];
  size_t glob_index = 0;

  //
  // Break up glob into scope globs.
  //
  for ( char const *s = glob, *glob_begin = SKIP_WS( s ); ; ) {
    if ( *s == ':' || *s == '\0' ) {
      assert( glob_index < glob_count );
      size_t const glob_len = (size_t)(s - glob_begin);
      scope_glob[ glob_index ] = check_strndup( glob_begin, glob_len );
      if ( *s++ == '\0' )
        break;
      assert( *s == ':' );
      ++s;
      glob_begin = SKIP_WS( s );
      ++glob_index;
    }
    else {
      assert( is_ident( *s ) || *s == '*' );
      ++s;
    }
  } // for

  c_scope_t const *scope = sname->head;

  if ( match_in_any_scope ) {
    //
    // For any-scope matches, skip past leading scopes in sname (if necessary)
    // since its trailing scopes are the ones that have to match.
    //
    // For example, if sname is `a::b::c::d` (scope_count = 4) and glob is
    // `**::c::d` (glob_count = 2 since the `**::` is stripped), then skip past
    // 2 scopes (4 - 2) in sname to arrive at `c::d` that will match.
    //
    for ( size_t diff_count = scope_count - glob_count; diff_count > 0;
          --diff_count, scope = scope->next ) {
      assert( scope != NULL );
    } // for
    assert( scope != NULL );
  }

  //
  // Finally, attempt to match each scope name against each scope glob.
  //
  bool is_match = true;
  for ( glob_index = 0; is_match && scope != NULL;
        ++glob_index, scope = scope->next ) {
    assert( glob_index < glob_count );
    char const *const name = c_scope_data( scope )->name;
    if ( fnmatch( scope_glob[ glob_index ], name, 0 ) != 0 )
      is_match = false;
  } // for

  for ( glob_index = 0; glob_index < glob_count; ++glob_index )
    FREE( scope_glob[ glob_index ] );

  return is_match;
}

bool c_sname_parse( char const *s, c_sname_t *sname ) {
  assert( s != NULL );
  assert( sname != NULL );

  c_sname_t rv;
  c_sname_init( &rv );

  for ( char const *end; parse_identifier( s, &end ); ) {
    char *const name = check_strndup( s, (size_t)(end - s) );

    // Ensure that the name is NOT a keyword.
    c_keyword_t const *const k = c_keyword_find( name, opt_lang, C_KW_CTX_ALL );
    if ( k != NULL ) {
      FREE( name );
      break;
    }
    c_sname_append_name( &rv, name );

    SKIP_WS( end );
    if ( *end == '\0' ) {
      *sname = rv;
      return true;
    }
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
  } // for

  c_sname_free( &rv );
  return false;
}

char const* c_sname_scope_name( c_sname_t const *sname ) {
  static strbuf_t sbuf;
  assert( sname != NULL );
  return scope_name_impl( &sbuf, sname, sname->tail );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
