/*
**      cdecl -- C gibberish translator
**      src/types.h
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

#ifndef cdecl_types_H
#define cdecl_types_H

/**
 * @file
 * Declares some types and many `typedef` definitions in one file.
 *
 * Some headers are bidirectionally dependent, so `typedef`s were used
 * originally rather than `include`.  However, some old C compilers don't like
 * multiple `typedef` definitions even if the types match.  Hence, just put
 * many `typedef` definitions in one file.
 *
 * Additionally, types that appear in Bison's `%union` declaration need to be
 * here to work around Bison's placement of `#include`s.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stdbool.h>
#include <stdint.h>                     /* for uint*_t */

/**
 * @defgroup type-declarations-group Type Declarations
 * Declares many types and `typedef` declarations in one file.
 * @{
 */

/**
 * Decimal print conversion specifier for \ref c_loc_num_t.
 */
#define PRI_c_loc_num_t           "%u"

////////// enumerations ///////////////////////////////////////////////////////

// Enumerations have to be declared before typedefs of them since ISO C doesn't
// allow forward declarations of enums.

/**
 * The argument kind for the `alignas` specifier.
 *
 * @note `alignas(` _expr_ `)` where _expr_ is an arbitrary expression is not
 * supported by **cdecl**.
 */
enum c_alignas_kind {
  C_ALIGNAS_NONE,                       ///< No `alignas` specifier.
  C_ALIGNAS_BYTES,                      ///< `alignas(` _bytes_ `)`
  C_ALIGNAS_TYPE                        ///< `alignas(` _type_ `)`
};

/**
 * Array kind.
 */
enum c_array_kind {
  C_ARRAY_EMPTY_SIZE,                   ///< E.g., `a[]`.
  C_ARRAY_INT_SIZE,                     ///< E.g., `a[4]`.
  C_ARRAY_NAMED_SIZE,                   ///< E.g., `a[n]`.
  C_ARRAY_VLA_STAR                      ///< E.g., `a[*]` (C99 and later only).
};

/**
 * C++ lambda capture kind.
 */
enum c_capture_kind {
  C_CAPTURE_VARIABLE,                   ///< Capture a variable.
  C_CAPTURE_COPY,                       ///< Capture by copy (`=`).
  C_CAPTURE_REFERENCE,                  ///< Capture by reference (`&`).
  C_CAPTURE_THIS,                       ///< Capture `this`.
  C_CAPTURE_STAR_THIS                   ///< Capture `*this`.
};

/**
 * C/C++ cast kind.
 */
enum c_cast_kind {
  C_CAST_C,                             ///< C-style cast.
  C_CAST_CONST,                         ///< C++ `const_cast`.
  C_CAST_DYNAMIC,                       ///< C++ `dynamic_cast`.
  C_CAST_REINTERPRET,                   ///< C++ `reinterpret_cast`.
  C_CAST_STATIC                         ///< C++ `static_cast`.
};

/**
 * User-specified C++ member or non-member function (or operator).
 *
 * @sa c_op_overload
 */
