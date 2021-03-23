/*
**      cdecl -- C gibberish translator
**      src/did_you_mean.c
**
**      Copyright (C) 2020-2021  Paul J. Lucas, et al.
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
 * Declares types, constants, and functions for implementing "Did you mean
 * ...?" suggestions.
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
#include "set_options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/**
 * Used by copy_typedefs() and copy_typedef_visitor() to pass and return data.
 */
struct copy_typedef_visitor_data {
  /// Pointer to a pointer to a candidate list or NULL to just get the count.
  did_you_mean_t  **pdym;

  size_t            count;              ///< The count.
};
typedef struct copy_typedef_visitor_data copy_typedef_visitor_data_t;

/**
 * The signature for a function passed to qsort().
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the data pointed to by \a i_data is less than, equal to, or
 * greater than the data pointed to by \a j_data.
 */
typedef int (*qsort_cmp_fn_t)( void const *i_data, void const *j_data );

/**
 * The edit distance must be less than or equal to this percent of a target
 * string's length in order to be considered "similar enough" to be a
 * reasonable suggestion.
 */
static unsigned const SIMILAR_ENOUGH_PERCENT = 40;

////////// local functions ////////////////////////////////////////////////////

/**
 * Duplicates \a s prefixed by \a prefix.
 *
 * @param prefix The null-terminated prefix string to duplicate.
 * @param s The null-terminated string to duplicate.
 * @return Returns a copy of \a s prefixed by \a prefix.
 */
PJL_WARN_UNUSED_RESULT
static char* check_prefix_strdup( char const *prefix, char const *s ) {
  size_t const prefix_len = strlen( prefix );
  char *const dup_s = MALLOC( char*, prefix_len + strlen( s ) + 1/*\0*/ );
  IF_EXIT( dup_s == NULL, EX_OSERR );
  strcpy( dup_s, prefix );
  strcpy( dup_s + prefix_len, s );
  return dup_s;
}

/**
 * Copies cdecl commands in the current language to the candidate list pointed
 * to by \a pdym.  If \a pdym is NULL, only counts the number of commands.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer
 * or NULL to just count commands, not copy.  If not NULL, on return, the
 * pointed-to pointer is incremented.
 * @return Returns said number of commands.
 */
PJL_NOWARN_UNUSED_RESULT
static size_t copy_commands( did_you_mean_t **const pdym ) {
  size_t count = 0;
  for ( c_command_t const *c = CDECL_COMMANDS; c->literal != NULL; ++c ) {
    if ( (c->lang_ids & opt_lang) != LANG_NONE ) {
      if ( pdym != NULL )
        (*pdym)++->token = check_strdup( c->literal );
      ++count;
    }
  } // for
  return count;
}

/**
 * Copies cdecl command-line options to the candidate list pointed to by \a
 * pdym.  If \a pdym is NULL, only counts the number of options.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer
 * or NULL to just count options, not copy.  If not NULL, on return, the
 * pointed-to pointer is incremented.
 * @return Returns said number of options.
 */
PJL_NOWARN_UNUSED_RESULT
static size_t copy_cli_options( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_CLI_OPTION( opt ) {
    if ( pdym != NULL )
      (*pdym)++->token = check_strdup( opt->name );
    ++count;
  } // for
  return count;
}

/**
 * Copies C/C++ keywords in the current language to the candidate list pointed
 * to by \a pdym.  If \a pdym is NULL, only counts the number of keywords.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer
 * or NULL to just count keywords, not copy.  If not NULL, on return, the
 * pointed-to pointer is incremented.
 * @param tpid The type part ID that a keyword must have in order to be copied
 * (or counted).
 * @return Returns said number of keywords.
 */
PJL_NOWARN_UNUSED_RESULT
static size_t copy_keywords( did_you_mean_t **const pdym,
                             c_type_part_id_t tpid ) {
  size_t count = 0;
  FOREACH_KEYWORD( k ) {
    if ( (k->lang_ids & opt_lang) != LANG_NONE &&
         c_type_id_tpid( k->type_id ) == tpid ) {
      if ( pdym != NULL )
        (*pdym)++->token = check_strdup( k->literal );
      ++count;
    }
  } // for
  return count;
}

