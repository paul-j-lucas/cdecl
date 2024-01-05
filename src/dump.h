/*
**      cdecl -- C gibberish translator
**      src/dump.h
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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

#ifndef cdecl_dump_H
#define cdecl_dump_H

/**
 * @file
 * Declares functions for dumping types for debugging.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup dump-group Debug Output
 * Functions for dumping types in [JSON5](https://json5.org) format (for
 * debugging).
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Dumps a Boolean value as either `true` or `false` (for debugging).
 *
 * @param b The Boolean to dump.
 * @param fout The `FILE` to dump to.
 */
void bool_dump( bool b, FILE *fout );

/**
 * Dumps \a align in [JSON5](https://json5.org) format (for debugging).
 *
 * @param align The \ref c_alignas to dump.
 * @param fout The `FILE` to dump to.
 */
void c_alignas_dump( c_alignas_t const *align, FILE *fout );

/**
 * Dumps \a ast in [JSON5](https://json5.org) format (for debugging).
 *
 * @param ast The AST to dump.  If NULL, `null` is printed instead.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_ast_list_dump()
 * @sa c_ast_pair_dump()
 */
void c_ast_dump( c_ast_t const *ast, FILE *fout );

/**
 * Dumps \a list of ASTs in [JSON5](https://json5.org) format (for debugging).
 *
 * @param list The \ref slist of ASTs to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_ast_dump()
 * @sa c_ast_pair_dump()
 */
void c_ast_list_dump( c_ast_list_t const *list, FILE *fout );

/**
 * Dumps \a astp in [JSON5](https://json5.org) format (for debugging).
 *
 * @param astp The \ref c_ast_pair to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_ast_dump()
 * @sa c_ast_list_dump()
 */
void c_ast_pair_dump( c_ast_pair_t const *astp, FILE *fout );

/**
 * Dumps \a sname in [JSON5](https://json5.org) format (for debugging).
 *
 * @param sname The scoped name to dump.  If empty, prints `null` instead.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_sname_list_dump()
 */
void c_sname_dump( c_sname_t const *sname, FILE *fout );

/**
 * Dumps \a list of scoped names in [JSON5](https://json5.org) format (for
 * debugging).
 *
 * @param list The list of scoped names to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_sname_dump()
 */
void c_sname_list_dump( slist_t const *list, FILE *fout );

/**
 * Dumps \a tid in [JSON5](https://json5.org) format (for debugging).
 *
 * @param tid The \ref c_tid_t to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_type_dump()
 */
void c_tid_dump( c_tid_t tid, FILE *fout );

/**
 * Dumps \a type in [JSON5](https://json5.org) format (for debugging).
 *
 * @param type The \ref c_type to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_tid_dump()
 */
void c_type_dump( c_type_t const *type, FILE *fout );

/**
 * Dumps \a macro in [JSON5](https://json5.org) format (for debugging).
 *
 * @param macro The \ref p_macro to dump.
 * @param dout The `FILE` to dump to.
 */
void p_macro_dump( p_macro_t const *macro, FILE *dout );

/**
 * Dumps \a list of preprocessor macro arguments in [JSON5](https://json5.org)
 * format (for debugging).
 *
 * @param list The list of macro arguments to dump.
 * @param indent The indentation to use.
 * @param dout The `FILE` to dump to.
 */
void p_arg_list_dump( p_arg_list_t const *list, unsigned indent, FILE *dout );

/**
 * Dumps \a list of preprocessor macro parameters in [JSON5](https://json5.org)
 * format (for debugging).
 *
 * @param list The list of \ref p_param to dump.
 * @param indent The indentation to use.
 * @param dout The `FILE` to dump to.
 */
void p_param_list_dump( p_param_list_t const *list, unsigned indent,
                        FILE *dout );

/**
 * Dumps \a token in [JSON5](https://json5.org) format (for debugging).
 *
 * @param token The \ref p_token to dump.
 * @param dout The `FILE` to dump to.
 *
 * @sa p_token_list_dump()
 */
void p_token_dump( p_token_t const *token, FILE *dout );

/**
 * Dumps \a list of preprocessor macro tokens in [JSON5](https://json5.org)
 * format (for debugging).
 *
 * @param list The list of \ref p_token to dump.
 * @param indent The indentation to use.
 * @param dout The `FILE` to dump to.
 *
 * @sa p_token_dump()
 */
void p_token_list_dump( p_token_list_t const *list, unsigned indent,
                        FILE *dout );

/**
 * Dumps \a list of strings in [JSON5](https://json5.org) format (for
 * debugging).
 *
 * @param list The list of strings to dump.
 * @param dout The `FILE` to dump to.
 *
 * @sa str_dump()
 */
void str_list_dump( slist_t const *list, FILE *dout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_dump_H */
/* vim:set et sw=2 ts=2: */
