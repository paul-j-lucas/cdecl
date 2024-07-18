/*
**      cdecl -- C gibberish translator
**      src/unit_test.c
**
**      Copyright (C) 2021-2024  Paul J. Lucas
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
#include "unit_test.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern variables
char const *me;
unsigned    test_failures;

/// @endcond

/**
 * @addtogroup unit-test-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Called at unit-test program termination via **atexit**(3) to print the
 * number of test failures and exits with either `EX_OK` if all tests passed or
 * `EX_SOFTWARE` if at least one test failed.
 */
_Noreturn
static void test_prog_exit( void ) {
  printf( "%u failures\n", test_failures );
  _Exit( test_failures > 0 ? EX_SOFTWARE : EX_OK );
}

// LCOV_EXCL_START
_Noreturn
static void test_prog_usage( void ) {
  EPRINTF( "usage: %s\n", me );
  exit( EX_USAGE );
}
// LCOV_EXCL_STOP

////////// extern functions ///////////////////////////////////////////////////

void test_prog_init( int argc, char const *argv[const] ) {
  ASSERT_RUN_ONCE();
  me = base_name( argv[0] );
  if ( --argc != 0 )
    test_prog_usage();                  // LCOV_EXCL_LINE
  ATEXIT( &test_prog_exit );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
