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

///////////////////////////////////////////////////////////////////////////////

typedef struct c_array    c_array_t;
typedef struct c_func     c_func_t;
typedef struct c_func_arg c_func_arg_t;
typedef enum   c_kind     c_kind_t;
typedef struct c_member   c_member_t;
typedef struct c_object   c_object_t;

/**
 * TODO
 */
enum c_kind {
  K_NONE,
  K_ARRAY,
  K_BLOCK,                              // Apple extension
  K_BUILTIN,                            // char, int, etc.
  K_FUNCTION,
  K_MEMBER,                             // C++ class data member
  K_NAME,
  K_POINTER,
  K_REFERENCE,
  K_STRUCT,                             // or union or C++ class
};

/**
 * TODO
 */
struct c_array {
  int         size;                     // -1 == no size
  c_object_t *of_type;
};

/**
 * TODO
 */
struct c_member {
  char const *class_name;
  c_object_t *of_type;
};

/**
 * TODO
 */
struct c_func_arg {
  c_object_t   *c_obj;
  c_func_arg_t *next;
};

struct c_func {
  c_object_t  *ret_type;
  c_func_arg_t *arg;
};

/**
 * TODO
 */
struct c_object {
  c_kind_t    kind;
  char const *name;

  union {
    c_array_t     array;                // K_ARRAY
    c_type_t      type;                 // K_BUILTIN
    c_func_t      func;                 // K_FUNCTION
    c_member_t    member;               // K_MEMBER
    c_object_t   *ptr_to;               // K_POINTER
    c_object_t   *ref_to;               // K_REFERENCE
  } as;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * TODO
 *
 * @param obj
 */
void c_object_free( c_object_t *obj );

/**
 * TODO
 *
 * @param kind TODO
 * @param name TODO
 */
c_object_t* c_object_new( c_kind_t kind, char const *name );

/**
 * TODO
 *
 * @param obj TODO
 * @param fout TODO
 */
void c_object_english( c_object_t *obj, FILE *fout );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_ast_H */
/* vim:set et sw=2 ts=2: */
