/*
**      PJL Library
**      src/red_black_test.c
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
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "red_black.h"
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <string.h>

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

static bool test_check_rb_node( rb_tree_t const *tree, rb_node_t const *node ) {
  TEST_FUNC_BEGIN();

  if ( node->color == RB_RED ) {
    TEST( node->child[0]->color == RB_BLACK );
    TEST( node->child[1]->color == RB_BLACK );
  }

  if ( node->child[0] != &tree->nil ) {
    TEST(
      (*tree->cmp_fn)(
        rb_node_data( tree, node->child[0] ), rb_node_data( tree, node )
      ) <= 0
    );
    TEST( test_check_rb_node( tree, node->child[0] ) );
  }

  if ( node->child[1] != &tree->nil ) {
    TEST(
      (*tree->cmp_fn)(
        rb_node_data( tree, node ), rb_node_data( tree, node->child[1] )
      ) <= 0
    );
    TEST( test_check_rb_node( tree, node->child[1] ) );
  }

  TEST_FUNC_END();
}

static bool test_check_rb_tree( rb_tree_t const *tree ) {
  TEST_FUNC_BEGIN();
  TEST( tree->root != NULL );
  TEST( tree->root->color == RB_BLACK );
  TEST( tree->nil.color == RB_BLACK );
  TEST( test_check_rb_node( tree, tree->root ) );
  TEST_FUNC_END();
}

static bool test_rb_visitor( void *node_data, void *visit_data ) {
  char const *const str = node_data;
  unsigned *const letter_offset_ptr = visit_data;
  if ( TEST( str != NULL ) )
    TEST( str[0] == STATIC_CAST( char, 'A' + *letter_offset_ptr ) );
  ++*letter_offset_ptr;
  return false;
}

////////// test functions /////////////////////////////////////////////////////

static bool test_insert1_find_delete( rb_dloc_t dloc ) {
  TEST_FUNC_BEGIN();
  rb_tree_t tree;
  rb_tree_init( &tree, dloc, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  if ( !TEST( rb_tree_insert( &tree, (void*)"A", 2 ).inserted ) )
    goto end_test;
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;

  rb_node_t *node = rb_tree_find( &tree, (void*)"A" );
  if ( TEST( node != NULL ) ) {
    rb_tree_delete( &tree, node );
    if ( !TEST( test_check_rb_tree( &tree ) ) )
      goto end_test;
    node = rb_tree_find( &tree, (void*)"A" );
    TEST( node == NULL );
  }

end_test:
  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
  TEST_FUNC_END();
}

static bool test_insert2_find_delete( rb_dloc_t dloc ) {
  TEST_FUNC_BEGIN();
  rb_tree_t tree;
  rb_tree_init( &tree, dloc, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  if ( !TEST( rb_tree_insert( &tree, (void*)"A", 2 ).inserted ) )
    goto end_test;
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;
  if ( !TEST( rb_tree_insert( &tree, (void*)"B", 2 ).inserted ) )
    goto end_test;
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;

  rb_node_t *node = rb_tree_find( &tree, (void*)"B" );
  if ( TEST( node != NULL ) ) {
    rb_tree_delete( &tree, node );
    if ( !TEST( test_check_rb_tree( &tree ) ) )
      goto end_test;
    node = rb_tree_find( &tree, (void*)"B" );
    TEST( node == NULL );
  }

end_test:
  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
  TEST_FUNC_END();
}

static bool test_script( rb_dloc_t dloc ) {
  TEST_FUNC_BEGIN();
  rb_tree_t tree;
  rb_tree_init( &tree, dloc, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  FOREACH_ARRAY_ELEMENT( rb_test_instruction_t, i, RB_TEST_SCRIPT ) {
    switch ( i->cmd ) {
      case RB_TEST_INSERT:;
        size_t const size = dloc == RB_DINT ? strlen( i->key ) + 1 : 0;
        TEST( rb_tree_insert( &tree, (void*)i->key, size ).inserted );
        if ( !TEST( test_check_rb_tree( &tree ) ) )
          goto end_test;
        break;
      case RB_TEST_DELETE:;
        rb_node_t *found = rb_tree_find( &tree, (void*)i->key );
        if ( TEST( found != NULL ) ) {
          rb_tree_delete( &tree, found );
          if ( !TEST( test_check_rb_tree( &tree ) ) )
            goto end_test;
          found = rb_tree_find( &tree, (void*)i->key );
          TEST( found == NULL );
        }
        break;
    } // switch
  } // for

end_test:
  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
  TEST_FUNC_END();
}

static bool test_various( rb_dloc_t dloc ) {
  TEST_FUNC_BEGIN();
  rb_tree_t tree;
  rb_tree_init( &tree, dloc, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );

  // test insertion
  TEST( rb_tree_insert( &tree, (void*)"A", 2 ).inserted );
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;
  TEST( rb_tree_insert( &tree, (void*)"B", 2 ).inserted );
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;
  TEST( rb_tree_insert( &tree, (void*)"C", 2 ).inserted );
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;
  TEST( rb_tree_insert( &tree, (void*)"D", 2 ).inserted );
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;

  // test insertion with existing data
  rb_insert_rv_t rv_rbi = rb_tree_insert( &tree, (void*)"A", 2 );
  if ( !TEST( test_check_rb_tree( &tree ) ) )
    goto end_test;
  if ( TEST( !rv_rbi.inserted ) )
    TEST( strcmp( rb_node_data( &tree, rv_rbi.node ), "A" ) == 0 );

  // test visitor
  unsigned letter_offset = 0;
  TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );

  // test find
  rb_node_t *node = rb_tree_find( &tree, "A" );
  if ( TEST( node != NULL ) ) {
    TEST( strcmp( rb_node_data( &tree, node ), "A" ) == 0 );

    // test delete
    rb_tree_delete( &tree, node );
    if ( !TEST( test_check_rb_tree( &tree ) ) )
      goto end_test;

    // test visitor again
    letter_offset = 1; // skip "A"
    TEST( rb_tree_visit( &tree, &test_rb_visitor, &letter_offset ) == NULL );
  }

end_test:
  rb_tree_cleanup( &tree, /*free_fn=*/NULL );
  TEST( rb_tree_empty( &tree ) );
  TEST_FUNC_END();
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *const argv[] ) {
  test_prog_init( argc, argv );

  test_insert1_find_delete( RB_DINT );
  test_insert1_find_delete( RB_DPTR );
  test_insert2_find_delete( RB_DINT );
  test_insert2_find_delete( RB_DPTR );

  test_various( RB_DINT );
  test_various( RB_DPTR );

  test_script( RB_DINT );
  test_script( RB_DPTR );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
