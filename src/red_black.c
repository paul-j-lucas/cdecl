/*
**      PJL Library
**      src/red_black.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas, et al.
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
 * Defines functions for manipulating a _Red-Black Tree_.
 *
 * @sa [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/),
 * 4th ed., Thomas H. Cormen, Charles E.  Leiserson, Ronald L. Rivest, and
 * Clifford Stein, MIT Press, ISBN 9780262046305, &sect; 13.
 * @sa [Red-Black Tree](https://en.wikipedia.org/wiki/Red-black_tree)
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define RED_BLACK_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "red_black.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */
#include <stdlib.h>                     /* or free(3) */

/// @endcond

/**
 * @addtogroup red-black-group
 * @{
 */

/**
 * Gets an lvalue reference to the child node pointer of \a NODE's parent,
 * i.e., the parent's pointer to \a NODE.
 *
 * @param NODE A pointer to the rb_node to get said reference from.
 * @return Returns said lvalue reference.
 */
#define RB_PARENT_PTR_TO(NODE)    ((NODE)->parent->child[ child_dir( (NODE) ) ])

/**
 * Red-black tree child direction.
 */
enum rb_dir {
  RB_L,                                 ///< Left child direction.
  RB_R                                  ///< Right child direction.
};
typedef enum rb_dir rb_dir_t;

////////// inline functions ///////////////////////////////////////////////////

/**
 * Convenience function for checking that a node's color is #RB_BLACK.
 *
 * @param node A pointer to the rb_node to check.
 * @return Returns `true` only if \a node is #RB_BLACK.
 *
 * @sa is_red()
 */
NODISCARD
static inline bool is_black( rb_node_t const *node ) {
  return node->color == RB_BLACK;
}

/**
 * Convenience function for checking whether a node is the left or right child
 * of its parent.
 *
 * @param node A pointer to the rb_node to check.
 * @param dir The direction to check for.
 * @return Returns `true` only if \a node is the \a dir child of its parent.
 *
 * @sa child_dir()
 */
NODISCARD
static inline bool is_dir_child( rb_node_t const *node, rb_dir_t dir ) {
  return node == node->parent->child[dir];
}

/**
 * Gets the direction of the child that \a node is of its parent.
 *
 * @param node A pointer to the rb_node to check.
 * @return Returns the direction of the child that \a node is of its parent.
 *
 * @sa is_dir_child()
 */
NODISCARD
static inline rb_dir_t child_dir( rb_node_t const *node ) {
  return is_dir_child( node, RB_L ) ? RB_L : RB_R;
}

/**
 * Convenience function for checking that a node's color is #RB_RED.
 *
 * @param node A pointer to the rb_node to check.
 * @return Returns `true` only if \a node is #RB_RED.
 *
 * @sa is_black()
 */
NODISCARD
static inline bool is_red( rb_node_t const *node ) {
  return node->color == RB_RED;
}

/**
 * Compares the data between \a node's data and \a data.
 *
 * @param tree A pointer to the rb_tree of \a node.
 * @param node The rb_node whose data to compare.
 * @param data A pointer to the data to compare against.
 * @return Returns a number less than 0, 0, or greater than 0 if \a node's data
 * is less than, equal to, or greater than \a data, respectively.
 */
