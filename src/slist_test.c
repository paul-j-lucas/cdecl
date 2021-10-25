/*
**      cdecl -- C gibberish translator
**      src/slist_test.c
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
#include "slist.h"
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// extern variables
char const          *me;                ///< Program name.

// local variables
static char const  *str_equal_to;
static unsigned     test_failures;

////////// local functions ////////////////////////////////////////////////////

static bool slist_node_str_equal( void *data ) {
  char const *const s = data;
  return strcmp( s, str_equal_to ) == 0;
}

static bool test_slist_cmp( void ) {
  TEST_FN_BEGIN();
  slist_t list, list2;
  slist_init( &list );
  slist_init( &list2 );

  // test 2 empty lists
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) == 0 );

  // test empty and non-empty lists
  slist_push_tail( &list2, (void*)"A" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) != 0 );
  slist_free( &list2, NULL, NULL );

  // test non-empty and empty lists
  slist_push_tail( &list, (void*)"A" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) != 0 );
  slist_free( &list, NULL, NULL );

  // test matching 1,2,3-element lists
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list2, (void*)"A" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) == 0 );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list2, (void*)"B" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) == 0 );
  slist_push_tail( &list, (void*)"C" );
  slist_push_tail( &list2, (void*)"C" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) == 0 );
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  // test 1-element non-matching lists
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) < 0 );
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  // test 1-element and 2-element lists
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list2, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  TEST( slist_cmp( &list, &list2, (slist_node_data_cmp_fn_t)&strcmp ) < 0 );
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_dup( void ) {
  TEST_FN_BEGIN();
  slist_t list, list2;
  slist_init( &list );
  slist_init( &list2 );
  char const A = 'A';
  char *p;

  // empty list
  list2 = slist_dup( &list, -1, NULL, NULL );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );

  // 1-element list
  slist_push_tail( &list, (void*)&A );
  list2 = slist_dup( &list, -1, NULL, NULL );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 1 );
  if ( TEST( (p = slist_peek_head( &list2 )) != NULL ) ) {
    TEST( *p == 'A' );
    TEST( p == &A );
  }
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  // 2-element list
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  list2 = slist_dup( &list, -1, NULL, NULL );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 2 );
  if ( TEST( (p = slist_peek_head( &list2 )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list2 )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  // 3-element list
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  list2 = slist_dup( &list, -1, NULL, NULL );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 3 );
  if ( TEST( (p = slist_peek_head( &list2 )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list2, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list2 )) != NULL ) )
    TEST( *p == 'C' );
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  // check data_dup_fn
  slist_push_tail( &list, (void*)&A );
  list2 = slist_dup( &list, -1, NULL, (slist_node_data_dup_fn_t)&strdup );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 1 );
  if ( TEST( (p = slist_peek_head( &list2 )) != NULL ) ) {
    TEST( *p == 'A' );
    if ( TEST( p != &A ) )
      free( p );
  }
  slist_free( &list, NULL, NULL );
  slist_free( &list2, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_free_if( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  // test match list[0] with list->len == 1
  slist_push_tail( &list, (void*)"A" );
  str_equal_to = "A";
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );

  // test match list[0] and list[1] with list->len == 2
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"A" );
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );

  // test match list[0] with list->len == 2
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  // test match list[0] and list[1] with list->len == 3
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  // test match list[1] with list->len == 3
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  str_equal_to = "B";
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_len( &list ) == 2 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_free( &list, NULL, NULL );

  // test match list[1] with list->len == 2
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_free( &list, NULL, NULL );

  // test match list[1] and list[2] with list->len == 3
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_peek_at( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  if ( TEST( (p = slist_peek_at( &list, 0 )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  TEST( slist_peek_at( &list, 3 ) == NULL );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_peek_atr( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  if ( TEST( (p = slist_peek_atr( &list, 0 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_peek_atr( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_atr( &list, 2 )) != NULL ) )
    TEST( *p == 'A' );
  TEST( slist_peek_at( &list, 4 ) == NULL );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_pop_head( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  TEST( slist_pop_head( &list ) == NULL );

  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  if ( TEST( (p = slist_pop_head( &list )) != NULL ) ) {
    TEST( *p == 'A' );
    TEST( slist_len( &list ) == 1 );
    TEST( !slist_empty( &list ) );
  }
  if ( TEST( (p = slist_pop_head( &list )) != NULL ) ) {
    TEST( *p == 'B' );
    TEST( slist_len( &list ) == 0 );
    TEST( slist_empty( &list ) );
  }
  TEST( slist_pop_head( &list ) == NULL );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_push_head( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_head( &list, (void*)"B" );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'B' );

  slist_push_head( &list, (void*)"A" );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_push_list_head( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_t list2;
  slist_init( &list2 );

  // empty lists
  slist_push_list_head( &list, &list2 );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );

  // empty first, 1-element second
  slist_push_tail( &list2, (void*)"A" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_free( &list, NULL, NULL );

  // empty first, 2-element second
  slist_push_tail( &list2, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  // empty first, 3-element second
  slist_push_tail( &list2, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  slist_push_tail( &list2, (void*)"C" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_free( &list, NULL, NULL );

  // 1-element first, 1-element second
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list2, (void*)"A" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  // 2-element first, 1-element second
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  slist_push_tail( &list2, (void*)"A" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_free( &list, NULL, NULL );

  // 3-element first, 1-element second
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  slist_push_tail( &list, (void*)"D" );
  slist_push_tail( &list2, (void*)"A" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 4 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'D' );
  slist_free( &list, NULL, NULL );

  // 3-element first, 2-element second
  slist_push_tail( &list, (void*)"C" );
  slist_push_tail( &list, (void*)"D" );
  slist_push_tail( &list, (void*)"E" );
  slist_push_tail( &list2, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  slist_push_list_head( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 5 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_peek_at( &list, 3 )) != NULL ) )
    TEST( *p == 'D' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'E' );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_push_list_tail( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_t list2;
  slist_init( &list2 );

  // empty lists
  slist_push_list_tail( &list, &list2 );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );

  // empty first, 1-element second
  slist_push_tail( &list2, (void*)"A" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_free( &list, NULL, NULL );

  // empty first, 2-element second
  slist_push_tail( &list2, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  // empty first, 3-element second
  slist_push_tail( &list2, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  slist_push_tail( &list2, (void*)"C" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_free( &list, NULL, NULL );

  // 1-element first, 1-element second
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list2, (void*)"B" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  // 2-element first, 1-element second
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list2, (void*)"C" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_free( &list, NULL, NULL );

  // 3-element first, 1-element second
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  slist_push_tail( &list2, (void*)"D" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 4 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'D' );
  slist_free( &list, NULL, NULL );

  // 3-element first, 2-element second
  slist_push_tail( &list, (void*)"A" );
  slist_push_tail( &list, (void*)"B" );
  slist_push_tail( &list, (void*)"C" );
  slist_push_tail( &list2, (void*)"D" );
  slist_push_tail( &list2, (void*)"E" );
  slist_push_list_tail( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 5 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_peek_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_peek_at( &list, 3 )) != NULL ) )
    TEST( *p == 'D' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'E' );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

static bool test_slist_push_tail( void ) {
  TEST_FN_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_tail( &list, (void*)"A" );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'A' );

  slist_push_tail( &list, (void*)"B" );
  TEST( slist_len( &list ) == 2 );
  if ( TEST( (p = slist_peek_head( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_peek_tail( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_free( &list, NULL, NULL );

  TEST_FN_END();
}

// LCOV_EXCL_START
static noreturn void usage( void ) {
  EPRINTF( "usage: %s\n", me );
  exit( EX_USAGE );
}
// LCOV_EXCL_STOP

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[] ) {
  me = base_name( argv[0] );
  if ( --argc != 0 )
    usage();                            // LCOV_EXCL_LINE

  if ( test_slist_push_head() && test_slist_push_tail() ) {
    test_slist_free_if();
    if ( test_slist_peek_at() ) {
      test_slist_peek_atr();
      test_slist_cmp();
      test_slist_dup();
      test_slist_push_list_head();
      test_slist_push_list_tail();
    }
    test_slist_pop_head();
  }

  printf( "%u failures\n", test_failures );
  exit( test_failures > 0 ? EX_SOFTWARE : EX_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
