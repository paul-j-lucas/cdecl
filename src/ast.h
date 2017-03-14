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
#include "util.h"

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

#define C_ARRAY_NO_SIZE   (-1)          /* for array[] */

///////////////////////////////////////////////////////////////////////////////


/**
 * Kinds of AST nodes.
 * Those with values >= 10 are "parent" nodes.
 */
enum c_kind {
  K_NONE                    =  0,
  K_BUILTIN                 =  1,       // void, char, int, etc.
  K_ENUM_CLASS_STRUCT_UNION =  2,
  K_NAME                    =  3,       // typeless function argument in K&R C
  K_ARRAY                   = 11,
  K_BLOCK                   = 12,       // Apple extension
  K_FUNCTION                = 13,
  K_POINTER                 = 14,
  // C++ only
  K_POINTER_TO_MEMBER       = 15,
  K_REFERENCE               = 16,
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
typedef struct c_parent   c_parent_t;
typedef struct c_ptr_mbr  c_ptr_mbr_t;
typedef struct c_ptr_ref  c_ptr_ref_t;

/**
 * The signature for functions passed to c_ast_visit().
 *
 * @param ast The c_ast to visit.
 * @param data Optional data passed from c_ast_visit() or c_ast_visit_up().
 * @return Returning \c true will cause traversal to stop and \a ast to be
 * returned to the called of c_ast_visit().
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
  c_ast_t    *parent;
  c_kind_t    kind;
  char const *name;
  c_type_t    type;
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
 * If \a ast is:
 *  + Not an array, makes \a array an array of \a ast.
 *  + An array, appends \a array to the end of the array AST chain.
 *
 * For example, given:
 *
 *  + \a ast = <code>array 3 of array 5 of int</code>
 *  + \a array = <code>array 7 of NULL</code>
 *
 * this function returns:
 *
 *  + <code>array 3 of array 5 of array 7 of int</code>
 *
 * @param ast The AST to append to.
 * @param array The array AST to append.  It's "of" type must be null.
 * @return If \a ast is an array, returns \a ast; otherwise returns \a array.
 */
c_ast_t* c_ast_append_array( c_ast_t *ast, c_ast_t *array );

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
c_ast_t* c_ast_new( c_kind_t kind, YYLTYPE const *loc );

/**
 * Sets the two-way pointer links between parent/child AST nodes.
 *
 * @param child The "child" AST node to set the parent of.
 * @Param parent The "parent" AST node whose child node is set.
 */
void c_ast_set_parent( c_ast_t *child, c_ast_t *parent );

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
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the c_ast the visitor stopped on or null.
 */
c_ast_t* c_ast_visit( c_ast_t *ast, c_ast_visitor visitor, void *data );

/**
 * Traverses up the AST towards the root.
 *
 * @param ast A leaf of the AST to begin visiting.
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the c_ast the visitor stopped on or null.
 */
c_ast_t* c_ast_visit_up( c_ast_t *ast, c_ast_visitor visitor, void *data );

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
