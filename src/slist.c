/*
**      cdecl -- C gibberish translator
**      src/slist.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
#define C_SLIST_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "slist.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdint.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

int slist_cmp( slist_t const *i_list, slist_t const *j_list,
               slist_node_data_cmp_fn_t data_cmp_fn ) {
  assert( i_list != NULL );
  assert( j_list != NULL );

  slist_node_t const *i_node = i_list->head, *j_node = j_list->head;
  for ( ; i_node != NULL && j_node != NULL;
        i_node = i_node->next, j_node = j_node->next ) {
    int const cmp = data_cmp_fn != NULL ?
      (*data_cmp_fn)( i_node->data, j_node->data ) :
      (int)((intptr_t)i_node->data - (intptr_t)j_node->data);
    if ( cmp != 0 )
      return cmp;
  } // for
  return i_node == NULL ? (j_node == NULL ? 0 : -1) : 1;
}

slist_t slist_dup( slist_t const *src_list, ssize_t n,
                   slist_data_dup_fn_t data_dup_fn,
                   slist_node_data_dup_fn_t node_data_dup_fn ) {
  slist_t dst_list;
  slist_init( &dst_list );

  if ( src_list != NULL && n != 0 ) {
    size_t un = (size_t)n;
    dst_list.data = data_dup_fn != NULL ?
      (*data_dup_fn)( src_list->data ) : src_list->data;
    for ( slist_node_t *src_node = src_list->head; src_node != NULL && un-- > 0;
          src_node = src_node->next ) {
      void *const dst_data = node_data_dup_fn != NULL ?
        (*node_data_dup_fn)( src_node->data ) : src_node->data;
      slist_push_tail( &dst_list, dst_data );
    } // for
  }

  return dst_list;
}

void slist_free( slist_t *list, slist_data_free_fn_t data_free_fn,
                 slist_node_data_free_fn_t node_data_free_fn ) {
  if ( list != NULL ) {
    if ( data_free_fn != NULL )
      (*data_free_fn)( list->data );
    for ( slist_node_t *p = list->head; p != NULL; ) {
      if ( node_data_free_fn != NULL )
        (*node_data_free_fn)( p->data );
      slist_node_t *const next = p->next;
      FREE( p );
      p = next;
    } // for
    slist_init( list );
  }
}

void* slist_peek_at( slist_t const *list, size_t offset ) {
  assert( list != NULL );
  if ( offset >= list->len )
    return NULL;

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

void* slist_pop_head( slist_t *list ) {
  assert( list != NULL );
  if ( list->head != NULL ) {
    void *const data = list->head->data;
    slist_node_t *const next = list->head->next;
    FREE( list->head );
    list->head = next;
    if ( list->head == NULL )
      list->tail = NULL;
    --list->len;
    return data;
  }
  return NULL;
}

void slist_push_head( slist_t *list, void *data ) {
  assert( list != NULL );
  slist_node_t *const new_head = MALLOC( slist_node_t, 1 );
  new_head->data = data;
  new_head->next = list->head;
  list->head = new_head;
  if ( list->tail == NULL )
    list->tail = new_head;
  ++list->len;
}

void slist_push_list_head( slist_t *dst_list, slist_t *src_list ) {
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

void slist_push_list_tail( slist_t *dst_list, slist_t *src_list ) {
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

void slist_push_tail( slist_t *list, void *data ) {
  assert( list != NULL );
  slist_node_t *const new_tail = MALLOC( slist_node_t, 1 );
  new_tail->data = data;
  new_tail->next = NULL;

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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
