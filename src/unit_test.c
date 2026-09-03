/*
**      PJL Library
**      src/unit_test.c
**
**      Copyright (C) 2021-2026  Paul J. Lucas
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
#include "pjl_config.h"
#include "unit_test.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

////////// extern variables ///////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

char const *prog_name;
int         test_exit_status;
unsigned    test_failures;

/// @endcond

/**
 * @addtogroup unit-test-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Called at unit-test program termination via **atexit**(3) to print the
 * number of test failures and set \ref test_exit_status.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa test_prog_init()
 */
static void test_prog_atexit( void ) {
  printf( "%u failures\n", test_failures );
  test_exit_status = test_failures > 0 ? EX_SOFTWARE : EX_OK;
}

/**
 * Prints the usage message to standard error and exits.
 */
_Noreturn
static void test_prog_usage( void ) {
  // LCOV_EXCL_START
  EPRINTF( "usage: %s\n", prog_name );
  exit( EX_USAGE );
  // LCOV_EXCL_STOP
}

////////// extern functions ///////////////////////////////////////////////////

bool test_expr( bool expr_is_true, char const *expr, int line ) {
  if ( !expr_is_true ) {
    EPRINTF( "%s:%d: %s\n", prog_name, line, expr );
    ++test_failures;
  }
  return expr_is_true;
}

void test_prog_init( int argc, char const *const argv[] ) {
  ASSERT_RUN_ONCE();
  prog_name = path_basename( argv[0] );
  if ( --argc != 0 )
    test_prog_usage();                  // LCOV_EXCL_LINE
  ATEXIT( &test_prog_atexit );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
