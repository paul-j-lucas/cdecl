/*
**      cdecl -- C gibberish translator
**      src/cdecl_dym.c
**
**      Copyright (C) 2020-2025  Paul J. Lucas
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
 * Defines types, constants, and functions for printing `cdecl`-specific "Did
 * you mean ...?" suggestions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl_dym.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "cdecl_command.h"
#include "cdecl_keyword.h"
#include "cli_options.h"
#include "did_you_mean.h"
#include "gibberish.h"
#include "help.h"
#include "lexer.h"
#include "p_macro.h"
#include "set_options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <string.h>

/// @endcond

/**
 * @addtogroup printing-suggestions-group
 * @{
 */

/**
 * Used by visitor functions to pass and return data.
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

// local functions
PJL_DISCARD
static size_t prep_all( dym_kind_t, did_you_mean_t** ),
              prep_c_keywords( did_you_mean_t**, c_tpid_t ),
              prep_cdecl_keywords( did_you_mean_t** ),
              prep_commands( did_you_mean_t** ),
              prep_cli_options( did_you_mean_t** ),
              prep_help_options( did_you_mean_t** ),
              prep_macros( did_you_mean_t** ),
              prep_set_options( did_you_mean_t** ),
              prep_typedefs( did_you_mean_t** );

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans-up memory used by \a dym.
 *
 * @remarks \ref did_you_mean::user_data contains a `bool` indicating whether
 * \ref did_you_mean::known "known" was dynamically allocated: if so, frees it;
 * otherwise does nothing.
 *
 * @param dym A pointer to a \ref did_you_mean to clean-up.
 */
