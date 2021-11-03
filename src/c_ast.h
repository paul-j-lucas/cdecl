/*
**      cdecl -- C gibberish translator
**      src/c_ast.h
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

#ifndef cdecl_c_ast_H
#define cdecl_c_ast_H

/**
 * @file
 * Declares types to represent an Abstract Syntax Tree (AST) for parsed C/C++
 * declarations as well as functions for traversing and manipulating an AST.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_kind.h"
#include "c_operator.h"
#include "c_sname.h"
#include "c_type.h"
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef C_AST_INLINE
# define C_AST_INLINE _GL_INLINE
#endif /* C_AST_INLINE */

/// @endcond

/**
 * For `c_array_ast.size`, denotes `array[]`.
 *
 * @sa #C_ARRAY_SIZE_VARIABLE
 */
#define C_ARRAY_SIZE_NONE     (-1)

/**
 * For `c_array_ast.size`, denotes `array[*]`.
 *
 * @sa #C_ARRAY_SIZE_NONE
 */
#define C_ARRAY_SIZE_VARIABLE (-2)

/**
 * For `c_function_ast.flags`, denotes that the function is unspecified
 * (unknown whether it's a member or non-member).
 *
 * @sa #C_FUNC_MASK_MEMBER
 * @sa #C_FUNC_MEMBER
 * @sa #C_FUNC_NON_MEMBER
 */
#define C_FUNC_UNSPECIFIED    0u

/**
 * For `c_function_ast.flags`, denotes that the function is a member.
 *
 * @sa #C_FUNC_MASK_MEMBER
 * @sa #C_FUNC_NON_MEMBER
 * @sa #C_FUNC_UNSPECIFIED
 */
#define C_FUNC_MEMBER         (1u << 0)

/**
 * For `c_function_ast.flags`, denotes that the function is a non-member.
 *
 * @sa #C_FUNC_MASK_MEMBER
 * @sa #C_FUNC_MEMBER
 * @sa #C_FUNC_UNSPECIFIED
 */
#define C_FUNC_NON_MEMBER     (1u << 1)

/**
 * For `c_function_ast.flags`, member bitmask.
 *
 * @sa #C_FUNC_MEMBER
 * @sa #C_FUNC_NON_MEMBER
 * @sa #C_FUNC_UNSPECIFIED
 */
#define C_FUNC_MASK_MEMBER    0x3u

/**
 * Convenience macro for iterating over the AST nodes of an AST list.
 *
 * @param VAR The `slist_node` loop variable.
 * @param AST_LIST A pointer to the AST list to iterate over.
 */
#define FOREACH_AST_LIST(VAR,AST_LIST) \
  FOREACH_SLIST( VAR, AST_LIST, NULL )

///////////////////////////////////////////////////////////////////////////////

/**
 * The argument type for the `alignas` specifier.
 */
enum c_alignas_arg {
  C_ALIGNAS_NONE,                       ///< No `alignas` specifier.
  C_ALIGNAS_EXPR,                       ///< `alignas(` _expr_ `)`
  C_ALIGNAS_TYPE                        ///< `alignas(` _type_ `)`
};

/**
 * Data for the `alignas` specifier.
 */
struct c_alignas {
  union {
    unsigned      expr;                 ///< Aligned to this number of bytes.
    c_ast_t      *type_ast;             ///< Aligned the same as this type.
  } as;                                 ///< Union discriminator.
  c_alignas_arg_t kind;                 ///< Kind of `alignas` argument.
  c_loc_t         loc;                  ///< Source location.
};

/**
 * The direction to traverse an AST using c_ast_visit().
 */
