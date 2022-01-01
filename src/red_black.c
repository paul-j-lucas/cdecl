/*
**      cdecl -- C gibberish translator
**      src/red_black.c
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
#include "red_black.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>                      /* for NULL */
#include <stdlib.h>                     /* or free(3) */

#define RB_FIRST(TREE)  (RB_ROOT(TREE)->child[RB_L])
#define RB_NIL          (&rb_nil)
#define RB_ROOT(TREE)   (&(TREE)->root)

/// @endcond

/**
 * Red-black tree child direction.
 */
enum rb_dir {
  RB_L,                                 ///< Left child direction.
  RB_R                                  ///< Right child direction.
};
typedef enum rb_dir rb_dir_t;

///////////////////////////////////////////////////////////////////////////////

// local functions
static void rb_tree_rotate_node( rb_tree_t*, rb_node_t*, rb_dir_t );

/**
 * Sentinel for NIL node.  Ideally, it should be `const` but isn't since
 * pointers-to-non-`const` point to it.
 */
static rb_node_t rb_nil = { NULL, { RB_NIL, RB_NIL }, RB_NIL, RB_BLACK };

////////// inline functions ///////////////////////////////////////////////////

/**
 * Convenience function for checking that a node is black.
 *
 * @param node A pointer to the rb_node to check.
 * @return Returns `true` only if \a node is black.
 *
 * @sa is_red()
 */
PJL_WARN_UNUSED_RESULT
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
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_dir( rb_node_t const *node, rb_dir_t dir ) {
  return node == node->parent->child[dir];
}

/**
 * Convenience function for checking that a node is red.
 *
 * @param node A pointer to the rb_node to check.
 * @return Returns `true` only if \a node is red.
 *
 * @sa is_black()
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_red( rb_node_t const *node ) {
  return node->color == RB_RED;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Frees all memory associated with \a node.
 *
 * @param node A pointer to the rb_node to free.
 * @param free_fn A pointer to a function used to free data associated with \a
 * node or NULL if unnecessary.
 */
static void rb_node_free( rb_node_t *node, rb_free_fn_t free_fn ) {
  assert( node != NULL );

  if ( node != RB_NIL ) {
    rb_node_free( node->child[RB_L], free_fn );
    rb_node_free( node->child[RB_R], free_fn );
    if ( free_fn != NULL )
      (*free_fn)( node->data );
    free( node );
  }
}

/**
 * Initializes an rb_node.
 *
 * @param node A pointer to the rb_node to initialize.
 */
static void rb_node_init( rb_node_t *node ) {
  assert( node != NULL );
  node->color = RB_BLACK;
  node->child[RB_L] = node->child[RB_R] = node->parent = RB_NIL;
  node->data = NULL;
}

/**
 * Gets the successor of \a node.
 *
 * @param tree A pointer to the red-black tree that \a node is part of.
 * @param node A pointer to the rb_node to get the successor of.
 * @return Returns said successor.
 */
PJL_WARN_UNUSED_RESULT
static rb_node_t* rb_tree_node_successor( rb_tree_t *tree, rb_node_t *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  rb_node_t *next = node->child[RB_R];

  if ( next != RB_NIL ) {
    while ( next->child[RB_L] != RB_NIL )
      next = next->child[RB_L];
  } else {
    // No right child, move up until we find it or hit the root.
    for ( next = node->parent; node == next->child[RB_R]; next = next->parent )
      node = next;
    if ( next == RB_ROOT(tree) )
      next = RB_NIL;
  }
  return next;
}

/**
 * Repairs the tree after a node has been deleted by rotating and repainting
 * colors to restore the 4 properties inherent in red-black trees.
 *
 * @param tree A pointer to the red-black tree to repair.
 * @param node A pointer to the rb_node to start the repair at.
 */
static void rb_tree_repair_node( rb_tree_t *tree, rb_node_t *node ) {
  assert( tree != NULL );
  assert( node != NULL );

  while ( is_black( node ) ) {
    rb_dir_t const dir = is_dir( node->parent, RB_R );
    rb_node_t *sibling = node->parent->child[dir];
    if ( is_red( sibling ) ) {
      sibling->color = RB_BLACK;
      node->parent->color = RB_RED;
      rb_tree_rotate_node( tree, node->parent, !dir );
      sibling = node->parent->child[dir];
    }
    if ( is_red( sibling->child[RB_L] ) || is_red( sibling->child[RB_R] ) ) {
      if ( is_black( sibling ) ) {
        sibling->child[!dir]->color = RB_BLACK;
        sibling->color = RB_RED;
        rb_tree_rotate_node( tree, sibling, dir );
        sibling = node->parent->child[dir];
      }
      sibling->color = node->parent->color;
      node->parent->color = RB_BLACK;
      sibling->child[dir]->color = RB_BLACK;
      rb_tree_rotate_node( tree, node->parent, !dir );
      break;
    }
    sibling->color = RB_RED;
    node = node->parent;
  } // while
}

/**
 * Rotates a subtree of a red-black tree.
 *
 * @param tree A pointer to the red-black tree to manipulate.
 * @param node A pointer to the rb_node to rotate.
 * @param dir The direction to rotate.
 */
