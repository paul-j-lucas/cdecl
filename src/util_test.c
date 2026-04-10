/*
**      cdecl -- C gibberish translator
**      src/util_test.c
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
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdlib.h>                     /* for free(3) */
#include <string.h>

#define STRLIT_LEN(S)             (S), STRLITLEN( (S) )

////////// test functions /////////////////////////////////////////////////////

static bool test_check_strdup_suffix( void ) {
  TEST_FUNC_BEGIN();

  char *const s = check_strdup_suffix( "hello", STRLIT_LEN( ", world" ) );
  if ( TEST( s != NULL ) ) {
    TEST( strcmp( s, "hello, world" ) == 0 );
    free( s );
  }

  TEST_FUNC_END();
}

static bool test_is_ident_prefix( void ) {
  TEST_FUNC_BEGIN();

  TEST( !str_is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELL" ) ) );
  TEST( !str_is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELLX" ) ) );
  TEST( str_is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELLO" ) ) );
  TEST( str_is_ident_prefix( STRLIT_LEN( "HELLO" ), STRLIT_LEN( "HELLO()" ) ) );

  TEST_FUNC_END();
}

static bool test_parse_identifier( void ) {
  TEST_FUNC_BEGIN();

  TEST( parse_identifier( "." ) == NULL );
  char const *const s = parse_identifier( "A(" );
  if ( TEST( s != NULL ) )
    TEST( *s == '(' );

  TEST_FUNC_END();
}

static bool test_strdup_tolower( void ) {
  TEST_FUNC_BEGIN();

  TEST( check_strdup_tolower( NULL ) == NULL );
  char *const s = check_strdup_tolower( "Hello" );
  if ( TEST( s != NULL ) ) {
    TEST( strcmp( s, "hello" ) == 0 );
    free( s );
  }

  TEST_FUNC_END();
}

static bool test_str_is_prefix( void ) {
  TEST_FUNC_BEGIN();

  TEST( !str_is_prefix( "", "HELLO" ) );
  TEST(  str_is_prefix( "HELL", "HELLO" ) );
  TEST(  str_is_prefix( "HELLO", "HELLO" ) );
  TEST( !str_is_prefix( "HELLX", "HELLO" ) );
  TEST( !str_is_prefix( "HELLOX", "HELLO" ) );

  TEST_FUNC_END();
}

static bool test_strncmp_in_set( void ) {
  TEST_FUNC_BEGIN();

  TEST( strncmp_in_set( "", "A", 0, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "A", "", 0, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "", "A", 1, IDENT_CHARS ) < 0 );
  TEST( strncmp_in_set( "A", "", 1, IDENT_CHARS ) > 0 );

  TEST( strncmp_in_set( "A", "A", 1, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "A", "B", 1, IDENT_CHARS ) < 0 );
  TEST( strncmp_in_set( "B", "A", 1, IDENT_CHARS ) > 0 );

  TEST( strncmp_in_set( "A", "A", 9, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "A", "B", 9, IDENT_CHARS ) < 0 );
  TEST( strncmp_in_set( "B", "A", 9, IDENT_CHARS ) > 0 );

  TEST( strncmp_in_set( "A", "AB", 2, IDENT_CHARS ) < 0 );
  TEST( strncmp_in_set( "AB", "A", 2, IDENT_CHARS ) > 0 );
  TEST( strncmp_in_set( "AB", "AX", 1, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "-A", "A", 2, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "A-", "A", 2, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "-A-", "A", 3, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "A", "-A", 2, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "A", "A-", 1, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "A", "-A-", 2, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "A-", "A-B", 2, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "AB", "AA", 3, IDENT_CHARS ) > 0 );
  TEST( strncmp_in_set( "AB", "A-A", 3, IDENT_CHARS ) > 0 );
  TEST( strncmp_in_set( "AB", "A~A", 3, IDENT_CHARS ) < 0 );

  TEST( strncmp_in_set( "A-B", "A-BC", 3, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "-e-a--st-", "east", 9, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "-e-a--st-", "east-const", 9, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "east-const", "-e-a--st-", 9, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "non-e", "non-empty", 5, IDENT_CHARS ) == 0 );
  TEST( strncmp_in_set( "non-empty", "non-e", 5, IDENT_CHARS ) == 0 );

  TEST( strncmp_in_set( "no-foo", "nofoo", 6, IDENT_CHARS ) == 0 );

  TEST_FUNC_END();
}

static bool test_strnspn( void ) {
  TEST_FUNC_BEGIN();

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

  TEST_FUNC_END();
}

static bool test_str_realloc_pcat( void ) {
  TEST_FUNC_BEGIN();

  char *s = MALLOC( char, STRLITLEN( "FGHI" ) + 1 );
  strcpy( s, "FGHI" );
  s = str_realloc_pcat( "AB", "CDE", s );
  if ( TEST( s != NULL ) ) {
    TEST( strcmp( s, "ABCDEFGHI" ) == 0 );
    free( s );
  }

  TEST_FUNC_END();
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *const argv[] ) {
  test_prog_init( argc, argv );

  test_check_strdup_suffix();
  test_is_ident_prefix();
  test_parse_identifier();
  test_strdup_tolower();
  test_str_is_prefix();
  test_strncmp_in_set();
  test_strnspn();
  test_str_realloc_pcat();
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
