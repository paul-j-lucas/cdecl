/*
**      cdecl -- C gibberish translator
**      src/p_predefine.c
**
**      Copyright (C) 2024-2026  Paul J. Lucas
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
#include "c_lang.h"
#include "cdecl.h"
#include "lexer.h"
#include "options.h"
#include "p_macro.h"
#include "p_token.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/// @endcond

/**
 * @ingroup p-macro-group
 * @defgroup p-predefined-macros-group Predefined macros
 * Functions that implement predefined macros.
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets the current value of the `__DATE__` macro.
 *
 * @return Returns said value.
 *
 * @warning The pointer returned is to a static buffer.
 *
 * @sa get___TIME___str()
 */
static char const* get___DATE___str( void ) {
  if ( cdecl_is_testing )
    return "Sep 09 1941";
  // LCOV_EXCL_START
  static char buf[ ARRAY_SIZE( "MMM DD YYYY" ) ];
  time_t const now = time( /*tloc=*/NULL );
  STRFTIME( buf, sizeof buf, "%b %e %Y", localtime( &now ) );
  return buf;
  // LCOV_EXCL_STOP
}

/**
 * Gets the current value of the `__FILE__` macro.
 *
 * @return Returns said value.
 *
 * @sa get___LINE___str()
 */
static char const* get___FILE___str( void ) {
  if ( cdecl_is_testing ) {
    static char const *const VALUE[] = { "testing.c", "testing.cpp" };
    return VALUE[ OPT_LANG_IS( CPP_ANY ) ];
  }
  // LCOV_EXCL_START
  return cdecl_input_path != NULL ? base_name( cdecl_input_path ) : "stdin";
  // LCOV_EXCL_STOP
}

/**
 * Gets the current value of the `__LINE__` macro.
 *
 * @return Returns said value.
 *
 * @warning The pointer returned is to a static buffer.
 *
 * @sa get___FILE___str()
 */
static char const* get___LINE___str( void ) {
  if ( cdecl_is_testing )
    return "42";
  // LCOV_EXCL_START
  static char buf[ MAX_DEC_INT_DIGITS(int) + 1/*\0*/ ];
  check_snprintf( buf, sizeof buf, "%d", yylineno );
  return buf;
  // LCOV_EXCL_STOP
}

/**
 * Gets the current value of the `__TIME__` macro.
 *
 * @return Returns said value.
 *
 * @warning The pointer returned is to a static buffer.
 *
 * @sa get___DATE___str()
 */
static char const* get___TIME___str( void ) {
  if ( cdecl_is_testing )
    return "12:34:56";
  // LCOV_EXCL_START
  static char buf[ ARRAY_SIZE( "hh:mm:ss" ) ];
  time_t const now = time( /*tloc=*/NULL );
  STRFTIME( buf, sizeof buf, "%H:%M:%S", localtime( &now ) );
  return buf;
  // LCOV_EXCL_STOP
}

/**
 * Checks whether the `__cplusplus` macro has a value in the current language
 * and possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG_CPP_ANY.
 *
 * @sa macro_dyn___STDC_VERSION__()
 */
static c_lang_id_t macro_dyn___cplusplus( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    char const *const value = c_lang___cplusplus( opt_lang_id );
    *ptoken = value == NULL ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( value ) );
  }
  return LANG_CPP_ANY;
}

/**
 * Checks whether the `__DATE__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___DATE__.
 *
 * @sa get___DATE___str()
 * @sa macro_dyn___TIME__()
 */
static c_lang_id_t macro_dyn___DATE__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    *ptoken = !OPT_LANG_IS( __DATE__ ) ? NULL :
      p_token_new( P_STR_LIT, check_strdup( get___DATE___str() ) );
  }
  return LANG___DATE__;
}

/**
 * Checks whether the `__FILE__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___FILE__.
 *
 * @sa macro_dyn___LINE__()
 */
static c_lang_id_t macro_dyn___FILE__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    *ptoken = !OPT_LANG_IS( __FILE__ ) ? NULL :
      p_token_new( P_STR_LIT, check_strdup( get___FILE___str() ) );
  }
  return LANG___FILE__;
}

/**
 * Checks whether the `__LINE__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___LINE__.
 *
 * @sa macro_dyn___FILE__()
 */
static c_lang_id_t macro_dyn___LINE__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    *ptoken = !OPT_LANG_IS( __LINE__ ) ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( get___LINE___str() ) );
  }
  return LANG___LINE__;
}

/**
 * Checks whether the `__STDC__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___STDC__.
 *
 * @sa macro_dyn___STDC_VERSION__()
 */
static c_lang_id_t macro_dyn___STDC__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    char const *const value = c_lang___STDC__( opt_lang_id );
    *ptoken = value == NULL ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( value ) );
  }
  return LANG___STDC__;
}

/**
 * Checks whether the `__STDC_VERSION__` macro has a value in the current
 * language and possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___STDC_VERSION__.
 *
 * @sa macro_dyn___cplusplus()
 * @sa macro_dyn___STDC__
 */
static c_lang_id_t macro_dyn___STDC_VERSION__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    char const *const value = c_lang___STDC_VERSION__( opt_lang_id );
    *ptoken = value == NULL ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( value ) );
  }
  return LANG___STDC_VERSION__;
}

/**
 * Checks whether the `__TIME__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___TIME__.
 *
 * @sa get___TIME___str()
 * @sa macro_dyn___DATE__()
 */
static c_lang_id_t macro_dyn___TIME__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    *ptoken = !OPT_LANG_IS( __TIME__ ) ? NULL :
      p_token_new( P_STR_LIT, check_strdup( get___TIME___str() ) );
  }
  return LANG___TIME__;
}

/**
 * Predefines a macro.
 *
 * @param name The name of the macro to predefine.
 * @param dyn_fn A \ref p_macro_dyn_fn_t.
 */
static void predefine_macro( char const *name, p_macro_dyn_fn_t dyn_fn ) {
  assert( name != NULL );
  assert( dyn_fn != NULL );

  p_macro_t *const macro = p_macro_define(
    check_strdup( name ),
    &(c_loc_t){
      .first_column = C_LOC_NUM_T( STRLITLEN( "#define " ) ),
      .last_column = C_LOC_NUM_T( STRLITLEN( "#define " ) + strlen( name ) - 1 )
    },
    /*param_list=*/NULL,
    /*replace_list=*/NULL
  );
  assert( macro != NULL );

  macro->dyn_fn = dyn_fn;
  macro->is_dynamic = true;
}

////////// extern functions ///////////////////////////////////////////////////

void p_predefine_macros( void ) {
  ASSERT_RUN_ONCE();

  predefine_macro( "__cplusplus",       &macro_dyn___cplusplus      );
  predefine_macro( "__DATE__",          &macro_dyn___DATE__         );
  predefine_macro( "__FILE__",          &macro_dyn___FILE__         );
  predefine_macro( "__LINE__",          &macro_dyn___LINE__         );
  predefine_macro( "__STDC__",          &macro_dyn___STDC__         );
  predefine_macro( "__STDC_VERSION__",  &macro_dyn___STDC_VERSION__ );
  predefine_macro( "__TIME__",          &macro_dyn___TIME__         );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