enum c_func_member {
  C_FUNC_UNSPECIFIED  = 0u,             ///< Unspecified.
  C_FUNC_MEMBER       = (1u << 0),      ///< Member function.
  C_FUNC_NON_MEMBER   = (1u << 1)       ///< Non-member function.
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
 * depending upon context, e.g. `*` is both the "multiplication" and the
 * "dereference" operator.
 */
enum c_op_id {
  C_OP_NONE,                            ///< No operator.
  C_OP_CO_AWAIT,                        ///< The `co_await` operator.
  C_OP_NEW,                             ///< The `new` operator.
  C_OP_NEW_ARRAY,                       ///< The `new[]` operator.
  C_OP_DELETE,                          ///< The `delete` operator.
  C_OP_DELETE_ARRAY,                    ///< The `delete[]` operator.
  C_OP_EXCLAM,                          ///< The `!` operator.
  C_OP_EXCLAM_EQUAL,                    ///< The `!=` operator.
  C_OP_PERCENT,                         ///< The `%` operator.
  C_OP_PERCENT_EQUAL,                   ///< The `%=` operator.
  C_OP_AMPER,                           ///< The `&` operator.
  C_OP_AMPER_AMPER,                     ///< The `&&` operator.
  C_OP_AMPER_EQUAL,                     ///< The `&=` operator.
  C_OP_PARENS,                          ///< The `()` operator.
  C_OP_STAR,                            ///< The `*` operator.
  C_OP_STAR_EQUAL,                      ///< The `*=` operator.
  C_OP_PLUS,                            ///< The `+` operator.
  C_OP_PLUS_PLUS,                       ///< The `++` operator.
  C_OP_PLUS_EQUAL,                      ///< The `+=` operator.
  C_OP_COMMA,                           ///< The `,` operator.
  C_OP_MINUS,                           ///< The `-` operator.
  C_OP_MINUS_MINUS,                     ///< The `--` operator.
  C_OP_MINUS_EQUAL,                     ///< The `-=` operator.
  C_OP_MINUS_GREATER,                   ///< The `->` operator.
  C_OP_MINUS_GREATER_STAR,              ///< The `->*` operator.
  C_OP_DOT,                             ///< The `.` operator.
  C_OP_DOT_STAR,                        ///< The `.*` operator.
  C_OP_SLASH,                           ///< The `/` operator.
  C_OP_SLASH_EQUAL,                     ///< The `/=` operator.
  C_OP_COLON_COLON,                     ///< The `::` operator.
  C_OP_LESS,                            ///< The `<` operator.
  C_OP_LESS_LESS,                       ///< The `<<` operator.
  C_OP_LESS_LESS_EQUAL,                 ///< The `<<=` operator.
  C_OP_LESS_EQUAL,                      ///< The `<=` operator.
  C_OP_LESS_EQUAL_GREATER,              ///< The `<=>` operator.
  C_OP_EQUAL,                           ///< The `=` operator.
  C_OP_EQUAL_EQUAL,                     ///< The `==` operator.
  C_OP_GREATER,                         ///< The `>` operator.
  C_OP_GREATER_EQUAL,                   ///< The `>=` operator.
  C_OP_GREATER_GREATER,                 ///< The `>>` operator.
  C_OP_GREATER_GREATER_EQUAL,           ///< The `>>=` operator.
  C_OP_QMARK_COLON,                     ///< The `?:` operator.
  C_OP_BRACKETS,                        ///< The `[]` operator.
  C_OP_CARET,                           ///< The `^` operator.
  C_OP_CARET_EQUAL,                     ///< The `^=` operator.
  C_OP_PIPE,                            ///< The `|` operator.
  C_OP_PIPE_EQUAL,                      ///< The `|=` operator.
  C_OP_PIPE_PIPE,                       ///< The `||` operator.
  C_OP_TILDE,                           ///< The `~` operator.
};

/**
 * **Cdecl** debug mode.
 */
enum cdecl_debug {
  CDECL_DEBUG_NO            = 0u,         ///< Do not print debug output.
  CDECL_DEBUG_YES           = (1u << 0),  ///< Print JSON5 debug output.

  /**
   * Include `unique_id` values in debug output.
   *
   * @note May be used _only_ in combination with #CDECL_DEBUG_YES.
   */
  CDECL_DEBUG_OPT_AST_UNIQUE_ID = (1u << 1)
};

/**
 * @ingroup showing-group
 * Which types or macros to show for the **cdecl** `show` command.
 * @remarks The values can be bitwise-or'd together.
 */
enum cdecl_show {
  /**
   * Show only predefined types that are valid in the current language (unless
   * bitwise-or'd with #CDECL_SHOW_OPT_IGNORE_LANG) or only predefined macros.
   */
  CDECL_SHOW_PREDEFINED       = (1u << 0),

  /**
   * Show only types that were user-defined in the current language or earlier
   * (unless bitwise-or'd with #CDECL_SHOW_OPT_IGNORE_LANG) or only user-
   * defined macros.
   */
  CDECL_SHOW_USER_DEFINED     = (1u << 1),

