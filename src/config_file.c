/*
**      cdecl -- C gibberish translator
**      src/config_file.c
**
**      Copyright (C) 2017-2026  Paul J. Lucas, et al.
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
 * Defines functions for reading **cdecl**'s configuration file.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "config_file.h"
#include "cdecl.h"
#include "options.h"
#include "parse.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#if HAVE_PWD_H
# include <pwd.h>                       /* for getpwuid() */
#endif /* HAVE_PWD_H */
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>                     /* for geteuid() */

/// @endcond

/**
 * @ingroup config-file-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

// LCOV_EXCL_START
/**
 * Gets the full path of the user's home directory.
 *
 * @return Returns said directory or NULL if it is not obtainable.
 */
NODISCARD
static char const* home_dir( void ) {
  static char const *home;

  RUN_ONCE {
    home = null_if_empty( getenv( "HOME" ) );
#if HAVE_GETEUID && HAVE_GETPWUID && HAVE_STRUCT_PASSWD_PW_DIR
    if ( home == NULL ) {
      struct passwd const *const pw = getpwuid( geteuid() );
      if ( pw != NULL )
        home = null_if_empty( pw->pw_dir );
    }
#endif /* HAVE_GETEUID && && HAVE_GETPWUID && HAVE_STRUCT_PASSWD_PW_DIR */
  }

  return home;
}
// LCOV_EXCL_STOP

////////// extern functions ///////////////////////////////////////////////////

void config_init( void ) {
  ASSERT_RUN_ONCE();

  char const *config_path = opt_config_path;
  if ( config_path == NULL )
    config_path = null_if_empty( getenv( "CDECLRC" ) );

  strbuf_t sbuf;
  strbuf_init( &sbuf );

  if ( config_path == NULL ) {
    // LCOV_EXCL_START
    char const *const home = home_dir();
    if ( home != NULL ) {
      strbuf_puts( &sbuf, home );
      config_path = strbuf_paths( &sbuf, CONF_FILE_NAME_DEFAULT );
    }
    // LCOV_EXCL_STOP
  }

  int rv_parse = EX_OK;

  if ( config_path != NULL ) {
    cdecl_input_path = config_path;

    FILE *const config_file = fopen( config_path, "r" );
    if ( config_file != NULL ) {
      bool const echo_file_markers = opt_echo_commands && !cdecl_is_interactive;
      if ( echo_file_markers )
        PRINTF( "/* begin \"%s\" */\n", config_path );
      rv_parse = cdecl_parse_file( config_file );
      if ( echo_file_markers )
        PRINTF( "/* end \"%s\" */\n", config_path );
      fclose( config_file );
    }
    else if ( opt_config_path != NULL ) {
      fatal_error( EX_NOINPUT, "\"%s\": %s\n", config_path, STRERROR() );
    }

    cdecl_input_path = NULL;
  }

  strbuf_cleanup( &sbuf );

  if ( rv_parse != EX_OK )
    exit( rv_parse );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
