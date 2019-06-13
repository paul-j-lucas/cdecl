/*
**      cdecl -- C gibberish translator
**      src/c_ast.h
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
#include "c_kind.h"
#include "c_operator.h"
#include "c_sname.h"
#include "c_type.h"
#include "gibberish.h"
#include "slist.h"
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_AST_INLINE
# define CDECL_AST_INLINE _GL_INLINE
#endif /* CDECL_AST_INLINE */

/// @endcond

#define C_ARRAY_SIZE_NONE     (-1)      /**< For `array[]`. */
#define C_ARRAY_SIZE_VARIABLE (-2)      /**< For `array[*]`. */

#define C_FUNC_UNSPECIFIED    0u        /**< Function is unspecified. */
#define C_FUNC_MEMBER         (1u << 0) /**< Function is a member. */
#define C_FUNC_NON_MEMBER     (1u << 1) /**< Function is a non-member. */

// bit masks
#define C_FUNC_MASK_MEMBER    0x3u      /**< Member/non-member bitmask. */

/**
 * Convenience macro to get the `c_ast` given an `slist_node`.
 *
 * @param NODE A pointer to an `slist_node`.
 * @return Returns a pointer to the `c_ast`.
 * @hideinitializer
 */
#define C_AST_DATA(NODE)          SLIST_NODE_DATA( c_ast_t*, (NODE) )

/**
 * Convenience macro to get the name of a scope.
 *
 * @param NODE A pointer to an `slist_node`.
 * @return Returns a pointer to the scope's name.
 * @hideinitializer
 */
#define C_SCOPE_NAME(NODE)        SLIST_NODE_DATA( char const*, (NODE) )

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
  c_ast_t    *of_ast;                   ///< What it's an array of.
  int         size;                     ///< Array size or `C_ARRAY_*`.
  c_type_id_t type_id;                  ///< E.g., `array[static const 10]`
};

/**
 * AST node for a C/C++ block (Apple extension).
 *
 * @note Members are laid out in the same order as `c_func`: this is taken
 * advantage of.
 */
struct c_block {
  c_ast_t  *ret_ast;                    ///< Return type.
  slist_t   args;                       ///< Block argument(s), if any.
};

/**
 * AST Node for a C++ constructor.
 *
 * @note Members are laid out in the same order as `c_func`: this is taken
 * advantage of.
 */
struct c_constructor {
  void     *ret_ast_not_used;           ///< So `args` is at same offset.
  slist_t   args;                       ///< Constructor argument(s), if any.
};

/**
 * AST node for a C/C++ `enum`, `class`, `struct`, or `union` type.
 */
struct c_ecsu {
  c_sname_t ecsu_sname;                 ///< enum/class/struct/union name
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
  unsigned  flags;                      ///< Member vs. non-member.
};

/**
 * AST node for a C++ overloaded operator.
 *
 * @note Members are laid out in the same order as `c_func`: this is taken
 * advantage of.
 */
struct c_oper {
  c_ast_t    *ret_ast;                  ///< Return type.
  slist_t     args;                     ///< Operator argument(s), if any.
  unsigned    flags;                    ///< Member vs. non-member.
  c_oper_id_t oper_id;                  ///< Which operator it is.
};

/**
 * AST node for a C++ pointer-to-member of a class.
 */
struct c_ptr_mbr {
  c_ast_t    *of_ast;                   ///< Member type.
  c_sname_t   class_sname;              ///< When a member function; or empty.
};

/**
 * AST node for a C/C++ pointer, or a C++ reference or rvalue reference.
 */
struct c_ptr_ref {
  c_ast_t  *to_ast;                     ///< What it's a pointer/reference to.
};

/**
 * AST Node for a C++ user-defined conversion operator.
 */
struct c_user_def_conv {
  c_ast_t  *conv_ast;                   ///< Conversion type.
};

/**
 * AST node for a C++ user-defined literal.
 *
 * @note Members are laid out in the same order as `c_func`: this is taken
 * advantage of.
 */
struct c_user_def_lit {
  c_ast_t  *ret_ast;                    ///< Return type.
  slist_t   args;                       ///< Literal argument(s).
};

/**
 * AST node for a parsed C/C++ declaration.
 */
struct c_ast {
  c_ast_depth_t         depth;          ///< How many `()` deep.
  c_ast_id_t            id;             ///< Unique id (starts at 1).
  c_kind_t              kind;           ///< Kind.
  c_sname_t             sname;          ///< Scoped name.
  c_type_id_t           type_id;        ///< Type.
  c_ast_t              *parent;         ///< Parent `c_ast` node, if any.
  c_loc_t               loc;            ///< Source location.

