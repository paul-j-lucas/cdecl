/*
**      PJL Library
**      src/strbuf_test.c
**
**      Copyright (C) 2025  Paul J. Lucas
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
#include "strbuf.h"
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

////////// test functions /////////////////////////////////////////////////////

static bool test_strbuf_paths( void ) {
  TEST_FUNC_BEGIN();
  strbuf_t sbuf;

  strbuf_init( &sbuf );
  strbuf_paths( &sbuf, "a" );
  TEST( strcmp( sbuf.str, "a" ) == 0 );
  strbuf_paths( &sbuf, "b" );
  TEST( strcmp( sbuf.str, "a/b" ) == 0 );

  strbuf_reset( &sbuf );
  strbuf_puts( &sbuf, "a/" );
  strbuf_paths( &sbuf, "b" );
  TEST( strcmp( sbuf.str, "a/b" ) == 0 );

  strbuf_reset( &sbuf );
  strbuf_paths( &sbuf, "a" );
  strbuf_paths( &sbuf, "/b" );
  TEST( strcmp( sbuf.str, "a/b" ) == 0 );

  strbuf_cleanup( &sbuf );
  TEST_FUNC_END();
}

static bool test_strbuf_put_quoted( void ) {
  TEST_FUNC_BEGIN();
  strbuf_t sbuf;

  strbuf_init( &sbuf );
  strbuf_puts_quoted( &sbuf, '\'', "a" );
  TEST( strcmp( sbuf.str, "'a'" ) == 0 );

  strbuf_reset( &sbuf );
  strbuf_puts_quoted( &sbuf, '"', "a" );
  TEST( strcmp( sbuf.str, "\"a\"" ) == 0 );

  strbuf_reset( &sbuf );
  strbuf_puts_quoted( &sbuf, '\'', "a 'b' c" );
  TEST( strcmp( sbuf.str, "'a \\\'b\\\' c'" ) == 0 );

  strbuf_reset( &sbuf );
  strbuf_puts_quoted( &sbuf, '"', "a \"b\" c" );
  TEST( strcmp( sbuf.str, "\"a \\\"b\\\" c\"" ) == 0 );

  strbuf_cleanup( &sbuf );
  TEST_FUNC_END();
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *const argv[] ) {
  test_prog_init( argc, argv );

  test_strbuf_paths();
  test_strbuf_put_quoted();
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
