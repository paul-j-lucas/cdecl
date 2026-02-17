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
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "cdecl.h"
#include "options.h"
#include "parse.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#if HAVE_PWD_H
# include <pwd.h>                       /* for getpwuid() */
#endif /* HAVE_PWD_H */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>                     /* for exit(3), getenv(3) */
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>                     /* for geteuid(2) */

/// @endcond

/**
 * @defgroup config-file-group Configuration File
 * Functions for reading **cdecl**'s configuration file.
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
    if ( (cdecl_test & CDECL_TEST_NO_HOME) == 0 ) {
      home = null_if_empty( getenv( "HOME" ) );
#if HAVE_GETEUID && HAVE_GETPWUID && HAVE_STRUCT_PASSWD_PW_DIR
      if ( home == NULL ) {
        struct passwd const *const pw = getpwuid( geteuid() );
        if ( pw != NULL )
          home = null_if_empty( pw->pw_dir );
      }
#endif /* HAVE_GETEUID && && HAVE_GETPWUID && HAVE_STRUCT_PASSWD_PW_DIR */
    }
  }

  return home;
}
// LCOV_EXCL_STOP

/**
 * Checks whether \a path exists and is a regular file.
 *
 * @param path The path to test.
 * @return Returns `true` only if \a path exists and is a regular file.
 */
NODISCARD
static bool path_is_file( char const *path ) {
  assert( path != NULL );
  struct stat st;
  return stat( path, &st ) == 0 && S_ISREG( st.st_mode );
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Reads the configuration file, if any.
 *
 * @remarks
 * @parblock
 * The path of the configuration file is determined as follows (in priority
 * order):
 *
 *  1. The value of either the `--config` or `-c` command-line option; or:
 *  2. The value of `CDECLRC` environment variable or:
 *  3. `~/.cdeclrc`; or:
 *  4. `$XDG_CONFIG_HOME/cdecl` or `~/.config/cdecl`; or:
 *  5. `$XDG_CONFIG_DIRS/cdecl` for each path or `/etc/xdg/cdecl`.
 * @endparblock
 *
 * @note This function must be called as most once.
 */
void config_init( void ) {
  ASSERT_RUN_ONCE();

  strbuf_t sbuf;
  strbuf_init( &sbuf );

  // 1. Try --config/-c command-line option.
  char const *config_path = opt_config_path;

  // 2. Try $CDECLRC.
  if ( config_path == NULL )
    config_path = null_if_empty( getenv( "CDECLRC" ) );

  // 3. Try $HOME/.cdeclrc.
  if ( config_path == NULL ) {
    char const *const home = home_dir();
    // LCOV_EXCL_START
    if ( home != NULL ) {
      strbuf_puts( &sbuf, home );
      strbuf_paths( &sbuf, "." CDECL "rc" );
      if ( path_is_file( sbuf.str ) )
        config_path = sbuf.str;
      else
        strbuf_reset( &sbuf );
    }
    // LCOV_EXCL_STOP
  }

  // 4. Try $XDG_CONFIG_HOME/cdecl and $HOME/.config/cdecl.
  if ( config_path == NULL ) {
    char const *const config_dir = null_if_empty( getenv( "XDG_CONFIG_HOME" ) );
    if ( config_dir != NULL ) {
      strbuf_puts( &sbuf, config_dir );
    }
    else {
      char const *const home = home_dir();
      // LCOV_EXCL_START
      if ( home != NULL ) {
        strbuf_puts( &sbuf, home );
        strbuf_paths( &sbuf, ".config" );
      }
      // LCOV_EXCL_STOP
    }
    if ( sbuf.len > 0 ) {
      strbuf_paths( &sbuf, CDECL );
      if ( path_is_file( sbuf.str ) )
        config_path = sbuf.str;
      else
        strbuf_reset( &sbuf );
    }
  }

  // 5. Try $XDG_CONFIG_DIRS/cdecl and /etc/xdg/cdecl.
  if ( config_path == NULL ) {
    char const *config_dirs = null_if_empty( getenv( "XDG_CONFIG_DIRS" ) );
    if ( config_dirs == NULL )
      config_dirs = "/etc/xdg";         // LCOV_EXCL_LINE
    for (;;) {
      char const *const next_sep = strchr( config_dirs, ':' );
      size_t const dir_len = next_sep != NULL ?
        STATIC_CAST( size_t, next_sep - config_dirs ) : strlen( config_dirs );
      if ( dir_len > 0 ) {
        strbuf_putsn( &sbuf, config_dirs, dir_len );
        strbuf_paths( &sbuf, CDECL );
        if ( path_is_file( sbuf.str ) ) {
          config_path = sbuf.str;
          break;
        }
        strbuf_reset( &sbuf );
      }
      if ( next_sep == NULL )
        break;
      config_dirs = next_sep + 1;
    } // for
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
      if ( echo_file_markers && rv_parse == EX_OK )
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
