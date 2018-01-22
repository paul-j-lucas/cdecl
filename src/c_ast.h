/*
**      cdecl -- C gibberish translator
**      src/c_ast.h
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

#ifndef cdecl_c_ast_H
#define cdecl_c_ast_H

/**
 * @file
 * Declares types to represent an Abstract Syntax Tree (AST) for parsed C/C++
 * declarations as well as functions for traversing and manipulating an AST.
 *
 * In all cases where an AST node contains a pointer to another, that pointer
 * is always declared first.  Since all the different kinds of AST nodes are
 * declared within a `union`, all the pointers are at the same offset.  This
 * makes traversing the AST easy.
 *
 * Similar same-offset tricks are done for other `struct` members as well.
 */

// local
#include "config.h"                     /* must go first */
#include "c_kind.h"
#include "c_type.h"
#include "slist.h"
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for offsetof() */
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_AST_INLINE
# define CDECL_AST_INLINE _GL_INLINE
#endif /* CDECL_AST_INLINE */

/// @endcond

#define C_ARRAY_SIZE_NONE     (-1)      /**< For `array[]`. */
#define C_ARRAY_SIZE_VARIABLE (-2)      /**< For `array[*]`. */

/**
 * Convenience macro to get the `c_ast` given an `slist_node`.
 *
 * @param NODE A pointer to an `slist_node`.
 * @return Returns a pointer to the `c_ast`.
 * @hideinitializer
 */
#define C_AST_DATA(NODE)          SLIST_DATA( c_ast_t*, (NODE) )

///////////////////////////////////////////////////////////////////////////////

/**
 * The direction to traverse an AST using `c_ast_visit()`.
 */
enum v_direction {
  V_DOWN,                               ///< Root to leaves.
  V_UP                                  ///< Leaf to root.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * The signature for functions passed to `c_ast_visit()`.
 *
 * @param ast The `c_ast` to visit.
 * @param data Optional data passed to `c_ast_visit()`.
 * @return Returning `true` will cause traversal to stop and \a ast to be
 * returned to the caller of `c_ast_visit()`.
 */
typedef bool (*c_ast_visitor_t)( c_ast_t *ast, void *data );

/**
 * @defgroup AST-group AST Nodes
 * The AST node `struct`s  contain data specific to each
 * <code>\ref c_kind_t</code>.
 * All `struct`s are placed into a `union` within `c_ast`.
 * @{
 */

/**
 * Generic "parent" AST node.
 *
 * @note All parent nodes have a `c_ast` pointer to what they're a parent of as
 * their first `struct` member: this is taken advantage of.
 */
struct c_parent {
  c_ast_t  *of_ast;                     ///< What it's a parent of.
};

/**
 * AST node for a C/C++ array.
 */
struct c_array {
  c_ast_t  *of_ast;                     ///< What it's an array of.
  int       size;                       ///< Array size or `C_ARRAY_*`.
  c_type_t  type;                       ///< E.g., `array[static const 10]`
};

/**
 * AST node for a C/C++ block (Apple extension).
 */
struct c_block {
  c_ast_t  *ret_ast;                    ///< Return type.
  slist_t   args;                       ///< Block argument(s), if any.
};

/**
 * AST node for a C/C++ `enum`, `class`, `struct`, or `union` type.
 */
struct c_ecsu {
  char const *ecsu_name;                ///< enum/class/struct/union name
};

/**
 * AST node for a C/C++ function.
 *
 * @note Members are laid out in the same order as `c_block`: this is taken
 * advantage of.
 */
struct c_func {
  c_ast_t  *ret_ast;                    ///< Return type.
  slist_t   args;                       ///< Function argument(s), if any.
};

/**
 * AST node for a C++ pointer-to-member of a class.
 */
struct c_ptr_mbr {
  c_ast_t    *of_ast;                   ///< Member type.
  char const *class_name;               ///< When a member function; or null.
};

/**
 * AST node for a C/C++ pointer, or a C++ reference or rvalue reference.
 */
struct c_ptr_ref {
  c_ast_t  *to_ast;                     ///< What it's a pointer/reference to.
};

/**
 * AST node for a parsed C/C++ declaration.
 */
struct c_ast {
  c_ast_depth_t depth;                  ///< How many `()` deep.
  c_ast_id_t    id;                     ///< Unique id (starts at 1).
  c_kind_t      kind;                   ///< Kind.
  char const   *name;                   ///< Name, if any.
  c_type_t      type;                   ///< Type.
  c_ast_t      *parent;                 ///< Parent `c_ast` node, if any.
  c_loc_t       loc;                    ///< Source location.

