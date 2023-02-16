/*
**      cdecl -- C gibberish translator
**      src/types.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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

/**
 * @file
 * Declares some types and many `typedef` definitions in one file.
 *
 * Some headers are bidirectionally dependent, so `typedef`s were used
 * originally rather than `include`.  However, some old C compilers don't like
 * multiple `typedef` definitions even if the types match.  Hence, just put all
 * `typedef` definitions in one file.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stdint.h>                     /* for uint*_t */

/**
 * Decimal print conversion specifier for \ref c_array_size_t.
 */
#define PRId_C_ARRAY_SIZE_T       "%d"

/**
 * Decimal print conversion specifier for \ref c_ast_id_t.
 *
 * @sa #PRId_C_AST_SID_T
 */
#define PRId_C_AST_ID_T           "%u"

/**
 * Decimal print conversion specifier for \ref c_ast_sid_t.
 *
 * @sa #PRId_C_AST_ID_T
 */
#define PRId_C_AST_SID_T          "%d"

/**
 * @defgroup type-declarations-group Type Declarations
 * Declares many types and `typedef` declarations in one file.
 * @{
 */

////////// enumerations ///////////////////////////////////////////////////////

// Enumerations have to be declared before typedefs of them since ISO C doesn't
// allow forward declarations of enums.

/**
 * The argument kind for the `alignas` specifier.
 */
enum c_alignas_kind {
  C_ALIGNAS_NONE,                       ///< No `alignas` specifier.
  C_ALIGNAS_EXPR,                       ///< `alignas(` _expr_ `)`
  C_ALIGNAS_TYPE                        ///< `alignas(` _type_ `)`
};

/**
 * C++ lambda capture kind.
 */
enum c_capture_kind {
  C_CAPTURE_VARIABLE,                   ///< Capture a variable.
  C_CAPTURE_COPY,                       ///< Capture by copy (`=`).
  C_CAPTURE_REFERENCE,                  ///< Capture by reference (`&`)
  C_CAPTURE_THIS,                       ///< Capture `this`.
  C_CAPTURE_STAR_THIS                   ///< Capture `*this`.
};

/**
 * C/C++ cast kinds.
 */
