/*
**      cdecl -- C gibberish translator
**      src/red_black.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas, et al.
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
 * <https://opensource.apple.com/source/sudo/sudo-46/src/redblack.c>
 */

/*
 * Copyright (c) 2004-2005, 2007 Todd C. Miller <Todd.Miller@courtesan.com>
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
 */

/*
 * Adapted from the code written by Emin Martinian:
 * <http://web.mit.edu/~emin/www/source_code/red_black_tree/index.html>
 *
 * Copyright (c) 2001 Emin Martinian
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that neither the name of Emin
 * Martinian nor the names of any contributors are be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Defines functions for manipulating a _Red-Black Tree_.
 *
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
 * Gets an lvalue reference to the first node in \a TREE.
 *
 * @param TREE A pointer to the red-black tree to get the first node of.
 * @return Returns said lvalue reference.
 *
 * @note This is a macro instead of an inline function so it'll:
 * + Work with either a `const` or non-`const` \a TREE.
 * + Be an lvalue reference.
 */
#define RB_FIRST(TREE)            (RB_ROOT(TREE)->child[RB_L])

/**
 * Gets an lvalue reference to the root node of \a TREE.
 *
 * @param TREE A pointer to the red-black tree to get the root node of.
 * @return Returns said lvalue reference.
 *
 * @note This is a macro instead of an inline function so it'll work with
 * either a `const` or non-`const` \a TREE.
 */
#define RB_NIL(TREE)              (&(TREE)->nil)

/**
 * Gets an lvalue reference to the child node pointer of \a NODE's parent,
 * i.e., the parent's pointer to \a NODE.
 *
 * @param NODE A pointer to the node to get said reference from.
 * @return Returns said lvalue reference.
 */
#define RB_PARENT_CHILD(NODE)     ((NODE)->parent->child[ child_dir( NODE ) ])

/**
 * Gets an lvalue reference to the root node of \a TREE.
 *
 * @param TREE A pointer to the red-black tree to get the root node of.
 * @return Returns said lvalue reference.
 *
 * @note This is a macro instead of an inline function so it'll work with
 * either a `const` or non-`const` \a TREE.
 */
#define RB_ROOT(TREE)             (&(TREE)->fake_root)

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
 * Gets whether \a node is "full," that is neither child node is nil.
 *
 * @param tree A pointer to the red-black tree \a node is part of.
 * @param node A pointer to the rb_node to check.
 * @return Returns `true` only if \a node is full.
 */
