/*
**      cdecl -- C gibberish translator
**      src/did_you_mean.c
**
**      Copyright (C) 2020-2024  Paul J. Lucas
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
 * Defines types, constants, and functions for printing suggestions for "Did
 * you mean ...?"
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "did_you_mean.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_sname.h"
#include "c_typedef.h"
#include "cdecl_command.h"
#include "cdecl_keyword.h"
#include "cli_options.h"
#include "dam_lev.h"
#include "p_macro.h"
#include "set_options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <stdlib.h>
#include <string.h>

/**
 * @addtogroup printing-suggestions-group
 * @{
 */

/**
 * Used by copy_typedefs() and copy_typedef_visitor() to pass and return data.
 */
struct dym_rb_visit_data {
  /// Pointer to a pointer to a candidate list or NULL to just get the count.
  did_you_mean_t  **pdym;

  size_t            count;              ///< The count.
};
typedef struct dym_rb_visit_data dym_rb_visit_data_t;

/**
 * The edit distance must be less than or equal to this percent of a target
 * string's length in order to be considered "similar enough" to be a
 * reasonable suggestion.
 */
static double const SIMILAR_ENOUGH_PERCENT = .37;

////////// local functions ////////////////////////////////////////////////////

/**
 * Copies C/C++ keywords in the current language to the candidate list pointed
 * to by \a pdym; if \a pdym is NULL, only counts the number of keywords.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count keywords, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @param tpid The type part ID that a keyword must have in order to be copied
 * (or counted).
 * @return If \a pdym is NULL, returns said number of keywords; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_c_keywords( did_you_mean_t **const pdym, c_tpid_t tpid ) {
  size_t count = 0;
  FOREACH_C_KEYWORD( ck ) {
    if ( opt_lang_is_any( ck->lang_ids ) && c_tid_tpid( ck->tid ) == tpid ) {
      if ( pdym == NULL )
        ++count;
      else
        (*pdym)++->literal = check_strdup( ck->literal );
    }
  } // for
  return count;
}

/**
 * Copies **cdecl** keywords in the current language to the candidate list
 * pointed to by \a pdym; if \a pdym is NULL, only counts the number of
 * keywords.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count keywords, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return If \a pdym is NULL, returns said number of keywords; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_cdecl_keywords( did_you_mean_t **const pdym ) {
  assert( cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH );

  size_t count = 0;
  FOREACH_CDECL_KEYWORD( cdk ) {
    if ( !opt_lang_is_any( cdk->lang_ids ) )
      continue;
    char const *literal;
    if ( cdk->lang_syn == NULL )
      literal = cdk->literal;
    else if ( (literal = c_lang_literal( cdk->lang_syn )) == NULL )
      continue;
    if ( pdym == NULL )
      ++count;
    else
      (*pdym)++->literal = check_strdup( literal );
  } // for
  return count;
}

/**
 * Copies **cdecl** commands in the current language to the candidate list
 * pointed to by \a pdym; if \a pdym is NULL, only counts the number of
 * commands.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count commands, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return If \a pdym is NULL, returns said number of commands; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_commands( did_you_mean_t **const pdym ) {
  size_t count = 0;
  FOREACH_CDECL_COMMAND( command ) {
    if ( opt_lang_is_any( command->lang_ids ) ) {
      if ( pdym == NULL )
        ++count;
      else
        (*pdym)++->literal = check_strdup( command->literal );
    }
  } // for
  return count;
}

/**
 * Copies **cdecl** command-line options to the candidate list pointed to by \a
 * pdym; if \a pdym is NULL, only counts the number of options.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count options, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return If \a pdym is NULL, returns said number of options; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_cli_options( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_CLI_OPTION( opt ) {
    if ( pdym == NULL )
      ++count;
    else
      (*pdym)++->literal = check_strdup( opt->name );
  } // for
  return count;
}

/**
 * A \ref p_macro visitor function to copy names of macro that are only valid
 * in the current language to the candidate list pointed to.
 *
 * @param macro The \ref p_macro to visit.
 * @param data A pointer to a \ref dym_rb_visit_data.
 * @return Always returns `false`.
 */
PJL_DISCARD
static bool copy_macro_vistor( p_macro_t const *macro, void *data ) {
  assert( macro != NULL );
  assert( data != NULL );

  if ( macro->is_dynamic &&
       !opt_lang_is_any( (*macro->dyn_fn)( /*ptoken=*/NULL ) ) ) {
    return false;
  }

  dym_rb_visit_data_t *const drvd = data;
  if ( drvd->pdym == NULL ) {
    ++drvd->count;
  } else {
    (*drvd->pdym)++->literal = check_strdup( macro->name );
  }
  return false;
}

