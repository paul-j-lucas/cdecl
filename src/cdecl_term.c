/*
**      cdecl -- C gibberish translator
**      src/cdecl_term.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
 * Defines functions for dealing with the terminal.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl_term.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdlib.h>                     /* for getenv(3) */
#ifdef ENABLE_TERM_SIZE
# include <fcntl.h>                     /* for open(2) */
# define _BOOL /* nothing */            /* prevent bool clash on AIX/Solaris */
# if defined(HAVE_CURSES_H)
#   include <curses.h>
# elif defined(HAVE_NCURSES_H)
#   include <ncurses.h>
# endif
# include <term.h>                      /* for setupterm(3) */
# undef _BOOL
# include <unistd.h>                    /* for close() */
#endif /* ENABLE_TERM_SIZE */

/// @endcond

// local constants

/// Default number of terminal columns.
static unsigned const COLUMNS_DEFAULT = 80;

// local variables
static unsigned (*get_columns_fn)();    ///< Columns-getting function.

/**
 * @addtogroup terminal-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets the default number of terminal columns.
 *
 * @return Returns said number of columns.
 */
NODISCARD
static unsigned get_columns_default( void ) {
  return COLUMNS_DEFAULT;
}

#ifdef ENABLE_TERM_SIZE
/**
 * Gets the number of columns of the terminal via **tigetnum**(3).
 *
 * @return Returns said number of columns or 0 upon error.
 */
NODISCARD
static unsigned get_columns_via_tigetnum( void ) {
  int         cterm_fd = -1;
  char        reason_buf[ 128 ];
  char const *reason = NULL;
  unsigned    rv = 0;

  char const *const term = null_if_empty( getenv( "TERM" ) );
  if ( unlikely( term == NULL ) ) {
    // LCOV_EXCL_START
    reason = "TERM environment variable not set";
    goto error;
    // LCOV_EXCL_STOP
  }

  char const *const cterm_path = ctermid( NULL );
  if ( unlikely( cterm_path == NULL || *cterm_path == '\0' ) ) {
    // LCOV_EXCL_START
    reason = "ctermid(3) failed to get controlling terminal";
    goto error;
    // LCOV_EXCL_STOP
  }

  if ( unlikely( (cterm_fd = open( cterm_path, O_RDWR )) == -1 ) ) {
    // LCOV_EXCL_START
    reason = STRERROR();
    goto error;
    // LCOV_EXCL_STOP
  }

  int sut_err;
  if ( setupterm( CONST_CAST( char*, term ), cterm_fd, &sut_err ) == ERR ) {
    // LCOV_EXCL_START
    reason = reason_buf;
    switch ( sut_err ) {
      case -1:
        reason = "terminfo database not found";
        break;
      case 0:
        check_snprintf(
          reason_buf, sizeof reason_buf,
          "TERM=%s not found in database or too generic", term
        );
        break;
      case 1:
        reason = "terminal is hardcopy";
        break;
      default:
        check_snprintf(
          reason_buf, sizeof reason_buf,
          "setupterm(3) returned error code %d", sut_err
        );
    } // switch
    goto error;
    // LCOV_EXCL_STOP
  }

  int const tigetnum_rv = tigetnum( CONST_CAST( char*, "cols" ) );
  switch ( tigetnum_rv ) {
    // LCOV_EXCL_START
    case -1:
      reason = "terminal lacks \"cols\" capability";
      break;
    case -2:                            // capname is not a numeric capability
      UNEXPECTED_INT_VALUE( tigetnum_rv );
    // LCOV_EXCL_STOP
    default:
      rv = STATIC_CAST( unsigned, tigetnum_rv );
  } // switch

error:
  if ( likely( cterm_fd != -1 ) )
    close( cterm_fd );
  if ( unlikely( reason != NULL ) ) {
    // LCOV_EXCL_START
    print_warning( /*loc=*/NULL, "can't get terminal columns: %s\n", reason );
    // LCOV_EXCL_STOP
  }

  return rv;
}
#endif /* ENABLE_TERM_SIZE */

////////// extern functions ///////////////////////////////////////////////////

void cdecl_term_init( void ) {
  ASSERT_RUN_ONCE();

#ifdef ENABLE_TERM_SIZE
  unsigned rv = get_columns_via_tigetnum();
  if ( rv > 0 ) {
    get_columns_fn = &get_columns_via_tigetnum;
    return;
  }
#endif /* ENABLE_TERM_SIZE */

  get_columns_fn = &get_columns_default;
}

unsigned term_get_columns( void ) {
  return (*get_columns_fn)();
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
