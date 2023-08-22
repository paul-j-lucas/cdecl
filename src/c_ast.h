/*
**      cdecl -- C gibberish translator
**      src/c_ast.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
 * Declares types to represent an _Abstract Syntax Tree_ (AST) for parsed C/C++
 * declarations as well as functions for traversing and manipulating an AST.
 *
 * @sa [Abstract Syntax Tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree)
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_kind.h"
#include "c_sname.h"
#include "c_type.h"
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_AST_H_INLINE
# define C_AST_H_INLINE _GL_INLINE
#endif /* C_AST_H_INLINE */

/// @endcond

/**
 * For c_function_ast.flags, denotes that the user didn't explicitly specify
 * either `member` or `non-member` in pseudo-English.
 *
 * @sa #C_FUNC_MEMBER
 * @sa #C_FUNC_NON_MEMBER
 */
#define C_FUNC_UNSPECIFIED        0u

/**
 * For c_function_ast.flags, denotes that the user explicitly specified
 * `member` in pseudo-English.
 *
 * @sa #C_FUNC_NON_MEMBER
 * @sa #C_FUNC_UNSPECIFIED
 */
#define C_FUNC_MEMBER             (1u << 0)

/**
 * For c_function_ast.flags, denotes that the user explicitly specified
 * `non-member` in pseudo-English.
 *
 * @sa #C_FUNC_MEMBER
 * @sa #C_FUNC_UNSPECIFIED
 */
#define C_FUNC_NON_MEMBER         (1u << 1)

/**
 * Convenience macro for iterating over all captures of a lambda AST.
 *
 * @param VAR The \ref c_capture_t loop variable.
 * @param AST The lambda AST to iterate the captures of.
 *
 * @sa c_ast_captures()
 * @sa #FOREACH_AST_FUNC_PARAM()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE_UNTIL()
 */
#define FOREACH_AST_LAMBDA_CAPTURE(VAR,AST) \
  FOREACH_AST_LAMBDA_CAPTURE_UNTIL( VAR, (AST), /*END=*/NULL )

/**
 * Convenience macro for iterating over all captures of a lambda AST up to but
 * not including \a END.
 *
 * @param VAR The \ref c_capture_t loop variable.
 * @param AST The lambda AST to iterate the captures of.
 * @param END A pointer to the capture to end before; may be NULL.
 *
 * @sa c_ast_captures()
 * @sa #FOREACH_AST_FUNC_PARAM_UNTIL()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE()
 */
#define FOREACH_AST_LAMBDA_CAPTURE_UNTIL(VAR,AST,END) \
  FOREACH_SLIST_NODE_UNTIL( VAR, &(AST)->lambda.capture_ast_list, (END) )

/**
 * Convenience macro for iterating over all parameters of a function-like AST.
 *
 * @param VAR The \ref c_param_t loop variable.
 * @param AST The function-like AST to iterate the parameters of.
 *
 * @sa c_ast_params()
 * @sa #FOREACH_AST_FUNC_PARAM_UNTIL()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE()
 */
#define FOREACH_AST_FUNC_PARAM(VAR,AST) \
  FOREACH_AST_FUNC_PARAM_UNTIL( VAR, (AST), /*END=*/NULL )

/**
 * Convenience macro for iterating over all parameters of a function-like AST
 * up to but not including \a END.
 *
 * @param VAR The \ref c_param_t loop variable.
 * @param AST The function-like AST to iterate the parameters of.
 * @param END A pointer to the parameter to end before; may be NULL.
 *
 * @sa c_ast_params()
 * @sa #FOREACH_AST_FUNC_PARAM()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE_UNTIL()
 */
#define FOREACH_AST_FUNC_PARAM_UNTIL(VAR,AST,END) \
  FOREACH_SLIST_NODE_UNTIL( VAR, &(AST)->func.param_ast_list, (END) )

#ifdef ENABLE_CDECL_DEBUG

