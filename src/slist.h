/*
**      cdecl -- C gibberish translator
**      src/slist.h
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

#ifndef cdecl_slist_H
#define cdecl_slist_H

/**
 * @file
 * Declares a singly-linked-list data structure and functions to manipulate it.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <unistd.h>                     /* for ssize_t */

_GL_INLINE_HEADER_BEGIN
#ifndef SLIST_H_INLINE
# define SLIST_H_INLINE _GL_INLINE
#endif /* SLIST_H_INLINE */

/// @endcond

/**
 * @defgroup slist-group Singly-Linked List
 * Macros, types, and functions for manipulating singly-linked lists.
 * @{
 */

/**
 * Convenience macro for iterating over all the nodes of \a SLIST.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param SLIST A pointer to the \ref slist to iterate over.
 *
 * @sa #FOREACH_SLIST_NODE_UNTIL()
 */
#define FOREACH_SLIST_NODE(VAR,SLIST) \
  FOREACH_SLIST_NODE_UNTIL( VAR, (SLIST), /*END=*/NULL )

/**
 * Convenience macro for iterating over the nodes of \a SLIST up to but not
 * including \a END.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param SLIST A pointer to the \ref slist to iterate over.
 * @param END A pointer to the node to end before; may be NULL.
 *
 * @sa #FOREACH_SLIST_NODE()
 */
#define FOREACH_SLIST_NODE_UNTIL(VAR,SLIST,END) \
  for ( slist_node_t *VAR = CONST_CAST( slist_t*, (SLIST) )->head; VAR != (END); VAR = VAR->next )

/**
 * Creates a `const` \ref slist variable on the stack with \a NODE_DATA as its
 * only element.
 *
 * @param VAR The name for the \ref slist variable.
 * @param NODE_DATA A pointer to the node data.
 */
