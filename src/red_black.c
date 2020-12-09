/*
**      cdecl -- C gibberish translator
**      src/red_black.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
 * Defined functions for manipulating a Red-Black Tree.
 *
 * @sa https://en.wikipedia.org/wiki/Red-black_tree
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "red_black.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>                      /* for NULL */

#define RB_FIRST(TREE)  (RB_ROOT(TREE)->left)
#define RB_NIL          (&rb_nil)
#define RB_ROOT(TREE)   (&(TREE)->root)

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local functions
static void rb_rotate_left( rb_tree_t*, rb_node_t* );
static void rb_rotate_right( rb_tree_t*, rb_node_t* );

/**
 * Sentinel for NIL node.  Ideally, it should be `const` but isn't since
 * pointers-to-non-`const` point to it.
 */
static rb_node_t rb_nil = { NULL, RB_NIL, RB_NIL, RB_NIL, RB_BLACK };

////////// inline functions ///////////////////////////////////////////////////

/**
 * Convenience function for checking that a node is black.
 *
 * @param node A pointer to the `rb_node` to check.
 * @return Returns `true` only if \a node is black.
 *
 * @sa is_red()
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_black( rb_node_t const *node ) {
  return node->color == RB_BLACK;
}

/**
 * Convenience function for checking whether a node is the left child of its
 * parent.
 *
 * @param node A pointer to the `rb_node` to check.
 * @return Returns `true` only if \a node is the left child of its parent.
 *
 * @sa is_right()
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_left( rb_node_t const *node ) {
  return node == node->parent->left;
}

/**
 * Convenience function for checking that a node is red.
 *
 * @param node A pointer to the `rb_node` to check.
 * @return Returns `true` only if \a node is red.
 *
 * @sa is_black()
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_red( rb_node_t const *node ) {
  return node->color == RB_RED;
}

/**
 * Convenience function for checking whether a node is the right child of its
 * parent.
 *
 * @param node A pointer to the `rb_node` to check.
 * @return Returns `true` only if \a node is the right child of its parent.
 *
 * @sa is_left()
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_right( rb_node_t const *node ) {
  return node == node->parent->right;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Initializes an `rb_node`.
 *
 * @param node A pointer to the `rb_node` to initialize.
 */
static void rb_node_init( rb_node_t *node ) {
  assert( node != NULL );
  node->data = NULL;
  node->left = node->right = node->parent = RB_NIL;
  node->color = RB_BLACK;
}

/**
 * Performs an in-order traversal of the red-black tree starting at \a node.
 *
 * @param tree A pointer to the red-black tree to visit.
 * @param node A pointer to the `rb_node` to start visiting at.
 * @param visitor The visitor to use.
 * @param aux_data Optional data passed to \a visitor.
 * @return Returns a pointer to the `rb_node` at which visiting stopped or null
 * if the entire sub-tree was visited.
 */
PJL_WARN_UNUSED_RESULT
static rb_node_t* rb_node_visit( rb_tree_t const *tree, rb_node_t *node,
                                 rb_visitor_t visitor, void *aux_data ) {
  assert( tree != NULL );
  assert( node != NULL );

  if ( node == RB_NIL )
    return NULL;

  rb_node_t *const stopped_node =
    rb_node_visit( tree, node->left, visitor, aux_data );
  if ( stopped_node != NULL )
    return stopped_node;

  if ( visitor( node->data, aux_data ) )
    return node;

  return rb_node_visit( tree, node->right, visitor, aux_data );
}

/**
 * Frees all memory associated with \a tree.
 *
 * @param node A pointer to the `rb_node` to free.
 * @param data_free_fn A pointer to a function used to free data associated
 * with each node or null if unnecessary.
 */
static void rb_tree_free_impl( rb_node_t *node, rb_data_free_t data_free_fn ) {
  assert( node != NULL );

  if ( node != RB_NIL ) {
    rb_tree_free_impl( node->left, data_free_fn );
    rb_tree_free_impl( node->right, data_free_fn );
    if ( data_free_fn != NULL )
      (*data_free_fn)( node->data );
    FREE( node );
  }
}

/**
 * Repairs the tree after a node has been deleted by rotating and repainting
 * colors to restore the 4 properties inherent in red-black trees.
 *
 * @param tree A pointer to the red-black tree to repair.
 * @param node A pointer to the `rb_node` to start the repair at.
 */
