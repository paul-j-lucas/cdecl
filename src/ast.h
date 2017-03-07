/*
**      cdecl -- C gibberish translator
**      src/ast.h
*/

#ifndef cdecl_ast_H
#define cdecl_ast_H

/**
 * @file
 * Contains types to represent an Abstract Syntax Tree (AST) for parsed C/C++
 * declarations.
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

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

#define C_ARRAY_NO_SIZE   (-1)          /* for array[] */

///////////////////////////////////////////////////////////////////////////////

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
  K_NAME,                               // typeless function argument in K&R C
  K_POINTER,
  K_POINTER_TO_MEMBER,                  // C++ only
  K_REFERENCE,                          // C++ only
};
typedef enum c_kind       c_kind_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct c_ast      c_ast_t;
typedef struct c_ast_list c_ast_list_t;
typedef struct c_array    c_array_t;
typedef struct c_block    c_block_t;
typedef struct c_builtin  c_builtin_t;
typedef struct c_ecsu     c_ecsu_t;
typedef struct c_func     c_func_t;
typedef struct c_ptr_mbr  c_ptr_mbr_t;
typedef struct c_ptr_ref  c_ptr_ref_t;

/**
 * The signature for functions passed to c_ast_visit().
 *
 * @param ast The c_ast to visit.
 * @return Returning \c true will cause traversal to stop and \a ast to be
 * returned to the called of c_ast_visit().
 */
typedef bool (*c_ast_visitor)( c_ast_t *ast );

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
 * AST node for a parse C/C++ declaration.
 */
struct c_ast {
  c_kind_t    kind;
  char const *name;
  c_type_t    type;
  c_ast_t    *next;                     // used for stacks and arg lists

  union {
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
 * Checks an entire AST for semantic validity.
 *
 * @param ast The AST to check.
 * @return Returns \c true only if the entire AST is valid.
 */
bool c_ast_check( c_ast_t const *ast );

/**
 * Cleans-up AST data.
 * (Currently, this checks that the number of c_ast objects freed match the
 * number allocated.)
 */
void c_ast_cleanup( void );

/**
 * Clones the given AST.
 *
 * @param ast The AST to clone.
 * @return Returns said clone.
 */
c_ast_t* c_ast_clone( c_ast_t const *ast );

/**
 * Prints the given AST as English.
 *
 * @param ast The AST to print.  May be null.
 * @param fout The FILE to print to.
 */
void c_ast_english( c_ast_t const *ast, FILE *fout );

/**
 * Garbage collects all allocated c_ast objects.
 */
void c_ast_gc( void );

/**
 * Prints the given AST as a C/C++ cast.
 *
 * @param ast The c_ast to print.
 * @param fout The FILE to print to.
 */
void c_ast_gibberish_cast( c_ast_t const *ast, FILE *fout );

/**
 * Prints the given AST as a C/C++ declaration.
 *
 * @param ast The c_ast to print.
 * @param fout The FILE to print to.
 */
void c_ast_gibberish_declare( c_ast_t const *ast, FILE *fout );

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
 * Clones the given c_ast_list.
 *
 * @param list The c_ast_list to clone.
 * @return Returns said clone.
 */
c_ast_list_t c_ast_list_clone( c_ast_list_t const *list );

/**
 * Dump the given c_ast_list as JSON (for debugging).
 *
 * @param list The c_ast_list to dump.
 * @param indent The initial indent.
 * @param jour The FILE to dump to.
 */
void c_ast_list_json( c_ast_list_t const *list, unsigned indent, FILE *fout );

/**
 * Gets the name from the given AST.
 *
 * @param ast The AST to get the name from.
 * @return Returns said name or null if none.
 */
char const* c_ast_name( c_ast_t *ast );

/**
 * Creates a new c_ast.
 *
 * @param kind The kind of c_ast to create.
 */
c_ast_t* c_ast_new( c_kind_t kind );

/**
 * Pops a c_ast from the head of a list.
 *
 * @param phead The pointer to the pointer to the head of the list.
 * @return Returns the popped c_ast or null if the list is empty.
 */
c_ast_t* c_ast_pop( c_ast_t **phead );

/**
 * Pushes a c_ast onto the front of a list.
 *
 * @param phead The pointer to the pointer to the head of the list.  The head
 * is updated to point to \a ast.
 * @param ast The pointer to the c_ast to add.  Its \c next pointer is set to
 * the old head of the list.
 */
void c_ast_push( c_ast_t **phead, c_ast_t *ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another c_ast).
 *
 * @param ast The AST to take trom.
 * @return Returns said name or null.
 */
char const* c_ast_take_name( c_ast_t *ast );

/**
 * Takes the storage type, if any, away from \a ast
 * (with the intent of giving it to another c_ast).
 * This is used is cases like:
 * @code
 *  explain static int f()
 * @endcode
 * that should be explained as:
 * @code
 *  declare f as static function () returning int
 * @endcode
 * and \e not:
 * @code
 *  declare f as function () returning static int
 * @endcode
 * i.e., the \c static has to be taken away from \c int and given to the
 * function because it's the function that's \c static, not the \c int.
 *
 * @param ast The AST to take trom.
 * @return Returns said storage class or T_NONE.
 */
c_type_t c_ast_take_storage( c_ast_t *ast );

/**
 * Checks \a ast to see if it contains a \c typedef.
 * If so, removes it.
 * This is used in cases like:
 * @code
 *  explain typedef int *p
 * @endcode
 * that should be explained as:
 * @code
 *  declare p as type pointer to int
 * @endcode
 * and \e not:
 * @code
 *  declare p as pointer to typedef int
 * @endcode
 *
 * @param ast The AST to check.
 * @return Returns \c true only if \a ast contains a \c typedef.
 */
bool c_ast_take_typedef( c_ast_t *ast );

/**
 * Does a depth-first traversal of an AST.
 *
 * @param ast The root of the AST to begin visiting.
 * @param visitor The visitor to use.
 * @return Returns a pointer to the c_ast the visitor stopped on or null.
 */
c_ast_t* c_ast_visit( c_ast_t *ast, c_ast_visitor visitor );

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
