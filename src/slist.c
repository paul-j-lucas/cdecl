/*
**      PJL Library
**      src/slist.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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

/**
 * @file
 * Defines functions to manipulate a singly-linked-list data structure.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define SLIST_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "slist.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <stdint.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

void* slist_at_nocheck( slist_t const *list, size_t offset ) {
  assert( list != NULL );

  slist_node_t *p;

  if ( offset == list->len - 1 ) {
    p = list->tail;
  } else {
    for ( p = list->head; offset-- > 0; p = p->next )
      ;
  }

  assert( p != NULL );
  return p->data;
}

void slist_cleanup( slist_t *list, slist_free_fn_t free_fn ) {
  if ( list == NULL )
    return;

  slist_node_t *curr = list->head, *next;

  if ( free_fn == NULL ) {              // avoid repeated check in loop
    for ( ; curr != NULL; curr = next ) {
      next = curr->next;
      free( curr );
    } // for
  }
  else {
    for ( ; curr != NULL; curr = next ) {
      (*free_fn)( curr->data );
      next = curr->next;
      free( curr );
    } // for
  }

  slist_init( list );
}

int slist_cmp( slist_t const *i_list, slist_t const *j_list,
               slist_cmp_fn_t cmp_fn ) {
  assert( i_list != NULL );
  assert( j_list != NULL );
  assert( cmp_fn != NULL );

  if ( i_list == j_list )
    return 0;

  slist_node_t const *i_node = i_list->head, *j_node = j_list->head;

  for ( ; i_node != NULL && j_node != NULL;
          i_node = i_node->next, j_node = j_node->next ) {
    int const cmp = (*cmp_fn)( i_node->data, j_node->data );
    if ( cmp != 0 )
      return cmp;
  } // for

  return i_node == NULL ? (j_node == NULL ? 0 : -1) : 1;
}

slist_t slist_dup( slist_t const *src_list, ssize_t n,
                   slist_dup_fn_t dup_fn ) {
  slist_t dst_list;
  slist_init( &dst_list );

  if ( src_list != NULL && n != 0 ) {
    size_t un = STATIC_CAST( size_t, n );
    if ( dup_fn == NULL ) {             // avoid repeated check in loop
      FOREACH_SLIST_NODE( src_node, src_list ) {
        if ( un-- == 0 )
          break;
        slist_push_back( &dst_list, src_node->data );
      } // for
    }
    else {
      FOREACH_SLIST_NODE( src_node, src_list ) {
        if ( un-- == 0 )
          break;
        void *const dst_data = (*dup_fn)( src_node->data );
        slist_push_back( &dst_list, dst_data );
      } // for
    }
  }

  return dst_list;
}

bool slist_equal( slist_t const *i_list, slist_t const *j_list,
                  slist_equal_fn_t equal_fn ) {
  assert( i_list != NULL );
  assert( j_list != NULL );
  assert( equal_fn != NULL );

  if ( i_list == j_list )
    return true;
  if ( slist_len( i_list ) != slist_len( j_list ) )
    return false;

  slist_node_t const *i_node = i_list->head, *j_node = j_list->head;

  for ( ; i_node != NULL && j_node != NULL;
          i_node = i_node->next, j_node = j_node->next ) {
    if ( !(*equal_fn)( i_node->data, j_node->data ) )
      return false;
  } // for

  return true;
}

bool slist_free_if( slist_t *list, slist_pred_fn_t pred_fn, void *data ) {
  assert( list != NULL );
  assert( pred_fn != NULL );

  size_t const orig_len = list->len;

  // special case: predicate matches list->head
  for (;;) {
    slist_node_t *const curr = list->head;
    if ( curr == NULL )
      goto done;
    if ( !(*pred_fn)( curr, data ) )
      break;
    if ( list->tail == curr )
      list->tail = NULL;
    list->head = curr->next;
    free( curr );
    --list->len;
  } // for

  assert( list->head != NULL );
  assert( list->tail != NULL );
  assert( list->len > 0 );

  // general case: predicate matches any node except list->head
  slist_node_t *prev = list->head;
  for (;;) {
    slist_node_t *const curr = prev->next;
    if ( curr == NULL )
      break;
    if ( !(*pred_fn)( curr, data ) ) {
      prev = curr;
      continue;
    }
    if ( list->tail == curr )
      list->tail = prev;
    prev->next = curr->next;
    free( curr );
    --list->len;
  } // for

done:
  return list->len < orig_len;
}

slist_t slist_move( slist_t *list ) {
  slist_t rv_list;
  if ( list != NULL ) {
    rv_list = *list;
    slist_init( list );
  } else {
    slist_init( &rv_list );
  }
  return rv_list;
}

void* slist_pop_back( slist_t *list ) {
  assert( list != NULL );
  if ( list->len < 2 )
    return slist_pop_front( list );

  slist_node_t *new_tail = list->head;
  while ( new_tail->next != list->tail )
    new_tail = new_tail->next;
  new_tail->next = NULL;

  void *const data = list->tail->data;
  free( list->tail );
  list->tail = new_tail;
  --list->len;
  return data;
}

void* slist_pop_front( slist_t *list ) {
  assert( list != NULL );
  if ( list->head == NULL )
    return NULL;
  void *const data = list->head->data;
  slist_node_t *const next = list->head->next;
  free( list->head );
  list->head = next;
  if ( list->head == NULL )
    list->tail = NULL;
  --list->len;
  return data;
}

void slist_push_back( slist_t *list, void *data ) {
  assert( list != NULL );
  slist_node_t *const new_tail = MALLOC( slist_node_t, 1 );
  *new_tail = (slist_node_t){ .data = data };

  if ( list->head == NULL ) {
    assert( list->tail == NULL );
    list->head = new_tail;
  } else {
    assert( list->tail != NULL );
    assert( list->tail->next == NULL );
    list->tail->next = new_tail;
  }
  list->tail = new_tail;
  ++list->len;
}

void slist_push_front( slist_t *list, void *data ) {
  assert( list != NULL );
  slist_node_t *const new_head = MALLOC( slist_node_t, 1 );
  *new_head = (slist_node_t){ .data = data, .next = list->head };
  list->head = new_head;
  if ( list->tail == NULL )
    list->tail = new_head;
  ++list->len;
}

void slist_push_list_back( slist_t *dst_list, slist_t *src_list ) {
  assert( dst_list != NULL );
  assert( src_list != NULL );
  if ( dst_list->head == NULL ) {
    assert( dst_list->tail == NULL );
    dst_list->head = src_list->head;
    dst_list->tail = src_list->tail;
  }
  else if ( src_list->head != NULL ) {
    assert( src_list->tail != NULL );
    assert( dst_list->tail->next == NULL );
    dst_list->tail->next = src_list->head;
    dst_list->tail = src_list->tail;
  }
  dst_list->len += src_list->len;
  slist_init( src_list );
}

void slist_push_list_front( slist_t *dst_list, slist_t *src_list ) {
  assert( dst_list != NULL );
  assert( src_list != NULL );
  if ( dst_list->head == NULL ) {
    assert( dst_list->tail == NULL );
    dst_list->head = src_list->head;
    dst_list->tail = src_list->tail;
  }
  else if ( src_list->head != NULL ) {
    assert( src_list->tail != NULL );
    src_list->tail->next = dst_list->head;
    dst_list->head = src_list->head;
  }
  dst_list->len += src_list->len;
  slist_init( src_list );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