  /**
   * Show types ignoring the language they were defined in.
   *
   * @note This _must_ be bitwise-or'd with either #CDECL_SHOW_PREDEFINED,
   * #CDECL_SHOW_USER_DEFINED, or both.
   */
  CDECL_SHOW_OPT_IGNORE_LANG  = (1u << 2)
};

////////// typedefs ///////////////////////////////////////////////////////////

typedef struct slist              slist_t;
typedef struct slist_node         slist_node_t;

typedef struct c_alignas          c_alignas_t;
typedef enum   c_alignas_kind     c_alignas_kind_t;
typedef struct c_apple_block_ast  c_apple_block_ast_t;
typedef struct c_array_ast        c_array_ast_t;
typedef enum   c_array_kind       c_array_kind_t;
typedef struct c_ast              c_ast_t;
typedef slist_t                   c_ast_list_t;   ///< AST list.
typedef struct c_ast_pair         c_ast_pair_t;
typedef struct c_bit_field_ast    c_bit_field_ast_t;
typedef struct c_builtin_ast      c_builtin_ast_t;

/**
 * C++ lambda capture.
 *
 * @remarks The \ref slist_node::data "data" is the capture's AST.
 */
typedef slist_node_t              c_capture_t;

typedef struct c_capture_ast      c_capture_ast_t;
typedef enum   c_capture_kind     c_capture_kind_t;
typedef struct c_cast_ast         c_cast_ast_t;
typedef enum   c_cast_kind        c_cast_kind_t;
typedef struct c_constructor_ast  c_constructor_ast_t;
typedef struct c_csu_ast          c_csu_ast_t;
typedef struct c_enum_ast         c_enum_ast_t;
typedef enum   c_func_member      c_func_member_t;
typedef struct c_function_ast     c_function_ast_t;
typedef enum   c_graph            c_graph_t;
typedef struct c_lambda_ast       c_lambda_ast_t;
typedef uint32_t                  c_lang_id_t;    ///< Languages bitmask.
typedef struct c_loc              c_loc_t;

/**
 * Underlying source location numeric type for \ref c_loc.
 *
 * @remarks This should be an unsigned type, but Flex & Bison generate code
 * that assumes it's signed.  Making it unsigned generates warnings; hence this
 * is kept as signed to prevent the warnings.
 */
typedef short                     c_loc_num_t;

typedef enum   c_op_id            c_op_id_t;
typedef struct c_operator         c_operator_t;
typedef struct c_operator_ast     c_operator_ast_t;

/**
 * C/C++ function-like parameter.
 *
 * @remarks The \ref slist_node::data "data" is the parameter's AST.
 */
typedef slist_node_t              c_param_t;

typedef struct c_parent_ast       c_parent_ast_t;
typedef struct c_ptr_mbr_ast      c_ptr_mbr_ast_t;
typedef struct c_ptr_ref_ast      c_ptr_ref_ast_t;

/**
 * A particular scope in an \a ref c_sname_t.
 *
 * @remarks The \ref slist_node::data "data" is a \ref c_scope_data.
 */
typedef slist_node_t              c_scope_t;

typedef struct c_sglob            c_sglob_t;
typedef slist_t                   c_sname_t;      ///< C++ scoped name.
typedef uint64_t                  c_tid_t;        ///< Type ID(s) bits.
typedef struct c_typedef          c_typedef_t;
typedef struct c_typedef_ast      c_typedef_ast_t;
typedef struct c_type             c_type_t;
typedef struct c_udef_conv_ast    c_udef_conv_ast_t;
typedef struct c_udef_lit_ast     c_udef_lit_ast_t;
typedef enum   cdecl_debug        cdecl_debug_t;
typedef enum   cdecl_show         cdecl_show_t;

/**
 * C preprocessor macro argument list.
 *
 * @remarks
 * @parblock
 * Multiple tokens can comprise a macro argument, e.g.:
 *
 *     M(a b)
 *
 * Hence, each argument in an argument list is a \ref p_token_list_t.
 * @endparblock
 */
typedef slist_t                   p_arg_list_t;

typedef struct p_macro            p_macro_t;
typedef struct p_param            p_param_t;
typedef slist_t                   p_param_list_t; ///< Macro parameter list.
typedef struct p_token            p_token_t;
typedef slist_t                   p_token_list_t; ///< Preprocessor token list.
typedef slist_node_t              p_token_node_t; ///< Preprocessor token node.
typedef union  user_data          user_data_t;

typedef c_loc_t YYLTYPE;                ///< Source location type for Bison.
/// @cond DOXYGEN_IGNORE
#define YYLTYPE_IS_DECLARED       1
#define YYLTYPE_IS_TRIVIAL        1
/// @endcond

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

////////// structs ////////////////////////////////////////////////////////////


/**
 * The source location used by Flex & Bison.
 */
struct c_loc {
  c_loc_num_t first_line;               ///< First line of location range.
  c_loc_num_t first_column;             ///< First column of location range.
  c_loc_num_t last_line;                ///< Last line of location range.
  c_loc_num_t last_column;              ///< Last column of location range.
};

/**
 * Data for the `alignas` specifier.
 */
struct c_alignas {
  union {
    unsigned        bytes;              ///< Aligned to this number of bytes.
    c_ast_t        *type_ast;           ///< Aligned the same as this type.
  };
  c_alignas_kind_t  kind;               ///< Kind of `alignas` argument.
  c_loc_t           loc;                ///< Source location.
};

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

/**
 * Convenience macro for specifying a zero-initialized user_data literal.
 */
#define USER_DATA_ZERO            ((user_data_t){ .i64 = 0 })

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_types_H */
/* vim:set et sw=2 ts=2: */
