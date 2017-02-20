/*
**      cdecl -- C gibberish translator
**      src/ast.h
*/

#ifndef cdecl_ast_H
#define cdecl_ast_H

// local
#include "types.h"

// standard
#include <stdio.h>                      /* for FILE */

/**
 * @file
 * Contains types to represent an Abstract Syntax Tree (AST) for parsed C/C++
 * declarations.
 */

///////////////////////////////////////////////////////////////////////////////

typedef struct c_ast      c_ast_t;
typedef struct c_ast_list c_ast_list_t;
typedef struct c_array    c_array_t;
typedef struct c_block    c_block_t;
typedef struct c_func     c_func_t;
typedef enum   c_kind     c_kind_t;
typedef struct c_ptr_mbr  c_ptr_mbr_t;
typedef struct c_ptr_ref  c_ptr_ref_t;

#define C_ARRAY_NO_SIZE   (-1)

/**
 * Kinds of AST nodes.
 */
enum c_kind {
  K_NONE,
  K_ARRAY,
  K_BLOCK,                              // Apple extension
  K_BUILTIN,                            // void, char, int, etc.
  K_ENUM_CLASS_STRUCT_UNION,
  K_FUNCTION,
  K_NAME,
  K_POINTER,
  K_PTR_TO_MEMBER,                      // C++ class data member
  K_REFERENCE,
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
  c_ast_t  *of_ast;
  int       size;
};

/**
 * AST object for a C/C++ block (Apple extension).
 */
struct c_block {
  c_ast_t      *ret_ast;
  c_ast_list_t  args;
  c_type_t      type;
};

/**
 * AST object for a C/C++ function.
 * (Note that the members are laid out in the same order as c_block: this is
 * taken advantage of.)
 */
struct c_func {
  c_ast_t      *ret_ast;
  c_ast_list_t  args;
};

/**
 * AST object for a C++ pointer-to-member of class.
 */
struct c_ptr_mbr {
  c_type_t    qualifier;                // T_CONST, T_RESTRICT, T_VOLATILE
  c_ast_t    *of_ast;
  char const *class_name;
};

/**
 * AST object for a C/C++ pointer or C++ reference.
 */
struct c_ptr_ref {
  c_type_t  qualifier;                  // T_CONST, T_RESTRICT, T_VOLATILE
  c_ast_t  *to_ast;
};

/**
 * AST object.
 */
struct c_ast {
  c_kind_t    kind;
  char const *name;
  c_ast_t    *next;

  union {
    c_array_t     array;
    c_block_t     block;
    c_type_t      type;
    c_func_t      func;
    c_ptr_mbr_t   ptr_mbr;
    c_ptr_ref_t   ptr_ref;
  } as;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans-up AST data.
 * (Currently, this checks that the number of c_ast object freed match the
 * number allocated.)
 */
void c_ast_cleanup( void );

/**
 * Prints the given c_ast as English.
 *
 * @param ast The c_ast to print as English.
 * @param fout The FILE to print to.
 */
void c_ast_english( c_ast_t const *ast, FILE *fout );

/**
 * Frees all the memory used by the given c_ast.
 *
 * @param ast The c_ast to free.
 */
void c_ast_free( c_ast_t *ast );

/**
 * Gets the name from the given c_ast.
 *
 * @param ast The c_ast to get the name from.
 * @return Returns said name or null if none.
 */
char const* c_ast_name( c_ast_t const *ast );

/**
 * Creates a new c_ast.
 *
 * @param kind The kind of object to create.
 */
c_ast_t* c_ast_new( c_kind_t kind );

/**
 * Gets the name of the given kind.
 *
 * @param kind The kind to get the name for.
 * @return Returns said name.
 */
char const* c_kind_name( c_kind_t kind );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_ast_H */
/* vim:set et sw=2 ts=2: */
