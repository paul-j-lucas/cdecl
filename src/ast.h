/*
**      cdecl -- C gibberish translator
**      src/ast.h
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

#ifndef cdecl_ast_H
#define cdecl_ast_H

/**
 * @file
 * Declares types to represent an Abstract Syntax Tree (AST) for parsed C/C++
 * declarations as well as functions for traversing and manipulating an AST.
 *
 * In all cases where an AST node contains a pointer to another, that pointer
 * is always declared first.  Since all the different kinds of AST nodes are
 * declared within a \c union, all the pointers are at the same offset.  This
 * makes traversing the AST easy.
 *
 * Similar same-offset tricks are done for other \c struct members as well.
 */

// local
#include "config.h"                     /* must go first */
#include "kinds.h"
#include "types.h"
#include "util.h"

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_AST_INLINE
# define CDECL_AST_INLINE _GL_INLINE
#endif /* CDECL_AST_INLINE */

#define C_ARRAY_NO_SIZE   (-1)          /* for array[] */

///////////////////////////////////////////////////////////////////////////////

/**
 * The direction to traverse an AST using c_ast_visit().
 */
enum v_direction {
  V_DOWN,                               // root -> leaves
  V_UP                                  // leaf -> root
};
typedef enum v_direction v_direction_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct c_ast      c_ast_t;
typedef unsigned          c_ast_id_t;
typedef struct c_ast_list c_ast_list_t;
typedef struct c_array    c_array_t;
typedef struct c_block    c_block_t;
typedef struct c_builtin  c_builtin_t;
typedef struct c_ecsu     c_ecsu_t;
typedef struct c_func     c_func_t;
typedef struct c_parent   c_parent_t;
typedef struct c_ptr_mbr  c_ptr_mbr_t;
typedef struct c_ptr_ref  c_ptr_ref_t;

/**
 * The signature for functions passed to c_ast_visit_down() or
 * c_ast_visit_up().
 *
 * @param ast The AST node to visit.
 * @param data Optional data passed from c_ast_visit_down() or
 * c_ast_visit_up().
 * @return Returning \c true will cause traversal to stop and \a ast to be
 * returned to the caller of c_ast_visit_down() or c_ast_visit_up().
 */
typedef bool (*c_ast_visitor_t)( c_ast_t *ast, void *data );

/**
 * Generic "parent" AST node.
 *
 * @note All parent nodes have a c_ast pointer to what they're a parent of as
 * their first \c struct member: this is taken advantage of.
 */
struct c_parent {
  c_ast_t  *of_ast;
};

/**
 * Linked-list of AST nodes.
 */
struct c_ast_list {
  c_ast_t  *head_ast;
  c_ast_t  *tail_ast;
};

/**
 * AST node for a C/C++ array.
 */
struct c_array {
  c_ast_t  *of_ast;                     // what it's an array of
  int       size;                       // or C_ARRAY_NO_SIZE
};

/**
 * AST node for a C/C++ block (Apple extension).
 */
struct c_block {
  c_ast_t      *ret_ast;                // return type
  c_ast_list_t  args;
};

/**
 * AST node for a C/C++ enum/class/struct/union type.
 */
struct c_ecsu {
  char const *ecsu_name;
};

/**
 * AST node for a C/C++ function.
 *
 * @note Members are laid out in the same order as c_block: this is taken
 * advantage of.
 */
struct c_func {
  c_ast_t      *ret_ast;                // return type
  c_ast_list_t  args;
};

/**
 * AST node for a C++ pointer-to-member of a class.
 */
struct c_ptr_mbr {
  c_ast_t    *of_ast;                   // member type
  c_type_t    qualifier;                // T_CONST, T_RESTRICT, T_VOLATILE
  char const *class_name;               // when a member function; or null
};

/**
 * AST node for a C/C++ pointer, or a C++ reference or rvalue reference.
 *
 * @note Members are laid out in the same order as c_ptr_mbr: this is taken
 * advantage of.)
 */
struct c_ptr_ref {
  c_ast_t  *to_ast;                     // what it's a pointer or reference to
  c_type_t  qualifier;                  // T_CONST, T_RESTRICT, T_VOLATILE
};

/**
 * AST node for a parsed C/C++ declaration.
 */