enum c_visit_dir {
  C_VISIT_DOWN,                         ///< Root to leaves.
  C_VISIT_UP                            ///< Leaf to root.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Type of optional data passed to c_ast_visit().
 *
 * @note This is `uint64_t` so it can hold either the largest possible integer
 * or a pointer.  It is _not_ `uintptr_t` because that can't hold a 64-bit
 * integer on a 32-bit pointer platform.
 */
typedef uint64_t c_ast_visitor_data_t;

/**
 * The signature for functions passed to c_ast_visit().
 *
 * @param ast The AST to visit.
 * @param data Optional data passed to c_ast_visit().
 * @return Returning `true` will cause traversal to stop and \a ast to be
 * returned to the caller of c_ast_visit().
 */
typedef bool (*c_ast_visitor_t)( c_ast_t *ast, c_ast_visitor_data_t data );

/**
 * @defgroup ast-nodes-group AST Nodes
 *
 * ## Layout
 *
 * The AST node `struct`s  contain data specific to each
 * <code>\ref c_ast_kind_t</code>.
 * In all cases where an AST node contains:
 *
 *  1. A pointer to another, that pointer is always declared first.
 *  2. Parameters, they are always declared second.
 *  3. Flags, they are always declared third.
 *
 * Since all the different kinds of AST nodes are declared within a `union`,
 * these `struct` members are at the same offsets.  This makes traversing and
 * manipulating an AST easy.
 *
 * ## Memory Management
 *
 * Typically, nodes of a tree data structure are freed by freeing the root node
 * followed by its child nodes in turn, recursively.  This is _not_ done for
 * AST nodes.
 * Instead, AST nodes created via c_ast_new() or c_ast_dup() are appended to a
 * <code>\ref c_ast_list_t</code>.  Nodes are later freed by traversing the
 * list and calling c_ast_free() on each node individually.
 * It's done this way to simplify node memory management.
 *
 * As an AST is being built, sometimes <code>\ref K_PLACEHOLDER</code> nodes
 * are created temporarily.  Later, once an actual node is created, the
 * <code>\ref K_PLACEHOLDER</code> node is replaced.  Rather than freeing a
 * <code>\ref K_PLACEHOLDER</code> node immediately (and, for a parent node,
 * set its "of" node to NULL just prior to being freed so as not to free its
 * child node also), it's simply left on the list.  Once parsing is complete,
 * the entire list is freed effectively "garbage collecting" all nodes.
 * @{
 */

/**
 * Generic "parent" AST node.
 *
 * @note All parent nodes have an AST pointer to what they're a parent of as
 * their first `struct` member: this is taken advantage of.
 */
struct c_parent_ast {
  c_ast_t        *of_ast;               ///< What it's a parent of.
};

/**
 * AST node for a C/C++ array.
 */
struct c_array_ast {
  c_ast_t        *of_ast;               ///< What it's an array of.

  /**
   * One of:
   *
   *  + The actual size of a C array.
   *  + #C_ARRAY_SIZE_NONE
   *  + #C_ARRAY_SIZE_VARIABLE
   */
  c_array_size_t  size;

  c_tid_t         stids;                ///< E.g., `array[static const 10]`
};

/**
 * AST node for a C/C++ block (Apple extension).
 *
 * @note Members are laid out in the same order as `c_function_ast`: this is
 * taken advantage of.
 *
 * @sa [Apple's Extensions to C](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1370.pdf)
 * @sa [Blocks Programming Topics](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks)
 */
struct c_apple_block_ast {
  c_ast_t        *ret_ast;              ///< Return type.
  c_ast_list_t    param_ast_list;       ///< Block parameters(s), if any.
};

/**
 * AST node for a built-in type.
 *
 * @note Members are laid out in the same order as `c_typedef_ast`: this is
 * taken advantage of.
 */
struct c_builtin_ast {
  /// So `bit_width` is at same offset as in `c_typedef_ast`.
  void           *not_used;

  c_bit_width_t   bit_width;            ///< Bit-field width when &gt; 0.
};

/**
 * AST node for a C++ constructor.
 *
 * @note Members are laid out in the same order as `c_function_ast`: this is
 * taken advantage of.
 */
struct c_constructor_ast {
  /// Constructors don't have a return type, but we need an unused pointer so
  /// `param_ast_list` is at same offset as in `c_function_ast`.
  void           *not_used;

  c_ast_list_t    param_ast_list;       ///< Constructor parameters(s), if any.
};

/**
 * AST node for a C/C++ `enum`, `class`, `struct`, or `union` type.
 *
 * @note Members are laid out in the same order as `c_ptr_mbr_ast`: this is
 * taken advantage of.
 */
struct c_ecsu_ast {
  c_ast_t        *of_ast;               ///< For `enum`, the fixed type, if any.
  c_sname_t       ecsu_sname;           ///< enum/class/struct/union name
};

/**
 * AST node for a C/C++ function.
 */
struct c_function_ast {
  c_ast_t        *ret_ast;              ///< Return type.
  c_ast_list_t    param_ast_list;       ///< Function parameter(s), if any.

