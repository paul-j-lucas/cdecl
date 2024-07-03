/*
**      cdecl -- C gibberish translator
**      src/util_test.c
**
**      Copyright (C) 2024  Paul J. Lucas
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
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#define STRLIT_LEN(S)             (S), STRLITLEN( (S) )

///////////////////////////////////////////////////////////////////////////////

// extern variables
char const          *me;                ///< Program name.

// local variables
static unsigned     test_failures;

////////// local functions ////////////////////////////////////////////////////

static bool test_check_strdup_suffix( void ) {
  TEST_FN_BEGIN();

  char *rv = check_strdup_suffix( "hello", STRLIT_LEN( ", world" ) );
  if ( TEST( rv != NULL ) ) {
    TEST( strcmp( rv, "hello, world" ) == 0 );
    free( rv );
  }

  TEST_FN_END();
}

static bool test_is_ident_prefix( void ) {
  TEST_FN_BEGIN();

  TEST( !is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELL" ) ) );
  TEST( !is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELLX" ) ) );
  TEST( is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELLO" ) ) );
  TEST( is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELLO()" ) ) );

  TEST_FN_END();
}

static bool test_parse_identifier( void ) {
  TEST_FN_BEGIN();

  TEST( parse_identifier( "." ) == NULL );
  TEST( *parse_identifier( "A(" ) == '(' );

  TEST_FN_END();
}

static bool test_strdup_tolower( void ) {
  TEST_FN_BEGIN();

  TEST( check_strdup_tolower( NULL ) == NULL );
  char *rv = check_strdup_tolower( "Hello" );
  if ( TEST( rv != NULL ) ) {
    TEST( strcmp( rv, "hello" ) == 0 );
    free( rv );
  }

  TEST_FN_END();
}

static bool test_str_is_prefix( void ) {
  TEST_FN_BEGIN();

  TEST( !str_is_prefix( "", "HELLO" ) );
  TEST(  str_is_prefix( "HELL", "HELLO" ) );
  TEST(  str_is_prefix( "HELLO", "HELLO" ) );
  TEST( !str_is_prefix( "HELLX", "HELLO" ) );
  TEST( !str_is_prefix( "HELLOX", "HELLO" ) );

  TEST_FN_END();
}

static bool test_str_realloc_pcat( void ) {
  TEST_FN_BEGIN();

  char *s = MALLOC( char, STRLITLEN( "FGHI" ) + 1 );
  strcpy( s, "FGHI" );
  s = str_realloc_pcat( "AB", "CDE", s );
  if ( TEST( s != NULL ) ) {
    TEST( strcmp( s, "ABCDEFGHI" ) == 0 );
  }
  free( s );

  TEST_FN_END();
}

static bool test_strnspn( void ) {
  TEST_FN_BEGIN();

  TEST( strnspn( "", "AB", 0 ) == 0 );

  TEST( strnspn( "A",   "AB", 0 ) == 0 );
  TEST( strnspn( "B",   "AB", 0 ) == 0 );
  TEST( strnspn( "X",   "AB", 0 ) == 0 );

  TEST( strnspn( "XA",  "AB", 0 ) == 0 );
  TEST( strnspn( "XB",  "AB", 0 ) == 0 );
  TEST( strnspn( "XAB", "AB", 0 ) == 0 );

  TEST( strnspn( "A",   "AB", 1 ) == 1 );
  TEST( strnspn( "B",   "AB", 1 ) == 1 );

  TEST( strnspn( "AB",  "AB", 2 ) == 2 );
  TEST( strnspn( "BA",  "AB", 2 ) == 2 );

  TEST( strnspn( "ABA", "AB", 2 ) == 2 );
  TEST( strnspn( "ABX", "AB", 2 ) == 2 );

  TEST_FN_END();
}

// LCOV_EXCL_START
_Noreturn
static void usage( void ) {
  EPRINTF( "usage: %s\n", me );
  exit( EX_USAGE );
}
// LCOV_EXCL_STOP

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[const] ) {
  me = base_name( argv[0] );
  if ( --argc != 0 )
    usage();                            // LCOV_EXCL_LINE

  test_check_strdup_suffix();
  test_is_ident_prefix();
  test_parse_identifier();
  test_strdup_tolower();
  test_str_is_prefix();
  test_str_realloc_pcat();
  test_strnspn();

  printf( "%u failures\n", test_failures );
  exit( test_failures > 0 ? EX_SOFTWARE : EX_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