NODISCARD
static inline bool is_node_full( rb_tree_t const *tree,
                                 rb_node_t const *node ) {
  return node->child[RB_L] != RB_NIL(tree) && node->child[RB_R] != RB_NIL(tree);
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

////////// local functions ////////////////////////////////////////////////////

/**
 * Frees all memory associated with \a node _including_ \a node itself.
 *
 * @param tree A pointer to the red-black tree to free \a node from.
 * @param node A pointer to the rb_node to free.
 * @param free_fn A pointer to a function used to free data associated with \a
 * node or NULL if unnecessary.
 */
static void rb_node_free( rb_tree_t *tree, rb_node_t *node,
                          rb_free_fn_t free_fn ) {
  assert( node != NULL );
  assert( node != RB_ROOT(tree) );

  if ( node != RB_NIL(tree) ) {
    rb_node_free( tree, node->child[RB_L], free_fn );
    rb_node_free( tree, node->child[RB_R], free_fn );
    if ( free_fn != NULL )
      (*free_fn)( node->data );
    free( node );
  }
}

/**
 * Gets the next node from \a node.
 *
 * @param tree A pointer to the red-black tree that \a node is part of.
 * @param node A pointer to the rb_node to get the next node from.
 * @return Returns said node.
 *
 * @sa "Introduction to Algorithms," 4th ed., section 12.2, p. 319.
 */
NODISCARD
static rb_node_t* rb_node_next( rb_tree_t *tree, rb_node_t *x_node ) {
  assert( tree != NULL );
  assert( x_node != NULL );

  rb_node_t *next = x_node->child[RB_R];

  if ( next != RB_NIL(tree) ) {
    // inline implementation ot TREE-MINIMUM(x), p. 318
    while ( next->child[RB_L] != RB_NIL(tree) )
      next = next->child[RB_L];
  } else {
    // find the lowest ancestor of x whose left child is an ancestor of x
    next = x_node->parent;
    while ( next != RB_NIL(tree) && x_node == next->child[RB_R] ) {
      x_node = next;
      next = next->parent;
    } // while
  }
  return next;
}

/**
 * Rotates a subtree of \a tree rooted at \a node.
 *
 * For example, given the following ordered tree, perform a left rotation on
 * node **N**:
 *
 *        N            T
 *       / \          / \
 *      M   T   =>   N   U
 *         / \      / \
 *        S   U    M   S
 *
 * **N** is rotated left (and down); **T** is rotated left (and up).  Note that
 * the order is preserved.  A right rotation is the mirror image.
 *
 * @param tree A pointer to the red-black tree to manipulate.
 * @param node A pointer to the rb_node to rotate.
 * @param dir The direction to rotate.
 *
 * @sa "Introduction to Algorithms," 4th ed., section 13.2, p. 336.
 */
static void rb_node_rotate( rb_tree_t *tree, rb_node_t *x_node, rb_dir_t dir ) {
  assert( tree != NULL );
  assert( x_node != NULL );

  rb_node_t *const y_temp = x_node->child[!dir];
  x_node->child[!dir] = y_temp->child[dir];

  if ( y_temp->child[dir] != RB_NIL(tree) )
    y_temp->child[dir]->parent = x_node;

  y_temp->parent = x_node->parent;
  RB_PARENT_CHILD( x_node ) = y_temp;
  y_temp->child[dir] = x_node;
  x_node->parent = y_temp;
}

#ifndef NDEBUG
/**
 * Checks that some properties of \a tree hold.
 *
 * @param tree A pointer to the red-black tree to check.
 */
static void rb_tree_check( rb_tree_t const *tree ) {
  assert( tree != NULL );
  assert( RB_FIRST(tree)->color == RB_BLACK );
}
#else
# define rb_tree_check(TREE)      do { } while (0)
#endif /* NDEBUG */

/**
 * Repairs a tree after a node has been deleted by rotating and repainting
 * colors to restore the 4 properties inherent in red-black trees.
 *
 * @param tree A pointer to the red-black tree to repair.
 * @param node A pointer to the rb_node to start the repair at.
 */
static void rb_delete_fixup( rb_tree_t *tree, rb_node_t *x_node ) {
  assert( tree != NULL );
  assert( x_node != NULL );

  while ( is_black( x_node ) ) {
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
      break;
    }
  } // while
}

/**
 * Repairs a tree after a node has been inserted by rotating and repainting
 * colors to restore the 4 properties inherent in red-black trees.
 *
 * @param tree A pointer to the red-black tree to repair.
 * @param z_node A pointer to the rb_node to start the repair at.
 *
 * @sa "Introduction to Algorithms," 4th ed., section 13.3, p. 339.
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
  // Note that because we use a sentinel for the root node we never need to
  // worry about replacing the root.
  //
  while ( is_red( z_node->parent ) ) {
    rb_dir_t const dir = child_dir( z_node->parent );
    rb_node_t *const y_uncle = z_node->parent->parent->child[!dir];
    if ( is_red( y_uncle ) ) {
      z_node->parent->color = RB_BLACK;
      y_uncle->color = RB_BLACK;
      z_node->parent->parent->color = RB_RED;
      z_node = z_node->parent->parent;
      continue;
    }
    if ( is_dir_child( z_node, !dir ) ) {
      z_node = z_node->parent;
      rb_node_rotate( tree, z_node, dir );
    }
    z_node->parent->color = RB_BLACK;
    z_node->parent->parent->color = RB_RED;
    rb_node_rotate( tree, z_node->parent->parent, !dir );
  } // while

  RB_FIRST(tree)->color = RB_BLACK;     // first node is always black
  rb_tree_check( tree );
}

/**
 * Performs an in-order traversal of the red-black tree starting at \a node.
 *
 * @param tree A pointer to the red-black tree to visit.
 * @param node A pointer to the rb_node to start visiting at.
 * @param visit_fn The visitor function to use.
 * @param v_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the rb_node at which visiting stopped or NULL
 * if the entire sub-tree was visited.
 */
NODISCARD
static rb_node_t* rb_node_visit( rb_tree_t const *tree, rb_node_t *node,
                                 rb_visit_fn_t visit_fn, void *v_data ) {
  assert( tree != NULL );
  assert( node != NULL );

  while ( node != RB_NIL(tree) ) {
    rb_node_t *const stopped_node =
      rb_node_visit( tree, node->child[RB_L], visit_fn, v_data );
    if ( stopped_node != NULL )
      return stopped_node;
    if ( (*visit_fn)( node->data, v_data ) )
      return node;
    node = node->child[RB_R];
  } // while

  return NULL;
}