/**
 * Copies cdecl `set` options to the candidate list pointed to by \a pdym.  If
 * \a pdym is NULL, only counts the number of options.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer
 * or NULL to just count options, not copy.  If not NULL, on return, the
 * pointed-to pointer is incremented.
 * @return Returns said number of options.
 */
static size_t copy_set_options( did_you_mean_t **const pdym ) {
  size_t count = 0;
  FOREACH_SET_OPTION( opt ) {
    switch ( opt->type ) {
      case SET_TOGGLE:
        if ( pdym != NULL ) {
          (*pdym)++->token = check_strdup( opt->name );
          (*pdym)++->token = check_prefix_strdup( "no",  opt->name );
        }
        ++count;
        break;
      case SET_AFF_ONLY:
        if ( pdym != NULL )
          (*pdym)++->token = check_strdup( opt->name );
        break;
      case SET_NEG_ONLY:
        if ( pdym != NULL )
          (*pdym)++->token = check_prefix_strdup( "no",  opt->name );
        break;
    } // switch
    ++count;
  } // for
  return count;
}

/**
 * A <code>\ref c_typedef</code> visitor function to copy names of types that
 * are only valid in the current language to the candidate list pointed to 
 *
 * @param tdef The `c_typedef` to visit.
 * @param data A pointer to a <code>\ref copy_typedef_visitor_data</code>.
 * @return Always returns `false`.
 */
static bool copy_typedef_visitor( c_typedef_t const *tdef, void *data ) {
  if ( (tdef->lang_ids & opt_lang) != LANG_NONE ) {
    copy_typedef_visitor_data_t *const ctvd = data;
    if ( ctvd->pdym != NULL ) {
      char const *const name = c_ast_full_name( tdef->ast );
      (*ctvd->pdym)++->token = check_strdup( name );
    }
    ++ctvd->count;
  }
  return false;
}

/**
 * Counts the number of `typedef`s that are only valid in the current language.
 *
 * @param pdym A pointer to the current <code>\ref did_you_mean</code> pointer
 * or NULL to just count typedefs, not copy.  If not NULL, on return, the
 * pointed-to pointer is incremented.
 * @return Returns said number of `typedef`s.
 */
PJL_NOWARN_UNUSED_RESULT
static size_t copy_typedefs( did_you_mean_t **const pdym ) {
  copy_typedef_visitor_data_t ctvd = { pdym, 0 };
  c_typedef_visit( &copy_typedef_visitor, &ctvd );
  return ctvd.count;
}

/**
 * Comparison function for two <code>\ref did_you_mean</code> objects.
 *
 * @param i_dym A pointer to the first <code>\ref did_you_mean</code>.
 * @param j_dym A pointer to the second <code>\ref did_you_mean</code>.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_dym is
 * less than, equal to, or greater than \a j_dym, respectively.
 */
PJL_WARN_UNUSED_RESULT
static int dym_cmp( did_you_mean_t const *i_dym, did_you_mean_t const *j_dym ) {
  int const cmp = (int)i_dym->dam_lev_dist - (int)j_dym->dam_lev_dist;
  return cmp != 0 ? cmp : strcmp( i_dym->token, j_dym->token );
}

/**
 * Frees memory used by \a dym.
 *
 * @param dym A pointer to the first <code>\ref did_you_mean</code> to free and
 * continuing until `token` is NULL.
 */
static void dym_free_tokens( did_you_mean_t const *dym ) {
  assert( dym != NULL );
  while ( dym->token != NULL )
    FREE( dym++->token );
}

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
 * some percentage, say, 40%, of the target string's length in order to be
 * considered "similar enough" to be a reasonable suggestion.
 *
 * @param dam_lev_dist A Damerau-Levenshtein edit distance.
 * @param percent The edit distance must be less than or equal to this percent
 * of \a target_len in order to be considered "similar enough" to be a
 * reasonable suggestion.
 * @param target_len The length of the target string.
 * @return Returns `true` only if \a dam_lev_dist is "similar enough."
 */