  union {
    c_parent_t          parent;         ///< "Parent" member(s).
    c_array_t           array;          ///< Array member(s).
    c_block_t           block;          ///< Block member(s).
    // nothing needed for K_BUILTIN
    c_constructor_t     constructor;    ///< Constructor member(s).
    // nothing needed for K_DESTRUCTOR
    c_ecsu_t            ecsu;           ///< `enum`, `class`, `struct`, `union`
    c_func_t            func;           ///< Function member(s).
    // nothing needed for K_NAME
    c_oper_t            oper;           ///< Operator member(s).
    c_ptr_mbr_t         ptr_mbr;        ///< Pointer-to-member member(s).
    c_ptr_ref_t         ptr_ref;        ///< Pointer or reference member(s).
    c_typedef_t const  *c_typedef;      ///< `typedef` member(s).
    c_user_def_conv_t   user_def_conv;  ///< User-defined conversion member(s).
    c_user_def_lit_t    user_def_lit;   ///< User-defined literal member(s).
    // nothing needed for K_VARIADIC
  } as;                                 ///< Union discriminator.
};

/** @} */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Convenience function for getting function, operator, or block arguments.
 *
 * @param ast The `c_ast` to get the arguments of.
 * @return Returns a pointer to the first argument or null if none.
 */
CDECL_AST_INLINE c_ast_arg_t const* c_ast_args( c_ast_t const *ast ) {
  return ast->as.func.args.head;
}

/**
 * Convenience function for getting the number of function, operator, or block
 * arguments.
 *
 * @param ast The `c_ast` to get the number of arguments of.
 * @return Returns said number of arguments.
 */
