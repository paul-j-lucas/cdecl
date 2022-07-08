/*
**      cdecl -- C gibberish translator
**      src/red_black.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas, et al.
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

/*
 * Adapted from the code:
 * <https://opensource.apple.com/source/sudo/sudo-46/src/redblack.h>
 */

/*
 * Copyright (c) 2004, 2007 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Sudo: redblack.h,v 1.4 2008/11/09 14:13:12 millert Exp $
 */

#ifndef cdecl_red_black_H
#define cdecl_red_black_H

/**
 * @file
 * Declares types to represent a _Red-Black Tree_ as well as functions for
 * manipulating said trees.
 *
 * @sa [Red-Black Tree](https://en.wikipedia.org/wiki/Red-black_tree)
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "util.h"

/// @cond DOXYGEN_IGNORE

_GL_INLINE_HEADER_BEGIN
#ifndef RED_BLACK_H_INLINE
# define RED_BLACK_H_INLINE _GL_INLINE
#endif /* RED_BLACK_H_INLINE */

/// @endcond

/**
 * @defgroup red-black-group Red-Black Tree
 * Types and functions for manipulating red-black trees.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

typedef enum   rb_color     rb_color_t;
typedef struct rb_insert_rv rb_insert_rv_t;
typedef struct rb_node      rb_node_t;
typedef struct rb_tree      rb_tree_t;

/**
 * The signature for a function passed to rb_tree_init() used to compare node
 * data.
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the data pointed to by \a i_data is less than, equal to, or
 * greater than the data pointed to by \a j_data.
 */
typedef int (*rb_cmp_fn_t)( void const *i_data, void const *j_data );

/**
 * The signature for a function passed to rb_tree_cleanup() used to free data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the data to free.
 */
typedef void (*rb_free_fn_t)( void *data );

/**
 * The signature for a function passed to rb_tree_visit().
 *
 * @param node_data A pointer to the node's data.
 * @param v_data Optional data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
typedef bool (*rb_visit_fn_t)( void *node_data, void *v_data );

/**
 * Red-black tree colors.
 */
enum rb_color {
  RB_BLACK,                             ///< Black.
  RB_RED                                ///< Red.
};

/**
 * A red-black tree node.
 *
 * @warning Only rb_node.data may be accessed by client code, and even then as
 * read-only.  All other fields are for internal use only.
 */
struct rb_node {
  /**
   * User data.
   *
   * @warning The data pointed to _must not_ be modified if that would change
   * the node's position within the tree according to the tree's \ref
   * rb_cmp_fn_t function.  For example, if `data` points to a `struct` like:
   * @code
   *  struct word_count {
   *      char     *word;
   *      unsigned  count;
   *  };
   * @endcode
   * then, assuming the tree's \ref rb_cmp_fn_t function compares only `word`,
   * client code may then only safely modify `count`.
   */
  void       *data;

  rb_node_t  *child[2];                 ///< Left/right (internal use only).
  rb_node_t  *parent;                   ///< Parent (internal use only).
  rb_color_t  color;                    ///< Node color (internal use only).
};

/**
 * A red-black tree.
 *
 * @sa rb_tree_init()
 */
struct rb_tree {
  rb_node_t   root;                     ///< Root node.
  rb_node_t   nil;                      ///< Nil node.
  rb_cmp_fn_t cmp_fn;                   ///< Data comparison function.
};

/**
 * The return value of rb_tree_insert().
 */
