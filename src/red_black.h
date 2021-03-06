/*
**      cdecl -- C gibberish translator
**      src/red_black.h
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
 * Declares types to represent a Red-Black Tree as well as functions for
 * manipulating said trees.
 *
 * @sa [Red-black tree](https://en.wikipedia.org/wiki/Red-black_tree)
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "util.h"

/**
 * @defgroup red-black-group Red-Black Tree
 * Types and functions for manipulating red-black trees.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

typedef struct rb_node  rb_node_t;
typedef struct rb_tree  rb_tree_t;
typedef enum   rb_color rb_color_t;

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
typedef int (*rb_data_cmp_t)( void const *i_data, void const *j_data );

/**
 * The signature for a function passed to rb_tree_free() used to free data
 * associated with each node (if necessary).
 *
 * @param data A pointer to the data to free.
 */
typedef void (*rb_data_free_t)( void *data );

/**
 * The signature for the function passed to rb_tree_visit().
 *
 * @param node_data A pointer to the node's data.
 * @param aux_data Optional data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
typedef bool (*rb_visitor_t)( void *node_data, void *aux_data );

/**
 * Red-black tree colors.
 */
enum rb_color {
  RB_BLACK,
  RB_RED
};

/**
 * A red-black tree node.
 */
struct rb_node {
  void       *data;                     ///< User data.
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
  rb_node_t     root;                   ///< Root node.
  rb_data_cmp_t data_cmp_fn;            ///< Data comparison function.
};

////////// extern functions ///////////////////////////////////////////////////

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
 * Attempts to find \a data in \a tree.
 *
 * @param tree A pointer to the red-black tree to search through.
 * @param data A pointer to the data to search for.
 * @return Returns a pointer to the node containing \a data or NULL if not
 * found.
 */
PJL_WARN_UNUSED_RESULT
rb_node_t* rb_tree_find( rb_tree_t *tree, void const *data );

/**
 * Frees all memory associated with \a tree but _not_ \a tree itself.
 *
 * @param tree The red-black tree to free.  If NULL, does nothing; otherwise,
 * reinitializes \a tree upon completion.
 * @param data_free_fn A pointer to a function used to free data associated
 * with each node or NULL if unnecessary.
 *
 * @sa rb_tree_init()
 */
void rb_tree_free( rb_tree_t *tree, rb_data_free_t data_free_fn );

/**
 * Initializes a red-black tree.
 *
 * @param tree The red-black tree to initialize.
 * @param data_cmp_fn A pointer to a function used to compare data between
 * nodes.
 *
 * @sa rb_tree_free()
 */
void rb_tree_init( rb_tree_t *tree, rb_data_cmp_t data_cmp_fn );

/**
 * Inserts \a data into \a tree.
 *
 * @param tree A pointer to the red-black tree to insert into.
 * @param data A pointer to the data to insert.
 * @return Returns NULL if \a data is inserted or a pointer to a node if \a
 * data already exists.
 *
 * @sa rb_tree_delete()
 */
PJL_WARN_UNUSED_RESULT
rb_node_t* rb_tree_insert( rb_tree_t *tree, void *data );

/**
 * Performs an in-order traversal of \a tree.
 *
 * @param tree A pointer to the red-black tree to visit.
 * @param visitor The visitor to use.
 * @param aux_data Optional data passed to \a visitor.
 * @return Returns a pointer to the node at which visiting stopped or NULL if
 * the entire tree was visited.
 */
PJL_WARN_UNUSED_RESULT
rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visitor_t visitor,
                          void *aux_data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_red_black_H */
/* vim:set et sw=2 ts=2: */
