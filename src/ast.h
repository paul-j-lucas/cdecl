/*
**      cdecl -- C gibberish translator
**      src/ast.h
*/

#ifndef cdecl_ast_H
#define cdecl_ast_H

/**
 * @file
 * Contains types to represent an Abstract Syntax Tree (AST) for parsed C/C++
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
 * Kinds of AST nodes.
 *
 * A given AST node may only have a single kind and \e not be a bitwise-or of
 * kinds.  However, a bitwise-or of kinds may be used to test whether a given
 * AST node is any \e one of those kinds.
 */
enum c_kind {
  K_NONE                    = 0x0001,
  K_BUILTIN                 = 0x0002,   // void, char, int, etc.
  K_ENUM_CLASS_STRUCT_UNION = 0x0004,
  K_NAME                    = 0x0008,   // typeless function argument in K&R C
  // "parent" kinds
  K_ARRAY                   = 0x0010,
  K_BLOCK                   = 0x0020,   // Apple extension
  K_FUNCTION                = 0x0040,
  K_POINTER                 = 0x0080,
  // "parent" kinds (C++ only)
  K_POINTER_TO_MEMBER       = 0x0100,
  K_REFERENCE               = 0x0200,
};
typedef enum c_kind c_kind_t;

#define K_PARENT_MIN          K_ARRAY

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
 * @param ast The c_ast to visit.
 * @param data Optional data passed from c_ast_visit_down() or
 * c_ast_visit_up().
 * @return Returning \c true will cause traversal to stop and \a ast to be
 * returned to the called of c_ast_visit_down().
 */
typedef bool (*c_ast_visitor)( c_ast_t *ast, void *data );

/**
 * Generic "parent" AST node.
 *
 * @note All parent nodes have a c_ast pointer to what they're a parent of as
 * their first \c struct member: this is taken advantage od.
 */
struct c_parent {
  c_ast_t  *of_ast;
};

/**
 * Linked-list of AST objects.
 */
struct c_ast_list {
  c_ast_t  *head_ast;
  c_ast_t  *tail_ast;
};

/**
 * AST object for a C/C++ array.
 */
struct c_array {
  c_ast_t  *of_ast;                     // what it's an array of
  int       size;                       // or C_ARRAY_NO_SIZE
};

/**
 * AST object for a C/C++ block (Apple extension).
 */
struct c_block {
  c_ast_t      *ret_ast;                // return type
  c_ast_list_t  args;
};

/**
 * AST object for a C/C++ enum/class/struct/union type.
 */
struct c_ecsu {
  char const *ecsu_name;
};

/**
 * AST object for a C/C++ function.
 *
 * @note Members are laid out in the same order as c_block: this is taken
 * advantage of.
 */
struct c_func {
  c_ast_t      *ret_ast;                // return type
  c_ast_list_t  args;
};

/**
 * AST object for a C++ pointer-to-member of a class.
 */
struct c_ptr_mbr {
  c_ast_t    *of_ast;                   // member type
  c_type_t    qualifier;                // T_CONST, T_RESTRICT, T_VOLATILE
  char const *class_name;
};

/**
 * AST object for a C/C++ pointer or C++ reference.
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
  unsigned    id;                       // unique id (starts at 1)
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
    c_ptr_mbr_t   ptr_mbr;
    c_ptr_ref_t   ptr_ref;
  } as;

  c_ast_t    *gc_next;                  // used for garbage collection
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
 * (Currently, this checks that the number of c_ast objects freed match the
 * number allocated.)
 */
void c_ast_cleanup( void );

/**
 * Garbage collects all allocated c_ast objects.
 */
void c_ast_gc( void );

/**
 * Checks whether the given AST node is a parent node.
 *
 * @param ast The \c c_ast to check.
 * @return Returns \c true only if it is.
 */
CDECL_AST_INLINE bool c_ast_is_parent( c_ast_t const *ast ) {
  return ast->kind >= K_PARENT_MIN;
}

/**
 * Dumps the given AST as JSON (for debugging).
 *
 * @param ast The c_ast to dump.
 * @paran indent The initial indent.
 * @param key0 The initial key or null for none.
 * @param fout The FILE to dump to.
 */
void c_ast_json( c_ast_t const *ast, unsigned indent, char const *key0,
                 FILE *fout );

/**
 * Appends a c_ast onto the end of a c_ast_list.
 *
 * @param list The c_ast_list to append onto.
 * @param ast The c_ast to append.  Does nothing if null.
 */
void c_ast_list_append( c_ast_list_t *list, c_ast_t *ast );

/**
 * Dump the given c_ast_list as JSON (for debugging).
 *
 * @param list The c_ast_list to dump.
 * @param indent The initial indent.
 * @param jour The FILE to dump to.
 */
void c_ast_list_json( c_ast_list_t const *list, unsigned indent, FILE *fout );

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
 * Traverses an AST visiting each node in turn.
 *
 * @param ast The AST to begin at.
 * @param dir The direction to visit.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the c_ast the visitor stopped on or null.
 */
CDECL_AST_INLINE c_ast_t* c_ast_visit( c_ast_t *ast, v_direction_t dir,
                                       c_ast_visitor visitor, void *data ) {
  c_ast_t* c_ast_visit_down( c_ast_t*, c_ast_visitor, void* );
  c_ast_t* c_ast_visit_up( c_ast_t*, c_ast_visitor, void* );

  return dir == V_DOWN ?
    c_ast_visit_down( ast, visitor, data ) :
    c_ast_visit_up( ast, visitor, data );
}

/**
 * A c_ast_visitor function used to find an AST node of a particular kind.
 *
 * @param ast The c_ast to check.
 * @param data The bitwise-or kind(s) (cast to <code>void*</code>) to find.
 * @return Returns \c true only if the kind of \a ast is one of \a data.
 */
bool c_ast_vistor_kind( c_ast_t *ast, void *data );

/**
 * A c_ast_visitor function used to find a name.
 *
 * @param ast The c_ast to check.
 * @param data Not used.
 * @return Returns \c true only if \a ast has a name.
 */
bool c_ast_visitor_name( c_ast_t *ast, void *data );

/**
 * Gets the name of the given kind.
 *
 * @param kind The kind to get the name for.
 * @return Returns said name.
 */
char const* c_kind_name( c_kind_t kind );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_ast_H */
/* vim:set et sw=2 ts=2: */