  /**
   * Bitwise-or of flags currently specifying whether the function is a member,
   * non-member, or unspecified function.
   *
   * @sa #C_FUNC_UNSPECIFIED
   * @sa #C_FUNC_MEMBER
   * @sa #C_FUNC_NON_MEMBER
   * @sa #C_FUNC_MASK_MEMBER
   */
  unsigned        flags;
};

/**
 * AST node for a C++ overloaded operator.
 *
 * @note Members are laid out in the same order as `c_function_ast`: this is
 * taken advantage of.
 */
struct c_operator_ast {
  c_ast_t        *ret_ast;              ///< Return type.
  c_ast_list_t    param_ast_list;       ///< Operator parameter(s), if any.

  /**
   * Bitwise-or of flags specifying whether the user specified an operator as a
   * member, non-member, or neither.  This is a mostly a subset of <code>\ref
   * c_operator.flags</code> except this can be #C_OP_UNSPECIFIED.
   *
   * @sa #C_OP_UNSPECIFIED
   * @sa #C_OP_MEMBER
   * @sa #C_OP_NON_MEMBER
   * @sa #C_OP_MASK_OVERLOAD
   */
  unsigned        flags;

  c_oper_id_t     oper_id;              ///< Which operator it is.
};

/**
 * AST node for a C++ pointer-to-member of a class.
 *
 * @note Members are laid out in the same order as `c_ecsu_ast`: this is taken
 * advantage of.
 */
struct c_ptr_mbr_ast {
  c_ast_t        *of_ast;               ///< Member type.
  c_sname_t       class_sname;          ///< When a member function; or empty.
};

/**
 * AST node for a C/C++ pointer, or a C++ reference or rvalue reference.
 */
struct c_ptr_ref_ast {
  c_ast_t        *to_ast;               ///< What it's a pointer/reference to.
};

/**
 * AST node for a C/C++ `typedef`.
 *
 * @note Even though <code>%c_typedef_ast</code> has an AST pointer as its
 * first `struct` member, it is _not_ a parent "of" the underlying type, but
 * instead a synonym "for" it; hence, it's _not_ included in #K_ANY_PARENT.
 *
 * @note C++ `using` declarations are stored as their equivalent `typedef`
 * declarations.
 */
struct c_typedef_ast {
  c_ast_t const  *for_ast;              ///< What it's a `typedef` for.
  c_bit_width_t   bit_width;            ///< Bit-field width when &gt; 0.
};

/**
 * AST node for a C++ user-defined conversion operator.
 */
struct c_udef_conv_ast {
  c_ast_t        *conv_ast;             ///< Conversion type.
};

/**
 * AST node for a C++ user-defined literal.
 *
 * @note Members are laid out in the same order as `c_function_ast`: this is
 * taken advantage of.
 */
struct c_udef_lit_ast {
  c_ast_t        *ret_ast;              ///< Return type.
  c_ast_list_t    param_ast_list;       ///< Literal parameter(s).
};

/**
 * AST node for a parsed C/C++ declaration.
 */
struct c_ast {
  c_alignas_t           align;          ///< Alignment, if any.
  c_ast_depth_t         depth;          ///< How many `()` deep.
  c_ast_kind_t          kind;           ///< AST kind.
  c_cast_kind_t         cast_kind;      ///< Cast kind, if any.
  c_loc_t               loc;            ///< Source location.
  c_sname_t             sname;          ///< Scoped name, if any.
  c_type_t              type;           ///< Type, if any.
  c_ast_t              *parent_ast;     ///< Parent AST node, if any.
  c_ast_id_t            unique_id;      ///< Unique id (starts at 1).

