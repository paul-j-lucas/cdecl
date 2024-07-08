/*
**      cdecl -- C gibberish translator
**      src/show.c
**
**      Copyright (C) 2023-2024  Paul J. Lucas, et al.
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
 * Defines functions for showing types for the `show` command.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "show.h"
#include "c_ast.h"
#include "c_lang.h"
#include "c_sglob.h"
#include "c_sname.h"
#include "c_typedef.h"
#include "decl_flags.h"
#include "gibberish.h"
#include "options.h"
#include "p_token.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */

/// @endcond

/**
 * @addtogroup showing-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Information for show_macro_visitor().
 */
struct show_macros_info {
  cdecl_show_t  show;                   ///< Which macros to show.
  FILE         *fout;                   ///< Where to print the macros.
};
typedef struct show_macros_info show_macros_info_t;

/**
 * Information for show_type_visitor().
 */
struct show_types_info {
  cdecl_show_t  show;                   ///< Which types to show.
  c_sglob_t     sglob;                  ///< Scoped glob to match, if any.
  unsigned      decl_flags;             ///< Declaration flags.
  FILE         *fout;                   ///< Where to print the types.
  bool          showed_any;             ///< Did we actually show any?
};
typedef struct show_types_info show_types_info_t;

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints \a param_list between parentheses and separated by commas.
 *
 * @param param_list The macro parameter list to print.
 * @param fout The `FILE` to print to.
 *
 * @sa print_token_list()
 */
static void print_param_list( p_param_list_t const *param_list, FILE *fout ) {
  assert( param_list != NULL );
  assert( fout != NULL );

  FPUTC( '(', fout );
  bool comma = false;
  FOREACH_SLIST_NODE( param_node, param_list ) {
    p_param_t const *const param = param_node->data;
    if ( true_or_set( &comma ) )
      FPUTS( ", ", fout );
    FPUTS( param->name, fout );
  } // for
  FPUTC( ')', fout );
}

/**
 * A visitor function to show (print) \a macro.
 *
 * @param macro The \ref p_macro to show.
 * @param data Optional data passed to the visitor: in this case, a \ref
 * show_macros_info.
 * @return Always returns `false`.
 */
NODISCARD
static bool show_macro_visitor( p_macro_t const *macro, void *data ) {
  assert( macro != NULL );
  assert( data != NULL );

  show_macros_info_t const *const smi = data;

  if ( macro->is_dynamic ) {
    if ( (smi->show & CDECL_SHOW_PREDEFINED) == 0 )
      goto no_show;
    if ( !opt_lang_is_any( (*macro->dyn_fn)( /*ptoken=*/NULL ) ) )
      goto no_show;
  } else {
    if ( (smi->show & CDECL_SHOW_USER_DEFINED) == 0 )
      goto no_show;
  }

  show_macro( macro, smi->fout );

no_show:
  return /*stop=*/false;
}

/**
 * A visitor function to show (print) \a tdef.
 *
 * @param tdef The \ref c_typedef to show.
 * @param data Optional data passed to the visitor: in this case, a \ref
 * show_types_info.
 * @return Always returns `false`.
 */
NODISCARD
static bool show_type_visitor( c_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  show_types_info_t *const sti = data;

  if ( (sti->show & CDECL_SHOW_OPT_IGNORE_LANG) == 0 &&
       !opt_lang_is_any( tdef->lang_ids ) ) {
    goto no_show;
  }

  if ( tdef->is_predefined ) {
    if ( (sti->show & CDECL_SHOW_PREDEFINED) == 0 )
      goto no_show;
  } else {
    if ( (sti->show & CDECL_SHOW_USER_DEFINED) == 0 )
      goto no_show;
  }

  if ( !c_sglob_empty( &sti->sglob ) &&
       !c_sname_match( &tdef->ast->sname, &sti->sglob ) ) {
    goto no_show;
  }

  show_type( tdef, sti->decl_flags, sti->fout );
  sti->showed_any = true;

no_show:
  return /*stop=*/false;
}

////////// extern functions ///////////////////////////////////////////////////

bool show_macro( p_macro_t const *macro, FILE *fout ) {
  assert( macro != NULL );
  assert( fout != NULL );

  if ( macro->is_dynamic ) {
    p_token_t *token;
    (*macro->dyn_fn)( &token );
    if ( token == NULL )
      return false;
    FPRINTF( fout,
      "%sdefine %s %s\n",
      other_token_c( "#" ), macro->name, p_token_str( token )
    );
    p_token_free( token );
  }
  else {
    FPRINTF( fout, "%sdefine %s", other_token_c( "#" ), macro->name );
    if ( macro->param_list != NULL )
      print_param_list( macro->param_list, fout );
    if ( !slist_empty( &macro->replace_list ) ) {
      FPUTC( ' ', fout );
      print_token_list( &macro->replace_list, fout );
    }
    FPUTC( '\n', fout );
  }

  return true;
}

void show_macros( cdecl_show_t show, FILE *fout ) {
  assert( fout != NULL );
  show_macros_info_t smi = { show, fout };
  p_macro_visit( &show_macro_visitor, &smi );
}

void show_type( c_typedef_t const *tdef, unsigned decl_flags, FILE *fout ) {
  assert( tdef != NULL );
  assert( fout != NULL );

  if ( (decl_flags & C_TYPE_DECL_ANY) == 0 )
    decl_flags |= tdef->decl_flags;

  if ( opt_semicolon )
    decl_flags |= C_GIB_OPT_SEMICOLON;

  print_type_decl( tdef, decl_flags, fout );
  FPUTC( '\n', fout );
}

bool show_types( cdecl_show_t show, char const *glob, unsigned decl_flags,
                 FILE *fout ) {
  assert( fout != NULL );

  show_types_info_t sti = {
    .show = show,
    .decl_flags = decl_flags,
    .fout = fout
  };
  c_sglob_init( &sti.sglob );
  c_sglob_parse( glob, &sti.sglob );

  c_typedef_visit( &show_type_visitor, &sti );

  if ( !sti.showed_any && (show & CDECL_SHOW_USER_DEFINED) != 0 &&
       glob != NULL && strchr( glob, '*' ) == NULL ) {
    //
    // We didn't show any specific user-defined types, so try showing specific
    // predefined types.
    //
    sti.show &= ~STATIC_CAST( unsigned, CDECL_SHOW_USER_DEFINED );
    sti.show |= CDECL_SHOW_PREDEFINED;
    c_typedef_visit( &show_type_visitor, &sti );
  }

  c_sglob_cleanup( &sti.sglob );
  return sti.showed_any;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
