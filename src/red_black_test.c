/*
**      cdecl -- C gibberish translator
**      src/red_black_test.c
**
**      Copyright (C) 2021-2023  Paul J. Lucas
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
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// extern variables
char const       *me;                   ///< Program name.

// local variables
static unsigned   test_failures;

////////// local functions ////////////////////////////////////////////////////

static void test_rb_validate_node( rb_tree_t const *tree,
                                   rb_node_t const *node ) {
  if ( node->color == RB_RED ) {
    TEST( node->child[0]->color == RB_BLACK );
    TEST( node->child[1]->color == RB_BLACK );
  }

  if ( node->child[0] != &tree->nil ) {
    TEST( (*tree->cmp_fn)( node->child[0]->data, node->data ) <= 0 );
    test_rb_validate_node( tree, node->child[0] );
  }

  if ( node->child[1] != &tree->nil ) {
    TEST( (*tree->cmp_fn)( node->data, node->child[1]->data ) <= 0 );
    test_rb_validate_node( tree, node->child[1] );
  }
}

static void test_rb_validate( rb_tree_t const *tree ) {
  test_rb_validate_node( tree, tree->fake_root.child[0] );
}

static bool test_rb_visitor( void *node_data, void *v_data ) {
  char const *const str = node_data;
  unsigned *const letter_offset_ptr = v_data;
  if ( TEST( str != NULL ) )
    TEST( str[0] == STATIC_CAST( char, 'A' + *letter_offset_ptr ) );
  ++*letter_offset_ptr;
  return false;
}

// LCOV_EXCL_START
_Noreturn
static void usage( void ) {
  EPRINTF( "usage: %s\n", me );
  exit( EX_USAGE );
}
// LCOV_EXCL_STOP

////////// tests //////////////////////////////////////////////////////////////

static void test_insert1_find_delete( void ) {
  rb_tree_t tree;
  rb_tree_init( &tree, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  if ( TEST( rb_tree_insert( &tree, (void*)"A" ).inserted ) ) {
    test_rb_validate( &tree );
    rb_node_t *const node = rb_tree_find( &tree, (void*)"A" );
    if ( TEST( node != NULL ) ) {
      void *const data = rb_tree_delete( &tree, node );
      test_rb_validate( &tree );
      if ( TEST( data != NULL ) )
        TEST( strcmp( data, "A" ) == 0 );
    }
  }

  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
}

static void test_insert2_find_delete( void ) {
  rb_tree_t tree;
  rb_tree_init( &tree, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  if ( TEST( rb_tree_insert( &tree, (void*)"A" ).inserted ) &&
       TEST( rb_tree_insert( &tree, (void*)"B" ).inserted ) ) {
    test_rb_validate( &tree );
    rb_node_t *const node = rb_tree_find( &tree, (void*)"B" );
    if ( TEST( node != NULL ) ) {
      void *const data = rb_tree_delete( &tree, node );
      test_rb_validate( &tree );
      if ( TEST( data != NULL ) )
        TEST( strcmp( data, "B" ) == 0 );
    }
  }

  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[const] ) {
  me = base_name( argv[0] );
  if ( --argc != 0 )
    usage();                            // LCOV_EXCL_LINE

  test_insert1_find_delete();
  test_insert2_find_delete();

  rb_tree_t tree;
  rb_tree_init( &tree, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );
  rb_node_t *node;
  rb_insert_rv_t rb_insert_rv;

  // test insertion
  TEST( rb_tree_insert( &tree, (void*)"A" ).inserted );
  test_rb_validate( &tree );
  TEST( rb_tree_insert( &tree, (void*)"B" ).inserted );
  test_rb_validate( &tree );
  TEST( rb_tree_insert( &tree, (void*)"C" ).inserted );
  test_rb_validate( &tree );
  TEST( rb_tree_insert( &tree, (void*)"D" ).inserted );
  test_rb_validate( &tree );

  // test insertion with existing data
  rb_insert_rv = rb_tree_insert( &tree, (void*)"A" );
  test_rb_validate( &tree );
  if ( TEST( !rb_insert_rv.inserted ) )
    TEST( strcmp( rb_insert_rv.node->data, "A" ) == 0 );

  // test visitor
  unsigned letter_offset = 0;
  TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );

  // test find
  node = rb_tree_find( &tree, "A" );
  if ( TEST( node != NULL ) ) {
    TEST( strcmp( node->data, "A" ) == 0 );

    // test delete
    void *const data = rb_tree_delete( &tree, node );
    test_rb_validate( &tree );
    if ( TEST( data != NULL ) ) {
      TEST( strcmp( data, "A" ) == 0 );

      // test visitor again
      letter_offset = 1; // skip "A"
      TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );
    }
  }

  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );

  printf( "%u failures\n", test_failures );
  exit( test_failures > 0 ? EX_SOFTWARE : EX_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