enum c_cast_kind {
  C_CAST_C,                             ///< C-style cast.
  C_CAST_CONST,                         ///< C++ `const_cast`.
  C_CAST_DYNAMIC,                       ///< C++ `dynamic_cast`.
  C_CAST_REINTERPRET,                   ///< C++ `reinterpret_cast`.
  C_CAST_STATIC                         ///< C++ `static_cast`.
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
 * @ingroup cpp-operators-group
 * C++ operators.
 *
 * @note Operators are named based on the characters comprising them rather
 * than their semantics because many operators have more than one meaning
 * depending upon context, e.g. `*` is both the "times" and the "dereference"
 * operator.
 */
enum c_oper_id {
  C_OP_NONE,            ///< No operator.
  C_OP_CO_AWAIT,        ///< The `co_await` operator.
  C_OP_NEW,             ///< The `new` operator.
  C_OP_NEW_ARRAY,       ///< The `new[]` operator.
  C_OP_DELETE,          ///< The `delete` operator.
  C_OP_DELETE_ARRAY,    ///< The `delete[]` operator.
  C_OP_EXCLAM,          ///< The `!` operator.
  C_OP_EXCLAM_EQ,       ///< The `!=` operator.
  C_OP_PERCENT,         ///< The `%` operator.
  C_OP_PERCENT_EQ,      ///< The `%=` operator.
  C_OP_AMPER,           ///< The `&` operator.
  C_OP_AMPER2,          ///< The `&&` operator.
  C_OP_AMPER_EQ,        ///< The `&=` operator.
  C_OP_PARENS,          ///< The `()` operator.
  C_OP_STAR,            ///< The `*` operator.
  C_OP_STAR_EQ,         ///< The `*=` operator.
  C_OP_PLUS,            ///< The `+` operator.
  C_OP_PLUS2,           ///< The `++` operator.
  C_OP_PLUS_EQ,         ///< The `+=` operator.
  C_OP_COMMA,           ///< The `,` operator.
  C_OP_MINUS,           ///< The `-` operator.
  C_OP_MINUS2,          ///< The `--` operator.
  C_OP_MINUS_EQ,        ///< The `-=` operator.
  C_OP_ARROW,           ///< The `->` operator.
  C_OP_ARROW_STAR,      ///< The `->*` operator.
  C_OP_DOT,             ///< The `.` operator.
  C_OP_DOT_STAR,        ///< The `.*` operator.
  C_OP_SLASH,           ///< The `/` operator.
  C_OP_SLASH_EQ,        ///< The `/=` operator.
  C_OP_COLON2,          ///< The `::` operator.
  C_OP_LESS,            ///< The `<` operator.
  C_OP_LESS2,           ///< The `<<` operator.
  C_OP_LESS2_EQ,        ///< The `<<=` operator.
  C_OP_LESS_EQ,         ///< The `<=` operator.
  C_OP_LESS_EQ_GREATER, ///< The `<=>` operator.
  C_OP_EQ,              ///< The `=` operator.
  C_OP_EQ2,             ///< The `==` operator.
  C_OP_GREATER,         ///< The `>` operator.
  C_OP_GREATER_EQ,      ///< The `>=` operator.
  C_OP_GREATER2,        ///< The `>>` operator.
  C_OP_GREATER2_EQ,     ///< The `>>=` operator.
  C_OP_QMARK_COLON,     ///< The `?:` operator.
  C_OP_BRACKETS,        ///< The `[]` operator.
  C_OP_CARET,           ///< The `^` operator.
  C_OP_CARET_EQ,        ///< The `^=` operator.
  C_OP_PIPE,            ///< The `|` operator.
  C_OP_PIPE_EQ,         ///< The `|=` operator.
  C_OP_PIPE2,           ///< The `||` operator.
  C_OP_TILDE,           ///< The `~` operator.
};

/**
 * Types of help.
 */
enum cdecl_help {
  CDECL_HELP_COMMANDS,                  ///< Help for **cdecl** commands.
  CDECL_HELP_ENGLISH,                   ///< Help for **cdecl** pseudo-English.
  CDECL_HELP_OPTIONS                    ///< Help for **cdecl** options.
};

/**
 * Mode of operation.
 */
enum cdecl_mode {
  CDECL_ENGLISH_TO_GIBBERISH,           ///< Convert English into gibberish.
  CDECL_GIBBERISH_TO_ENGLISH            ///< Decipher gibberish into English.
};

////////// typedefs ///////////////////////////////////////////////////////////

typedef struct slist              slist_t;
typedef struct slist_node         slist_node_t;

typedef struct c_alignas          c_alignas_t;
typedef enum   c_alignas_kind     c_alignas_kind_t;
typedef struct c_apple_block_ast  c_apple_block_ast_t;
typedef struct c_array_ast        c_array_ast_t;

/**
 * One of:
 *
 *  + The actual size of a C array, but only when &ge; 0.
 *  + #C_ARRAY_SIZE_NONE
 *  + #C_ARRAY_SIZE_VARIABLE
 */
typedef int                       c_array_size_t;

typedef struct c_ast              c_ast_t;
typedef unsigned                  c_ast_id_t;     ///< Unique AST node ID.
typedef slist_t                   c_ast_list_t;   ///< AST list.
typedef struct c_ast_pair         c_ast_pair_t;
typedef int                       c_ast_sid_t;    ///< Signed \ref c_ast_id_t.
typedef struct c_bit_field_ast    c_bit_field_ast_t;
typedef struct c_builtin_ast      c_builtin_ast_t;
typedef slist_node_t              c_capture_t;    ///< Lambda capture.
typedef struct c_capture_ast      c_capture_ast_t;
typedef enum   c_capture_kind     c_capture_kind_t;
typedef struct c_cast_ast         c_cast_ast_t;
typedef enum   c_cast_kind        c_cast_kind_t;
typedef struct c_constructor_ast  c_constructor_ast_t;
typedef struct c_csu_ast          c_csu_ast_t;
typedef struct c_enum_ast         c_enum_ast_t;
typedef struct c_function_ast     c_function_ast_t;
typedef enum   c_graph            c_graph_t;
typedef struct c_keyword          c_keyword_t;
typedef struct c_lambda_ast       c_lambda_ast_t;
typedef struct c_lang             c_lang_t;
typedef uint32_t                  c_lang_id_t;    ///< Languages bitmask.
typedef struct c_lang_lit         c_lang_lit_t;
typedef struct c_loc              c_loc_t;
typedef struct c_operator         c_operator_t;
typedef struct c_operator_ast     c_operator_ast_t;
typedef enum   c_oper_id          c_oper_id_t;
typedef slist_node_t              c_param_t;      ///< Function-like parameter.
typedef struct c_parent_ast       c_parent_ast_t;
typedef struct c_ptr_mbr_ast      c_ptr_mbr_ast_t;
typedef struct c_ptr_ref_ast      c_ptr_ref_ast_t;
typedef slist_node_t              c_scope_t;      ///< Scope in \ref c_sname_t.
typedef struct c_scope_data       c_scope_data_t;
typedef struct c_sglob            c_sglob_t;
typedef slist_t                   c_sname_t;      ///< C++ scoped name.
typedef uint64_t                  c_tid_t;        ///< Type ID(s) bits.
typedef struct c_typedef          c_typedef_t;
typedef struct c_typedef_ast      c_typedef_ast_t;
typedef struct c_type             c_type_t;
typedef struct c_udef_conv_ast    c_udef_conv_ast_t;
typedef struct c_udef_lit_ast     c_udef_lit_ast_t;
typedef struct cdecl_command      cdecl_command_t;
typedef enum   cdecl_help         cdecl_help_t;
typedef struct cdecl_keyword      cdecl_keyword_t;
typedef enum   cdecl_mode         cdecl_mode_t;
typedef struct print_params       print_params_t;
typedef union  user_data          user_data_t;

typedef c_loc_t YYLTYPE;                ///< Source location type for Bison.
/// @cond DOXYGEN_IGNORE
#define YYLTYPE_IS_DECLARED       1
#define YYLTYPE_IS_TRIVIAL        1
/// @endcond

////////// structs ////////////////////////////////////////////////////////////

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
  // Cdecl doesn't use either of these, but Bison generates code that does, so
  // we need to keep them.
  //
  int last_line;                        ///< Last line of location range.
  int last_column;                      ///< Last column of location range.
};