/**
 * Counts the number of macros that are only valid in the current language.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count macros, not copy.  If not NULL, on return, the pointed-to pointer
 * is incremented.
 * @return If \a pdym is NULL, returns said number of macross; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_macros( did_you_mean_t **const pdym ) {
  dym_rb_visit_data_t drvd = { pdym, 0 };
  p_macro_visit( &copy_macro_vistor, &drvd );
  return drvd.count;
}

/**
 * Copies **cdecl** `set` options to the candidate list pointed to by \a pdym;
 * if \a pdym is NULL, only counts the number of options.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count options, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return If \a pdym is NULL, returns said number of options; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_set_options( did_you_mean_t **const pdym ) {
  size_t count = 0;
  FOREACH_SET_OPTION( opt ) {
    switch ( opt->kind ) {
      case SET_OPTION_TOGGLE:
        if ( pdym == NULL ) {
          count += 2;
        } else {
          (*pdym)++->literal = check_strdup( opt->name );
          (*pdym)++->literal = check_prefix_strdup( "no", 2, opt->name );
        }
        break;
      case SET_OPTION_AFF_ONLY:
        if ( pdym == NULL )
          ++count;
        else
          (*pdym)++->literal = check_strdup( opt->name );
        break;
      case SET_OPTION_NEG_ONLY:
        if ( pdym == NULL )
          ++count;
        else
          (*pdym)++->literal = check_prefix_strdup( "no", 2, opt->name );
        break;
    } // switch
  } // for
  return count;
}

/**
 * A \ref c_typedef visitor function to copy names of types that are only valid
 * in the current language to the candidate list pointed to.
 *
 * @param tdef The c_typedef to visit.
 * @param data A pointer to a \ref dym_rb_visit_data.
 * @return Always returns `false`.
 */
PJL_DISCARD
static bool copy_typedef_visitor( c_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  if ( opt_lang_is_any( tdef->lang_ids ) ) {
    dym_rb_visit_data_t *const drvd = data;
    if ( drvd->pdym == NULL ) {
      ++drvd->count;
    } else {
      char const *const name = c_sname_full_name( &tdef->ast->sname );
      (*drvd->pdym)++->literal = check_strdup( name );
    }
  }
  return false;
}

/**
 * Counts the number of `typedef`s that are only valid in the current language.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count typedefs, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return If \a pdym is NULL, returns said number of `typedef`s; otherwise the
 * return value is unspecifed.
 */
PJL_DISCARD
static size_t copy_typedefs( did_you_mean_t **const pdym ) {
  dym_rb_visit_data_t drvd = { pdym, 0 };
  c_typedef_visit( &copy_typedef_visitor, &drvd );
  return drvd.count;
}

/**
 * Comparison function for two \ref did_you_mean objects.
 *
 * @param i_dym A pointer to the first \ref did_you_mean.
 * @param j_dym A pointer to the second \ref did_you_mean.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_dym is
 * less than, equal to, or greater than \a j_dym, respectively.
 */
NODISCARD
static int dym_cmp( did_you_mean_t const *i_dym, did_you_mean_t const *j_dym ) {
  ssize_t const cmp =
    STATIC_CAST( ssize_t, i_dym->dam_lev_dist ) -
    STATIC_CAST( ssize_t, j_dym->dam_lev_dist );
  return  cmp != 0 ? STATIC_CAST( int, cmp ) :
          strcmp( i_dym->literal, j_dym->literal );
}

/**
 * Frees memory used by \a dym.
 *
 * @param dym A pointer to the first \ref did_you_mean to free and continuing
 * until `literal` is NULL.
 */
static void dym_free_literals( did_you_mean_t const *dym ) {
  assert( dym != NULL );
  while ( dym->literal != NULL )
    FREE( dym++->literal );
}

/**
 * Gets whether \a dam_lev_dist is "similar enough" to be a candidate.
 *
 * @remarks Using a Damerau-Levenshtein edit distance alone to implement "Did
 * you mean ...?" can yield poor results if you just always use the results
 * with the least distance.  For example, given a source string of "fixed" and
 * the best target string of "float", it's probably safe to assume that because
 * "fixed" is so different from "float" that there's no way "float" was meant.
 * It would be better to offer _no_ suggestions than not-even-close
 * suggestions.
 * @par
 * Hence, you need a heuristic to know whether a least edit distance is
 * "similar enough" to the target string even to bother offering suggestions.
 * This can be done by checking whether the distance is less than or equal to
 * some percentage of the target string's length in order to be considered
 * "similar enough" to be a reasonable suggestion.
 *
 * @param dam_lev_dist A Damerau-Levenshtein edit distance.
 * @param percent A value in the range (0,1).  The edit distance must be less
 * than or equal to this percent of \a target_len in order to be considered
 * "similar enough" to be a reasonable suggestion.
 * @param target_len The length of the target string.
 * @return Returns `true` only if \a dam_lev_dist is "similar enough."
 */
NODISCARD
static bool is_similar_enough( size_t dam_lev_dist, double percent,
                               size_t target_len ) {
  assert( percent > 0 && percent < 1 );
  return dam_lev_dist <=
    STATIC_CAST( size_t, STATIC_CAST( double, target_len ) * percent + 0.5 );
}