static void rb_tree_rotate_node( rb_tree_t *tree, rb_node_t *node,
                                 rb_dir_t dir ) {
  assert( tree != NULL );
  assert( node != NULL );

  rb_node_t *const temp = node->child[!dir];
  node->child[!dir] = temp->child[dir];

  if ( temp->child[dir] != RB_NIL )
    temp->child[dir]->parent = node;
  temp->parent = node->parent;
  node->parent->child[ is_dir( node, RB_R ) ] = temp;
  temp->child[dir] = node;
  node->parent = temp;
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
PJL_WARN_UNUSED_RESULT
static rb_node_t* rb_tree_visit_node( rb_tree_t const *tree, rb_node_t *node,
                                      rb_visit_fn_t visit_fn, void *v_data ) {
  assert( tree != NULL );
  assert( node != NULL );

  while ( node != RB_NIL ) {
    rb_node_t *const stopped_node =
      rb_tree_visit_node( tree, node->child[RB_L], visit_fn, v_data );
    if ( stopped_node != NULL )
      return stopped_node;
    if ( (*visit_fn)( node->data, v_data ) )
      return node;
    node = node->child[RB_R];
  } // while

  return NULL;
}

////////// extern functions ///////////////////////////////////////////////////

void rb_tree_cleanup( rb_tree_t *tree, rb_free_fn_t free_fn ) {
  if ( tree != NULL && RB_FIRST(tree) != NULL ) {
    rb_node_free( RB_FIRST(tree), free_fn );
    rb_node_init( RB_ROOT(tree) );
    tree->cmp_fn = NULL;
  }
}

void* rb_tree_delete( rb_tree_t *tree, rb_node_t *delete_node ) {
  assert( tree != NULL );
  assert( delete_node != NULL );

  void *const data = delete_node->data;

  rb_node_t *const y =
    delete_node->child[RB_L] == RB_NIL || delete_node->child[RB_R] == RB_NIL ?
      delete_node :
      rb_tree_node_successor( tree, delete_node );

  rb_node_t *const x = y->child[ y->child[RB_L] == RB_NIL ];

  if ( (x->parent = y->parent) == RB_ROOT(tree) )
    RB_FIRST(tree) = x;
  else
    y->parent->child[ is_dir( y, RB_R ) ] = x;

  if ( is_black( y ) )
    rb_tree_repair_node( tree, x );

  if ( y != delete_node ) {
    y->color = delete_node->color;
    y->child[RB_L] = delete_node->child[RB_L];
    y->child[RB_R] = delete_node->child[RB_R];
    y->parent = delete_node->parent;
    delete_node->child[RB_L]->parent = delete_node->child[RB_R]->parent = y;
    delete_node->parent->child[ is_dir( delete_node, RB_R ) ] = y;
  }

  free( delete_node );
  return data;
}

rb_node_t* rb_tree_find( rb_tree_t const *tree, void const *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  for ( rb_node_t *node = RB_FIRST(tree); node != RB_NIL; ) {
    int const cmp = (*tree->cmp_fn)( data, node->data );
    if ( cmp == 0 )
      return node;
    node = node->child[ cmp > 0 ];
  } // for
  return NULL;
}

void rb_tree_init( rb_tree_t *tree, rb_cmp_fn_t cmp_fn ) {
  assert( tree != NULL );
  assert( cmp_fn != NULL );

  rb_node_init( RB_ROOT(tree) );
  tree->cmp_fn = cmp_fn;
}

rb_node_t* rb_tree_insert( rb_tree_t *tree, void *data ) {
  assert( tree != NULL );
  assert( data != NULL );

  rb_node_t *node = RB_FIRST(tree);
  rb_node_t *parent = RB_ROOT(tree);

  // Find correct insertion point.
  while ( node != RB_NIL ) {
    int const cmp = (*tree->cmp_fn)( data, node->data );
    if ( cmp == 0 )
      return node;
    parent = node;
    node = node->child[ cmp > 0 ];
  } // while

  node = MALLOC( rb_node_t, 1 );
  node->color = RB_RED;
  node->child[RB_L] = node->child[RB_R] = RB_NIL;
  node->data = data;
  node->parent = parent;

  rb_dir_t dir = parent != RB_ROOT(tree) &&
    (*tree->cmp_fn)( data, parent->data ) > 0;
  assert( parent->child[dir] == RB_NIL );
  parent->child[dir] = node;

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
    dir = is_dir( node->parent, RB_R );
    rb_node_t *const uncle = node->parent->parent->child[dir];
    if ( is_red( uncle ) ) {
      node->parent->color = RB_BLACK;
      uncle->color = RB_BLACK;
      node->parent->parent->color = RB_RED;
      node = node->parent->parent;
      continue;
    }
    if ( is_dir( node, dir ) ) {
      node = node->parent;
      rb_tree_rotate_node( tree, node, !dir );
    }
    node->parent->color = RB_BLACK;
    node->parent->parent->color = RB_RED;
    rb_tree_rotate_node( tree, node->parent->parent, dir );
  } // while

  RB_FIRST(tree)->color = RB_BLACK;     // first node is always black
  return NULL;
}

rb_node_t* rb_tree_visit( rb_tree_t const *tree, rb_visit_fn_t visit_fn,
                          void *v_data ) {
  assert( visit_fn != NULL );

  rb_tree_t *const nonconst_tree = CONST_CAST( rb_tree_t*, tree );
  return rb_tree_visit_node(
    nonconst_tree, RB_FIRST(nonconst_tree), visit_fn, v_data
  );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
