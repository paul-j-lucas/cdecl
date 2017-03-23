/*
**      cdecl -- C gibberish translator
**      src/ast_util.h
*/

#ifndef cdecl_ast_util_H
#define cdecl_ast_util_H

/**
 * @file
 * Contains various cdecl-specific algorithms for construcing an Abstract
 * Syntax Tree (AST) for parsed C/C++ declarations.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "util.h"

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_AST_UTIL_INLINE
# define CDECL_AST_UTIL_INLINE _GL_INLINE
#endif /* CDECL_AST_UTIL_INLINE */

///////////////////////////////////////////////////////////////////////////////

/**
 * Adds an array to the AST being built.
 *
 * @param ast The AST to append to.
 * @param array The array AST to append.  It's "of" type must be null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array );

/**
 * Checks an entire AST for semantic validity.
 *
 * @param ast The AST to check.
 * @return Returns \c true only if the entire AST is valid.
 */
bool c_ast_check( c_ast_t const *ast );

/**
 * Adds a function (or block) to the AST being built.
 *
 * @param ast The AST to append to.
 * @param ret_type_ast The AST of the return-type of the function (or block).
 * @param func The function (or block) AST to append.  It's "of" type must be
 * null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *ret_type_ast, c_ast_t *func );

/**
 * Prints the given AST as English.
 *
 * @param ast The AST to print.  May be null.
 * @param fout The FILE to print to.
 */
void c_ast_english( c_ast_t const *ast, FILE *fout );

/**
 * Traverses the AST attempting to find an AST node having \a kind.
 *
 * @param ast The AST to begin at.
 * @param dir The direction to visit.
 * @param kind The bitwise-or kind(s) to find.
 * @return Returns a pointer to an AST node having \a kind or null if none.
 */
CDECL_AST_UTIL_INLINE c_ast_t* c_ast_find_kind( c_ast_t *ast,
                                                v_direction_t dir,
                                                c_kind_t kind ) {
  void *const data = REINTERPRET_CAST( void*, kind );
  return c_ast_visit( ast, dir, c_ast_vistor_kind, data );
}

/**
 * Prints the given AST as a C/C++ cast.
 *
 * @param ast The AST to print.
 * @param fout The FILE to print to.
 */
void c_ast_gibberish_cast( c_ast_t const *ast, FILE *fout );

/**
 * Prints the given AST as a C/C++ declaration.
 *
 * @param ast The AST to print.
 * @param fout The FILE to print to.
 */
void c_ast_gibberish_declare( c_ast_t const *ast, FILE *fout );

/**
 * Gets the name from the given AST.
 *
 * @param ast The AST to begin the search at.
 * @param dir The direction to search.
 * @return Returns said name or null if none.
 */
char const* c_ast_name( c_ast_t const *ast, v_direction_t dir );

/**
 * "Patches" the given type AST into the given declaration AST only if:
 *  + The type AST has no parent.
 *  + The type AST's depth is less than the declaration AST's depth.
 *  + The declaration AST still contains an AST node of type K_NONE.
 *
 * @param type_ast The AST of the initial type.
 * @param decl_ast The AST of a declaration.
 */
void c_ast_patch_none( c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another c_ast).
 *
 * @param ast The AST (or one of its child nodes) to take from.
 * @return Returns said name or null.  The caller is responsible for freeing
 * the string.
 */
char const* c_ast_take_name( c_ast_t *ast );

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

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_ast_util_H */
/* vim:set et sw=2 ts=2: */
