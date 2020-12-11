/*
**      cdecl -- C gibberish translator
**      src/did_you_mean.c
**
**      Copyright (C) 2020  Paul J. Lucas, et al.
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

// local
#include "pjl_config.h"                 /* must go first */
#include "did_you_mean.h"
#include "c_ast.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_sname.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets whether \a dam_lev_dist is "similar enough" to be a candidate.
 *
 * Using a Damerau-Levenshtein edit distance alone to implement "Did you mean
 * ...?" can yield poor results if you just always use the results with the
 * least distance.  For example, given a source string of "fixed" and the best
 * target string of "float", it's probably safe to assume that because "fixed"
 * is so different from "float" that there's no way "float" was meant.  It
 * would be better to offer _no_ suggestions than not-even-close suggestions.
 *
 * Hence, you need a heuristic to know whether a least edit distance is
 * "similar enough" to the target string even to bother offering suggestions.
 * This can be done by checking whether the distance is less than or equal to
 * some percentage, say, 33%, of the target string's length.  This means that
 * the source string must be at least a 66% match of the target string in order
 * to be considered "similar enough" to be a reasonable suggestion.
 *
 * @param dam_lev_dist A Damerau-Levenshtein edit distance.
 * @param target_len The length of the target string.
 * @return Returns `true` only if \a dam_lev_dist is "similar enough."
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_similar_enough( dam_lev_t dam_lev_dist,
                                      size_t target_len ) {
  return dam_lev_dist <= (dam_lev_t)((double)target_len * 0.33 + 0.5);
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Copies cdecl commands to the candidate list.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer.
 * On return, it's incremented.
 */
static void copy_commands( did_you_mean_t **const pdym ) {
  for ( c_command_t const *c = CDECL_COMMANDS; c->literal != NULL; ++c ) {
    if ( (c->lang_ids & opt_lang) != LANG_NONE )
      (*pdym)++->token = check_strdup( c->literal );
  } // for
}

/**
 * Copies C/C++ keywords to the candidate list.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer.
 * On return, it's incremented.
 * @param copy_types If `true`, copy only keywords that are types; if `false`,
 * copy only keywords that are not types.
 */
static void copy_keywords( did_you_mean_t **const pdym,
                           bool copy_types ) {
  for ( c_keyword_t const *k = NULL; (k = c_keyword_next( k )) != NULL; ) {
    if ( (k->lang_ids & opt_lang) == LANG_NONE )
      continue;
    bool const is_base_type = c_type_id_part_id( k->type_id ) == TPID_BASE;
    if ( copy_types ) {
      if ( !is_base_type )
        continue;
    } else {
      if ( is_base_type )
        continue;
    }
    (*pdym)++->token = check_strdup( k->literal );
  } // for
}

/**
 * A <code>\ref c_typedef</code> visitor function to copy `typedef` names to
 * the candidate list.
 *
 * @param type The `c_typedef` to visit.
 * @param data A pointer to the current <code>\ref did_you_mean</code>
 * pointer.  On return, it's incremented.
 * @return Always returns `false`.
 */
static bool copy_typedef_visitor( c_typedef_t const *type, void *data ) {
  char const *const name = c_sname_full_name( &type->ast->sname );
  did_you_mean_t **const pdym = data;
  (*pdym)++->token = check_strdup( name );
  return false;
}

/**
 * Counts the number of cdecl commands in the current language.
 *
 * @return Returns said number of commands.
 */
PJL_WARN_UNUSED_RESULT
static size_t count_commands( void ) {
  size_t n = 0;
  for ( c_command_t const *c = CDECL_COMMANDS; c->literal != NULL; ++c ) {
    if ( (c->lang_ids & opt_lang) != LANG_NONE )
      ++n;
  } // for
  return n;
}

/**
 * Counts the number of C/C++ keywords in the current language.
 *
 * @param count_types If `true`, count only keywords that are types; if
 * `false`, count only keywords that are not types.
 * @return Returns said number of keywords.
 */
PJL_WARN_UNUSED_RESULT
static size_t count_keywords( bool count_types ) {
  size_t n = 0;
  for ( c_keyword_t const *k = NULL; (k = c_keyword_next( k )) != NULL; ) {
    if ( (k->lang_ids & opt_lang) == LANG_NONE )
      continue;
    bool const is_base_type = c_type_id_part_id( k->type_id ) == TPID_BASE;
    if ( count_types ) {
      if ( !is_base_type )
        continue;
    } else {
      if ( is_base_type )
        continue;
    }
    ++n;
  } // for
  return n;
}

/**
 * A <code>\ref c_typedef</code> visitor function to count the number of
 * `typedef`s.
 *
 * @param type The <code>\ref c_typedef</code> to visit.
 * @param data A pointer to the current count.
 * @return Always returns `false`.
 */
static bool count_typedef_visitor( c_typedef_t const *type, void *data ) {
  (void)type;
  size_t *const n = data;
  ++*n;
  return false;
}

/**
 * Counts the number of `typedef`s.
 *
 * @return Returns said number of `typedef`s.
 */
PJL_WARN_UNUSED_RESULT
static size_t count_typedefs( void ) {
  size_t n = 0;
  c_typedef_visit( &count_typedef_visitor, &n );
  return n;
}

/**
 * Frees memory used by \a dym.
 *
 * @param dym A pointer to the first <code>\ref did_you_mean</code> to free and
 * continuing until `token` is null.
 */
static void dym_free_tokens( did_you_mean_t const *dym ) {
  while ( dym->token != NULL )
    FREE( dym++->token );
}

/**
 * Comparison function for **qsort**(3) that compares two <code>\ref
 * did_you_mean</code> objects.
 *
 * @param i_data A pointer to the first <code>\ref did_you_mean</code>.
 * @param j_data A pointer to the second <code>\ref did_you_mean</code>.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
static int qsort_did_you_mean_cmp( void const *i_data, void const *j_data ) {
  did_you_mean_t const *const i_dym = i_data;
  did_you_mean_t const *const j_dym = j_data;
  return i_dym->dam_lev_dist != j_dym->dam_lev_dist ?
    (int)i_dym->dam_lev_dist - (int)j_dym->dam_lev_dist :
    strcmp( i_dym->token, j_dym->token );
}

////////// extern functions ///////////////////////////////////////////////////

void dym_free( did_you_mean_t const *dym_array ) {
  assert( dym_array != NULL );
  dym_free_tokens( dym_array );
  FREE( dym_array );
}

did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_token ) {
  if ( kinds == DYM_NONE )
    return NULL;
  assert( unknown_token != NULL );

  size_t size = 0;
  if ( (kinds & DYM_COMMANDS) != DYM_NONE )
    size += count_commands();
  if ( (kinds & DYM_C_KEYWORDS) != DYM_NONE )
    size += count_keywords( /*count_types=*/false );
  if ( (kinds & DYM_C_TYPES) != DYM_NONE )
    size += count_keywords( /*count_types=*/true ) + count_typedefs();

  did_you_mean_t *const dym_array = MALLOC( did_you_mean_t, size + 1 );
  did_you_mean_t *dym = dym_array;

  if ( (kinds & DYM_COMMANDS) != DYM_NONE ) {
    copy_commands( &dym );
  }
  if ( (kinds & DYM_C_KEYWORDS) != DYM_NONE ) {
    copy_keywords( &dym, /*copy_types=*/false );
  }
  if ( (kinds & DYM_C_TYPES) != DYM_NONE ) {
    copy_keywords( &dym, /*copy_types=*/true );
    c_typedef_visit( &copy_typedef_visitor, &dym );
  }
  MEM_ZERO( dym );                      // one past last is zero'd

  /*
   * Adapted from the code:
   * <https://github.com/git/git/blob/3a0b884caba2752da0af626fb2de7d597c844e8b/help.c#L516>
   */

  // calculate Damerau-Levenshtein edit distance for all candidates
  for ( dym = dym_array; dym->token != NULL; ++dym )
    dym->dam_lev_dist = dam_lev_dist( unknown_token, dym->token );

  // sort by Damerau-Levenshtein distance
  qsort( dym_array, size, sizeof( did_you_mean_t ), &qsort_did_you_mean_cmp );

  dam_lev_t const best_dist = dym_array->dam_lev_dist;
  size_t const best_len = strlen( dym_array->token );
  size_t best_count = 0;

  if ( is_similar_enough( best_dist, best_len ) ) {
    // include all candidates that have the same distance
    for ( dym = dym_array;
          ++best_count < size && (++dym)->dam_lev_dist == best_dist; )
      ;
    //
    // Free tokens past the best ones and set the one past the last to null to
    // mark the end.
    //
    dym_free_tokens( dym_array + best_count );
    dym_array[ best_count ].token = NULL;

    return dym_array;
  }

  dym_free( dym_array );
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
