/*
**      cdecl -- C gibberish translator
**      src/show.c
**
**      Copyright (C) 2023  Paul J. Lucas, et al.
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
#include "english.h"
#include "gibberish.h"
#include "options.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

/**
 * @addtogroup showing-types-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Information for show_type_visitor().
 */
struct show_info {
  cdecl_show_t  show;                   ///< Which types to show.
  c_sglob_t     sglob;                  ///< Scoped glob to match, if any.
  unsigned      gib_flags;              ///< Gibberish flags.
  FILE         *tout;                   ///< Where to print the types.
};
typedef struct show_info show_info_t;

////////// local functions ////////////////////////////////////////////////////

/**
 * A visitor function to show (print) \a tdef.
 *
 * @param tdef The \ref c_typedef to show.
 * @param data Optional data passed to the visitor: in this case, a \ref
 * show_info.
 * @return Always returns `false`.
 */
NODISCARD
static bool show_type_visitor( c_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  show_info_t const *const si = data;

  if ( (si->show & CDECL_SHOW_IGNORE_LANG) == 0 &&
       !opt_lang_is_any( tdef->lang_ids ) ) {
    goto no_show;
  }

  if ( tdef->is_predefined ) {
    if ( (si->show & CDECL_SHOW_PREDEFINED) == 0 )
      goto no_show;
  } else {
    if ( (si->show & CDECL_SHOW_USER_DEFINED) == 0 )
      goto no_show;
  }

  if ( !c_sglob_empty( &si->sglob ) &&
       !c_sname_match( &tdef->ast->sname, &si->sglob ) ) {
    goto no_show;
  }

  show_type( tdef, si->gib_flags, si->tout );

no_show:
  return /*stop=*/false;
}

////////// extern functions ///////////////////////////////////////////////////

void show_type( c_typedef_t const *tdef, unsigned gib_flags, FILE *tout ) {
  assert( tdef != NULL );
  assert( tout != NULL );

  if ( gib_flags == C_GIB_NONE ) {
    c_typedef_english( tdef, tout );
  } else {
    if ( opt_semicolon )
      gib_flags |= C_GIB_FINAL_SEMI;
    c_typedef_gibberish( tdef, gib_flags, tout );
  }

  FPUTC( '\n', tout );
}

void show_types( cdecl_show_t show, char const *glob, unsigned gib_flags,
                 FILE *tout ) {
  assert( tout != NULL );

  show_info_t si = {
    .show = show,
    .gib_flags = gib_flags,
    .tout = tout
  };
  c_sglob_init( &si.sglob );
  c_sglob_parse( glob, &si.sglob );

  c_typedef_visit( &show_type_visitor, &si );

  c_sglob_cleanup( &si.sglob );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