struct rb_insert_rv {
  rb_node_t  *node;                     ///< The node either found or inserted.
  bool        inserted;                 ///< Was \a node inserted?
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans-up all memory associated with \a tree but does _not_ free \a tree
 * itself.
 *
 * @param tree The red-black tree to clean up.  If NULL, does nothing;
 * otherwise, reinitializes \a tree upon completion.
 * @param free_fn A pointer to a function used to free data associated with
 * each node or NULL if unnecessary.
 *
 * @sa rb_tree_init()
 */
void rb_tree_cleanup( rb_tree_t *tree, rb_free_fn_t free_fn );

/**
 * Deletes \a node from \a tree.
 *
 * @param tree A pointer to the red-black tree to delete \a node from.
 * @param node A pointer to the node to delete.
 * @return Returns a pointer to the data of \a node.  It is the caller's
 * responsibility to delete said data if necessary.
 *
 * @sa rb_tree_insert()
 */
PJL_WARN_UNUSED_RESULT
void* rb_tree_delete( rb_tree_t *tree, rb_node_t *node );

/**
 * Gets whether \a tree is empry.
 *
 * @param tree A pointer to the red-black tree to check.
 * @return Returns `true` only if \a tree is empty.
 */
RED_BLACK_H_INLINE PJL_WARN_UNUSED_RESULT
bool rb_tree_empty( rb_tree_t const *tree ) {
  return tree->root.child[0] == &tree->nil && tree->root.child[1] == &tree->nil;
}

/**
 * Attempts to find \a data in \a tree.
 *
 * @param tree A pointer to the red-black tree to search through.
 * @param data A pointer to the data to search for.
 * @return Returns a pointer to the node containing \a data or NULL if not
 * found.
 *
 * @warning Even though this function returns a pointer to a non-`const` node,
 * the node's data _must not_ be modified if that would change the node's
 * position within the tree according to the tree's data comparison function.
 */
PJL_WARN_UNUSED_RESULT
rb_node_t* rb_tree_find( rb_tree_t const *tree, void const *data );

/**
 * Gets the first node in \a tree, if any.
 *
 * @param tree A pointer to the red-black tree to get the first node of.
 * @return Returns a pointer to the first node in \a tree if not empty or NULL
 * if empty.
 *
 * @warning Even though this function returns a pointer to a non-`const` node,
 * the node's data _must not_ be modified if that would change the node's
 * position within the tree according to the tree's data comparison function.
 */
RED_BLACK_H_INLINE PJL_WARN_UNUSED_RESULT
rb_node_t* rb_tree_first( rb_tree_t const *tree ) {
  rb_node_t *const first = tree->root.child[0];
  return first == &tree->nil ? NULL : first;
}

/**
 * Initializes a red-black tree.
 *
 * @param tree The red-black tree to initialize.
 * @param cmp_fn A pointer to a function used to compare data between nodes.
 *
 * @sa rb_tree_cleanup()
 */
void rb_tree_init( rb_tree_t *tree, rb_cmp_fn_t cmp_fn );

/**
 * Inserts \a data into \a tree.
 *
 * @param tree A pointer to the red-black tree to insert into.
 * @param data A pointer to the data to insert.
 * @return Returns an \ref rb_insert_rv where its node points to either the
 * newly inserted node or the existing node having the same \a data and \a
 * inserted is `true` only if \a data was inserted.
 *
 * @warning Even though this function returns a pointer to a non-`const` node,
 * the node's data _must not_ be modified if that would change the node's
 * position within the tree according to the tree's data comparison function.
 *
 * @sa rb_tree_delete()
 */
PJL_WARN_UNUSED_RESULT
rb_insert_rv_t rb_tree_insert( rb_tree_t *tree, void *data );

/**
 * Performs an in-order traversal of \a tree.
 *
 * @param tree A pointer to the red-black tree to visit.
 * @param visit_fn The visitor function to use.
 * @param v_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the node at which visiting stopped or NULL if
 * the entire tree was visited.
 *
 * @warning Even though this function returns a pointer to a non-`const` node,
 * the node's data _must not_ be modified if that would change the node's
 * position within the tree according to the tree's data comparison function.
 */
PJL_WARN_UNUSED_RESULT
rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visit_fn_t visit_fn,
                          void *v_data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_red_black_H */
/* vim:set et sw=2 ts=2: */