static void rb_tree_repair( rb_tree_t *tree, rb_node_t *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  while ( is_black( node ) ) {
    if ( is_left( node ) ) {
      rb_node_t *sibling = node->parent->right;
      if ( is_red( sibling ) ) {
        sibling->color = RB_BLACK;
        node->parent->color = RB_RED;
        rb_rotate_left( tree, node->parent );
        sibling = node->parent->right;
      }
      if ( is_black( sibling->right ) && is_black( sibling->left ) ) {
        sibling->color = RB_RED;
        node = node->parent;
      } else {
        if ( is_black( sibling ) ) {
          sibling->left->color = RB_BLACK;
          sibling->color = RB_RED;
          rb_rotate_right( tree, sibling );
          sibling = node->parent->right;
        }
        sibling->color = node->parent->color;
        node->parent->color = RB_BLACK;
        sibling->right->color = RB_BLACK;
        rb_rotate_left( tree, node->parent );
        break;
      }
    } else {                            // if ( is_right( node ) )
      rb_node_t *sibling = node->parent->left;
      if ( is_red( sibling ) ) {
        sibling->color = RB_BLACK;
        node->parent->color = RB_RED;
        rb_rotate_right( tree, node->parent );
        sibling = node->parent->left;
      }
      if ( is_black( sibling->right ) && is_black( sibling->left ) ) {
        sibling->color = RB_RED;
        node = node->parent;
      } else {
        if ( is_black( sibling->left ) ) {
          sibling->right->color = RB_BLACK;
          sibling->color = RB_RED;
          rb_rotate_left( tree, sibling );
          sibling = node->parent->left;
        }
        sibling->color = node->parent->color;
        node->parent->color = RB_BLACK;
        sibling->left->color = RB_BLACK;
        rb_rotate_right( tree, node->parent );
        break;
      }
    }
  } // while
}

/**
 * Rotates a subtree of a red-black tree left.
 *
 * @param tree A pointer to the red-black tree to manipulate.
 * @param node A pointer to the `rb_node` to rotate.
 *
 * @sa rb_rotate_right
 */
static void rb_rotate_left( rb_tree_t *tree, rb_node_t *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  rb_node_t *const child = node->right;
  node->right = child->left;

  if ( child->left != RB_NIL )
    child->left->parent = node;
  child->parent = node->parent;

  if ( is_left( node ) )
    node->parent->left = child;
  else
    node->parent->right = child;

  child->left = node;
  node->parent = child;
}

/**
 * Rotates a subtree of a red-black tree right.
 *
 * @param tree A pointer to the red-black tree to manipulate.
 * @param node A pointer to the `rb_node` to rotate.
 *
 * @sa rb_rotate_left
 */
static void rb_rotate_right( rb_tree_t *tree, rb_node_t *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  rb_node_t *const child = node->left;
  node->left = child->right;

  if ( child->right != RB_NIL )
    child->right->parent = node;
  child->parent = node->parent;

  if ( is_left( node ) )
    node->parent->left = child;
  else
    node->parent->right = child;

  child->right = node;
  node->parent = child;
}

/**
 * Gets the successor of \a node.
 *
 * @param tree A pointer to the red-black tree that \a node is part of.
 * @param node A pointer to the `rb_node` to get the successor of.
 * @return Returns said successor.
 */
PJL_WARN_UNUSED_RESULT
static rb_node_t* rb_successor( rb_tree_t *tree, rb_node_t *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  rb_node_t *succ = node->right;

  if ( succ != RB_NIL ) {
    while ( succ->left != RB_NIL )
      succ = succ->left;
  } else {
    // No right child, move up until we find it or hit the root.
    for ( succ = node->parent; node == succ->right; succ = succ->parent )
      node = succ;
    if ( succ == RB_ROOT(tree) )
      succ = RB_NIL;
  }
  return succ;
}

////////// extern functions ///////////////////////////////////////////////////

