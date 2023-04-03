/*
**      cdecl -- C gibberish translator
**      src/c_sname.c
**
**      Copyright (C) 2019-2023  Paul J. Lucas
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
#define C_SNAME_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_sname.h"
#include "c_keyword.h"
#include "c_sglob.h"
#include "c_type.h"
#include "c_typedef.h"
#include "color.h"
#include "english.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "strbuf.h"
#include "util.h"

// standard
#include <assert.h>
#include <fnmatch.h>
#include <stdlib.h>                     /* for free(3) */

/**
 * @addtogroup sname-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_sname_full_name() and c_sname_scope_name() that writes
 * the scope names from outermost to innermost separated by `::` into a buffer.
 *
 * @param sbuf The buffer to write into.
 * @param sname The scoped name to write.
 * @param end_scope The scope to stop before or NULL for all scopes.
 * @return If not NULL, returns \a sbuf&ndash;>str; otherwise returns the empty
 * string.
 */
NODISCARD
static char const* c_sname_name_impl( strbuf_t *sbuf, c_sname_t const *sname,
                                      c_scope_t const *end_scope ) {
  assert( sbuf != NULL );
  assert( sname != NULL );

  strbuf_reset( sbuf );
  bool colon2 = false;

  FOREACH_SNAME_SCOPE_UNTIL( scope, sname, end_scope ) {
    strbuf_sepsn( sbuf, "::", 2, &colon2 );
    c_scope_data_t const *const data = c_scope_data( scope );
    if ( data->type.stids != TS_NONE ) {
      // For nested inline namespaces, e.g., namespace A::inline B::C.
      strbuf_printf( sbuf, "%s ", c_tid_name_c( data->type.stids ) );
    }
    strbuf_puts( sbuf, data->name );
  } // for

  return sbuf->str != NULL ? sbuf->str : "";
}

/**
 * Helper function for c_sname_parse() and c_sname_parse_dtor().
 *
 * @param s The string to parse.
 * @param rv_sname The scoped name to parse into.
 * @param is_dtor `true` only if a destructor name, e.g., `S::T::~T`, is to be
 * parsed.
 * @return
 *  + The number of characters of \a s that were successfully parsed.
 *  + If \a is_dtor is `true`, that includes whether and the scope count &ge;
 *    2, the last two scope names match, and the last scope name is preceded by
 *    either `~` or `compl `.
 *  + Otherwise 0.
 */
size_t c_sname_parse_impl( char const *s, c_sname_t *rv_sname, bool is_dtor ) {
  assert( s != NULL );
  assert( rv_sname != NULL );

  bool parsed_tilde = !is_dtor;
  c_sname_t temp_sname;
  c_sname_init( &temp_sname );

  char const *const s_orig = s;
  char const *end;
  char const *prev_end = s_orig;
  char const *prev_name = "";

  while ( (end = parse_identifier( s )) != NULL ) {
    char *const name = check_strndup( s, STATIC_CAST( size_t, end - s ) );

    // Ensure that the name is NOT a keyword.
    c_keyword_t const *const ck =
      c_keyword_find( name, opt_lang, C_KW_CTX_DEFAULT );
    if ( ck != NULL ) {
      FREE( name );
      // ck->literal is set to L_* so == is OK
      if ( is_dtor && ck->literal == L_compl ) {
        char const *const t = s + strlen( L_compl );
        if ( isspace( *t ) ) {          // except treat "compl" as '~'
          s = t + 1;
          SKIP_WS( s );
          parsed_tilde = true;
          continue;
        }
      }
      if ( c_sname_empty( &temp_sname ) )
        return 0;
      goto done;
    }
    c_sname_append_name( &temp_sname, name );

    prev_end = end;
    SKIP_WS( end );
    if ( *end == '\0' && parsed_tilde ) {
      if ( is_dtor && strcmp( name, prev_name ) != 0 )
        goto error;
      goto done;
    }
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
    if ( is_dtor && *s == '~' ) {
      ++s;
      SKIP_WS( s );
      parsed_tilde = true;
    }
    prev_name = name;
  } // while

error:
  c_sname_cleanup( &temp_sname );
  return 0;

done:
  *rv_sname = temp_sname;
  return STATIC_CAST( size_t, prev_end - s_orig );
}

