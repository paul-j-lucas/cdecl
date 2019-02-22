/*
**      cdecl -- C gibberish translator
**      src/slist.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_SLIST_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "slist.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

void slist_free( slist_t *list, slist_data_free_fn_t data_free_fn ) {
  if ( list != NULL ) {
    for ( slist_node_t *p = list->head; p != NULL; ) {
      slist_node_t *const node = p;
      p = p->next;
      if ( data_free_fn != NULL )
        (*data_free_fn)( node->data );
      FREE( node );
    } // for
    slist_init( list );
  }
}

unsigned slist_len( slist_t const *list ) {
  unsigned len = 0;
  if ( list != NULL ) {
    for ( slist_node_t *p = list->head; p != NULL; p = p->next )
      ++len;
  }
  return len;
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
}

void slist_push_list_tail( slist_t *dst, slist_t *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  if ( dst->head == NULL ) {
    assert( dst->tail == NULL );
    dst->head = src->head;
    dst->tail = src->tail;
  }
  else if ( src->head != NULL ) {
    assert( src->tail != NULL );
    assert( dst->tail->next == NULL );
    dst->tail->next = src->head;
    dst->tail = src->tail;
  }
  src->head = src->tail = NULL;
}

void* slist_push_tail( slist_t *list, void *data ) {
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
  return data;
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

/* vim:set et sw=2 ts=2: */
