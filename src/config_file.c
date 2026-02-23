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
#include "print.h"
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
#include <sysexits.h>
#include <unistd.h>                     /* for geteuid(2) */

/// @endcond

/**
 * @defgroup config-file-group Configuration File
 * Functions for reading **cdecl**'s configuration file.
 * @{
 */

/**
 * Options for the config_open() function.
 */
enum config_opts {
  CONFIG_OPT_NONE              = 0,       ///< No options.
  CONFIG_OPT_ERROR_IS_FATAL    = 1 << 0,  ///< An error is fatal.
  CONFIG_OPT_IGNORE_NOT_FOUND  = 1 << 1   ///< Ignore file not found.
};
typedef enum config_opts config_opts_t;

NODISCARD
static FILE*        config_open( char const*, config_opts_t );

NODISCARD
static char const*  home_dir( void );

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds and opens the configuration file.
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
 * @param config_path The full path to a configuration file.  May be NULL.
 * @param sbuf The strbuf to use. It _must_ be initialized.
 * @return Returns the `FILE*` for the configuration file if found or NULL if
 * not.
 */
NODISCARD
static FILE* config_find( char const *config_path, strbuf_t *sbuf ) {
  assert( sbuf != NULL );

  char const *home = NULL;

  // 1. Try --config/-c command-line option.
  FILE *config_file = config_open( config_path, CONFIG_OPT_ERROR_IS_FATAL );

  // 2. Try $CDECLRC.
  if ( config_file == NULL ) {
    char const *const cdeclrc_path = null_if_empty( getenv( "CDECLRC" ) );
    config_file = config_open( cdeclrc_path, CONFIG_OPT_NONE );
  }

  // 3. Try $HOME/.cdeclrc.
  if ( config_file == NULL && (home = home_dir()) != NULL ) {
    // LCOV_EXCL_START
    strbuf_puts( sbuf, home );
    strbuf_paths( sbuf, "." CDECL "rc" );
    config_file = config_open( sbuf->str, CONFIG_OPT_IGNORE_NOT_FOUND );
    strbuf_reset( sbuf );
    // LCOV_EXCL_STOP
  }

  // 4. Try $XDG_CONFIG_HOME/cdecl and $HOME/.config/cdecl.
  if ( config_file == NULL ) {
    char const *const config_dir = null_if_empty( getenv( "XDG_CONFIG_HOME" ) );
    if ( config_dir != NULL ) {
      strbuf_puts( sbuf, config_dir );
    }
    else if ( home != NULL ) {
      // LCOV_EXCL_START
      strbuf_puts( sbuf, home );
      strbuf_paths( sbuf, ".config" );
      // LCOV_EXCL_STOP
    }
    if ( sbuf->len > 0 ) {
      strbuf_paths( sbuf, CDECL );
      config_file = config_open( sbuf->str, CONFIG_OPT_IGNORE_NOT_FOUND );
      strbuf_reset( sbuf );
    }
  }

  // 5. Try $XDG_CONFIG_DIRS/cdecl and /etc/xdg/cdecl.
  if ( config_file == NULL ) {
    char const *config_dirs = null_if_empty( getenv( "XDG_CONFIG_DIRS" ) );
    if ( config_dirs == NULL )
      config_dirs = "/etc/xdg";         // LCOV_EXCL_LINE
    for (;;) {
      char const *const next_sep = strchr( config_dirs, ':' );
      size_t const dir_len = next_sep != NULL ?
        STATIC_CAST( size_t, next_sep - config_dirs ) : strlen( config_dirs );
      if ( dir_len > 0 ) {
        strbuf_putsn( sbuf, config_dirs, dir_len );
        strbuf_paths( sbuf, CDECL );
        config_file = config_open( sbuf->str, CONFIG_OPT_IGNORE_NOT_FOUND );
        strbuf_reset( sbuf );
        if ( config_file != NULL )
          break;
      }
      if ( next_sep == NULL )
        break;
      config_dirs = next_sep + 1;
    } // for
  }

  return config_file;
}

/**
 * Tries to open a configuration file given by \a path.
 *
 * @note If successful, also sets \ref cdecl_input_path to \a path.
 *
 * @param path The full path to try to open.  May be NULL.
 * @param opts The configuration options, if any.
 * @return Returns a `FILE*` to the open file upon success or NULL upon either
 * error or if \a path is NULL.
 */
NODISCARD
static FILE* config_open( char const *path, config_opts_t opts ) {
  if ( path == NULL )
    return NULL;
  FILE *const config_file = fopen( path, "r" );
  if ( config_file == NULL ) {
    switch ( errno ) {
      case ENOENT:
        if ( (opts & CONFIG_OPT_IGNORE_NOT_FOUND) != 0 )
          break;
        FALLTHROUGH;
      default:
        EPRINTF( "%s: ", prog_name );
        if ( (opts & CONFIG_OPT_ERROR_IS_FATAL) != 0 ) {
          print_error( /*loc=*/NULL,
            "configuration file \"%s\": %s\n", path, STRERROR()
          );
          exit( EX_NOINPUT );
        }
        print_warning( /*loc=*/NULL,
          "configuration file \"%s\": %s\n", path, STRERROR()
        );
        break;
    } // switch
  }
  else {
    cdecl_input_path = path;
  }
  return config_file;
}

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

////////// extern functions ///////////////////////////////////////////////////

/**
 * Initializes **cdecl** via a configuration file.
 *
 * @note This function must be called as most once.
 */
void config_init( void ) {
  ASSERT_RUN_ONCE();

  strbuf_t sbuf;
  strbuf_init( &sbuf );

  FILE *const config_file = config_find( opt_config_path, &sbuf );

  int rv_parse = EX_OK;

  if ( config_file != NULL ) {
    bool const echo_file_markers = opt_echo_commands && !cdecl_is_interactive;
    if ( echo_file_markers )
      PRINTF( "/* begin \"%s\" */\n", cdecl_input_path );
    rv_parse = cdecl_parse_file( config_file );
    if ( echo_file_markers && rv_parse == EX_OK )
      PRINTF( "/* end \"%s\" */\n", cdecl_input_path );
    fclose( config_file );
    cdecl_input_path = NULL;
  }

  strbuf_cleanup( &sbuf );

  if ( rv_parse != EX_OK )
    exit( rv_parse );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