/**
 * The signature for a function passed to **qsort**(3).
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the data pointed to by \a i_data is less than, equal to, or
 * greater than the data pointed to by \a j_data.
 */
typedef int (*qsort_cmp_fn_t)( void const *i_data, void const *j_data );

/**
 * "User data" passed as additional data to certain functions.
 *
 * @note This isn't just a `void*` as is typically used for "user data" since
 * `void*` can't hold a 64-bit integer value on 32-bit platforms.
 * @note Almost all built-in types are included to avoid casting.
 * @note `long double` is not included since that would double the size of the
 * `union`.
 */
union user_data {
  bool                b;                ///< `bool` value.

  char                c;                ///< `char` value.
  signed char         sc;               ///< `signed char` value.
  unsigned char       uc;               ///< `unsigned char` value.

  short               s;                ///< `short` value.
  int                 i;                ///< `int` value.
  long                l;                ///< `long` value.
  long long           ll;               ///< `long long` value.

  unsigned short      us;               ///< `unsigned short` value.
  unsigned int        ui;               ///< `unsigned int` value.
  unsigned long       ul;               ///< `unsigned long` value.
  unsigned long long  ull;              ///< `unsigned long long` value.

  int8_t              i8;               ///< `int8_t` value.
  int16_t             i16;              ///< `int16_t` value.
  int32_t             i32;              ///< `int32_t` value.
  int64_t             i64;              ///< `int64_t` value.

  uint8_t             ui8;              ///< `uint8_t` value.
  uint16_t            ui16;             ///< `uint16_t` value.
  uint32_t            ui32;             ///< `uint32_t` value.
  uint64_t            ui64;             ///< `uint64_t` value.

  float               f;                ///< `float` value.
  double              d;                ///< `double` value.

  void               *p;                ///< Pointer (to non-`const`) value.
  void const         *pc;               ///< Pointer to `const` value.
};

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_types_H */
/* vim:set et sw=2 ts=2: */