#define SLIST_VAR_INIT(VAR,NODE_DATA) \
  slist_node_t UNIQUE_NAME(VAR##_node) = { NULL, CONST_CAST(void*, (NODE_DATA)) }; \
  slist_t const VAR = { &UNIQUE_NAME(VAR##_node), &UNIQUE_NAME(VAR##_node), 1 }

///////////////////////////////////////////////////////////////////////////////

/**
 * The signature for a function passed to slist_cmp() used to compare data
 * associated with each node (if necessary).
 *
 * @param i_data A pointer to the first node's data to compare.
 * @param j_data A pointer to the second node's data to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
typedef int (*slist_cmp_fn_t)( void const *i_data, void const *j_data );

/**
 * The signature for a function passed to slist_dup() used to duplicate data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the node's data to duplicate.
 * @return Returns a duplicate of \a data.
 */
typedef void* (*slist_dup_fn_t)( void const *data );

/**
 * The signature for a function passed to slist_cleanup() used to free data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the node's data to free.
 */
typedef void (*slist_free_fn_t)( void *data );

/**
 * The signature for a function passed to slist_free_if() used to determine
 * whether a node should be freed.
 *
 * @param node A pointer to the node being visited.
 * @param user_data Optional data passed to slist_free_if().
 * @return Returns `true` only if the node should be freed.
 */
typedef bool (*slist_pred_fn_t)( slist_node_t *node, user_data_t user_data );

///////////////////////////////////////////////////////////////////////////////

/**
 * Singly-linked list.
 */
struct slist {
  slist_node_t *head;                   ///< Pointer to list head.
  slist_node_t *tail;                   ///< Pointer to list tail.
  size_t        len;                    ///< Length of list.
};

/**
 * Singly-linked-list node.
 */
struct slist_node {
  slist_node_t *next;                   ///< Pointer to next node or NULL.
  void         *data;                   ///< Pointer to node's data.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Peeks at the node's data at \a offset of \a list.
 *
 * @param list A pointer to the \ref slist.
 * @param offset The offset (starting at 0) of the node's data to get.
 * @return Returns the node's data at \a offset.
 *
 * @warning \a offset is _not_ checked to ensure it's &lt; the list's length.
 * A value &ge; the list's length results in undefined behavior.
 *
 * @note This function isn't normally called directly; use either slist_at() or
 * slist_atr() instead.
 * @note This is an O(n) operation.
 *
 * @sa slist_at()
 * @sa slist_atr()
 * @sa slist_back()
 * @sa slist_front()
 */
NODISCARD
void* slist_at_nocheck( slist_t const *list, size_t offset );

/**
 * Cleans-up all memory associated with \a list but _not_ \a list itself.
 *
 * @param list A pointer to the list to clean up.  If NULL, does nothing;
 * otherwise, reinitializes \a list upon completion.
 * @param free_fn A pointer to a function to use to free the data at each node
 * of \a list or NULL if none is required.
 *
 * @sa slist_free_if()
 * @sa slist_init()
 */
void slist_cleanup( slist_t *list, slist_free_fn_t free_fn );

/**
 * Compares two lists.
 *
 * @param i_list The first list.
 * @param j_list The second list.
 * @param cmp_fn A pointer to a function to use to compare data at each node of
 * \a i_list and \a j_list or NULL if none is required (hence the data will be
 * compared directly as signed integers).
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_list is
 * less than, equal to, or greater than \a j_list, respectively.
 */
NODISCARD
int slist_cmp( slist_t const *i_list, slist_t const *j_list,
               slist_cmp_fn_t cmp_fn );

/**
 * Duplicates \a src_list and all of its nodes.
 *
 * @param src_list The \ref slist to duplicate; may ne NULL.
 * @param n The number of nodes to duplicate; -1 is equivalent to slist_len().
 * @param dup_fn A pointer to a function to use to duplicate the data at each
 * node of \a src_list or NULL if none is required (hence a shallow copy will
 * be done).
 * @return Returns a duplicate of \a src_list or an empty list only if \a
 * src_list is NULL.  The caller is responsible for calling slist_cleanup() on
 * it.
 *
 * @sa slist_cleanup()
 */
NODISCARD
slist_t slist_dup( slist_t const *src_list, ssize_t n, slist_dup_fn_t dup_fn );

/**
 * Frees select nodes from \a list for which \a pred_fn returns `true`.
 *
 * @param list A pointer to the list to possibly free nodes from.
 * @param pred_fn The predicate function to use.
 * @param user_data Optional data passed to \a pred_fn.
 * @return Returns `true` only if any nodes were freed.
 *
 * @note This function _only_ frees matching nodes from \a list and _not_ the
 * data at each node.  If the data at each node needs to be freed, \a pred_fn
 * can do that before returning `true`.
 * @note This is an O(n) operation.
 *
 * @sa slist_cleanup()
 */
PJL_DISCARD
bool slist_free_if( slist_t *list, slist_pred_fn_t pred_fn,
                    user_data_t user_data );

/**
 * Pops data from the back of \a list.
 *
 * @param list The pointer to the \ref slist.
 * @return Returns the node's data from the back of \a list.  The caller is
 * responsible for freeing it if necessary.
 *
 * @note This is an O(n) operation.
 *
 * @sa slist_pop_front()
 */
NODISCARD
void* slist_pop_back( slist_t *list );

/**
 * Pops data from the front of \a list.
 *
 * @param list The pointer to the \ref slist.
 * @return Returns the node's data from the front of \a list.  The caller is
 * responsible for freeing it if necessary.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_pop_back()
 */
NODISCARD
void* slist_pop_front( slist_t *list );

/**
 * Appends \a data onto the back of \a list.
 *
 * @param list The \ref slist to push onto.
 * @param data The data to pushed.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_front()
 * @sa slist_push_list_back()
 */
void slist_push_back( slist_t *list, void *data );

/**
 * Pushes a node onto the front of \a list.
 *
 * @param list A pointer to the \ref slist.
 * @param data The pointer to the data to add.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_back()
 * @sa slist_push_list_front()
 */
void slist_push_front( slist_t *list, void *data );

/**
 * Pushes \a src_list onto the back of \a dst_list.
 *
 * @param dst_list The \ref slist to push onto.
 * @param src_list The \ref slist to push.  It is made empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_back()
 * @sa slist_push_list_front()
 */
void slist_push_list_back( slist_t *dst_list, slist_t *src_list );

/**
 * Pushes \a src_list onto the front of \a dst_list.
 *
 * @param dst_list The \ref slist to push onto.
 * @param src_list The \ref slist to push.  It is made empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_push_front()
 * @sa slist_push_list_back()
 */
void slist_push_list_front( slist_t *dst_list, slist_t *src_list );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Peeks at the node's data at \a offset of \a list.
 *
 * @param list A pointer to the \ref slist.
 * @param offset The offset (starting at 0) of the node's data to get.
 * @return Returns the node's data at \a offset or NULL if \a offset &ge;
 * slist_len().
 *
 * @note This is an O(n) operation.
 *
 * @sa slist_at_nocheck()
 * @sa slist_atr()
 * @sa slist_back()
 * @sa slist_front()
 */
NODISCARD SLIST_H_INLINE
void* slist_at( slist_t const *list, size_t offset ) {
  return offset < list->len ? slist_at_nocheck( list, offset ) : NULL;
}

/**
 * Peeks at the node's data at \a roffset from the back of \a list.
 *
 * @param list A pointer to the \ref slist.
 * @param roffset The reverse offset (starting at 0) of the node's data to get.
 * @return Returns the node's data at \a roffset or NULL if \a roffset &ge;
 * slist_len().
 *
 * @note This is an O(n) operation.
 *
 * @sa slist_at_nocheck()
 * @sa slist_at()
 * @sa slist_back()
 * @sa slist_front()
 */
NODISCARD SLIST_H_INLINE
void* slist_atr( slist_t const *list, size_t roffset ) {
  return roffset < list->len ?
    slist_at_nocheck( list, list->len - (roffset + 1) ) : NULL;
}

/**
 * Peeks at the node's data at the back of \a list.
 *
 * @param list A pointer to the \ref slist.
 * @return Returns the node's data at the back of \a list or NULL if \a list is
 * empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_at_nocheck()
 * @sa slist_at()
 * @sa slist_atr()
 * @sa slist_front()
 */
NODISCARD SLIST_H_INLINE
void* slist_back( slist_t const *list ) {
  return list->tail != NULL ? list->tail->data : NULL;
}

/**
 * Gets whether \a list is empty.
 *
 * @param list A pointer to the \ref slist to check.
 * @return Returns `true` only if \a list is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_len()
 */
NODISCARD SLIST_H_INLINE
bool slist_empty( slist_t const *list ) {
  return list->head == NULL;
}

/**
 * Peeks at the node's data at the front of \a list.
 *
 * @param list A pointer to the \ref slist.
 * @return Returns the node's data at the front of \a list or NULL if \a list
 * is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_at_nocheck()
 * @sa slist_at()
 * @sa slist_atr()
 * @sa slist_back()
 */
NODISCARD SLIST_H_INLINE
void* slist_front( slist_t const *list ) {
  return list->head != NULL ? list->head->data : NULL;
}

/**
 * Initializes \a list.
 *
 * @param list A pointer to the \ref slist to initialize.
 *
 * @note This need not be called for either global or `static` lists.
 *
 * @sa slist_cleanup()
 * @sa slist_move()
 */
SLIST_H_INLINE
void slist_init( slist_t *list ) {
  *list = (slist_t){ 0 };
}

/**
 * Gets the length of \a list.
 *
 * @param list A pointer to the \ref slist to get the length of.
 * @return Returns said length.
 *
 * @note This is an O(1) operation.
 *
 * @sa slist_empty()
 */
NODISCARD SLIST_H_INLINE
size_t slist_len( slist_t const *list ) {
  return list->len;
}

/**
 * Reinitializes \a list and returns its former value so that it can be "moved"
 * into another list via assignment.  For example:
 * ```
 *  slist_t new_list = slist_move( &old_list );
 * ```
 * is equivalent to:
 * ```
 *  slist_t new_list = old_list;
 *  slist_init( &old_list );
 * ```
 *
 * @remarks In many cases, a simple assignment would be fine; however, if
 * there's code that modifies `old_list` afterwards, it would interfere with
 * `new_list` since both point to the same underlying elements.
 *
 * @param list The \ref slist to move.  May be NULL.
 * @return Returns the former value of \a list or an empty list if \a list is
 * NULL.
 *
 * @sa slist_init()
 */
NODISCARD
slist_t slist_move( slist_t *list );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_slist_H */
/* vim:set et sw=2 ts=2: */