static void dym_cleanup( did_you_mean_t const *dym ) {
  if ( dym != NULL ) {
    bool const free_known = POINTER_CAST( bool, dym->user_data );
    if ( free_known )
      FREE( dym->known );
  }
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
 * @param dym A pointer to the \ref did_you_mean to check.
 * @return Returns `true` only if \a dam_lev_dist is "similar enough."
 */
NODISCARD
static bool is_similar_enough( did_you_mean_t const *dym ) {
  return  dym->dam_lev_dist <=
          STATIC_CAST( size_t, dym->known_len * SIMILAR_ENOUGH_PERCENT + 0.5 );
}

/**
 * Either determines the number of candidate suggestions or sets \ref
 * did_you_mean::known "known" of every element to a suggestion.
 *
 * @param kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param pdym If NULL, determines the number of candidate suggestions; if non-
 * NULL, sets \ref did_you_mean::known "known" of every element to a
 * suggestion.
 * @return If \a pdym is NULL, returns the number of suggestions; otherwise the
 * return value is unspecified.
 */
PJL_DISCARD
static size_t prep_all( dym_kind_t kinds, did_you_mean_t **pdym ) {
  return  ((kinds & DYM_COMMANDS) != DYM_NONE ?
            prep_commands( pdym ) : 0)

        + ((kinds & DYM_CLI_OPTIONS) != DYM_NONE ?
            prep_cli_options( pdym ) : 0)

        + ((kinds & DYM_HELP_OPTIONS) != DYM_NONE ?
            prep_help_options( pdym ) : 0)

        + ((kinds & DYM_SET_OPTIONS) != DYM_NONE ?
            prep_set_options( pdym ) : 0)

        + ((kinds & DYM_C_ATTRIBUTES) != DYM_NONE ?
            prep_c_keywords( pdym, C_TPID_ATTR ) : 0)

        + ((kinds & DYM_C_KEYWORDS) != DYM_NONE ?
            prep_c_keywords( pdym, C_TPID_NONE ) +
            prep_c_keywords( pdym, C_TPID_STORE ) : 0)

        + ((kinds & DYM_C_MACROS) != DYM_NONE ?
            prep_macros( pdym ) : 0)

        + ((kinds & DYM_C_TYPES) != DYM_NONE ?
            prep_c_keywords( pdym, C_TPID_BASE ) +
            prep_typedefs( pdym ) : 0)

        + ((kinds & DYM_CDECL_KEYWORDS) != DYM_NONE ?
            prep_cdecl_keywords( pdym ) : 0);
}

/**
 * Either determines the number of C keyword suggestions or sets \ref
 * did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @param tpid The type part ID that a keyword must have in order to be copied
 * (or counted).
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_c_keywords( did_you_mean_t **pdym, c_tpid_t tpid ) {
  size_t count = 0;
  FOREACH_C_KEYWORD( ck ) {
    if ( opt_lang_is_any( ck->lang_ids ) && c_tid_tpid( ck->tid ) == tpid ) {
      if ( pdym == NULL )
        ++count;
      else
        (*pdym)++->known = ck->literal;
    }
  } // for
  return count;
}

/**
 * Either determines the number of **cdecl** keyword suggestions or sets \ref
 * did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_cdecl_keywords( did_you_mean_t **pdym ) {
  assert( is_english_to_gibberish() );

  size_t count = 0;
  FOREACH_CDECL_KEYWORD( cdk ) {
    if ( !opt_lang_is_any( cdk->lang_ids ) )
      continue;
    char const *known;
    if ( cdk->lang_syn == NULL )
      known = cdk->literal;
    else if ( (known = c_lang_literal( cdk->lang_syn )) == NULL )
      continue;
    if ( pdym == NULL )
      ++count;
    else
      (*pdym)++->known = known;
  } // for
  return count;
}

/**
 * Either determines the number of **cdecl** command suggestions or sets \ref
 * did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_commands( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_CDECL_COMMAND( command ) {
    if ( opt_lang_is_any( command->lang_ids ) ) {
      if ( pdym == NULL )
        ++count;
      else
        (*pdym)++->known = command->literal;
    }
  } // for
  return count;
}

/**
 * Either determines the number of **cdecl** command-line-option suggestions or
 * sets \ref did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_cli_options( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_CLI_OPTION( opt ) {
    if ( pdym == NULL )
      ++count;
    else
      (*pdym)++->known = opt->name;
  } // for
  return count;
}

/**
 * Either determines the number of **cdecl** help-option suggestions or sets
 * \ref did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_help_options( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_HELP_OPTION( opt ) {
    if ( pdym == NULL )
      ++count;
    else
      (*pdym)++->known = *opt;
  } // for
  return count;
}

/**
 * A \ref p_macro visitor function that either counts a macro suggestion or
 * sets a \ref did_you_mean::known "known" to a suggestion.
 *
 * @param macro The \ref p_macro to visit.
 * @param visit_data A pointer to a \ref dym_rb_visit_data.
 * @return Always returns `false`.
 */
PJL_DISCARD
static bool prep_macro_vistor( p_macro_t const *macro, void *visit_data ) {
  assert( macro != NULL );
  assert( visit_data != NULL );

  if ( macro->is_dynamic &&
       !opt_lang_is_any( (*macro->dyn_fn)( /*ptoken=*/NULL ) ) ) {
    return false;
  }

  dym_rb_visit_data_t *const drvd = visit_data;
  if ( drvd->pdym == NULL )
    ++drvd->count;
  else
    (*drvd->pdym)++->known = macro->name;
  return false;
}

/**
 * Either determines the number of macro suggestions or sets \ref
 * did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_macros( did_you_mean_t **pdym ) {
  dym_rb_visit_data_t drvd = { pdym, 0 };
  p_macro_visit( &prep_macro_vistor, &drvd );
  return drvd.count;
}

/**
 * Either determines the number of **cdecl** set-option suggestions or sets
 * \ref did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_set_options( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_SET_OPTION( opt ) {
    switch ( opt->kind ) {
      case SET_OPTION_TOGGLE:
        if ( pdym == NULL ) {
          count += 2;
        } else {
          (*pdym)++->known = opt->name;
          (*pdym)->known = check_prefix_strdup( "no", 2, opt->name );
          (*pdym)->user_data = POINTER_CAST( void*, true );
          ++*pdym;
        }
        break;
      case SET_OPTION_AFF_ONLY:
        if ( pdym == NULL )
          ++count;
        else
          (*pdym)++->known = opt->name;
        break;
      case SET_OPTION_NEG_ONLY:
        if ( pdym == NULL ) {
          ++count;
        } else {
          (*pdym)->known = check_prefix_strdup( "no", 2, opt->name );
          (*pdym)->user_data = POINTER_CAST( void*, true );
          ++*pdym;
        }
        break;
    } // switch
  } // for
  return count;
}

/**
 * A \ref c_typedef visitor function that either counts a `typedef` suggestion
 * or sets a \ref did_you_mean::known "known" to a suggestion.
 *
 * @param tdef The c_typedef to visit.
 * @param visit_data A pointer to a \ref dym_rb_visit_data.
 * @return Always returns `false`.
 */
PJL_DISCARD
static bool prep_typedef_visitor( c_typedef_t const *tdef, void *visit_data ) {
  assert( tdef != NULL );
  assert( visit_data != NULL );

  if ( opt_lang_is_any( tdef->lang_ids ) ) {
    dym_rb_visit_data_t *const drvd = visit_data;
    if ( drvd->pdym == NULL ) {
      ++drvd->count;
    } else {
      char const *const name = c_sname_gibberish( &tdef->ast->sname );
      (*drvd->pdym)->known = check_strdup( name );
      (*drvd->pdym)->user_data = POINTER_CAST( void*, true );
      ++*drvd->pdym;
    }
  }
  return false;
}

/**
 * Either determines the number of `typedef` suggestions or sets \ref
 * did_you_mean::known "known" of every element to a suggestion.
 *
 * @param pdym If NULL, determines the number of suggestions; if non-NULL, sets
 * \ref did_you_mean::known "known" of every element to a suggestion.  Upon
 * return, the pointer to which \a pdym points is advanced by the number of
 * suggestions.
 * @return If \a pdym is NULL, returns said number of suggestions; otherwise
 * the return value is unspecified.
 */
PJL_DISCARD
static size_t prep_typedefs( did_you_mean_t **pdym ) {
  dym_rb_visit_data_t drvd = { pdym, 0 };
  c_typedef_visit( &prep_typedef_visitor, &drvd );
  return drvd.count;
}

////////// extern functions ///////////////////////////////////////////////////

void cdecl_dym_free( did_you_mean_t const *dym_array ) {
  dym_free( dym_array, &dym_cleanup );
}

did_you_mean_t const* cdecl_dym_new( dym_kind_t kinds, char const *unknown ) {
  assert( unknown != NULL );

  if ( kinds == DYM_NONE )
    return NULL;

  size_t const dym_size = prep_all( kinds, /*pdym=*/NULL );
  if ( dym_size == 0 )
    return NULL;

  did_you_mean_t *const dym_array =
    calloc( dym_size + 1, sizeof( did_you_mean_t ) );

  did_you_mean_t *dym = dym_array;
  prep_all( kinds, &dym );

  return dym_calc( unknown, dym_array, &is_similar_enough, &dym_cleanup ) ?
    dym_array : NULL;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