////////// extern functions ///////////////////////////////////////////////////

int c_scope_data_cmp( c_scope_data_t const *i_data,
                      c_scope_data_t const *j_data ) {
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

void c_sname_append_name( c_sname_t *sname, char const *name ) {
  assert( sname != NULL );
  assert( name != NULL );
  c_scope_data_t *const data = MALLOC( c_scope_data_t, 1 );
  data->name = name;
  data->type = T_NONE;
  slist_push_back( sname, data );
}

bool c_sname_check( c_sname_t const *sname, c_loc_t const *sname_loc ) {
  assert( sname != NULL );
  assert( !c_sname_empty( sname ) );
  assert( sname_loc != NULL );

  size_t const sname_count = c_sname_count( sname );
  if ( sname_count > 1 ) {
    c_type_t const *const scope_type = c_sname_first_type( sname );
    bool const is_inline_namespace =
      c_tid_is_any( scope_type->stids, TS_INLINE ) &&
      c_tid_is_any( scope_type->btids, TB_NAMESPACE );
    if ( is_inline_namespace ) {
      print_error( sname_loc,
        "nested namespace can not be %s\n",
        c_tid_name_error( TS_INLINE )
      );
      return false;
    }
  }

  c_tid_t prev_btids = TB_NONE;
  unsigned prev_order = 0;

  FOREACH_SNAME_SCOPE( scope, sname ) {
    c_type_t *const scope_type = &c_scope_data( scope )->type;
    //
    // Temporarily set scope->next to NULL to chop off any scopes past the
    // given scope to look up a partial sname. For example, given "A::B::C",
    // see if "A::B" exists.  If it does, check that the sname's scope's type
    // matches the previously declared sname's scope's type.
    //
    c_scope_t *const orig_next = scope->next;
    scope->next = NULL;
    c_typedef_t const *const tdef = c_typedef_find_sname( sname );
    if ( tdef != NULL ) {
      c_type_t const *const tdef_type = c_sname_local_type( &tdef->ast->sname );
      if ( c_tid_is_any( tdef_type->btids, TB_ANY_SCOPE | TB_ENUM ) &&
           !c_type_equiv( scope_type, tdef_type ) ) {
        if ( c_tid_is_any( scope_type->btids, TB_ANY_SCOPE ) ) {
          //
          // The scope's type is a scope-type and doesn't match a previously
          // declared scope-type, e.g.:
          //
          //      namespace N { class C; }
          //      namespace N::C { class D; }
          //                ^
          //      11: error: "N::C" was previously declared as class
          //
          print_error( sname_loc,
            "\"%s\" was previously declared as %s:\n",
            c_sname_full_name( sname ),
            c_type_name_error( tdef_type )
          );
          color_start( stderr, sgr_caret );
          EPUTC( '>' );
          color_end( stderr, sgr_caret );
          EPUTC( ' ' );
          print_type( tdef, stderr );
          scope->next = orig_next;
          return false;
        }

        //
        // Otherwise, copy the previously declared scope's type to the current
        // scope's type.
        //
        *scope_type = *tdef_type;
      }
    }
    scope->next = orig_next;

    unsigned const scope_order = c_tid_scope_order( scope_type->btids );
    if ( scope_order < prev_order ) {
      print_error( sname_loc,
        "%s can not nest inside %s\n",
        c_tid_name_error( scope_type->btids ),
        c_tid_name_error( prev_btids )
      );
      return false;
    }
    prev_btids = scope_type->btids;
    prev_order = scope_order;
  } // for

  return true;
}

void c_sname_cleanup( c_sname_t *sname ) {
  slist_cleanup( sname, POINTER_CAST( slist_free_fn_t, &c_scope_data_free ) );
}

void c_sname_fill_in_namespaces( c_sname_t *sname ) {
  assert( sname != NULL );
  c_type_t const *const local_type = c_sname_local_type( sname );
  if ( !c_tid_is_any( local_type->btids, TB_NAMESPACE ) )
    return;

  FOREACH_SNAME_SCOPE_UNTIL( scope, sname, sname->tail ) {
    c_type_t *const type = &c_scope_data( scope )->type;
    if ( c_type_is_none( type ) || c_tid_is_any( type->btids, TB_SCOPE ) ) {
      type->btids &= c_tid_compl( TB_SCOPE );
      type->btids |= TB_NAMESPACE;
    }
  } // for
}

void c_sname_free( c_sname_t *sname ) {
  c_sname_cleanup( sname );
  free( sname );
}

char const* c_sname_full_name( c_sname_t const *sname ) {
  static strbuf_t sbuf;
  return sname != NULL ?
    c_sname_name_impl( &sbuf, sname, /*end_scope=*/NULL ) : "";
}

bool c_sname_is_ctor( c_sname_t const *sname ) {
  assert( sname != NULL );
  if ( c_sname_count( sname ) < 2 )
    return false;
  char const *const class_name = c_sname_name_atr( sname, 1 );
  char const *const local_name = c_sname_local_name( sname );
  return strcmp( local_name, class_name ) == 0;
}

void c_sname_list_cleanup( slist_t *list ) {
  slist_cleanup( list, POINTER_CAST( slist_free_fn_t, &c_sname_free ) );
}

bool c_sname_match( c_sname_t const *sname, c_sglob_t const *sglob ) {
  assert( sname != NULL );
  assert( sglob != NULL );

  c_scope_t const *scope = sname->head;
  size_t const scope_count = c_sname_count( sname );

  if ( !sglob->match_in_any_scope ) {
    if ( sglob->count != scope_count ) {
      //
      // For non-any-scope matches, the number of scope globs must equal the
      // number of scopes in sname and it doesn't so it can't possibly match.
      //
      return false;
    }
  }
  else if ( scope_count < sglob->count ) {
    //
    // For any-scope matches, if the number of scopes in sname is less than the
    // number of scope globs, it can't possibly match.
    //
    return false;
  }
  else {
    //
    // For any-scope matches, skip past leading scopes in sname (if necessary)
    // since its trailing scopes are the ones that have to match.
    //
    // For example, if sname is `a::b::c::d` (scope_count = 4) and glob is
    // `**::c::d` (glob_count = 2 since the `**::` is stripped), then skip past
    // 2 scopes (4 - 2) in sname to arrive at `c::d` that will match.
    //
    for ( size_t diff_count = scope_count - sglob->count; diff_count > 0;
          --diff_count, scope = scope->next ) {
      assert( scope != NULL );
    } // for
    assert( scope != NULL );
  }

  //
  // Finally, attempt to match each scope name against each scope glob.
  //
  for ( size_t sglob_index = 0; scope != NULL;
        ++sglob_index, scope = scope->next ) {
    assert( sglob_index < sglob->count );
    char const *const name = c_scope_data( scope )->name;
    if ( fnmatch( sglob->pattern[ sglob_index ], name, /*flags=*/0 ) != 0 )
      return false;
  } // for

  return true;
}

size_t c_sname_parse( char const *s, c_sname_t *rv_sname ) {
  return c_sname_parse_impl( s, rv_sname, /*is_dtor=*/false );
}

bool c_sname_parse_dtor( char const *s, c_sname_t *rv_sname ) {
  return c_sname_parse_impl( s, rv_sname, /*is_dtor=*/true ) > 0;
}

char const* c_sname_scope_name( c_sname_t const *sname ) {
  static strbuf_t sbuf;
  return sname != NULL ? c_sname_name_impl( &sbuf, sname, sname->tail ) : "";
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
