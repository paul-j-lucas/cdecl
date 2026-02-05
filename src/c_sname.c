/*
**      cdecl -- C gibberish translator
**      src/c_sname.c
**
**      Copyright (C) 2019-2026  Paul J. Lucas
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
#include "c_sname.h"
#include "c_ast.h"
#include "c_keyword.h"
#include "c_sglob.h"
#include "c_type.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "p_macro.h"
#include "print.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for isspace(3) */
#include <fnmatch.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>                      /* for stderr */
#include <stdlib.h>                     /* for free(3) */
#include <string.h>                     /* for strcmp(3), strlen(3) */

/**
 * @addtogroup sname-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

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
NODISCARD
size_t c_sname_parse_impl( char const *s, c_sname_t *rv_sname, bool is_dtor ) {
  assert( s != NULL );
  assert( rv_sname != NULL );

  bool parsed_tilde = !is_dtor;
  c_sname_t temp_sname;
  c_sname_init( &temp_sname );

  char const *const orig_s = s;
  char const *end;
  char const *prev_end = orig_s;
  char const *prev_name = "";

  while ( (end = parse_identifier( s )) != NULL ) {
    char *const name = check_strndup( s, STATIC_CAST( size_t, end - s ) );

    // Ensure that the name is NOT a keyword.
    c_keyword_t const *const ck =
      c_keyword_find( name, opt_lang_id, C_KW_CTX_DEFAULT );
    if ( ck != NULL ) {
      free( name );
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
        goto error;
      goto done;
    }
    c_sname_push_back_name( &temp_sname, name );

    prev_end = end;
    SKIP_WS( end );
    if ( *end == '\0' && parsed_tilde ) {
      if ( is_dtor && strcmp( name, prev_name ) != 0 )
        goto error;
      goto done;
    }
    if ( STRNCMPLIT( end, "::" ) != 0 )
      break;
    s = end + STRLITLEN( "::" );
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
  return STATIC_CAST( size_t, prev_end - orig_s );
}

////////// extern functions ///////////////////////////////////////////////////

int c_scope_data_cmp( c_scope_data_t const *i_data,
                      c_scope_data_t const *j_data ) {
  assert( i_data != NULL );
  assert( j_data != NULL );
  return strcmp( i_data->name, j_data->name );
}

c_scope_data_t* c_scope_data_dup( c_scope_data_t const *src ) {
  if ( src == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  c_scope_data_t *const dst = MALLOC( c_scope_data_t, 1 );
  *dst = (c_scope_data_t){
    .name = check_strdup( src->name ),
    .type = src->type
  };
  return dst;
}

void c_scope_data_free( c_scope_data_t *data ) {
  if ( data != NULL ) {
    free( data->name );
    free( data );
  }
}

bool c_sname_check( c_sname_t const *sname, c_loc_t const *sname_loc ) {
  assert( sname != NULL );
  assert( sname_loc != NULL );

  if ( c_sname_empty( sname ) )
    return true;                        // LCOV_EXCL_LINE

  bool ok = true;
  c_tid_t prev_btids = TB_NONE;

  c_sname_t partial_sname;
  c_sname_init( &partial_sname );

  FOREACH_SNAME_SCOPE( scope, sname ) {
    c_scope_data_t const *const scope_data = c_scope_data( scope );
    char const *const name = scope_data->name;

    ok = !macro_is_predefined( name );
    if ( !ok ) {
      print_error( sname_loc, "\"%s\" is a predefined macro\n", name );
      break;
    }

    //
    // Build up partial_sname scope by scope to look up a partial sname. For
    // example, given "A::B::C", see if "A" exists, then if "A::B" exists.
    //
    // For any that does, check that the sname's scope's type matches the
    // previously declared sname's scope's type.
    //
    c_sname_push_back_name( &partial_sname, check_strdup( name ) );

    c_type_t const scope_type = scope_data->type;
    c_typedef_t const *const tdef = c_typedef_find_sname( &partial_sname );

    if ( tdef != NULL ) {
      c_type_t const *const tdef_type = c_sname_local_type( &tdef->ast->sname );
      if ( c_tid_is_any( tdef_type->btids, TB_ANY_SCOPE | TB_enum ) &&
           !c_type_equiv( &scope_type, tdef_type ) ) {
        ok = !c_tid_is_any( scope_type.btids, TB_ANY_SCOPE );
        if ( !ok ) {
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
            "\"%s\" was previously declared as \"%s\" (\"",
            c_sname_local_name( &partial_sname ),
            c_type_error( tdef_type )
          );
          print_type_decl( tdef, tdef->decl_flags, stderr );
          EPUTS( "\")\n" );
          break;
        }
      }
    }

    ok = c_tid_scope_order_ok( prev_btids, scope_type.btids );
    if ( !ok ) {
      print_error( sname_loc,
        "%s can not nest inside %s\n",
        c_tid_error( scope_type.btids ),
        c_tid_error( prev_btids )
      );
      break;
    }

    prev_btids = scope_type.btids;
  } // for

  c_sname_cleanup( &partial_sname );
  return ok;
}

void c_sname_cleanup( c_sname_t *sname ) {
  slist_cleanup( sname, POINTER_CAST( slist_free_fn_t, &c_scope_data_free ) );
}

void c_sname_free( c_sname_t *sname ) {
  c_sname_cleanup( sname );
  free( sname );
}

char const* c_sname_global_name( c_sname_t const *sname ) {
  if ( sname != NULL ) {
    c_scope_data_t const *const global_data = c_sname_global_data( sname );
    if ( global_data != NULL )
      return global_data->name;
  }
  return "";                            // LCOV_EXCL_LINE
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

bool c_sname_is_inline_nested_namespace( c_sname_t const *sname ) {
  assert( sname != NULL );
  if ( c_sname_count( sname ) < 2 )
    return false;
  c_type_t const *const scope_type = c_sname_global_type( sname );
  return  c_tid_is_any( scope_type->stids, TS_inline ) &&
          c_tid_is_any( scope_type->btids, TB_namespace );
}

char const* c_sname_local_name( c_sname_t const *sname ) {
  if ( sname != NULL ) {
    c_scope_data_t const *const local_data = c_sname_local_data( sname );
    if ( local_data != NULL )
      return local_data->name;
  }
  return "";                            // LCOV_EXCL_LINE
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

void c_sname_push_back_name( c_sname_t *sname, char *name ) {
  assert( sname != NULL );
  assert( name != NULL );
  c_scope_data_t *const data = MALLOC( c_scope_data_t, 1 );
  *data = (c_scope_data_t){ .name = name, .type = T_NONE };
  slist_push_back( sname, data );
}

c_sname_t c_sname_scope_sname( c_sname_t const *sname ) {
  c_sname_t rv_sname;
  c_sname_init( &rv_sname );

  if ( sname != NULL ) {
    FOREACH_SNAME_SCOPE_UNTIL( scope, sname, sname->tail ) {
      c_scope_data_t const *const scope_data = c_scope_data( scope );
      c_sname_push_back_name( &rv_sname, check_strdup( scope_data->name ) );
      c_sname_local_data( &rv_sname )->type = scope_data->type;
    } // for
  }

  return rv_sname;
}

void c_sname_set( c_sname_t *dst_sname, c_sname_t *src_sname ) {
  if ( dst_sname != src_sname ) {
    c_sname_cleanup( dst_sname );
    c_sname_push_back_sname( dst_sname, src_sname );
  }
}

void c_sname_set_all_types( c_sname_t *sname ) {
  assert( sname != NULL );

  if ( c_sname_empty( sname ) )
    return;                             // LCOV_EXCL_LINE

  FOREACH_SNAME_SCOPE_UNTIL( scope, sname, sname->tail ) {
    c_type_t *const scope_type = &c_scope_data( scope )->type;
    //
    // Temporarily set scope->next to NULL to chop off any scopes past the
    // current scope to look up a partial sname. For example, given "A::B::C",
    // see if "A::B" exists.  If it does, copy that sname's scope's type.
    //
    c_scope_t *const orig_next = scope->next;
    scope->next = NULL;

    c_typedef_t const *const tdef = c_typedef_find_sname( sname );
    if ( tdef != NULL ) {
      slist_node_t *sname_node = sname->head;
      FOREACH_SNAME_SCOPE( tdef_scope, &tdef->ast->sname ) {
        assert( sname_node != NULL );
        c_scope_data( sname_node )->type = c_scope_data( tdef_scope )->type;
        sname_node = sname_node->next;
      } // for
    }
    else if ( scope_type->btids == TB_SCOPE || c_type_is_none( scope_type ) ) {
      *scope_type = C_TYPE_LIT_B( TB_namespace );
    }

    scope->next = orig_next;
  } // for
}

void c_sname_warn( c_sname_t const *sname, c_loc_t const *sname_loc ) {
  assert( sname != NULL );
  assert( sname_loc != NULL );

  FOREACH_SNAME_SCOPE( scope, sname ) {
    char const *const name = c_scope_data( scope )->name;

    // First, check to see if the name is a keyword in some other language.
    c_keyword_t const *const ck =
      c_keyword_find( name, LANG_ANY, C_KW_CTX_DEFAULT );
    if ( ck != NULL ) {
      print_warning( sname_loc,
        "\"%s\" is a keyword in %s\n",
        name, c_lang_name( c_lang_oldest( ck->lang_ids ) )
      );
      continue;
    }

    // Next, check to see if the name is reserved in any language.
    c_lang_id_t const reserved_lang_ids = is_reserved_name( name );
    if ( reserved_lang_ids != LANG_NONE ) {
      print_warning( sname_loc, "\"%s\" is a reserved identifier", name );
      char const *const coarse_name = c_lang_coarse_name( reserved_lang_ids );
      if ( coarse_name != NULL )
        EPRINTF( " in %s", coarse_name );
      EPUTC( '\n' );
    }
  } // for
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

extern inline char const* c_sname_name_atr( c_sname_t const*, size_t );
extern inline int c_sname_cmp( c_sname_t const*, c_sname_t const* );
extern inline size_t c_sname_count( c_sname_t const* );
extern inline c_sname_t c_sname_dup( c_sname_t const* );
extern inline bool c_sname_empty( c_sname_t const* );
extern inline bool c_sname_equal( c_sname_t const*, c_sname_t const* );
extern inline c_type_t const* c_sname_global_type( c_sname_t const* );
extern inline void c_sname_init( c_sname_t* );
extern inline void c_sname_init_name( c_sname_t*, char* );
extern inline c_type_t const* c_sname_local_type( c_sname_t const* );
extern inline c_sname_t c_sname_move( c_sname_t* );
extern inline void c_sname_push_back_sname( c_sname_t*, c_sname_t* );
extern inline void c_sname_push_front_sname( c_sname_t*, c_sname_t* );
extern inline c_type_t const* c_sname_scope_type( c_sname_t const* );
extern inline void c_sname_set_scope_type( c_sname_t*, c_type_t const* );

/* vim:set et sw=2 ts=2: */
