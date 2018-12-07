/*
**      cdecl -- C gibberish translator
**      src/slist.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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

#ifndef cdecl_slist_H
#define cdecl_slist_H

/**
 * @file
 * Declares a singly-linked-list data structure and functions to manipulate it.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stddef.h>                     /* for NULL */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_SLIST_INLINE
# define CDECL_SLIST_INLINE _GL_INLINE
#endif /* CDECL_SLIST_INLINE */

/// @endcond

/**
 * @defgroup slist-group Singly-Linked List
 * Types and functions for manipulating singly-linked lists.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

typedef struct slist      slist_t;
typedef struct slist_node slist_node_t;

/**
 * The signature for a function passed to `slist_free()` used to free data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the data to free.
 */
typedef void (*slist_data_free_fn_t)( void *data );

///////////////////////////////////////////////////////////////////////////////

/**
 * Singly-linked list.
 */
struct slist {
  slist_node_t *head;                   ///< Pointer to list head.
  slist_node_t *tail;                   ///< Pointer to list tail.
};

/**
 * Singly-linked-list node.
 */
struct slist_node {
  void *data;                           ///< Pointer to user data.
  slist_node_t *next;                   ///< Pointer to next node or null.
};

/**
 * Convenience macro to get the data cast to \a DATA_TYPE given an
 * `slist_node`.
 *
 * @param DATA_TYPE The type of the data to cast to.
 * @param NODE The `slist_node` to get the data of.
 * @return Returns said data cast to \a DATA_TYPE.
 * @hideinitializer
 */
#define SLIST_DATA(DATA_TYPE,NODE) \
  REINTERPRET_CAST( DATA_TYPE, (NODE)->data )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Appends \a data onto the end of \a list.
 *
 * @param list The <code>\ref slist</code> to append onto.
 * @param data The data to append.
 * @return Returns \a data.
 */
void* slist_append( slist_t *list, void *data );

/**
 * Convenience macro that appends \a DATA onto the end of \a LIST.
 *
 * @param DATA_TYPE The type of the data to cast to.
 * @param LIST A pointer to the <code>\ref slist</code> to append to.
 * @param DATA The data to append.
 * @return Returns \a DATA cast to \a DATA_TYPE.
 * @hideinitializer
 */
#define SLIST_APPEND(DATA_TYPE,LIST,DATA) \
  REINTERPRET_CAST( DATA_TYPE, slist_append( (LIST), REINTERPRET_CAST( void*, (DATA) ) ) )

/**
 * Appends \a src onto the end of \a dst.
 *
 * @param dst The <code>\ref slist</code> to append onto.
 * @param src The <code>\ref slist</code> to append.  It is made empty.
 */
void slist_append_list( slist_t *dst, slist_t *src );

/**
 * Frees all memory associated with \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.  If null, does
 * nothing.
 * @param data_free_fn A pointer to a function to use to free the data at each
 * node of \a list or null if none is required.
 */
void slist_free( slist_t *list, slist_data_free_fn_t data_free_fn );

/**
 * Initializes \a list.  This is not necessary for either global or `static`
 * lists.
 *
 * @param list A pointer to the <code>\ref slist</code> to initialize.
 */
CDECL_SLIST_INLINE void slist_init( slist_t *list ) {
  list->head = list->tail = NULL;
}

/**
 * Gets the length of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code> to get the length of.
 * @return Returns said length.
 */
unsigned slist_len( slist_t const *list );

/**
 * Pops data from the head of \a list.
 *
 * @param list The pointer to the <code>\ref slist</code>.
 * @return Returns the data from the head of \a list.  The caller is
 * responsible for deleting it (if necessary).
 */
void* slist_pop( slist_t *list );

/**
 * Convenience macro that pops data from the head of \a LIST and casts it to
 * the requested type.
 *
 * @param DATA_TYPE The type of the data to cast to.
 * @param LIST A pointer to the <code>\ref slist</code>.
 * @return Returns the data from the head of \a LIST cast to \a DATA_TYPE or
 * null (or equivalent) if the <code>\ref slist</code> is empty.  The caller is
 * responsible for deleting it (if necessary).
 * @hideinitializer
 */
#define SLIST_POP(DATA_TYPE,LIST) \
  REINTERPRET_CAST( DATA_TYPE, slist_pop( LIST ) )

/**
 * Pushes a node onto the front of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @param data The pointer to the data to add.
 */
void slist_push( slist_t *list, void *data );

/**
 * Peeks at the data at the head of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @return Returns a pointer to the data from the node at the head of \a list
 * or null if \a list is empty.
 */
CDECL_SLIST_INLINE void* slist_top( slist_t const *list ) {
  return list->head != NULL ? list->head->data : NULL;
}

/**
 * Convenience macro that peeks at the data at the head of \a LIST and casts it
 * to the requested type.
 *
 * @param DATA_TYPE The type of the data.
 * @param LIST A pointer to the <code>\ref slist</code>.
 * @return Returns the data from the head of \a LIST cast to \a DATA_TYPE or
 * null (or equivalent) if the <code>\ref slist</code> is empty.
 * @hideinitializer
 */
#define SLIST_TOP(DATA_TYPE,LIST) \
  REINTERPRET_CAST( DATA_TYPE, slist_top( LIST ) )

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_slist_H */
/* vim:set et sw=2 ts=2: */