PJL_WARN_UNUSED_RESULT
static bool is_similar_enough( dam_lev_t dam_lev_dist, unsigned percent,
                               size_t target_len ) {
  assert( percent < 100 );
  double const percent_fract = percent / 100.0;
  return dam_lev_dist <= (dam_lev_t)((double)target_len * percent_fract + 0.5);
}

////////// extern functions ///////////////////////////////////////////////////

void dym_free( did_you_mean_t const *dym_array ) {
  if ( dym_array != NULL ) {
    dym_free_tokens( dym_array );
    FREE( dym_array );
  }
}

did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_token ) {
  if ( kinds == DYM_NONE )
    return NULL;
  assert( unknown_token != NULL );

  size_t dym_size = 0;
  if ( (kinds & DYM_COMMANDS) != DYM_NONE )
    dym_size += copy_commands( /*pdym=*/NULL );
  if ( (kinds & DYM_CLI_OPTIONS) != DYM_NONE )
    dym_size += copy_cli_options( /*pdym=*/NULL );
  if ( (kinds & DYM_SET_OPTIONS) != DYM_NONE )
    dym_size += copy_set_options( /*pdym=*/NULL );
  if ( (kinds & DYM_C_KEYWORDS) != DYM_NONE )
    dym_size += copy_keywords( /*pdym=*/NULL, C_TPID_NONE )
              + copy_keywords( /*pdym=*/NULL, C_TPID_STORE );
  if ( (kinds & DYM_C_ATTRIBUTES) != DYM_NONE )
    dym_size += copy_keywords( /*pdym=*/NULL, C_TPID_ATTR );
  if ( (kinds & DYM_C_TYPES) != DYM_NONE )
    dym_size += copy_keywords( /*pdym=*/NULL, C_TPID_BASE )
              + copy_typedefs( /*pdym=*/NULL );

  if ( dym_size == 0 )
    return NULL;

  did_you_mean_t *const dym_array = MALLOC( did_you_mean_t, dym_size + 1 );
  did_you_mean_t *dym = dym_array;

  if ( (kinds & DYM_COMMANDS) != DYM_NONE ) {
    copy_commands( &dym );
  }
  if ( (kinds & DYM_CLI_OPTIONS) != DYM_NONE ) {
    copy_cli_options( &dym );
  }
  if ( (kinds & DYM_SET_OPTIONS) != DYM_NONE ) {
    copy_set_options( &dym );
  }
  if ( (kinds & DYM_C_KEYWORDS) != DYM_NONE ) {
    copy_keywords( &dym, C_TPID_NONE );
    copy_keywords( &dym, C_TPID_STORE );
  }
  if ( (kinds & DYM_C_ATTRIBUTES) != DYM_NONE ) {
    copy_keywords( &dym, C_TPID_ATTR );
  }
  if ( (kinds & DYM_C_TYPES) != DYM_NONE ) {
    copy_keywords( &dym, C_TPID_BASE );
    copy_typedefs( &dym );
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
  qsort(
    dym_array, dym_size, sizeof( did_you_mean_t ),
    (qsort_cmp_fn_t)&dym_cmp
  );

  dam_lev_t const best_dist = dym_array->dam_lev_dist;
  size_t const best_len = strlen( dym_array->token );

  if ( is_similar_enough( best_dist, SIMILAR_ENOUGH_PERCENT, best_len ) ) {
    // include all candidates that have the same distance
    size_t best_count = 0;
    for ( dym = dym_array;
          ++best_count < dym_size && (++dym)->dam_lev_dist == best_dist; )
      ;
    //
    // Free tokens past the best ones and set the one past the last to NULL to
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
