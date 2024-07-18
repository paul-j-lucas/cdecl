/*
**      PJL Library
**      src/red_black_test.c
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
#include "red_black.h"
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#define RB_NIL(TREE)              (&(TREE)->nil)

///////////////////////////////////////////////////////////////////////////////

enum rb_test_cmd {
  RB_TEST_INSERT,
  RB_TEST_DELETE
};
typedef enum rb_test_cmd rb_test_cmd_t;

struct rb_test_instruction {
  rb_test_cmd_t   cmd;
  char const     *key;
};
typedef struct rb_test_instruction rb_test_instruction_t;

// This sequence of instructions used to cause the tree's invariants to break,
// so it's now a test.
static rb_test_instruction_t const RB_TEST_SCRIPT[] = {
  { RB_TEST_INSERT, "cx_throw()" },
  { RB_TEST_INSERT, "CX_IMPL_DEF_ARGS(CX_IMPL_THROW_,)" },
  { RB_TEST_INSERT, "CX_IMPL_NAME2(CX_IMPL_THROW_, CX_IMPL_NARG())" },
  { RB_TEST_INSERT, "CX_IMPL_NARG()" },
  { RB_TEST_INSERT, "CX_IMPL_NARG_HELPER1(CX_IMPL_HAS_COMMA( ), CX_IMPL_HAS_COMMA( CX_IMPL_COMMA () ), CX_IMPL_NARG_( , CX_IMPL_REV_SEQ_N ))" },
  { RB_TEST_INSERT, "CX_IMPL_HAS_COMMA()" },
  { RB_TEST_INSERT, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_DELETE, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_INSERT, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_INSERT, "CX_IMPL_ARG_N(, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0)" },
  { RB_TEST_DELETE, "CX_IMPL_ARG_N(, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0)" },
  { RB_TEST_DELETE, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_DELETE, "CX_IMPL_HAS_COMMA()" },
  { RB_TEST_INSERT, "CX_IMPL_COMMA" },
  { RB_TEST_INSERT, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_DELETE, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_INSERT, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_INSERT, "CX_IMPL_ARG_N(CX_IMPL_COMMA (), 1, 1, 1, 1, 1, 1, 1, 1, 0, 0)" },
  { RB_TEST_DELETE, "CX_IMPL_ARG_N(CX_IMPL_COMMA (), 1, 1, 1, 1, 1, 1, 1, 1, 0, 0)" },
  { RB_TEST_DELETE, "CX_IMPL_HAS_COMMA_N" },
  { RB_TEST_DELETE, "CX_IMPL_COMMA" },
  { RB_TEST_INSERT, "CX_IMPL_REV_SEQ_N" },
  { RB_TEST_DELETE, "CX_IMPL_REV_SEQ_N" },
  { RB_TEST_INSERT, "CX_IMPL_REV_SEQ_N" },
  { RB_TEST_INSERT, "CX_IMPL_ARG_N(, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)" },
  { RB_TEST_DELETE, "CX_IMPL_ARG_N(, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)" },
  { RB_TEST_DELETE, "CX_IMPL_REV_SEQ_N" },
  { RB_TEST_INSERT, "CX_IMPL_NARG_HELPER2(0, 0, 1)" },
  { RB_TEST_INSERT, "CX_IMPL_NARG_HELPER3_00(1)" },
  { RB_TEST_DELETE, "CX_IMPL_NARG_HELPER3_00(1)" },
  { RB_TEST_DELETE,  "CX_IMPL_NARG_HELPER2(0, 0, 1)" },
  { RB_TEST_DELETE, "CX_IMPL_NARG_HELPER1(CX_IMPL_HAS_COMMA( ), CX_IMPL_HAS_COMMA( CX_IMPL_COMMA () ), CX_IMPL_NARG_( , CX_IMPL_REV_SEQ_N ))" },
};

////////// local functions ////////////////////////////////////////////////////

static void test_check_rb_node( rb_tree_t const *tree, rb_node_t const *node ) {
  if ( node->color == RB_RED ) {
    TEST( node->child[0]->color == RB_BLACK );
    TEST( node->child[1]->color == RB_BLACK );
  }

  if ( node->child[0] != RB_NIL(tree) ) {
    TEST( (*tree->cmp_fn)( node->child[0]->data, node->data ) <= 0 );
    test_check_rb_node( tree, node->child[0] );
  }

  if ( node->child[1] != RB_NIL(tree) ) {
    TEST( (*tree->cmp_fn)( node->data, node->child[1]->data ) <= 0 );
    test_check_rb_node( tree, node->child[1] );
  }
}

static void test_check_rb_tree( rb_tree_t const *tree ) {
  TEST( tree->root != NULL );
  TEST( tree->root->color == RB_BLACK );
  TEST( RB_NIL(tree)->color == RB_BLACK );
  test_check_rb_node( tree, tree->root );
}

static bool test_rb_visitor( void *node_data, void *v_data ) {
  char const *const str = node_data;
  unsigned *const letter_offset_ptr = v_data;
  if ( TEST( str != NULL ) )
    TEST( str[0] == STATIC_CAST( char, 'A' + *letter_offset_ptr ) );
  ++*letter_offset_ptr;
  return false;
}

////////// tests //////////////////////////////////////////////////////////////

static void test_insert1_find_delete( void ) {
  rb_tree_t tree;
  rb_tree_init( &tree, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  if ( TEST( rb_tree_insert( &tree, (void*)"A" ).inserted ) ) {
    test_check_rb_tree( &tree );
    rb_node_t *const node = rb_tree_find( &tree, (void*)"A" );
    if ( TEST( node != NULL ) ) {
      void *const data = rb_tree_delete( &tree, node );
      test_check_rb_tree( &tree );
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
    test_check_rb_tree( &tree );
    rb_node_t *const node = rb_tree_find( &tree, (void*)"B" );
    if ( TEST( node != NULL ) ) {
      void *const data = rb_tree_delete( &tree, node );
      test_check_rb_tree( &tree );
      if ( TEST( data != NULL ) )
        TEST( strcmp( data, "B" ) == 0 );
    }
  }

  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
}

static void test_script( void ) {
  rb_tree_t tree;
  rb_tree_init( &tree, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  FOREACH_ARRAY_ELEMENT( rb_test_instruction_t, i, RB_TEST_SCRIPT ) {
    switch ( i->cmd ) {
      case RB_TEST_INSERT:
        TEST( rb_tree_insert( &tree, (void*)i->key ).inserted );
        break;
      case RB_TEST_DELETE:
        NO_OP;
        rb_node_t *const found = rb_tree_find( &tree, (void*)i->key );
        if ( TEST( found != NULL ) ) {
          char const *const data = rb_tree_delete( &tree, found );
          if ( TEST( data != NULL ) )
            TEST( strcmp( data, i->key ) == 0 );
        }
        break;
    } // switch
  } // for

  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[const] ) {
  test_prog_init( argc, argv );

  test_insert1_find_delete();
  test_insert2_find_delete();

  rb_tree_t tree;
  rb_tree_init( &tree, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );
  rb_node_t *node;
  rb_insert_rv_t rb_insert_rv;

  // test insertion
  TEST( rb_tree_insert( &tree, (void*)"A" ).inserted );
  test_check_rb_tree( &tree );
  TEST( rb_tree_insert( &tree, (void*)"B" ).inserted );
  test_check_rb_tree( &tree );
  TEST( rb_tree_insert( &tree, (void*)"C" ).inserted );
  test_check_rb_tree( &tree );
  TEST( rb_tree_insert( &tree, (void*)"D" ).inserted );
  test_check_rb_tree( &tree );

  // test insertion with existing data
  rb_insert_rv = rb_tree_insert( &tree, (void*)"A" );
  test_check_rb_tree( &tree );
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
    test_check_rb_tree( &tree );
    if ( TEST( data != NULL ) ) {
      TEST( strcmp( data, "A" ) == 0 );

      // test visitor again
      letter_offset = 1; // skip "A"
      TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );
    }
  }

  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );

  test_script();
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