  union {
    c_parent_ast_t      parent;         ///< "Parent" member(s).
    c_array_ast_t       array;          ///< Array member(s).
    c_apple_block_ast_t block;          ///< Block member(s).
    c_builtin_ast_t     builtin;        ///< Built-in type member(s).
    c_constructor_ast_t constructor;    ///< Constructor member(s).
    // nothing needed for K_DESTRUCTOR
    c_ecsu_ast_t        ecsu;           ///< `enum`, `class`, `struct`, `union`
    c_function_ast_t    func;           ///< Function member(s).
    // nothing needed for K_NAME
    c_operator_ast_t    oper;           ///< Operator member(s).
    c_ptr_mbr_ast_t     ptr_mbr;        ///< Pointer-to-member member(s).
    c_ptr_ref_ast_t     ptr_ref;        ///< Pointer or reference member(s).
    c_typedef_ast_t     tdef;           ///< `typedef` member(s).
    c_udef_conv_ast_t   udef_conv;      ///< User-defined conversion member(s).
    c_udef_lit_ast_t    udef_lit;       ///< User-defined literal member(s).
    // nothing needed for K_VARIADIC
  } as;                                 ///< Union discriminator.
};

/** @} */

////////// extern functions ///////////////////////////////////////////////////

/**
 * @defgroup ast-functions-group AST Functions
 * Functions for accessing and manipulating AST nodes.
 * @{
 */

/**
 * Cleans up all AST data.
 *
 * @remarks Currently, this only checks that the number of AST nodes freed
 * equals the number allocated.
 */
void c_ast_cleanup( void );

/**
 * Duplicates the entire AST starting at \a ast.
 *
 * @param ast The AST to duplicate.
 * @param ast_list If not NULL, the duplicated AST nodes are appended to the
 * list.
 * @return Returns the duplicated AST.
 *
 * @sa c_ast_free()
 * @sa c_ast_new()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_dup( c_ast_t const *ast, c_ast_list_t *ast_list );

/**
 * Checks whether two ASTs are equivalent, i.e., represent the same type.
 *
 * @param i_ast The first AST.
 * @param j_ast The second AST.
 * @return Returns `true` only if the two ASTs are equivalent.
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_equiv( c_ast_t const *i_ast, c_ast_t const *j_ast );

/**
 * Frees all the memory used by \a ast including \a ast itself.
 *
 * @param ast The AST to free.  If NULL, does nothing.
 *
 * @note Even though \a ast invariably is part of a larger abstract syntax
 * _tree_, this function frees _only_ \a ast and _not_ any child AST node \a
 * ast may have.  Hence to free all AST nodes, they all be kept track of
 * independently via some other data structure, e.g., a <code>\ref
 * c_ast_list_t</code>.
 *
 * @sa c_ast_dup()
 * @sa c_ast_list_free()
 * @sa c_ast_new()
 */
void c_ast_free( c_ast_t *ast );

/**
 * Checks whether \a ast is an AST of one of \a kinds.
 *
 * @param ast The AST to check.
 * @param kinds The bitwise-or of the kinds(s) \a ast can be.
 * @return Returns `true` only if \a ast is one of \a kinds.
 *
 * @sa c_ast_is_parent()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
bool c_ast_is_kind_any( c_ast_t const *ast, c_ast_kind_t kinds ) {
  return (ast->kind & kinds) != 0;
}

/**
 * Checks whether \a ast is an "orphan," that is: either has no parent AST or
 * its parent no longer points to \a ast.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is an orphan.
 *
 * @sa c_ast_is_parent()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
bool c_ast_is_orphan( c_ast_t const *ast ) {
  return ast->parent_ast == NULL || ast->parent_ast->as.parent.of_ast != ast;
}

/**
 * Checks whether \a ast is a parent node.
 *
 * @param ast The AST to check.  If NULL, does nothing.
 * @return Returns `true` only if it is.
 *
 * @sa c_ast_is_kind_any()
 * @sa c_ast_is_orphan()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast != NULL && c_ast_is_kind_any( ast, K_ANY_PARENT );
}

/**
 * Appends \a name to the name of \a ast.
 *
 * @param ast The AST to append to the name of.
 * @param name The name to append.  Ownership is taken.
 *
 * @sa c_ast_append_sname()
 * @sa c_ast_prepend_sname()
 * @sa c_ast_set_name()
 */
C_AST_INLINE
void c_ast_append_name( c_ast_t *ast, char *name ) {
  c_sname_append_name( &ast->sname, name );
}

/**
 * Appends \a sname to the name of \a ast.
 *
 * @param ast The AST to append to the name of.
 * @param sname The scoped name to append.  It is cleared.
 *
 * @sa c_ast_append_name()
 * @sa c_ast_prepend_sname()
 * @sa c_ast_set_name()
 * @sa c_ast_set_sname()
 */
C_AST_INLINE
void c_ast_append_sname( c_ast_t *ast, c_sname_t *sname ) {
  c_sname_append_sname( &ast->sname, sname );
}

/**
 * Gets the fully scoped name of \a ast.
 *
 * @param ast The AST to get the scoped name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_ast_name_atr()
 * @sa c_ast_local_name()
 * @sa c_ast_scope_name()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
char const* c_ast_full_name( c_ast_t const *ast ) {
  return c_sname_full_name( &ast->sname );
}

/**
 * Frees only the list nodes of \a list but _not_ \a list itself.
 *
 * @param list The AST list to free the nodes of.
 *
 * @sa c_ast_free()
 */
void c_ast_list_free( c_ast_list_t *list );

/**
 * Gets the local (last) name of \a ast, e.g., the local name of `S::T::x` is
 * `x`.
 *
 * @param ast The AST to get the local name of.
 * @return Returns said name or the empty string if the name is empty.
 *
 * @sa c_ast_full_name()
 * @sa c_ast_name_atr()
 * @sa c_ast_scope_name()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
char const* c_ast_local_name( c_ast_t const *ast ) {
  return c_sname_local_name( &ast->sname );
}

/**
 * Gets the scope-type of the name of \a ast (which is the type of the
 * innermost scope).
 *
 * @param ast The AST node to get the scope-type of the name of.
 * @return Returns the scope-type.
 *
 * @sa c_ast_set_local_type()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
c_type_t const* c_ast_local_type( c_ast_t const *ast ) {
  return c_sname_local_type( &ast->sname );
}

/**
 * Gets the name at \a roffset of \a ast.
 *
 * @param ast The AST to get the name at \a roffset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a roffset or the empty string if \a roffset
 * &gt;= c_ast_name_count().
 *
 * @sa c_ast_full_name()
 * @sa c_ast_local_name()
 * @sa c_ast_scope_name()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
char const* c_ast_name_atr( c_ast_t const *ast, size_t roffset ) {
  return c_sname_name_atr( &ast->sname, roffset );
}

/**
 * Gets the number of names of \a ast, e.g., `S::T::x` is 3.
 *
 * @param ast The AST to get the number of names of.
 * @return Returns said number of names.
 *
 * @note This is named "count" rather than "len" to avoid misinterpretation
 * that "len" would be the total length of the strings and `::` separators.
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
size_t c_ast_name_count( c_ast_t const *ast ) {
  return c_sname_count( &ast->sname );
}

/**
 * Duplicates the name of \a ast.
 *
 * @param ast The AST to duplicate the name of.
 * @return Returns the name of \a ast duplicated.  The caller is responsible
 * for calling c_sname_free() on it.
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
c_sname_t c_ast_name_dup( c_ast_t const *ast ) {
  return c_sname_dup( &ast->sname );
}

/**
 * Checks whether the name of \a ast is empty.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if the name of \a ast is empty.
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
bool c_ast_name_empty( c_ast_t const *ast ) {
  return c_sname_empty( &ast->sname );
}

/**
 * Creates a new AST node.
 *
 * @param kind The kind of AST to create.
 * @param depth How deep within `()` it is.
 * @param loc A pointer to the token location data.
 * @param ast_list If not NULL, the new AST is appended to the list.
 * @return Returns a pointer to a new AST.
 *
 * @sa c_ast_dup()
 * @sa c_ast_free()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_new( c_ast_kind_t kind, c_ast_depth_t depth,
                    c_loc_t const *loc, c_ast_list_t *ast_list );

/**
 * Convenience function for getting function-like parameters.
 *
 * @param ast The AST to get the parameters of.
 * @return Returns a pointer to the first parameter or NULL if none.
 *
 * @sa c_ast_params_count()
 * @sa c_param_ast()
 * @sa #FOREACH_PARAM
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
c_ast_param_t const* c_ast_params( c_ast_t const *ast ) {
  return ast->as.func.param_ast_list.head;
}

/**
 * Convenience macro for iterating over all function-like parameters.
 *
 * @param VAR The `c_ast_param_t` loop variable.
 * @param AST The AST to iterate the parameters of.
 *
 * @sa c_ast_params()
 */
#define FOREACH_PARAM(VAR,AST) \
  FOREACH_SLIST( VAR, &(AST)->as.func.param_ast_list, NULL )

/**
 * Convenience function for getting the number of function-like parameters.
 *
 * @param ast The AST to get the number of parameters of.
 * @return Returns said number of parameters.
 *
 * @sa c_ast_params()
 * @sa c_ast_params_count()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
size_t c_ast_params_count( c_ast_t const *ast ) {
  return slist_len( &ast->as.func.param_ast_list );
}

/**
 * Checks whether the parent AST of \a ast (if any) is \a kind.
 *
 * @param ast The AST to check the parent of.
 * @param kind The kind to check for.
 * @return Returns `true` only if the parent of \a ast is \a kind.
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
bool c_ast_parent_is_kind( c_ast_t const *ast, c_ast_kind_t kind ) {
  return ast->parent_ast != NULL && ast->parent_ast->kind == kind;
}

/**
 * Prepends \a sname to the name of \a ast.
 *
 * @param ast The AST to prepend to the name of.
 * @param sname The scoped name to prepend.  It is cleared.
 *
 * @sa c_ast_append_name()
 * @sa c_ast_append_sname()
 * @sa c_ast_set_name()
 * @sa c_ast_set_sname()
 */
C_AST_INLINE
void c_ast_prepend_sname( c_ast_t *ast, c_sname_t *sname ) {
  c_sname_prepend_sname( &ast->sname, sname );
}

/**
 * Gets the scope name of \a ast in C++ form.
 *
 * @param ast The AST to get the scope name of.
 * @return Returns said name or the empty string if \a ast doesn't have a scope
 * name.
 *
 * @sa c_ast_full_name()
 * @sa c_ast_local_name()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
char const* c_ast_scope_name( c_ast_t const *ast ) {
  return c_sname_scope_name( &ast->sname );
}

/**
 * Sets the scope-type of the name of \a ast (which is the type of the
 * innermost scope).
 *
 * @param ast The AST to set the type of the name of.
 * @param type The scope-type.
 *
 * @sa c_ast_local_type()
 */
C_AST_INLINE
void c_ast_set_local_type( c_ast_t *ast, c_type_t const *type ) {
  c_sname_set_local_type( &ast->sname, type );
}

/**
 * Sets the name of \a ast.
 *
 * @param ast The AST node to set the name of.
 * @param name The name to set.  Ownership is taken.
 *
 * @sa c_ast_append_name()
 * @sa c_ast_append_sname()
 * @sa c_ast_prepend_sname()
 * @sa c_ast_set_sname()
 */
void c_ast_set_name( c_ast_t *ast, char *name );

/**
 * Sets the two-way pointer links between parent/child AST nodes.
 *
 * @param child_ast The "child" AST node to set the parent of.
 * @param parent_ast The "parent" AST node whose child node is set.
 */
void c_ast_set_parent( c_ast_t *child_ast, c_ast_t *parent_ast );

/**
 * Sets the name of \a ast.
 *
 * @param ast The AST node to set the name of.
 * @param sname The scoped name to set.  It is cleared.
 *
 * @sa c_ast_append_name()
 * @sa c_ast_append_sname()
 * @sa c_ast_prepend_sname()
 * @sa c_ast_set_name()
 */
void c_ast_set_sname( c_ast_t *ast, c_sname_t *sname );

/**
 * Does a depth-first, post-order traversal of an AST.
 *
 * @param ast The AST to start from.  If NULL, does nothing.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the AST the visitor stopped on or NULL.
 *
 * @note Function-like parameters are _not_ traversed into.  They're considered
 * distinct ASTs.
 */
PJL_NOWARN_UNUSED_RESULT
c_ast_t* c_ast_visit( c_ast_t *ast, c_visit_dir_t dir, c_ast_visitor_t visitor,
                      c_ast_visitor_data_t data );

/**
 * Convenience function to get the AST given a `c_ast_param_t`.
 *
 * @param param A pointer to a `c_ast_param_t`.
 * @return Returns a pointer to the AST.
 *
 * @sa c_ast_params()
 * @sa c_ast_params_count()
 */
C_AST_INLINE PJL_WARN_UNUSED_RESULT
c_ast_t const* c_param_ast( c_ast_param_t const *param ) {
  return param->data;
}

/** @} */

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_H */
/* vim:set et sw=2 ts=2: */
