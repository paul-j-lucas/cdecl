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

typedef struct c_array    c_array_t;
typedef struct c_ast      c_ast_t;
typedef struct c_ast_list c_ast_list_t;
typedef struct c_func     c_func_t;
typedef enum   c_kind     c_kind_t;
typedef struct c_member   c_member_t;
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
  K_FUNCTION,
  K_MEMBER,                             // C++ class data member
  K_NAME,
  K_POINTER,
  K_REFERENCE,
  K_STRUCT,                             // or union or C++ class
};

/**
 * Linked-list of AST objects.
 */
struct c_ast_list {
  c_ast_t  *head;
  c_ast_t  *tail;
};

/**
 * AST object for a C/C++ array.
 */
struct c_array {
  int       size;
  c_ast_t  *of_ast;
};

/**
 * AST object for a C/C++ function.
 */
struct c_func {
  c_ast_t      *ret_ast;
  c_ast_list_t  args;
};

/**
 * AST object for a C/C++ pointer or C++ reference.
 */
struct c_ptr_ref {
  c_type_t  qualifier;                  // T_CONST, T_RESTRICT, T_VOLATILE
  c_ast_t  *to_ast;
};

/**
 * AST object for a C++ pointer-to-class-member.
 */
struct c_member {
  c_type_t    qualifier;                // T_CONST, T_RESTRICT, T_VOLATILE
  char const *class_name;
  c_ast_t    *of_ast;
};

/**
 * AST object.
 */
struct c_ast {
  c_kind_t  kind;
  c_ast_t  *next;

  union {
    c_array_t     array;                // K_ARRAY
    c_type_t      type;                 // K_BUILTIN
    c_func_t      func;                 // K_FUNCTION
    c_member_t    member;               // K_MEMBER
    char const   *name;                 // K_NAME
    c_ptr_ref_t   ptr_ref;              // K_POINTER or K_REFERENCE
  } as;
};

////////// extern functions ///////////////////////////////////////////////////

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
 * Creates a new c_ast.
 *
 * @param kind The kind of object to create.
 */
c_ast_t* c_ast_new( c_kind_t kind );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_ast_H */
/* vim:set et sw=2 ts=2: */