/**
 * Decimal print conversion specifier for \ref c_ast_id_t.
 */
#define PRId_C_AST_ID_T           "%u"

#endif /* ENABLE_CDECL_DEBUG */

///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_CDECL_DEBUG
typedef unsigned  c_ast_id_t;           ///< Unique AST node ID.
#endif /* ENABLE_CDECL_DEBUG */

/**
 * The direction to traverse an AST using c_ast_visit().
 */
enum c_ast_visit_dir {
  C_VISIT_DOWN,                         ///< Root to leaves.
  C_VISIT_UP                            ///< Leaf to root.
};
typedef enum c_ast_visit_dir c_ast_visit_dir_t;

/**
 * The signature for functions passed to c_ast_visit().
 *
 * @param ast The AST to visit.
 * @param data Optional data passed to c_ast_visit().
 * @return Returning `true` will cause traversal to stop and \a ast to be
 * returned to the caller of c_ast_visit(); `false` will will cause traversal
 * to continue.
 */
typedef bool (*c_ast_visit_fn_t)( c_ast_t *ast, user_data_t data );

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup ast-nodes-group AST Nodes
 *
 * ## Layout
 *
 * The AST node `struct`s contain members specific to each \ref c_ast_kind_t.
 * In all cases where an AST node contains:
 *
 *  1. A pointer to another, that pointer is always declared first.
 *  2. A bit-width or parameter list, that is always declared second.
 *  3. A scoped name or flags, that are always declared third.
 *
 * Since all the different kinds of AST nodes are declared within a `union`,
 * these `struct` members are at the same offsets.  This makes traversing and
 * manipulating an AST easy.
 *
 * To ensure the members remain at the same offsets, a series of
 * `static_assert`s are in c_ast.c.
 *
 * ## Memory Management
 *
 * Typically, nodes of a tree data structure are freed by freeing the root node
 * followed by its child nodes in turn, recursively.  This is _not_ done for
 * AST nodes.  Instead, AST nodes created via c_ast_new() or c_ast_dup() are
 * appended to a \ref c_ast_list_t.  Nodes are later freed by traversing the
 * list and calling c_ast_free() on each node individually.  It's done this way
 * to simplify node memory management.
 *
 * As an AST is being built, sometimes \ref K_PLACEHOLDER nodes are created
 * temporarily.  Later, once an actual node is created, the \ref K_PLACEHOLDER
 * node is replaced.  Rather than freeing a \ref K_PLACEHOLDER node immediately
 * (and, for a parent node, set its "of" node to NULL just prior to being freed
 * so as not to free its child node also), it's simply left on the list.  Once
 * parsing is complete, the entire list is freed effectively "garbage
 * collecting" all nodes.
 * @{
 */

/**
 * Generic AST node for a #K_ANY_PARENT.
 *
 * @note All parent nodes have an AST pointer to what they're a parent of as
 * their first `struct` member: this is taken advantage of.
 */
struct c_parent_ast {
  c_ast_t        *of_ast;               ///< What it's a parent of.
};

/**
 * AST node for a #K_ARRAY.
 */
struct c_array_ast {
  c_ast_t        *of_ast;               ///< What it's an array of.
  c_array_kind_t  kind;                 ///< Kind.
  union {
    unsigned      size_int;             ///< For #C_ARRAY_INT_SIZE.
    char const   *size_name;            ///< For #C_ARRAY_NAMED_SIZE.
  };
};

/**
 * AST node for a #K_APPLE_BLOCK.
 *
 * @note Members are laid out in the same order as c_function_ast: this is
 * taken advantage of.
 *
 * @sa [Apple's Extensions to C](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1370.pdf)
 * @sa [Blocks Programming Topics](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks)
 */
struct c_apple_block_ast {
  c_ast_t      *ret_ast;                ///< Return type.
  c_ast_list_t  param_ast_list;         ///< Block parameter(s), if any.
};

/**
 * Generic AST node for a #K_ANY_BIT_FIELD.
 *
 * @sa c_builtin_ast
 * @sa c_enum_ast
 * @sa c_typedef_ast
 */
struct c_bit_field_ast {
  /// @cond DOXYGEN_IGNORE
  /// So bit_width is at the same offset as in c_builtin_ast, c_enum_ast, and
  /// c_typedef_ast.
  DECL_UNUSED(c_ast_t*);
  /// @endcond

  unsigned  bit_width;                  ///< Bit-field width when &gt; 0.
};

/**
 * AST node for a #K_BUILTIN.
 *
 * @note Members are laid out in the same order as c_enum_ast and
 * c_typedef_ast: this is taken advantage of.
 */
struct c_builtin_ast {
  /// @cond DOXYGEN_IGNORE
  /// So bit_width is at the same offset as in c_bit_field_ast, c_enum_ast, and
  /// c_typedef_ast.
  DECL_UNUSED(c_ast_t*);
  /// @endcond

  unsigned      bit_width;              ///< Bit-field width when &gt; 0.

  /**
   * Additional data for some \ref c_type::btids "base types" within \ref
   * c_ast::type "type".
   */
  union {
    struct {
      unsigned  width;                  ///< Width.
    } BitInt;                           ///< `_BitInt` data.
  };
};

/**
 * AST node for a #K_CAPTURE.
 */
struct c_capture_ast {
  c_capture_kind_t kind;                ///< Capture kind.
};

/**
 * AST node for a #K_CAST.
 */
struct c_cast_ast {
  c_ast_t        *to_ast;               ///< Cast type.
  c_cast_kind_t   kind;                 ///< Cast kind.
};

/**
 * AST node for a #K_CONSTRUCTOR.
 *
 * @note Members are laid out in the same order as c_function_ast: this is
 * taken advantage of.
 */
struct c_constructor_ast {
  /// @cond DOXYGEN_IGNORE
  /// Constructors don't have a return type, but we need an unused pointer so
  /// param_ast_list is at the same offset as in c_function_ast.
  DECL_UNUSED(c_ast_t*);
  /// @endcond

  c_ast_list_t  param_ast_list;         ///< Constructor parameter(s), if any.
};

/**
 * AST node for a #K_CLASS_STRUCT_UNION.
 *
 * @note Members are laid out in the same order as c_enum_ast and
 * c_ptr_mbr_ast: this is taken advantage of.
 */
struct c_csu_ast {
  /// @cond DOXYGEN_IGNORE
  /// Class, struct, and union types have neither an "of" type nor a bit width,
  /// but csu_sname needs to be at the same offset as enum_sname and
  /// class_sname.
  DECL_UNUSED(c_ast_t*);
  DECL_UNUSED(unsigned);
  /// @endcond

  c_sname_t   csu_sname;                ///< Class, struct, or union name.
};

/**
 * AST node for a #K_ENUM.
 *
 * @note Members are laid out in the same order as c_builtin_ast, c_csu_ast,
 * c_ptr_mbr_ast, and c_typedef_ast: this is taken advantage of.
 */
struct c_enum_ast {
  c_ast_t    *of_ast;                   ///< The fixed type, if any.
  unsigned    bit_width;                ///< Bit-field width when &gt; 0.
  c_sname_t   enum_sname;               ///< Enumeration name.
};

/**
 * AST node for a #K_FUNCTION.
 */
struct c_function_ast {
  c_ast_t      *ret_ast;                ///< Return type.
  c_ast_list_t  param_ast_list;         ///< Function parameter(s), if any.

  /**
   * Indicates whether the function is a member, non-member, or neither.
   *
   * @sa #C_FUNC_UNSPECIFIED
   * @sa #C_FUNC_MEMBER
   * @sa #C_FUNC_NON_MEMBER
   */
  unsigned      flags;
};

/**
 * AST node for a #K_LAMBDA.
 *
 * @note Members are laid out in the same order as c_function_ast: this is
 * taken advantage of.
 */
struct c_lambda_ast {
  c_ast_t      *ret_ast;                ///< Return type.
  c_ast_list_t  param_ast_list;         ///< Lambda parameter(s), if any.
  c_ast_list_t  capture_ast_list;       ///< Lambda captures(s), if any.
};

/**
 * AST node for a #K_OPERATOR.
 *
 * @note Members are laid out in the same order as c_function_ast: this is
 * taken advantage of.
 */
struct c_operator_ast {
  c_ast_t            *ret_ast;          ///< Return type.
  c_ast_list_t        param_ast_list;   ///< Operator parameter(s), if any.

  /**
   * Indicates whether the user specified an operator as a member, non-member,
   * or neither.
   *
   * @sa #C_OPER_UNSPECIFIED
   * @sa #C_OPER_MEMBER
   * @sa #C_OPER_NON_MEMBER
   */
  unsigned            flags;

  c_operator_t const *operator;         ///< Which operator it is.
};

/**
 * AST node for a #K_POINTER_TO_MEMBER.
 *
 * For example, a declaration like:
 *
 *      c++decl> explain int C::*p
 *      declare p as pointer to member of class C int
 *
 * has an AST like:
 *
 *      decl_c: {
 *        sname: { string: "p", scopes: "none" },
 *        kind: { value: 0x800, string: "pointer to member" },
 *        ...
 *        type: { btid: 0x4000001, ..., string: "class" },
 *        ptr_mbr: {
 *          class_sname: { string: "C", scopes: "class" },
 *          to_ast: {
 *            sname: { string: "" },
 *            kind: { value: 0x2, string: "built-in type" },
 *            ...
 *            type: { btid: 0x4001, ..., string: "int" },
 *            ...
 *          }
 *        }
 *      }
 *
 * @note Members are laid out in the same order as c_csu_ast and c_enum_ast:
 * this is taken advantage of.
 */
struct c_ptr_mbr_ast {
  c_ast_t    *to_ast;                   ///< Member type.

  /// @cond DOXYGEN_IGNORE
  /// So class_sname is at the same offset as in c_csu_ast and c_enum_ast.
  DECL_UNUSED(unsigned);
  /// @endcond

  c_sname_t   class_sname;              ///< Class pointer-to-member of.
};

/**
 * AST node for a #K_POINTER, #K_REFERENCE, or #K_RVALUE_REFERENCE.
 */
struct c_ptr_ref_ast {
  c_ast_t  *to_ast;                     ///< What it's a pointer/reference to.
};

/**
 * AST node for a #K_TYPEDEF.
 *
 * @note Even though this has an AST pointer as its first `struct` member, it
 * is _not_ a parent "of" the underlying type, but instead a synonym "for" it;
 * hence, it's _not_ included in #K_ANY_PARENT, but it is, however, included in
 * #K_ANY_REFERRER.
 *
 * @note C++ `using` declarations are stored as their equivalent `typedef`
 * declarations.
 */
struct c_typedef_ast {
  c_ast_t const  *for_ast;              ///< What it's a `typedef` for.
  unsigned        bit_width;            ///< Bit-field width when &gt; 0.
};

/**
 * AST node for a #K_USER_DEF_CONVERSION.
 */
struct c_udef_conv_ast {
  c_ast_t  *to_ast;                     ///< What it's a conversion to.
};

/**
 * AST node for a #K_USER_DEF_LITERAL.
 *
 * @note Members are laid out in the same order as c_function_ast: this is
 * taken advantage of.
 */
struct c_udef_lit_ast {
  c_ast_t      *ret_ast;                ///< Return type.
  c_ast_list_t  param_ast_list;         ///< Literal parameter(s).
};

/**
 * AST node for a parsed C/C++ declaration.
 */
struct c_ast {
  c_alignas_t           align;      ///< Alignment, if any.
  unsigned              depth;      ///< How many `()` deep.
  c_ast_kind_t          kind;       ///< AST kind.
  c_loc_t               loc;        ///< Source location.
  c_sname_t             sname;      ///< Scoped name, if any.
  c_type_t              type;       ///< Type, if any.
  c_ast_t              *parent_ast; ///< Parent AST node, if any.
#ifdef ENABLE_CDECL_DEBUG
  c_ast_id_t            unique_id;  ///< Unique id (starts at 1).
#endif /* ENABLE_CDECL_DEBUG */

  /**
   * Additional data for each \ref kind.
   */
  union {
    c_parent_ast_t      parent;     ///< #K_ANY_PARENT members.
    c_array_ast_t       array;      ///< #K_ARRAY members.
    c_bit_field_ast_t   bit_field;  ///< #K_ANY_BIT_FIELD members.
    c_apple_block_ast_t block;      ///< #K_APPLE_BLOCK members.
    c_builtin_ast_t     builtin;    ///< #K_BUILTIN members.
    c_capture_ast_t     capture;    ///< #K_CAPTURE members.
    c_cast_ast_t        cast;       ///< #K_CAST members.
    c_csu_ast_t         csu;        ///< #K_CLASS_STRUCT_UNION members.
    c_constructor_ast_t ctor;       ///< #K_CONSTRUCTOR members.
                    // nothing needed for K_DESTRUCTOR
    c_enum_ast_t        enum_;      ///< #K_ENUM members.
    c_function_ast_t    func;       ///< #K_FUNCTION members.
                    // nothing needed for K_NAME
    c_lambda_ast_t      lambda;     ///< #K_LAMBDA members.
    c_operator_ast_t    oper;       ///< #K_OPERATOR members.
    c_ptr_mbr_ast_t     ptr_mbr;    ///< #K_POINTER_TO_MEMBER members.
    c_ptr_ref_ast_t     ptr_ref;    ///< #K_POINTER or #K_ANY_REFERENCE members.
    c_typedef_ast_t     tdef;       ///< #K_TYPEDEF members.
    c_udef_conv_ast_t   udef_conv;  ///< #K_USER_DEF_CONVERSION members.
    c_udef_lit_ast_t    udef_lit;   ///< #K_USER_DEF_LITERAL members.
                    // nothing needed for K_VARIADIC
  };
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
 *
 * @sa c_ast_free()
 * @sa c_ast_new()
 */
void c_ast_cleanup( void );

/**
 * Duplicates \a ast.
 *
 * @param ast The AST to duplicate.
 * @param node_list The list to append the duplicated AST nodes onto.
 * @return Returns the duplicated AST.
 *
 * @sa c_ast_free()
 * @sa c_ast_new()
 */
NODISCARD
c_ast_t* c_ast_dup( c_ast_t const *ast, c_ast_list_t *node_list );

/**
 * Duplicates \a src_list.
 *
 * @param src_list The AST list to duplicate.  If NULL, does nothing.
 * @param node_list The list to append the duplicated AST nodes onto.
 * @return Returns the duplicated AST list.
 *
 * @sa c_ast_dup()
 * @sa c_ast_list_cleanup()
 */
NODISCARD
c_ast_list_t c_ast_list_dup( c_ast_list_t const *src_list,
                             c_ast_list_t *node_list );

/**
 * Checks whether two ASTs are equal _except_ for their names.
 *
 * @param i_ast The first AST; may be NULL.
 * @param j_ast The second AST; may be NULL.
 * @return Returns `true` only if the two ASTs are equal _except_ for their
 * names.
 *
 * @sa c_ast_list_equal()
 */
NODISCARD
bool c_ast_equal( c_ast_t const *i_ast, c_ast_t const *j_ast );

/**
 * Frees all memory used by \a ast _including_ \a ast itself.
 *
 * @param ast The AST to free.  If NULL, does nothing.
 *
 * @note Even though \a ast invariably is part of a larger abstract syntax
 * tree, this function frees _only_ \a ast and _not_ any child AST node \a ast
 * may have.  Hence to free all AST nodes, they all be kept track of
 * independently via some other data structure, e.g., a \ref c_ast_list_t.
 *
 * @sa c_ast_cleanup()
 * @sa c_ast_dup()
 * @sa c_ast_list_cleanup()
 * @sa c_ast_new()
 */
void c_ast_free( c_ast_t *ast );

/**
 * Checks whether \a ast is an "orphan," that is: either has no parent AST or
 * its parent no longer points to \a ast.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is an orphan.
 *
 * @sa c_ast_is_parent()
 * @sa c_ast_is_referrer()
 */
NODISCARD C_AST_H_INLINE
bool c_ast_is_orphan( c_ast_t const *ast ) {
  return ast->parent_ast == NULL || ast->parent_ast->parent.of_ast != ast;
}

/**
 * Checks whether \a ast is a #K_ANY_PARENT.
 *
 * @param ast The AST to check; may be NULL.
 * @return Returns `true` only if it is.
 *
 * @sa c_ast_is_orphan()
 * @sa c_ast_is_referrer()
 */
NODISCARD C_AST_H_INLINE
bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast != NULL && (ast->kind & K_ANY_PARENT) != 0;
}

/**
 * Checks whether \a ast is a #K_ANY_REFERRER.
 *
 * @param ast The AST to check; may be NULL.
 * @return Returns `true` only if it is.
 *
 * @sa c_ast_is_orphan()
 * @sa c_ast_is_parent()
 */
NODISCARD C_AST_H_INLINE
bool c_ast_is_referrer( c_ast_t const *ast ) {
  return ast != NULL && (ast->kind & K_ANY_REFERRER) != 0;
}

/**
 * Cleans-up \a list by freeing only its nodes but _not_ \a list itself.
 *
 * @param list The AST list to free the list nodes of.
 *
 * @sa c_ast_free()
 */
void c_ast_list_cleanup( c_ast_list_t *list );

/**
 * Checks whether to AST lists are equals _except_ for AST node names.
 *
 * @param i_list The first AST list.
 * @param j_list The second AST list.
 * @return Returns `true` only if the to AST lists are equal _except_ for AST
 * node names.
 *
 * @sa c_ast_equal()
 */
NODISCARD
bool c_ast_list_equal( c_ast_list_t const *i_list,
                       c_ast_list_t const *j_list );

/**
 * Creates a new AST node.
 *
 * @param kind The kind of AST to create.
 * @param depth How deep within `()` it is.
 * @param loc A pointer to the token location data.
 * @param node_list The list to append the new AST node onto.
 * @return Returns a pointer to a new AST.
 *
 * @sa c_ast_cleanup()
 * @sa c_ast_dup()
 * @sa c_ast_free()
 */
NODISCARD
c_ast_t* c_ast_new( c_ast_kind_t kind, unsigned depth, c_loc_t const *loc,
                    c_ast_list_t *node_list );

/**
 * Convenience function for getting lambda captures.
 *
 * @param ast The lambda AST to get the captures of.
 * @return Returns a pointer to the first capture or NULL if none.
 *
 * @sa c_ast_captures_count()
 * @sa c_ast_params()
 * @sa c_capture_ast()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE()
 */
NODISCARD C_AST_H_INLINE
c_capture_t const* c_ast_captures( c_ast_t const *ast ) {
  assert( ast->kind == K_LAMBDA );
  return ast->lambda.capture_ast_list.head;
}

/**
 * Convenience function for getting the number of function-like parameters.
 *
 * @param ast The function-like AST to get the number of parameters of.
 * @return Returns said number of parameters.
 *
 * @sa c_ast_captures()
 * @sa c_ast_params_count()
 * @sa c_capture_ast()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE()
 */
NODISCARD C_AST_H_INLINE
size_t c_ast_captures_count( c_ast_t const *ast ) {
  assert( ast->kind == K_LAMBDA );
  return slist_len( &ast->lambda.capture_ast_list );
}

/**
 * Convenience function for getting function-like parameters.
 *
 * @param ast The function-like AST to get the parameters of.
 * @return Returns a pointer to the first parameter or NULL if none.
 *
 * @sa c_ast_captures()
 * @sa c_ast_params_count()
 * @sa c_param_ast()
 * @sa #FOREACH_AST_FUNC_PARAM()
 * @sa #FOREACH_AST_FUNC_PARAM_UNTIL()
 */
NODISCARD C_AST_H_INLINE
c_param_t const* c_ast_params( c_ast_t const *ast ) {
  assert( (ast->kind & K_ANY_FUNCTION_LIKE) != 0 );
  return ast->func.param_ast_list.head;
}

/**
 * Convenience function for getting the number of function-like parameters.
 *
 * @param ast The function-like AST to get the number of parameters of.
 * @return Returns said number of parameters.
 *
 * @sa c_ast_captures_count()
 * @sa c_ast_params()
 * @sa c_param_ast()
 * @sa #FOREACH_AST_FUNC_PARAM()
 * @sa #FOREACH_AST_FUNC_PARAM_UNTIL()
 */
NODISCARD C_AST_H_INLINE
size_t c_ast_params_count( c_ast_t const *ast ) {
  assert( (ast->kind & K_ANY_FUNCTION_LIKE) != 0 );
  return slist_len( &ast->func.param_ast_list );
}

/**
 * Sets the two-way pointer links between parent/child AST nodes.
 *
 * @param child_ast The "child" AST node to set the parent of; may be NULL.  If
 * it already has a parent, it's overwritten.
 * @param parent_ast The "parent" AST node to set the child of; may be NULL.
 * If it already has a child, it's overwritten.
 */
void c_ast_set_parent( c_ast_t *child_ast, c_ast_t *parent_ast );

/**
 * Does a pre-order traversal of an AST.
 *
 * @param ast The AST to start from.  If NULL, does nothing.
 * @param dir The direction to visit.
 * @param visit_fn The visitor function to use.
 * @param data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the AST the visitor stopped on or NULL.
 *
 * @note Function-like parameters are _not_ traversed into.  They're considered
 * distinct ASTs.
 */
PJL_DISCARD
c_ast_t* c_ast_visit( c_ast_t *ast, c_ast_visit_dir_t dir,
                      c_ast_visit_fn_t visit_fn, user_data_t data );

/**
 * Convenience function to get the AST given a \ref c_capture_t.
 *
 * @param capture A pointer to a \ref c_capture_t or NULL.
 * @return Returns a pointer to the AST or NULL if \a capture is NULL.
 *
 * @sa c_ast_captures()
 * @sa c_ast_captures_count()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE()
 * @sa #FOREACH_AST_LAMBDA_CAPTURE_UNTIL()
 */
NODISCARD C_AST_H_INLINE
c_ast_t const* c_capture_ast( c_capture_t const *capture ) {
  return capture != NULL ? capture->data : NULL;
}

/**
 * Convenience function to get the AST given a \ref c_param_t.
 *
 * @param param A pointer to a \ref c_param_t or NULL.
 * @return Returns a pointer to the AST or NULL if \a param is NULL.
 *
 * @sa c_ast_params()
 * @sa c_ast_params_count()
 * @sa c_capture_ast()
 * @sa #FOREACH_AST_FUNC_PARAM()
 * @sa #FOREACH_AST_FUNC_PARAM_UNTIL()
 */
NODISCARD C_AST_H_INLINE
c_ast_t const* c_param_ast( c_param_t const *param ) {
  return param != NULL ? param->data : NULL;
}

/** @} */

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_H */
/* vim:set et sw=2 ts=2: */
