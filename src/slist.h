/*
**      cdecl -- C gibberish translator
**      src/slist.h
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

#ifndef cdecl_slist_H
#define cdecl_slist_H

/**
 * @file
 * Declares a singly-linked-list data structure and functions to manipulate it.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef C_SLIST_INLINE
# define C_SLIST_INLINE _GL_INLINE
#endif /* C_SLIST_INLINE */

/// @endcond

/**
 * @defgroup slist-group Singly-Linked List
 * Types and functions for manipulating singly-linked lists.
 * @{
 */

/**
 * Creates a single-node `slist` on the stack with \a NODE_DATA.
 *
 * @param VAR_NAME The name for the `slist` variable.
 * @param LIST_DATA A pointer to the list data, if any.
 * @param NODE_DATA A pointer to the node data.
 */
#define SLIST_VAR_INIT(VAR_NAME,LIST_DATA,NODE_DATA)                          \
  slist_node_t VAR_NAME##_node = { NULL, CONST_CAST( void*, (NODE_DATA) ) };  \
  slist_t VAR_NAME = { &VAR_NAME##_node, &VAR_NAME##_node, 1, (LIST_DATA) }

///////////////////////////////////////////////////////////////////////////////

typedef struct slist      slist_t;
typedef struct slist_node slist_node_t;

/**
 * The signature for a function passed to slist_dup() used to duplicate data
 * associated with the list, if any.
 *
 * @param data A pointer to the data to duplicate.
 * @return Returns a duplicate of \a data.
 */
typedef void* (*slist_data_dup_fn_t)( void const *data );

/**
 * The signature for a function passed to slist_free() used to free data
 * associated with the list, if any.
 *
 * @param data A pointer to the data to free.
 */
typedef void (*slist_data_free_fn_t)( void *data );

/**
 * The signature for a function passed to slist_cmp() used to compare data
 * associated with each node (if necessary).
 *
 * @param i_data A pointer to the first data to compare.
 * @param j_data A pointer to the second data to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
typedef int (*slist_node_data_cmp_fn_t)( void const *i_data,
                                         void const *j_data );

/**
 * The signature for a function passed to slist_dup() used to duplicate data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the data to duplicate.
 * @return Returns a duplicate of \a data.
 */
typedef void* (*slist_node_data_dup_fn_t)( void const *data );

/**
 * The signature for a function passed to slist_free() used to free data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the data to free.
 */
typedef void (*slist_node_data_free_fn_t)( void *data );

///////////////////////////////////////////////////////////////////////////////

/**
 * Singly-linked list.
 */
struct slist {
  slist_node_t *head;                   ///< Pointer to list head.
  slist_node_t *tail;                   ///< Pointer to list tail.
  size_t        len;                    ///< Length of list.
  void         *data;                   ///< Pointer to user data, if any.
};

/**
 * Singly-linked-list node.
 */
struct slist_node {
  slist_node_t *next;                   ///< Pointer to next node or null.
  void         *data;                   ///< Pointer to user data.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Compares two lists.
 *
 * @param i_list The first list.
 * @param j_list The second list.
 * @param data_cmp_fn A pointer to a function to use to compare data at each
 * node of \a i_list and \a j_list or null if none is required (hence the data
 * will be compared directly).
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_list is
 * less than, equal to, or greater than \a j_list, respectively.
 */
PJL_WARN_UNUSED_RESULT
int slist_cmp( slist_t const *i_list, slist_t const *j_list,
               slist_node_data_cmp_fn_t data_cmp_fn );

/**
 * Duplicates \a src and all of its nodes.
 *
 * @param src The <code>\ref slist</code> to duplicate.  It may ne null.
 * @param n The number of nodes to duplicate; -1 is equivalent to slist_len().
 * @param data_dup_fn A pointer to a function to use to duplicate the data of
 * \a src or null if none is required (hence a shallow copy will be done).
 * @param node_data_dup_fn A pointer to a function to use to duplicate the data
 * at each node of \a src or null if none is required (hence a shallow copy
 * will be done).
 * @return Returns a duplicate of \a src.
 */
PJL_WARN_UNUSED_RESULT
slist_t slist_dup( slist_t const *src, ssize_t n,
                   slist_data_dup_fn_t data_dup_fn,
                   slist_node_data_dup_fn_t node_data_dup_fn );

/**
 * Gets whether \a list is empty.
 *
 * @param list A pointer to the <code>\ref slist</code> to check.
 * @return Returns `true` only if \a list is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_len()
 */
C_SLIST_INLINE PJL_WARN_UNUSED_RESULT
bool slist_empty( slist_t const *list ) {
  return list->head == NULL;
}

/**
 * Frees all memory associated with \a list but _not_ \a list itself.
 *
 * @param list A pointer to the list to free.  If null, does nothing;
 * otherwise, reinitializes \a list upon completion.
 * @param data_free_fn A pointer to a function to use to free the data
 * associated with \a list or null if none is required.
 * @param node_data_free_fn A pointer to a function to use to free the data at
 * each node of \a list or null if none is required.
 */
void slist_free( slist_t *list, slist_data_free_fn_t data_free_fn,
                 slist_node_data_free_fn_t node_data_free_fn );

/**
 * Initializes \a list.  This is not necessary for either global or `static`
 * lists.
 *
 * @param list A pointer to the <code>\ref slist</code> to initialize.
 */
C_SLIST_INLINE
void slist_init( slist_t *list ) {
  MEM_ZERO( list );
}

/**
 * Gets the length of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code> to get the length of.
 * @return Returns said length.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_empty()
 */
C_SLIST_INLINE PJL_WARN_UNUSED_RESULT
size_t slist_len( slist_t const *list ) {
  return list->len;
}

/**
 * Peeks at the data at \a offset of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @param offset The offset (starting at 0) of the data to get.
 * @return Returns the data from the node at \a offset or null if \a offset
 * &gt;= slist_len().
 *
 * @note This is an O(n) operation.
 *
 * @sa slist_peek_atr()
 * @sa slist_peek_head()
 * @sa slist_peek_tail()
 */
PJL_WARN_UNUSED_RESULT
void* slist_peek_at( slist_t const *list, size_t offset );

/**
 * Peeks at the data at \a roffset from the tail of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @param roffset The reverse offset (starting at 0) of the data to get.
 * @return Returns the data from the node at \a roffset or null if \a roffset
 * &gt;= slist_len().
 *
 * @note This is an O(n) operation.
 *
 * @sa slist_peek_at()
 * @sa slist_peek_tail()
 */
C_SLIST_INLINE PJL_WARN_UNUSED_RESULT
void* slist_peek_atr( slist_t const *list, size_t roffset ) {
  return roffset < list->len ?
    slist_peek_at( list, list->len - (roffset + 1) ) : NULL;
}

/**
 * Peeks at the data at the head of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @return Returns the data from the node at the head of \a list or null if \a
 * list is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_peek_at()
 * @sa slist_peek_tail()
 */
C_SLIST_INLINE PJL_WARN_UNUSED_RESULT
void* slist_peek_head( slist_t const *list ) {
  return list->head != NULL ? list->head->data : NULL;
}

/**
 * Peeks at the data at the tail of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @return Returns the data from the node at the tail of \a list or null if \a
 * list is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_peek_atr()
 * @sa slist_peek_head()
 */
C_SLIST_INLINE PJL_WARN_UNUSED_RESULT
void* slist_peek_tail( slist_t const *list ) {
  return list->tail != NULL ? list->tail->data : NULL;
}

/**
 * Pops data from the head of \a list.
 *
 * @param list The pointer to the <code>\ref slist</code>.
 * @return Returns the data from the head of \a list.  The caller is
 * responsible for deleting it (if necessary).
 *
 * @note This is an O(1) operation.
 */
PJL_WARN_UNUSED_RESULT
void* slist_pop_head( slist_t *list );

/**
 * Pushes a node onto the head of \a list.
 *
 * @param list A pointer to the <code>\ref slist</code>.
 * @param data The pointer to the data to add.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_list_head()
 * @sa slist_push_tail()
 */
void slist_push_head( slist_t *list, void *data );

/**
 * Pushes \a src onto the head of \a dst.
 *
 * @param dst The <code>\ref slist</code> to push onto.
 * @param src The <code>\ref slist</code> to push.  It is made empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_head()
 * @sa slist_push_list_tail()
 */
void slist_push_list_head( slist_t *dst, slist_t *src );

/**
 * Pushes \a src onto the tail of \a dst.
 *
 * @param dst The <code>\ref slist</code> to push onto.
 * @param src The <code>\ref slist</code> to push.  It is made empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_list_head()
 * @sa slist_push_tail()
 */
void slist_push_list_tail( slist_t *dst, slist_t *src );

/**
 * Appends \a data onto the tail of \a list.
 *
 * @param list The <code>\ref slist</code> to push onto.
 * @param data The data to pushed.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_head()
 * @sa slist_push_list_tail()
 */
void slist_push_tail( slist_t *list, void *data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_slist_H */
/* vim:set et sw=2 ts=2: */