  union {
    c_parent_t          parent;         ///< "Parent" member(s).
    c_array_t           array;          ///< Array member(s).
    c_block_t           block;          ///< Block member(s).
    // nothing needed for K_BUILTIN
    c_ecsu_t            ecsu;           ///< `enum`, `class`, `struct`, `union`
    c_func_t            func;           ///< Function member(s).
    // nothing needed for K_NAME
    c_ptr_mbr_t         ptr_mbr;        ///< Pointer-to-member member(s).
    c_ptr_ref_t         ptr_ref;        ///< Pointer or reference member(s).
    c_typedef_t const  *c_typedef;      ///< `typedef` member(s).
    // nothing needed for K_VARIADIC
  } as;                                 ///< Union discriminator.
};

/** @} */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Convenience function for getting block or function arguments.
 *
 * @param ast The `c_ast` to get the arguments of.
 * @return Returns a pointer to the first argument or null if none.
 */
CDECL_AST_INLINE c_ast_arg_t const* c_ast_args( c_ast_t const *ast ) {
  return ast->as.func.args.head;
}

/**
 * Cleans-up all AST data.
 * (Currently, this checks that the number of `c_ast` nodes freed match the
 * number allocated.)
 */
void c_ast_cleanup( void );

/**
 * Checks whether the two ASTs are equivalent, i.e., represent the same type.
 *
 * @param ast_i The first `c_ast`.
 * @param ast_j The second `c_ast`.
 * @return Returns `true` only if the two ASTs are equivalent.
 */
bool c_ast_equiv( c_ast_t const *ast_i, c_ast_t const *ast_j );

/**
 * Frees all the memory used by \a ast.
 *
 * @param ast The `c_ast` to free.  May be null.
 */
void c_ast_free( c_ast_t *ast );

/**
 * Checks whether \a ast is a parent node.
 *
 * @param ast The `c_ast` to check.  May be null.
 * @return Returns `true` only if it is.
 */
CDECL_AST_INLINE bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast != NULL && c_kind_is_parent( ast->kind );
}

/**
 * Creates a new `c_ast`.
 *
 * @param kind The kind of `c_ast` to create.
 * @param depth How deep within `()` it is.
 * @param loc A pointer to the token location data.
 * @return Returns a pointer to a new c_ast.
 */
c_ast_t* c_ast_new( c_kind_t kind, c_ast_depth_t depth, c_loc_t const *loc );

/**
 * Gets the root `c_ast` node of \a ast.
 *
 * @param ast The `c_ast` node to get the root of.
 * @return Returns said `c_ast` node.
 */
c_ast_t* c_ast_root( c_ast_t *ast );

/**
 * Sets the two-way pointer links between parent/child `c_ast` nodes.
 *
 * @param child The "child" `c_ast` node to set the parent of.
 * @param parent The "parent" `c_ast` node whose child node is set.
 */
void c_ast_set_parent( c_ast_t *child, c_ast_t *parent );

/**
 * Does a depth-first, post-order traversal of an AST.
 *
 * @param ast The `c_ast` to begin at.  May be null.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the `c_ast` the visitor stopped on or null.
 *
 * @note Function (or block) argument(s) are \e not traversed into. They're
 * considered distinct ASTs.
 */
CDECL_AST_INLINE c_ast_t* c_ast_visit( c_ast_t *ast, v_direction_t dir,
                                       c_ast_visitor_t visitor, void *data ) {
  c_ast_t* c_ast_visit_down( c_ast_t*, c_ast_visitor_t, void* );
  c_ast_t* c_ast_visit_up( c_ast_t*, c_ast_visitor_t, void* );

  return dir == V_DOWN ?
    c_ast_visit_down( ast, visitor, data ) :
    c_ast_visit_up( ast, visitor, data );
}

/**
 * Does a depth-first, post-order traversal of an AST looking for a `c_ast`
 * node that satisfies the visitor.
 *
 * @param ast The `c_ast` to begin at.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns `true` only if \a visitor ever returned `true`.
 *
 * @note Function (or block) argument(s) are \e not traversed into. They're
 * considered distinct ASTs.
 */
CDECL_AST_INLINE bool c_ast_found( c_ast_t const *ast, v_direction_t dir,
                                   c_ast_visitor_t visitor, void *data ) {
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  c_ast_t *const found_ast = c_ast_visit( nonconst_ast, dir, visitor, data );
  return found_ast != NULL;
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_H */
/* vim:set et sw=2 ts=2: */