struct c_ast {
  c_ast_t    *next;                     // must be first struct member
  unsigned    depth;                    // how many () deep
  c_ast_id_t  id;                       // unique id (starts at 1)
  c_kind_t    kind;
  char const *name;
  c_type_t    type;
  c_ast_t    *parent;
  YYLTYPE     loc;

  union {
    c_parent_t    parent;
    c_array_t     array;
    c_block_t     block;
    // nothing needed for K_BUILTIN
    c_ecsu_t      ecsu;
    c_func_t      func;
    // nothing needed for K_NAME
    c_ptr_mbr_t   ptr_mbr;
    c_ptr_ref_t   ptr_ref;
    // nothing needed for K_VARIADIC
  } as;

  c_ast_t    *gc_next;                  // used for garbage collection
};

/**
 * A pair of c_ast pointers used as one of the synthesized attribute types in
 * the parser.
 */
struct c_ast_pair {
  /**
   * A pointer to the AST being built.
   */
  c_ast_t *ast;

  /**
   * Array and function (or block) declarations need a separate AST pointer
   * that points to their \c of_ast or \c ret_ast (respectively) to be the
   * "target" of subsequent additions to the AST.
   */
  c_ast_t *target_ast;
};
typedef struct c_ast_pair c_ast_pair_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Convenience function for getting block/function arguments.
 *
 * @param ast The c_ast to get the arguments of.
 * @return Returns a pointe to the first argument or null if none.
 */
CDECL_AST_INLINE c_ast_t const* c_ast_args( c_ast_t const *ast ) {
  return ast->as.func.args.head_ast;
}

/**
 * Cleans-up AST data.
 * (Currently, this checks that the number of c_ast nodes freed match the
 * number allocated.)
 */
void c_ast_cleanup( void );

/**
 * Garbage collects all allocated c_ast nodes.
 */
void c_ast_gc( void );

/**
 * Checks whether the given AST node is a parent node.
 *
 * @param ast The \c c_ast to check.  May be null.
 * @return Returns \c true only if it is.
 */
CDECL_AST_INLINE bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast && c_kind_is_parent( ast->kind );
}

/**
 * Appends a c_ast onto the end of a c_ast_list.
 *
 * @param list The c_ast_list to append onto.
 * @param ast The c_ast to append.  Does nothing if null.
 */
void c_ast_list_append( c_ast_list_t *list, c_ast_t *ast );

/**
 * Creates a new c_ast.
 *
 * @param kind The kind of c_ast to create.
 * @param depth How deep within () it is.
 * @param loc A pointer to the token location data.
 */
c_ast_t* c_ast_new( c_kind_t kind, unsigned depth, YYLTYPE const *loc );

/**
 * Gets the root AST node of \a ast.
 *
 * @param ast the AST node to get the root
 * @return Returns said AST node.
 */
c_ast_t* c_ast_root( c_ast_t *ast );

/**
 * Sets the two-way pointer links between parent/child AST nodes.
 *
 * @param child The "child" AST node to set the parent of.
 * @Param parent The "parent" AST node whose child node is set.
 */
void c_ast_set_parent( c_ast_t *child, c_ast_t *parent );

/**
 * Does a depth-first, post-order traversal of an AST.
 *
 * @param ast The AST to begin at.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the c_ast the visitor stopped on or null.
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
 * Does a depth-first, post-order traversal of an AST looking for an AST node
 * that satisfies the visitor.
 *
 * @param ast The AST to begin at.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns \c true only if \a visitor ever returned \c true.
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

/**
 * A visitor function used to find an AST node of a particular kind.
 *
 * @param ast The c_ast to check.
 * @param data The bitwise-or kind(s) (cast to <code>void*</code>) to find.
 * @return Returns \c true only if the kind of \a ast is one of \a data.
 */
bool c_ast_vistor_kind( c_ast_t *ast, void *data );

/**
 * A visitor function used to find a name.
 *
 * @param ast The c_ast to check.
 * @param data Not used.
 * @return Returns \c true only if \a ast has a name.
 */
bool c_ast_visitor_name( c_ast_t *ast, void *data );

/**
 * A visitor function used to find a type.
 *
 * @param ast The c_ast to check.
 * @param data The bitwise-or type(s) (cast to <code>void*</code>) to find.
 * @return Returns \c true only if the type of \a ast is one of \a data.
 */
bool c_ast_vistor_type( c_ast_t *ast, void *data );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_ast_H */
/* vim:set et sw=2 ts=2: */