CDECL_AST_INLINE size_t c_ast_args_count( c_ast_t const *ast ) {
  return slist_len( &ast->as.func.args );
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
 * Convenience function for getting the head of the scope list.
 *
 * @param ast The `c_ast` to get the scope list of.
 * @return Returns a pointer to the first scope entry.
 */
CDECL_AST_INLINE c_scope_t const* c_ast_scope( c_ast_t const *ast ) {
  return ast->sname.head;
}

/**
 * Sets the two-way pointer links between parent/child `c_ast` nodes.
 *
 * @param child The "child" `c_ast` node to set the parent of.
 * @param parent The "parent" `c_ast` node whose child node is set.
 */
void c_ast_set_parent( c_ast_t *child, c_ast_t *parent );

/**
 * Appends \a sname to the name of \a ast.
 *
 * @param ast The `c_ast` to append to the name of.
 * @param sname The scoped name to append.  It is cleared.
 *
 * @sa c_ast_sname_prepend_sname()
 */
CDECL_AST_INLINE void c_ast_sname_append_sname( c_ast_t *ast,
                                                c_sname_t *sname ) {
  c_sname_append_sname( &ast->sname, sname );
}

/**
 * Gets the number of names of \a ast, e.g., `S::T::x` is 3.
 *
 * @param ast The `c_ast` to get the number of names of.
 * @return Returns said number of names.
 */
CDECL_AST_INLINE size_t c_ast_sname_count( c_ast_t const *ast ) {
  return c_sname_count( &ast->sname );
}

/**
 * Duplicates the name of \a ast.
 *
 * @param ast The `c_ast` to duplicate the name of.
 * @return Returns the name of \a ast duplicated.
 */
c_sname_t c_ast_sname_dup( c_ast_t const *ast );

/**
 * Checks whether the name of \a ast is empty.
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if the name of \a ast is empty.
 */
CDECL_AST_INLINE bool c_ast_sname_empty( c_ast_t const *ast ) {
  return c_sname_empty( &ast->sname );
}

/**
 * Gets the fully scoped name of \a ast.
 *
 * @param ast The `c_ast` to get the scoped name of.
 * @return Returns said name.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 */
CDECL_AST_INLINE char const* c_ast_sname_full_name( c_ast_t const *ast ) {
  return c_sname_full_name( &ast->sname );
}

/**
 * Gets whether the scoped name of \a ast is a constructor name, e.g. `S::T::T`.
 *
 * @note This also checks for destructor names since the `~` is elided.
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if the last two names of the scoped name of \a
 * ast match.
 */
CDECL_AST_INLINE bool c_ast_sname_is_ctor( c_ast_t const *ast ) {
  return c_sname_is_ctor( &ast->sname );
}

/**
 * Gets the local name of \a ast.
 *
 * @param ast The `c_ast` to get the local name of.
 *
 * @sa c_ast_sname_full_name()
 * @sa c_ast_sname_scope_name()
 */
CDECL_AST_INLINE char const* c_ast_sname_local_name( c_ast_t const *ast ) {
  return c_sname_local_name( &ast->sname );
}

/**
 * Gets the name at \a offset of \a ast.
 *
 * @param ast The `c_ast` to get the name at \a offset of.
 * @param offset The offset (starting at 0) of the name to get.
 * @return Returns the name at \a offset or the empty string if \a offset &gt;=
 * c_ast_sname_count().
 */
CDECL_AST_INLINE char const* c_ast_sname_name_at( c_ast_t const *ast,
                                                  size_t offset ) {
  return c_sname_name_at( &ast->sname, offset );
}

/**
 * Gets the name at \a roffset of \a ast.
 *
 * @param ast The `c_ast` to get the name at \a offset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a offset or the empty string if \a offset &gt;=
 * c_ast_sname_count().
 */
CDECL_AST_INLINE char const* c_ast_sname_name_atr( c_ast_t const *ast,
                                                   size_t roffset ) {
  return c_sname_name_atr( &ast->sname, roffset );
}

/**
 * Prepends \a sname to the name of \a ast.
 *
 * @param ast The `c_ast` to prepend to the name of.
 * @param sname The scoped name to prepend.  It is cleared.
 *
 * @sa c_ast_sname_append_sname()
 */
CDECL_AST_INLINE void c_ast_sname_prepend_sname( c_ast_t *ast,
                                                 c_sname_t *sname ) {
  c_sname_prepend_sname( &ast->sname, sname );
}

/**
 * Gets the scope name of \a ast in C++ form.
 *
 * @param ast The `c_ast` to get the scope name of.
 * @return Returns said name or the empty string if \a ast doesn't have a scope
 * name.
 *
 * @sa c_ast_sname_full_name()
 * @sa c_ast_sname_local_name()
 */
CDECL_AST_INLINE char const* c_ast_sname_scope_name( c_ast_t const *ast ) {
  return c_sname_scope_name( &ast->sname );
}

/**
 * Sets the name of \a ast.
 *
 * @param ast The `c_ast` node to set the name of.
 * @param name The name to set.
 *
 * @sa c_ast_sname_set_sname()
 */
void c_ast_sname_set_name( c_ast_t *ast, char *name );

/**
 * Sets the name of \a ast.
 *
 * @param ast The `c_ast` node to set the name of.
 * @param sname The scoped name to set.  It is not duplicated.
 *
 * @sa c_ast_sname_set_name()
 */
void c_ast_sname_set_sname( c_ast_t *ast, c_sname_t *sname );

/**
 * Sets the scope type of the name of \a ast.
 *
 * @param ast The `c_ast` to set the type of the name of.
 * @param type_id The scope type.
 *
 * @sa c_ast_sname_type()
 */
CDECL_AST_INLINE void c_ast_sname_set_type( c_ast_t *ast,
                                            c_type_id_t type_id ) {
  c_sname_set_type( &ast->sname, type_id );
}

/**
 * Gets the scope type of the name of \a ast.
 *
 * @param ast The `c_ast` node to get the scope type of the name of.
 * @return Returns the scope type.
 *
 * @sa c_ast_sname_set_type()
 */
CDECL_AST_INLINE c_type_id_t c_ast_sname_type( c_ast_t const *ast ) {
  return c_sname_type( &ast->sname );
}

/**
 * Gets the type name of the sname of \a ast.
 *
 * @param ast The `c_ast` node to get the type name of.
 * @return Returns said name.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 */
CDECL_AST_INLINE char const* c_ast_sname_type_name( c_ast_t const *ast ) {
  return c_type_name( c_ast_sname_type( ast ) );
}

/**
 * Does a depth-first, post-order traversal of an AST.
 *
 * @param ast The `c_ast` to begin at.  May be null.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the `c_ast` the visitor stopped on or null.
 *
 * @note Function, operator, or block argument(s) are \e not traversed into.
 * They're considered distinct ASTs.
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
 * @note Function, operator, or block argument(s) are \e not traversed into.
 * They're considered distinct ASTs.
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