void* rb_node_delete( rb_tree_t *tree, rb_node_t *delete_node ) {
  assert( tree != NULL );
  assert( delete_node != NULL );

  void *const data = delete_node->data;

  rb_node_t *const y =
    delete_node->left == RB_NIL || delete_node->right == RB_NIL ?
      delete_node :
      rb_successor( tree, delete_node );

  rb_node_t *const x = y->left == RB_NIL ? y->right : y->left;

  if ( (x->parent = y->parent) == RB_ROOT(tree) ) {
    RB_FIRST(tree) = x;
  } else {
    if ( is_left( y ) )
      y->parent->left = x;
    else
      y->parent->right = x;
  }

  if ( is_black( y ) )
    rb_tree_repair( tree, x );

  if ( y != delete_node ) {
    y->color = delete_node->color;
    y->left = delete_node->left;
    y->right = delete_node->right;
    y->parent = delete_node->parent;
    delete_node->left->parent = delete_node->right->parent = y;
    if ( is_left( delete_node ) )
      delete_node->parent->left = y;
    else
      delete_node->parent->right = y;
  }

  FREE( delete_node );
  return data;
}

rb_node_t* rb_tree_find( rb_tree_t *tree, void const *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  for ( rb_node_t *node = RB_FIRST(tree); node != RB_NIL; ) {
    int const cmp = (*tree->data_cmp_fn)( data, node->data );
    if ( cmp == 0 )
      return node;
    node = cmp < 0 ? node->left : node->right;
  } // for
  return NULL;
}

void rb_tree_free( rb_tree_t *tree, rb_data_free_t data_free_fn ) {
  if ( tree != NULL && RB_FIRST(tree) != NULL ) {
    rb_tree_free_impl( RB_FIRST(tree), data_free_fn );
    rb_node_init( RB_ROOT(tree) );
    tree->data_cmp_fn = NULL;
  }
}

void rb_tree_init( rb_tree_t *tree, rb_data_cmp_t data_cmp_fn ) {
  assert( tree != NULL );
  assert( data_cmp_fn != NULL );

  rb_node_init( RB_ROOT(tree) );
  tree->data_cmp_fn = data_cmp_fn;
}

rb_node_t* rb_tree_insert( rb_tree_t *tree, void *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  rb_node_t *node = RB_FIRST(tree);
  rb_node_t *parent = RB_ROOT(tree);

  // Find correct insertion point.
  while ( node != RB_NIL ) {
    parent = node;
    int const cmp = (*tree->data_cmp_fn)( data, node->data );
    if ( cmp == 0 )
      return node;
    node = cmp < 0 ? node->left : node->right;
  } // while

  node = MALLOC( rb_node_t, 1 );
  node->data = data;
  node->color = RB_RED;
  node->left = node->right = RB_NIL;
  node->parent = parent;

  if ( parent == RB_ROOT(tree) ||
       (*tree->data_cmp_fn)( data, parent->data ) < 0 ) {
    parent->left = node;
  } else {
    parent->right = node;
  }

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
  //     further iterations to fixup the old parent.
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
  while ( is_red( node->parent ) ) {
    rb_node_t *uncle;
    if ( is_left( node->parent ) ) {
      uncle = node->parent->parent->right;
      if ( is_red( uncle ) ) {
do_red: node->parent->color = RB_BLACK;
        uncle->color = RB_BLACK;
        node->parent->parent->color = RB_RED;
        node = node->parent->parent;
        continue;
      }
      if ( is_right( node ) ) {
        node = node->parent;
        rb_rotate_left( tree, node );
      }
      node->parent->color = RB_BLACK;
      node->parent->parent->color = RB_RED;
      rb_rotate_right( tree, node->parent->parent );
    } else {                            // if ( is_right( node->parent ) )
      uncle = node->parent->parent->left;
      if ( is_red( uncle ) )
        goto do_red;
      if ( is_left( node ) ) {
        node = node->parent;
        rb_rotate_right( tree, node );
      }
      node->parent->color = RB_BLACK;
      node->parent->parent->color = RB_RED;
      rb_rotate_left( tree, node->parent->parent );
    }
  } // while

  RB_FIRST(tree)->color = RB_BLACK;     // first node is always black
  return NULL;
}

rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visitor_t visitor,
                          void *aux_data ) {
  assert( tree != NULL );
  assert( visitor != NULL );

  return rb_node_visit(
    CONST_CAST( rb_tree_t*, tree ),
    CONST_CAST( rb_node_t*, RB_FIRST(tree) ),
    visitor, aux_data
  );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