/**
 * Resets \a tree to empty.
 *
 * @param tree A pointer to the red-black tree to reset.
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

  tree->nil = (rb_node_t){
    .data = NULL,
    .child = { RB_NIL(tree), RB_NIL(tree) },
    .parent = RB_NIL(tree),
    .color = RB_BLACK
  };

  *RB_ROOT(tree) = *RB_NIL(tree);
  tree->cmp_fn = NULL;
}

////////// extern functions ///////////////////////////////////////////////////

void rb_tree_cleanup( rb_tree_t *tree, rb_free_fn_t free_fn ) {
  if ( tree != NULL ) {
    rb_node_free( tree, RB_FIRST(tree), free_fn );
    rb_tree_reset( tree );
  }
}

void* rb_tree_delete( rb_tree_t *tree, rb_node_t *z_delete ) {
  assert( tree != NULL );
  assert( z_delete != NULL );
  assert( z_delete != RB_NIL(tree) );

  void *const data = z_delete->data;

  rb_node_t *const y_proxy =
    is_node_full( tree, z_delete ) ? rb_node_next( tree, z_delete ) : z_delete;

  rb_node_t *const x_proxy_child =
    y_proxy->child[ y_proxy->child[RB_L] == RB_NIL(tree) ];

  x_proxy_child->parent = y_proxy->parent;
  if ( y_proxy->parent == RB_ROOT(tree) )
    RB_FIRST(tree) = x_proxy_child;
  else
    RB_PARENT_CHILD( y_proxy ) = x_proxy_child;

  if ( y_proxy != z_delete ) {
    y_proxy->color = z_delete->color;
    y_proxy->child[RB_L] = z_delete->child[RB_L];
    y_proxy->child[RB_R] = z_delete->child[RB_R];
    y_proxy->parent = z_delete->parent;
    z_delete->child[RB_L]->parent = z_delete->child[RB_R]->parent = y_proxy;
    RB_PARENT_CHILD( z_delete ) = y_proxy;
  }

  if ( is_black( y_proxy ) )
    rb_delete_fixup( tree, x_proxy_child );

  free( z_delete );
  RB_FIRST(tree)->color = RB_BLACK;     // first node is always black
  rb_tree_check( tree );
  return data;
}

rb_node_t* rb_tree_find( rb_tree_t const *tree, void const *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  for ( rb_node_t *node = RB_FIRST(tree); node != RB_NIL(tree); ) {
    int const cmp = (*tree->cmp_fn)( data, node->data );
    if ( cmp == 0 )
      return node;
    node = node->child[ cmp >= 0 ];
  } // for
  return NULL;
}

void rb_tree_init( rb_tree_t *tree, rb_cmp_fn_t cmp_fn ) {
  assert( tree != NULL );
  assert( cmp_fn != NULL );
  rb_tree_reset( tree );
  tree->cmp_fn = cmp_fn;
}

rb_insert_rv_t rb_tree_insert( rb_tree_t *tree, void *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  // See "Introduction to Algorithms," 4th ed., section 13.3, p. 338.

  rb_node_t *x_node = RB_FIRST(tree);
  rb_node_t *y_parent = RB_ROOT(tree);

  //
  // Find either the existing node having the same data -OR- the parent for the
  // new node.
  //
  while ( x_node != RB_NIL(tree) ) {
    int const cmp = (*tree->cmp_fn)( data, x_node->data );
    if ( cmp == 0 )
      return (rb_insert_rv_t){ x_node, .inserted = false };
    y_parent = x_node;
    x_node = x_node->child[ cmp >= 0 ];
  } // while

  x_node = MALLOC( rb_node_t, 1 );
  *x_node = (rb_node_t){
    .data = data,
    .child = { RB_NIL(tree), RB_NIL(tree) },
    .parent = y_parent,
    .color = RB_RED                     // new nodes are always red
  };

  // Determine which child of the parent the new node should be.
  rb_dir_t const dir = STATIC_CAST( rb_dir_t,
    y_parent != RB_ROOT(tree) && (*tree->cmp_fn)( data, y_parent->data ) >= 0
  );
  assert( y_parent->child[dir] == RB_NIL(tree) );
  y_parent->child[dir] = x_node;

  rb_insert_fixup( tree, x_node );
  return (rb_insert_rv_t){ x_node, .inserted = true };
}

rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visit_fn_t visit_fn,
                          void *v_data ) {
  assert( visit_fn != NULL );

  rb_tree_t *const nonconst_tree = CONST_CAST( rb_tree_t*, tree );
  return rb_node_visit(
    nonconst_tree, RB_FIRST(nonconst_tree), visit_fn, v_data
  );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
