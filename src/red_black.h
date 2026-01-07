/*
**      PJL Library
**      src/red_black.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas, et al.
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

#ifndef pjl_red_black_H
#define pjl_red_black_H

/**
 * @file
 * Declares types to represent a _Red-Black Tree_ as well as functions for
 * manipulating said trees.
 *
 * @sa [Red-Black Tree](https://en.wikipedia.org/wiki/Red-black_tree)
 */

// local
#include "pjl_config.h"                 /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>                     /* for max_align_t */

#ifndef NODISCARD
# define NODISCARD                      /* nothing */
#endif /* NODISCARD */
#ifndef PJL_DISCARD
# define PJL_DISCARD                    /* nothing */
#endif /* PJL_DISCARD */

/// @endcond

/**
 * @defgroup red-black-group Red-Black Tree
 * Types for defining and functions for manipulating red-black trees.
 *
 * @sa [Red-Black Tree](https://en.wikipedia.org/wiki/Red-black_tree)
 *
 * @{
 */

////////// macros /////////////////////////////////////////////////////////////

/**
 * Gets a pointer to the internal data of \a NODE for when \ref
 * rb_dloc::RB_DINT "RB_DINT" was used with rb_tree_init().
 *
 * @param NODE The rb_node to get a pointer to the data of.
 * @return Returns a pointer to the data internal to \a NODE.
 *
 * @note As a function-like macro, the preprocessor will not expand it if not
 * followed by `(` in which case it will be the \ref rb_dloc enumeration value.
 *
 * @warning The node's \ref rb_node::data "data" _must not_ be modified if that
 * would change the node's position within the tree according to its \ref
 * rb_tree::cmp_fn "cmp_fn".
 *
 * @sa #RB_DPTR()
 */
#define RB_DINT(NODE)             ( (void*)(NODE)->data )

/**
 * Gets an lvalue reference to a pointer to the external data of \a NODE for
 * when \ref rb_dloc::RB_DPTR "RB_DPTR" was used with rb_tree_init(). As an
 * lvalue reference, `RB_DPTR` can appear on the left-hand side of an `=` and
 * be assigned to.
 *
 * @param NODE The rb_node to get a pointer to the data of.
 * @return Returns a pointer to the data \a NODE points to.
 *
 * @note As a function-like macro, the preprocessor will not expand it if not
 * followed by `(` in which case it will be the \ref rb_dloc enumeration value.
 *
 * @warning The node's \ref rb_node::data "data" _must not_ be modified if that
 * would change the node's position within the tree according to its \ref
 * rb_tree::cmp_fn "cmp_fn".
 *
 * @sa #RB_DINT()
 */
#define RB_DPTR(NODE)             ( *(void**)RB_DINT( (NODE) ) )

////////// enumerations ///////////////////////////////////////////////////////

/**
 * Red-black tree colors.
 */
enum rb_color {
  RB_BLACK,                             ///< Black.
  RB_RED                                ///< Red.
};

/**
 * Red-black tree node data location.
 */
enum rb_dloc {
  /**
   * Nodes contain data internally.  The advantages are:
   *
   *  + No separate call to `malloc()` is needed to allocate the data and no
   *    separate call to `free()` is needed to deallocate it.
   *  + The data can be accessed faster since it's already been loaded with the
   *    node (cache hit).
   *
   * The disadvantages are:
   *
   *  - Inserting data into a tree requires copying it into a node.
   *  - If you want to delete a node from the tree but keep its data, you have
   *    to copy it out first.
   */
  RB_DINT,

  /**
   * Nodes contain a pointer to the data.  The advantages are:
   *
   *  + Inserting data into a tree requires copying only a pointer into a node.
   *  + Hence, the lifetime of data is separate from the tree.
   *
   * The disadvantages are:
   *
   *  - A separate call to `malloc()` is needed to allocate the data and a
   *    separate call to `free()` is needed to deallocate it.
   *  - Accessing the data is slower since it's in a different memory location
   *    than the node (cache miss).
   */
  RB_DPTR
};

////////// typedefs ///////////////////////////////////////////////////////////

typedef enum   rb_color     rb_color_t;
typedef enum   rb_dloc      rb_dloc_t;
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
 * @param node_data A pointer to the rb_node's data.
 * @param visit_data Optional data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
typedef bool (*rb_visit_fn_t)( void *node_data, void *visit_data );

////////// structs ////////////////////////////////////////////////////////////

/**
 * A red-black tree node.
 *
 * @warning Only \ref data may be accessed by client code.  All other fields
 * are for internal use only.
 */
struct rb_node {
  rb_node_t  *child[2];                 ///< Left/right (internal use only).
  rb_node_t  *parent;                   ///< Parent (internal use only).
  rb_color_t  color;                    ///< Node color (internal use only).

  /**
   * User data.
   *
   * @warning The data _must not_ be modified if that would change the node's
   * position within the tree according to the tree's \ref rb_tree::cmp_fn
   * "cmp_fn".  For example, if `data` is a `struct` like:
   *
   *      struct word_count {
   *          char     *word;
   *          unsigned  count;
   *      };
   *
   * then, assuming the tree's \ref rb_tree::cmp_fn "cmp_fn" compares only
   * `word`, client code may then only safely modify `count`.
   */
  alignas( max_align_t ) char data[];
};

/**
 * A red-black tree.
 *
 * @sa rb_tree_init()
 * @sa [Red-Black Tree](https://en.wikipedia.org/wiki/Red-black_tree)
 */
struct rb_tree {
  /**
   * Root node of the tree or points to \ref nil if the tree is empty.
   */
  rb_node_t  *root;

