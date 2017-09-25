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

#ifndef cdecl_c_ast_H
#define cdecl_c_ast_H

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
#include "c_kind.h"
#include "c_type.h"
#include "typedefs.h"
#include "util.h"

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_AST_INLINE
# define CDECL_AST_INLINE _GL_INLINE
#endif /* CDECL_AST_INLINE */

#define C_ARRAY_SIZE_NONE     (-1)      /* for array[] */
#define C_ARRAY_SIZE_VARIABLE (-2)      /* for array[*] */

///////////////////////////////////////////////////////////////////////////////

/**
 * The direction to traverse an AST using c_ast_visit().
 */
enum v_direction {
  V_DOWN,                               // root -> leaves
  V_UP                                  // leaf -> root
};

///////////////////////////////////////////////////////////////////////////////

typedef unsigned c_ast_id_t;

/**
 * The signature for functions passed to c_ast_visit_down() or
 * c_ast_visit_up().
 *
 * @param ast The AST node to visit.
 * @param data Optional data passed to c_ast_visit_down() or c_ast_visit_up().
 * @return Returning \c true will cause traversal to stop and \a ast to be
 * returned to the caller of c_ast_visit_down() or c_ast_visit_up().
 */
typedef bool (*c_ast_visitor_t)( c_ast_t *ast, void *data );

/**
 * Linked-list of AST nodes.
 */
struct c_ast_list {
  c_ast_t  *head_ast;
  c_ast_t  *tail_ast;
};

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
 * AST node for a C/C++ array.
 */
struct c_array {
  c_ast_t  *of_ast;                     // what it's an array of
  int       size;                       // or C_ARRAY_*
  c_type_t  type;                       // e.g., array[ static const 10 ]
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
  char const *class_name;               // when a member function; or null
};

/**
 * AST node for a C/C++ pointer, or a C++ reference or rvalue reference.
 */
struct c_ptr_ref {
  c_ast_t  *to_ast;                     // what it's a pointer or reference to
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
  c_loc_t     loc;

  union {
    c_parent_t          parent;
    c_array_t           array;
    c_block_t           block;
    // nothing needed for K_BUILTIN
    c_ecsu_t            ecsu;
    c_func_t            func;
    // nothing needed for K_NAME
    c_ptr_mbr_t         ptr_mbr;
    c_ptr_ref_t         ptr_ref;
    c_typedef_t const  *c_typedef;
    // nothing needed for K_VARIADIC
  } as;

  /**
   * Every c_ast that's dynamically allocated is added to a static linked list
   * so that they all can be garbage collected in one go.  This pointer is used
   * for that list.
   *
   * This is much easier than having to free manually on parsing success or
   * rely on Bison \c \%destructor code for parsing failure.
   */
  c_ast_t *gc_next;
};

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
 * Checks whether the two ASTs are equivalent, i.e., represent the same type.
 *
 * @param ast_i The first AST.
 * @param ast_j The second AST.
 * @return Returns \c true only if the two ASTs are equivalent.
 */
bool c_ast_equiv( c_ast_t const *ast_i, c_ast_t const *ast_j );

/**
 * Frees all the memory used by the given c_ast.
 *
 * @param ast The c_ast to free.  May be NULL.
 */
void c_ast_free( c_ast_t *ast );

/**
 * Garbage collects all allocated c_ast nodes.
 */
void c_ast_gc( void );

/**
 * Releases all the current c_ast nodes so they will not be garbage collected.
 */
void c_ast_gc_release( void );

/**
 * Checks whether the given AST node is a parent node.
 *
 * @param ast The \c c_ast to check.  May be null.
 * @return Returns \c true only if it is.
 */
CDECL_AST_INLINE bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast != NULL && c_kind_is_parent( ast->kind );
}

/**
 * Appends a c_ast onto the end of a c_ast_list.
 *
 * @param list The c_ast_list to append onto.
 * @param ast The c_ast to append.  Does nothing if null.
 */
void c_ast_list_append( c_ast_list_t *list, c_ast_t *ast );

/**
 * Frees all the memory used by the given c_ast_list.
 *
 * @param list A pointer to the list to free.  Does nothing if null.
 */
void c_ast_list_free( c_ast_list_t *list );

/**
 * Creates a new c_ast.
 *
 * @param kind The kind of c_ast to create.
 * @param depth How deep within () it is.
 * @param loc A pointer to the token location data.
 */
c_ast_t* c_ast_new( c_kind_t kind, unsigned depth, c_loc_t const *loc );

/**
 * Gets the root AST node of \a ast.
 *
 * @param ast The AST node to get the root of.
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
 * @param ast The AST to begin at.  May be null.
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

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_H */
/* vim:set et sw=2 ts=2: */
