/*
**      cdecl -- C gibberish translator
**      src/red_black_test.c
**
**      Copyright (C) 2021  Paul J. Lucas
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
#include "red_black.h"
#include "util.h"

// standard
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints that \a EXPR failed and increments the test failure count.
 *
 * @param EXPR The stringified expression that failed.
 * @return Always returns `false`.
 */
#define FAILED(EXPR) \
  ( EPRINTF( "%s:%d: " EXPR "\n", me, __LINE__ ), !++test_failures )

/**
 * Tests \a EXPR and prints that it failed only if it failed.
 *
 * @param EXPR The expression to evaluate.
 * @return Returns `true` only if \a EXPR is non-zero; `false` only if zero.
 */
#define TEST(EXPR)                ( !!(EXPR) || FAILED( #EXPR ) )

///////////////////////////////////////////////////////////////////////////////

// extern variables
char const       *me;                   ///< Program name.

// local variables
static unsigned   test_failures;

////////// local functions ////////////////////////////////////////////////////

static int test_rb_data_cmp( void const *i_data, void const *j_data ) {
  char const *const i_str = i_data;
  char const *const j_str = j_data;
  return strcmp( i_str, j_str );
}

static bool test_rb_visitor( void *node_data, void *aux_data ) {
  char const *const str = node_data;
  unsigned *const letter_offset_ptr = aux_data;
  if ( TEST( str != NULL ) )
    TEST( str[0] == (char)('A' + *letter_offset_ptr) );
  ++*letter_offset_ptr;
  return false;
}

static noreturn void usage( void ) {
  EPRINTF( "usage: %s\n", me );
  exit( EX_USAGE );
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[] ) {
  me = base_name( argv[0] );
  if ( --argc != 0 )
    usage();

  rb_tree_t tree;
  rb_tree_init( &tree, &test_rb_data_cmp );
  rb_node_t *node;

  // test insertion
  TEST( rb_tree_insert( &tree, (void*)"A" ) == NULL );
  TEST( rb_tree_insert( &tree, (void*)"B" ) == NULL );
  TEST( rb_tree_insert( &tree, (void*)"C" ) == NULL );
  TEST( rb_tree_insert( &tree, (void*)"D" ) == NULL );

  // test insertion with existing data
  node = rb_tree_insert( &tree, (void*)"A" );
  if ( TEST( node != NULL ) )
    TEST( strcmp( node->data, "A" ) == 0 );

  // test visitor
  unsigned letter_offset = 0;
  TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );

  // test find
  node = rb_tree_find( &tree, "A" );
  if ( TEST( node != NULL ) ) {
    TEST( strcmp( node->data, "A" ) == 0 );

    // test delete
    void *const data = rb_tree_delete( &tree, node );
    if ( TEST( data != NULL ) ) {
      TEST( strcmp( data, "A" ) == 0 );

      // test visitor again
      letter_offset = 1; // skip "A"
      TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );
    }
  }

  rb_tree_free( &tree, NULL );

  printf( "%u failures\n", test_failures );
  exit( test_failures > 0 ? EX_SOFTWARE : EX_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