NODISCARD
static inline int rb_tree_cmp( rb_tree_t const *tree, rb_node_t *node,
                               void const *data ) {
  return (*tree->cmp_fn)( data, rb_node_data( tree, node ) );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Frees all memory associated with \a node _including_ \a node itself.
 *
 * @param tree A pointer to the rb_tree to free \a node from.
 * @param node A pointer to the rb_node to free.
 * @param free_fn A pointer to a function used to free data associated with \a
 * node or NULL if unnecessary.
 */
static void rb_node_free( rb_tree_t *tree, rb_node_t *node,
                          rb_free_fn_t free_fn ) {
  assert( node != NULL );

  if ( node != &tree->nil ) {
    rb_node_free( tree, node->child[RB_L], free_fn );
    rb_node_free( tree, node->child[RB_R], free_fn );
    if ( free_fn != NULL )
      (*free_fn)( rb_node_data( tree, node ) );
    free( node );
  }
}

/**
 * Rotates a subtree of \a tree rooted at \a node.
 *
 * @remarks
 * @parblock
 * For example, given the following ordered trees:
 *
 *        B                  D
 *       / \    left -->    / \
 *      A   D              B   E
 *         / \  <- right  / \
 *        C   E          A   C
 *
 *       (1)                (2)
 *
 * perform either rotation:
 *
 * 1. **B** is rotated left (and down) and **D** is rotated left (and up) to
 *    yield (2).
 * 2. **D** is rotated right (and down) and **B** is rotated right (and up) to
 *    yield (1).
 *
 * Note that in both cases, the order of the nodes is preserved.
 * @endparblock
 *
 * @param tree A pointer rb_tree to manipulate.
 * @param x_node A pointer to the rb_node to rotate.
 * @param dir The direction to rotate.
 *
 * @sa [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/), 4th ed., &sect; 13.2, p. 336.
 */
static void rb_node_rotate( rb_tree_t *tree, rb_node_t *x_node, rb_dir_t dir ) {
  assert( tree != NULL );
  assert( x_node != NULL );

  rb_node_t *const y_node = x_node->child[!dir];
  x_node->child[!dir] = y_node->child[dir];
  y_node->child[dir]->parent = x_node;
  y_node->parent = x_node->parent;

  if ( x_node->parent == &tree->nil )
    tree->root = y_node;
  else
    RB_PARENT_PTR_TO( x_node ) = y_node;

  y_node->child[dir] = x_node;
  x_node->parent = y_node;
}

#ifndef NDEBUG

#ifdef RB_CHECK_ALL_NODES
/**
 * Checks that a node's properties hold.
 *
 * @param tree A pointer rb_tree to check.
 * @param node A pointer to the rb_node to check.
 */
static void rb_node_check( rb_tree_t const *tree, rb_node_t const *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  if ( node == &tree->nil )
    return;

  if ( is_red( node ) ) {
    assert( is_black( node->child[RB_L] ) );
    assert( is_black( node->child[RB_R] ) );
  }

  rb_node_check( tree, node->child[RB_L] );
  rb_node_check( tree, node->child[RB_R] );
}
#else
# define rb_node_check(TREE,NODE) (void)0
#endif /* RB_CHECK_ALL_NODES */

/**
 * Checks that some properties of \a tree hold.
 *
 * @remarks
 * @parblock
 * From [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/),
 * 4th ed., &sect; 13.1, p. 331:
 *
 * > A red-black tree is a binary search tree that satisfies the following
 * > _red-black properties_:
 * >
 * >  1. Every node is either red or black.
 * >  2. The root is black.
 * >  3. Every leaf (NIL) is black.
 * >  4. If a node is red, then both its children are black.
 * >  5. For each node, all simple paths from the node to descendant leaves
 * >     contain the same number of black nodes.
 *
 * For this code, (1) can never not be true; we check (2) and (3) by default;
 * we check (4) only if <code>%RB_CHECK_ALL_NODES</code> is defined; we don't
 * check (5).
 * @endparblock
 *
 * @param tree A pointer rb_tree to check.
 */
static void rb_tree_check( rb_tree_t const *tree ) {
  assert( tree != NULL );
  assert( tree->root != NULL );
  assert( tree->root->color == RB_BLACK );
  assert( tree->nil.color == RB_BLACK );
  rb_node_check( tree, tree->root );
}

#else
# define rb_tree_check(TREE)      (void)0
#endif /* NDEBUG */

/**
 * Repairs a tree after a node has been deleted by rotating and repainting
 * colors to restore the properties inherent in red-black trees.
 *
 * @param tree A pointer to the rb_tree to repair.
 * @param x_node A pointer to the rb_node to start the repair at.
 *
 * @sa [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/),
 * 4th ed., &sect; 13.4, p. 351.
 */
static void rb_delete_fixup( rb_tree_t *tree, rb_node_t *x_node ) {
  assert( tree != NULL );
  assert( x_node != NULL );

  while ( x_node != tree->root && is_black( x_node ) ) {
    rb_dir_t const dir = child_dir( x_node );
    rb_node_t *w_sibling = x_node->parent->child[!dir];
    if ( is_red( w_sibling ) ) {
      w_sibling->color = RB_BLACK;
      x_node->parent->color = RB_RED;
      rb_node_rotate( tree, x_node->parent, dir );
      w_sibling = x_node->parent->child[!dir];
    }
    if ( is_black( w_sibling->child[RB_L] ) &&
         is_black( w_sibling->child[RB_R] ) ) {
      w_sibling->color = RB_RED;
      x_node = x_node->parent;
    }
    else {
      if ( is_black( w_sibling->child[!dir] ) ) {
        w_sibling->child[dir]->color = RB_BLACK;
        w_sibling->color = RB_RED;
        rb_node_rotate( tree, w_sibling, !dir );
        w_sibling = x_node->parent->child[!dir];
      }
      w_sibling->color = x_node->parent->color;
      x_node->parent->color = RB_BLACK;
      w_sibling->child[!dir]->color = RB_BLACK;
      rb_node_rotate( tree, x_node->parent, dir );
      x_node = tree->root;
    }
  } // while

  x_node->color = RB_BLACK;
}

/**
 * Repairs a tree after a node has been inserted by rotating and repainting
 * colors to restore the properties inherent in red-black trees.
 *
 * @param tree A pointer to the rb_tree to repair.
 * @param z_node A pointer to the rb_node to start the repair at.
 *
 * @sa [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/),
 * 4th ed., &sect; 13.3, p. 339.
 */
static void rb_insert_fixup( rb_tree_t *tree, rb_node_t *z_node ) {
  assert( tree != NULL );
  assert( z_node != NULL );
  //
  // If the parent node is black, we're all set; if it's red, we have the
  // following possible cases to deal with.  We iterate through the rest of the
  // tree to make sure none of the required properties is violated.
  //
  //  1. The uncle is red.  We repaint both the parent and uncle black and
  //     repaint the grandparent node red.
  //
  //  2. The uncle is black and the new node is the right child of its parent,
  //     and the parent in turn is the left child of its parent.  We do a left
  //     rotation to switch the roles of the parent and child, relying on
  //     further iterations to repair the old parent.
  //
  //  3. The uncle is black and the new node is the left child of its parent,
  //     and the parent in turn is the left child of its parent.  We switch the
  //     colors of the parent and grandparent and perform a right rotation
  //     around the grandparent.  This makes the former parent the parent of
  //     the new node and the former grandparent.
  //
  while ( is_red( z_node->parent ) ) {
    rb_dir_t const dir = child_dir( z_node->parent );
    rb_node_t *const y_uncle = z_node->parent->parent->child[!dir];
    if ( is_red( y_uncle ) ) {
      z_node->parent->color = RB_BLACK;
      z_node = z_node->parent->parent;
      z_node->color = RB_RED;
      y_uncle->color = RB_BLACK;
    }
    else {
      if ( is_dir_child( z_node, !dir ) ) {
        z_node = z_node->parent;
        rb_node_rotate( tree, z_node, dir );
      }
      z_node->parent->color = RB_BLACK;
      z_node->parent->parent->color = RB_RED;
      rb_node_rotate( tree, z_node->parent->parent, !dir );
    }
  } // while

  tree->root->color = RB_BLACK;         // root is always black
}

/**
 * Performs an in-order traversal of the red-black tree starting at \a node.
 *
 * @param tree A pointer to the rb_tree to visit.
 * @param node A pointer to the rb_node to start visiting at.
 * @param visit_fn The visitor function to use.
 * @param user_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the rb_node at which visiting stopped or NULL
 * if the entire sub-tree was visited.
 */
NODISCARD
static rb_node_t* rb_node_visit( rb_tree_t const *tree, rb_node_t *node,
                                 rb_visit_fn_t visit_fn, void *user_data ) {
  assert( tree != NULL );
  assert( node != NULL );

  while ( node != &tree->nil ) {
    rb_node_t *const stopped_node =
      rb_node_visit( tree, node->child[RB_L], visit_fn, user_data );
    if ( stopped_node != NULL )
      return stopped_node;
    if ( (*visit_fn)( rb_node_data( tree, node ), user_data ) )
      return node;
    node = node->child[RB_R];
  } // while

  return NULL;
}

/**
 * Replaces the subtree rooted at \a u_node by the subtree rooted at \a v_node.
 *
 * @param tree A pointer to the rb_tree to do the transplant in.
 * @param u_node The root of the subtree to be replaced.
 * @param v_node The root of the subtree to replace with.
 *
 * @sa [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/),
 * 4th ed., &sect; 13.4, p. 347.
 */
static void rb_transplant( rb_tree_t *tree, rb_node_t *u_node,
                           rb_node_t *v_node ) {
  assert( tree != NULL );
  assert( u_node != NULL );
  assert( v_node != NULL );

  if ( u_node->parent == &tree->nil )
    tree->root = v_node;
  else
    RB_PARENT_PTR_TO( u_node ) = v_node;
  v_node->parent = u_node->parent;
}

/**
 * Gets the node having the minimum element of the subtree rooted at \a x_node.
 *
 * @param tree The red-black tree.
 * @param x_node A pointer to a subtree of \a tree.
 * @return Returns said node.
 *
 * @sa [Introduction to Algorithms](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/),
 * 4th ed., &sect; 12.2, p. 318.
 */
NODISCARD
static rb_node_t* rb_tree_minimum( rb_tree_t *tree, rb_node_t *x_node ) {
  assert( tree != NULL );
  assert( x_node != NULL );

  while ( x_node->child[RB_L] != &tree->nil )
    x_node = x_node->child[RB_L];
  return x_node;
}

/**
 * Resets \a tree to empty.
 *
 * @param tree A pointer to the rb_tree to reset.
 *
 * @warning Unlike rb_tree_cleanup(), this function does _not_ free nodes of a
 * non-empty tree.  This function is to be used only on new (empty) trees or on
 * newly cleaned-up (now empty) trees.
 *
 * @sa rb_tree_cleanup()
 * @sa rb_tree_init()
 */
static void rb_tree_reset( rb_tree_t *tree ) {
  assert( tree != NULL );

  tree->root = &tree->nil;
  tree->cmp_fn = NULL;
  tree->nil = (rb_node_t){
    .child = { &tree->nil, &tree->nil },
    .parent = &tree->nil,
    .color = RB_BLACK
  };
}

////////// extern functions ///////////////////////////////////////////////////

void rb_tree_cleanup( rb_tree_t *tree, rb_free_fn_t free_fn ) {
  if ( tree != NULL ) {
    rb_node_free( tree, tree->root, free_fn );
    rb_tree_reset( tree );
  }
}

void rb_tree_delete( rb_tree_t *tree, rb_node_t *z_delete ) {
  assert( tree != NULL );
  assert( z_delete != NULL );
  assert( z_delete != &tree->nil );

  // See "Introduction to Algorithms," 4th ed., &sect; 13.4, p. 348.

  rb_node_t *x_node;
  rb_node_t *y_node = z_delete;
  rb_color_t y_original_color = y_node->color;

  if ( z_delete->child[RB_L] == &tree->nil ) {
    x_node = z_delete->child[RB_R];
    rb_transplant( tree, z_delete, z_delete->child[RB_R] );
  }
  else if ( z_delete->child[RB_R] == &tree->nil ) {
    x_node = z_delete->child[RB_L];
    rb_transplant( tree, z_delete, z_delete->child[RB_L] );
  }
  else {
    y_node = rb_tree_minimum( tree, z_delete->child[RB_R] );
    y_original_color = y_node->color;
    x_node = y_node->child[RB_R];
    if ( y_node != z_delete->child[RB_L] ) {
      rb_transplant( tree, y_node, y_node->child[RB_R] );
      y_node->child[RB_R] = z_delete->child[RB_R];
      y_node->child[RB_R]->parent = y_node;
    } else {
      x_node->parent = y_node;
    }
    rb_transplant( tree, z_delete, y_node );
    y_node->child[RB_L] = z_delete->child[RB_L];
    y_node->child[RB_L]->parent = y_node;
    y_node->color = z_delete->color;
  }

  if ( y_original_color == RB_BLACK )
    rb_delete_fixup( tree, x_node );
  rb_tree_check( tree );

  free( z_delete );
}

rb_node_t* rb_tree_find( rb_tree_t const *tree, void const *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  for ( rb_node_t *node = tree->root; node != &tree->nil; ) {
    int const cmp = rb_tree_cmp( tree, node, data );
    if ( cmp == 0 )
      return node;
    node = node->child[ cmp > 0 ];
  } // for
  return NULL;
}

void rb_tree_init( rb_tree_t *tree, rb_dloc_t dloc, rb_cmp_fn_t cmp_fn ) {
  assert( tree != NULL );
  assert( cmp_fn != NULL );
  rb_tree_reset( tree );
  tree->cmp_fn = cmp_fn;
  tree->dloc = dloc;
}

rb_insert_rv_t rb_tree_insert( rb_tree_t *tree, void *data, size_t data_size ) {
  assert( tree != NULL );
  assert( data != NULL );
  assert( tree->dloc == RB_DLOC_PTR || data_size > 0 );

  // See "Introduction to Algorithms," 4th ed., &sect; 13.3, p. 338.

  rb_node_t *x_node = tree->root;
  rb_node_t *y_parent = &tree->nil;

  //
  // Find either the existing node having the same data -OR- the parent for the
  // new node.
  //
  while ( x_node != &tree->nil ) {
    int const cmp = rb_tree_cmp( tree, x_node, data );
    if ( cmp == 0 )
      return (rb_insert_rv_t){ x_node, .inserted = false };
    y_parent = x_node;
    x_node = x_node->child[ cmp > 0 ];
  } // while

  if ( tree->dloc == RB_DLOC_PTR )
    data_size = sizeof( void* );
  rb_node_t *const z_new_node = malloc( sizeof( rb_node_t ) + data_size );
  *z_new_node = (rb_node_t){
    .child = { &tree->nil, &tree->nil },
    .parent = y_parent,
    .color = RB_RED                     // new nodes are always red
  };

  switch ( tree->dloc ) {
    case RB_DLOC_INT:
      memcpy( z_new_node->data, data, data_size );
      break;
    case RB_DLOC_PTR:
      RB_DPTR( z_new_node ) = data;
      break;
  } // switch

  if ( y_parent == &tree->nil ) {
    tree->root = z_new_node;            // tree was empty
  } else {
    // Determine which child of the parent the new node should be.
    rb_dir_t const dir =
      STATIC_CAST( rb_dir_t, rb_tree_cmp( tree, y_parent, data ) >= 0 );
    assert( y_parent->child[dir] == &tree->nil );
    y_parent->child[dir] = z_new_node;
  }

  rb_insert_fixup( tree, z_new_node );
  rb_tree_check( tree );

  return (rb_insert_rv_t){ z_new_node, .inserted = true };
}

rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visit_fn_t visit_fn,
                          void *user_data ) {
  assert( visit_fn != NULL );
  return rb_node_visit( tree, tree->root, visit_fn, user_data );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
