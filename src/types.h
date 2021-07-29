/*
**      cdecl -- C gibberish translator
**      src/types.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

#ifndef cdecl_types_H
#define cdecl_types_H

// standard
#include <stdint.h>                     /* for uint*_t */

/**
 * @file
 * Declares some types and many `typedef` definitions in one file.
 *
 * Some headers are bidirectionally dependent, so `typedef`s were used
 * originally rather than `include`.  However, some old C compilers don't like
 * multiple `typedef` definitions even if the types match.  Hence, just put all
 * `typedef` definitions in one file.
 */

/**
 * @defgroup type-declarations-group Type Declarations
 * Declares many types and `typedef` declarations in one file.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

typedef struct c_alignas          c_alignas_t;
typedef enum   c_alignas_arg      c_alignas_arg_t;
typedef struct c_apple_block_ast  c_apple_block_ast_t;
typedef struct c_array_ast        c_array_ast_t;

/**
 * One of:
 *
 *  + The actual size of a C array.
 *  + #C_ARRAY_SIZE_NONE
 *  + #C_ARRAY_SIZE_VARIABLE
 */
typedef int                       c_array_size_t;

typedef struct c_ast              c_ast_t;
typedef unsigned                  c_ast_depth_t;  ///< How many `()` deep.
typedef unsigned                  c_ast_id_t;     ///< Unique AST node id.
typedef struct slist              c_ast_list_t;   ///< AST list.
typedef struct c_ast_pair         c_ast_pair_t;
typedef struct slist_node         c_ast_param_t;  ///< Function-like parameter.
typedef unsigned                  c_bit_width_t;  ///< Bit-field width.
typedef struct c_builtin_ast      c_builtin_ast_t;
typedef struct c_command          c_command_t;
typedef enum   c_command_kind     c_command_kind_t;
typedef struct c_constructor_ast  c_constructor_ast_t;
typedef struct c_ecsu_ast         c_ecsu_ast_t;
typedef struct c_function_ast     c_function_ast_t;
typedef enum   c_gib_kind         c_gib_kind_t;
typedef enum   c_graph            c_graph_t;
typedef enum   c_help             c_help_t;
typedef struct c_keyword          c_keyword_t;
typedef enum   c_keyword_ctx      c_keyword_ctx_t;
typedef enum   c_kind_id          c_kind_id_t;
typedef struct c_lang             c_lang_t;
typedef uint32_t                  c_lang_id_t;    ///< Languages bitmask.
typedef struct c_lang_lit         c_lang_lit_t;
typedef struct c_loc              c_loc_t;
typedef enum   c_mode             c_mode_t;
typedef struct c_operator_ast     c_operator_ast_t;
typedef enum   c_oper_id          c_oper_id_t;
typedef struct c_operator         c_operator_t;
typedef struct c_parent_ast       c_parent_ast_t;
typedef struct c_ptr_mbr_ast      c_ptr_mbr_ast_t;
typedef struct c_ptr_ref_ast      c_ptr_ref_ast_t;
typedef struct slist_node         c_scope_t;      ///< Scope in a c_sname_t.
typedef struct c_scope_data       c_scope_data_t;
typedef struct slist              c_sname_t;      ///< C++ scoped name.
typedef uint64_t                  c_tid_t;        ///< Type ID(s) bitmask.
typedef enum   c_tpid             c_tpid_t;
typedef struct c_typedef          c_typedef_t;
typedef enum   c_typedef_add_rv   c_typedef_add_rv_t;
typedef struct c_typedef_ast      c_typedef_ast_t;
typedef struct c_type             c_type_t;
typedef struct c_udef_conv_ast    c_udef_conv_ast_t;
typedef struct c_udef_lit_ast     c_udef_lit_ast_t;
typedef enum   c_visit_dir        c_visit_dir_t;
typedef enum   yytokentype        yytokentype;

typedef c_loc_t YYLTYPE;                ///< Source location type for Bison.
/// @cond DOXYGEN_IGNORE
#define YYLTYPE_IS_DECLARED       1
#define YYLTYPE_IS_TRIVIAL        1
/// @endcond

/**
 * A pair of AST pointers used as one of the synthesized attribute types in the
 * parser.
 */
struct c_ast_pair {
  /**
   * A pointer to the AST being built.
   */
  c_ast_t *ast;

  /**
   * Array and function-like declarations need a separate AST pointer that
   * points to their `of_ast` or `ret_ast` (respectively) to be the "target" of
   * subsequent additions to the AST.
   */
  c_ast_t *target_ast;
};

/**
 * The kind of cdecl command in least-to-most restrictive order.
 */
enum c_command_kind {
  C_COMMAND_ANYWHERE,                   ///< Command is OK anywhere.
  C_COMMAND_FIRST_ARG,                  ///< `$ cdecl` _command_ _args_
  C_COMMAND_PROG_NAME,                  ///< `$` _command_ _args_
  C_COMMAND_LANG_ONLY                   ///< `cdecl>` _command_ _args_
};

/**
 * A cdecl command.
 */
struct c_command {
  char const       *literal;            ///< The command literal.
  c_command_kind_t  kind;               ///< The kind of command.
  c_lang_id_t       lang_ids;           ///< Language(s) command is in.
};

/**
 * Kind of gibberish to print.  The gibberish printed varies slightly depending
 * on the kind.
 *
 * A given gibberish may only be a single kind and _not_ be a bitwise-or of
 * kinds.  However, a bitwise-or of kinds may be used to test whether a given
 * gibberish is any _one_ of those kinds.
 */
enum c_gib_kind {
  C_GIB_NONE    = 0u,                   ///< Not gibberish (hence, English).
  C_GIB_DECL    = (1u << 0),            ///< Gibberish is a declaration.
  C_GIB_CAST    = (1u << 1),            ///< Gibberish is a cast.
  C_GIB_TYPEDEF = (1u << 2),            ///< Gibberish is a `typedef`.
  C_GIB_USING   = (1u << 3)             ///< Gibberish is a `using`.
};

/**
 * Di/Trigraph mode.
 */
enum c_graph {
  C_GRAPH_NONE,                         ///< Ordinary characters.
  C_GRAPH_DI,                           ///< Digraphs.
  C_GRAPH_TRI                           ///< Trigraphs.
};

/**
 * Types of help.
 */
enum c_help {
  C_HELP_COMMANDS,                      ///< Help for cdecl commands.
  C_HELP_ENGLISH                        ///< Help for cdecl pseudo-English.
};

/**
 * The source location used by Bison.
 */
struct c_loc {
  //
  // These should be either unsigned or size_t, but Bison generates code that
  // tests these for >= 0 which is always true for unsigned types so it
  // generates warnings; hence these are kept as int to eliminate the warnings.
  //
  int first_line;                       ///< First line of location range.
  int first_column;                     ///< First column of location range.
  //
  // Cdecl doesn't use either of these.
  //
  int last_line;                        ///< Last line of location range.
  int last_column;                      ///< Last column of location range.
};

/**
 * Mode of operation.
 */
enum c_mode {
  C_ENGLISH_TO_GIBBERISH,               ///< Convert English into gibberish.
  C_GIBBERISH_TO_ENGLISH                ///< Decipher gibberish into English.
};

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_types_H */
/* vim:set et sw=2 ts=2: */