  /**
   * Data comparison function.
   *
   * @warning This value may be changed _only_ when the tree is empty.
   */
  rb_cmp_fn_t cmp_fn;

  /**
   * Node data location.
   */
  rb_dloc_t   dloc;

  /**
   * A convenience sentinel for all leaf nodes.
   *
   * @remarks
   * @parblock
   * Its only invariant is that its \ref rb_node::color "color" _must_ be
   * #RB_BLACK.  Its children and parent can take on arbitrary values.
   *
   * There is one nil per tree instead of a single static nil for all trees
   * because those values can change.  In a multithreaded program, updates to
   * different trees could affect such a single nil that would result in
   * undefined behavior.
   * @endparblock
   */
  rb_node_t   nil;
};

/**
 * The return value of rb_tree_insert().
 */
struct rb_insert_rv {
  /**
   * The \ref rb_node "node" either found or inserted.  Use \ref inserted to
   * know which.
   *
   * @warning Even though this is a pointer to a non-`const` \ref rb_node, the
   * node's \ref rb_node::data "data" _must not_ be modified if that would
   * change the node's position within the tree according to its \ref
   * rb_tree::cmp_fn "cmp_fn".
   */
  rb_node_t *node;

  /**
   * If `true`, \ref node refers to the newly inserted node; if `false`, \ref
   * node refers to the existing node having the same \ref rb_node::data
   * "data" according to the tree's \ref rb_tree::cmp_fn "cmp_fn".
   */
  bool inserted;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets a pointer to \a node's data.
 *
 * @param tree A pointer to the rb_tree of \a node.
 * @param node The rb_node to get the data of.
 * @return Returns said data.
 *
 * @note Normally, either #RB_DINT or #RB_DPTR is used to get a pointer to a
 * node's data.  This function would only be used in code that should work with
 * a tree using either data location.
 *
 * @sa #RB_DINT
 * @sa #RB_DPTR
 */
NODISCARD
inline void* rb_node_data( rb_tree_t const *tree, rb_node_t const *node ) {
  return tree->dloc == RB_DINT ? RB_DINT( node ) : RB_DPTR( node );
}

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
 * @param tree A pointer to the rb_tree to delete \a node from.
 * @param node A pointer to the rb_node to delete.
 *
 * @sa rb_tree_insert()
 */
void rb_tree_delete( rb_tree_t *tree, rb_node_t *node );

/**
 * Gets whether \a tree is empty.
 *
 * @param tree A pointer to the rb_tree to check.
 * @return Returns `true` only if \a tree is empty.
 */
NODISCARD
inline bool rb_tree_empty( rb_tree_t const *tree ) {
  return tree->root == &tree->nil;
}

/**
 * Attempts to find \a data in \a tree.
 *
 * @param tree A pointer to the rb_tree to search through.
 * @param data A pointer to the data to search for.
 * @return Returns a pointer to the rb_node containing \a data or NULL if not
 * found.
 *
 * @warning Even though this function returns a pointer to a non-`const` \ref
 * rb_node, the node's \ref rb_node::data "data" _must not_ be modified if that
 * would change the node's position within the tree according to its \ref
 * rb_tree::cmp_fn "cmp_fn".
 */
NODISCARD
rb_node_t* rb_tree_find( rb_tree_t const *tree, void const *data );

/**
 * Initializes a red-black tree.
 *
 * @param tree The red-black tree to initialize.
 * @param dloc Where data for each node is stored.
 * @param cmp_fn A pointer to a function used to compare data between nodes.
 *
 * @sa rb_tree_cleanup()
 */
void rb_tree_init( rb_tree_t *tree, rb_dloc_t dloc, rb_cmp_fn_t cmp_fn );

/**
 * Inserts \a data into \a tree.
 *
 * @param tree A pointer to the rb_tree to insert into.
 * @param data A pointer to the data to insert.
 * @param data_size If \a tree's \ref rb_tree::dloc "dloc" is:
 *  + #RB_DINT: The size of \a data.  If a node is inserted, then this number
 *    of bytes are copied from \a data into the new node's \ref rb_node::data
 *    "data".
 *  + #RB_DPTR: Not used.  If a node is inserted, then the pointer value of \a
 *    data itself is copied into the new node's \ref rb_node::data "data".
 *
 * @return Returns an \ref rb_insert_rv where its \ref rb_insert_rv::node
 * "node" points to either the newly inserted node or the existing node having
 * the same \ref rb_node::data "data" and \ref rb_insert_rv::inserted
 * "inserted" is `true` only if \ref rb_node::data "data" was inserted.
 *
 * @warning Even though this function returns an \ref rb_insert_rv containing a
 * pointer to a non-`const` \ref rb_insert_rv::node "node", the node's \ref
 * rb_node::data "data" _must not_ be modified if that would change the node's
 * position within the tree according its \ref rb_tree::cmp_fn "cmp_fn".
 *
 * @sa rb_tree_delete()
 */
NODISCARD
rb_insert_rv_t rb_tree_insert( rb_tree_t *tree, void *data, size_t data_size );

/**
 * Performs an in-order traversal of \a tree.
 *
 * @param tree A pointer to the rb_tree to visit.
 * @param visit_fn The visitor function to use.
 * @param visit_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the rb_node at which visiting stopped or NULL
 * if the entire tree was visited.
 *
 * @warning Even though this function returns a pointer to a non-`const` \ref
 * rb_node, the node's \ref rb_node::data "data" _must not_ be modified if that
 * would change the node's position within the tree according to its \ref
 * rb_tree::cmp_fn "cmp_fn".
 */
PJL_DISCARD
rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visit_fn_t visit_fn,
                          void *visit_data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* pjl_red_black_H */
/* vim:set et sw=2 ts=2: */