////////// extern functions ///////////////////////////////////////////////////

void dym_free( did_you_mean_t const *dym_array ) {
  if ( dym_array != NULL ) {
    dym_free_literals( dym_array );
    FREE( dym_array );
  }
}

did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_literal ) {
  if ( kinds == DYM_NONE )
    return NULL;
  assert( unknown_literal != NULL );

  // Pre-flight to calculate array size; the order here doesn't matter.
  size_t const dym_size =
    ((kinds & DYM_COMMANDS) != DYM_NONE ?
      copy_commands( /*pdym=*/NULL ) : 0) +
    ((kinds & DYM_CLI_OPTIONS) != DYM_NONE ?
      copy_cli_options( /*pdym=*/NULL ) : 0) +
    ((kinds & DYM_SET_OPTIONS) != DYM_NONE ?
      copy_set_options( /*pdym=*/NULL ) : 0) +
    ((kinds & DYM_C_KEYWORDS) != DYM_NONE ?
      copy_c_keywords( /*pdym=*/NULL, C_TPID_NONE ) +
      copy_c_keywords( /*pdym=*/NULL, C_TPID_STORE ) : 0) +
    ((kinds & DYM_C_ATTRIBUTES) != DYM_NONE ?
      copy_c_keywords( /*pdym=*/NULL, C_TPID_ATTR ) : 0) +
    ((kinds & DYM_C_MACROS) != DYM_NONE ?
      copy_macros( /*pdym=*/NULL ) : 0) +
    ((kinds & DYM_C_TYPES) != DYM_NONE ?
      copy_c_keywords( /*pdym=*/NULL, C_TPID_BASE ) +
      copy_typedefs( /*pdym=*/NULL ) : 0) +
    ((kinds & DYM_CDECL_KEYWORDS) != DYM_NONE ?
      copy_cdecl_keywords( /*pdym=*/NULL ) : 0);

  if ( dym_size == 0 )
    return NULL;                        // LCOV_EXCL_LINE

  did_you_mean_t *const dym_array = MALLOC( did_you_mean_t, dym_size + 1 );
  did_you_mean_t *dym = dym_array;

  // The order here doesn't matter either.
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
    copy_c_keywords( &dym, C_TPID_NONE );
    copy_c_keywords( &dym, C_TPID_STORE );
  }
  if ( (kinds & DYM_C_ATTRIBUTES) != DYM_NONE ) {
    copy_c_keywords( &dym, C_TPID_ATTR );
  }
  if ( (kinds & DYM_C_MACROS) != DYM_NONE ) {
    copy_macros( &dym );
  }
  if ( (kinds & DYM_C_TYPES) != DYM_NONE ) {
    copy_c_keywords( &dym, C_TPID_BASE );
    copy_typedefs( &dym );
  }
  if ( (kinds & DYM_CDECL_KEYWORDS) != DYM_NONE ) {
    copy_cdecl_keywords( &dym );
  }
  MEM_ZERO( dym );                      // one past last is zero'd

  // calculate the maximum source and target lengths
  size_t const source_len = strlen( unknown_literal );
  size_t max_target_len = 0;
  for ( dym = dym_array; dym->literal != NULL; ++dym ) {
    size_t const len = strlen( dym->literal );
    if ( len > max_target_len )
      max_target_len = len;
  } // for

  /*
   * Adapted from the code:
   * <https://github.com/git/git/blob/3a0b884caba2752da0af626fb2de7d597c844e8b/help.c#L516>
   */

  // calculate Damerau-Levenshtein edit distance for all candidates
  void *const dam_lev_mem = dam_lev_new( source_len, max_target_len );
  for ( dym = dym_array; dym->literal != NULL; ++dym ) {
    dym->dam_lev_dist = dam_lev_dist(
      dam_lev_mem,
      unknown_literal, source_len,
      dym->literal, strlen( dym->literal )
    );
  } // for
  free( dam_lev_mem );

  // sort by Damerau-Levenshtein distance
  qsort(
    dym_array, dym_size, sizeof( did_you_mean_t ),
    POINTER_CAST( qsort_cmp_fn_t, &dym_cmp )
  );

  size_t const best_dist = dym_array->dam_lev_dist;
  if ( best_dist == 0 ) {
    //
    // This means unknown_literal was an exact match for a literal which means
    // we shouldn't suggest it for itself.
    //
    goto none;
  }

  size_t const best_len = strlen( dym_array->literal );
  if ( !is_similar_enough( best_dist, SIMILAR_ENOUGH_PERCENT, best_len ) )
    goto none;

  // include all candidates that have the same distance
  for ( dym = dym_array;
        (++dym)->literal != NULL && dym->dam_lev_dist == best_dist; )
    ;

  //
  // Free literals past the best ones and set the one past the last to NULL to
  // mark the end.
  //
  dym_free_literals( dym );
  dym->literal = NULL;

  return dym_array;

none:
  dym_free( dym_array );
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
