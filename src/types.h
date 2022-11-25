/*
**      cdecl -- C gibberish translator
**      src/types.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
 * @ingroup c-kinds-group
 * Kinds of AST nodes comprising a C/C++ declaration.
 *
 * A given AST node may only have a single kind and _not_ be a bitwise-or of
 * kinds.  However, a bitwise-or of kinds may be used to test whether a given
 * AST node is any _one_ of those kinds.
 */
enum c_ast_kind {
  /**
   * Temporary node in an AST.  This is needed in two cases:
   *
   * 1. Array declarations or casts.  Consider:
   *
   *         int a[2][3]
   *
   *    At the first `[`, we know it's an _array 2 of [something of]*_ `int`,
   *    but we don't yet know either what the "something" is or whether it will
   *    turn out to be nothing.  It's not until the second `[` that we know
   *    it's an _array 2 of array 3 of_ `int`.  (Had the `[3]` not been there,
   *    then it would have been just _array 2 of_ `int`.)
   *
   * 2. Nested declarations or casts (inside parentheses).  Consider:
   *
   *         int (*a)[2]
   *
   *    At the `*`, we know it's a _pointer to [something of]*_ `int`, but,
   *    similar to the array case, we don't yet know either what the
   *    "something" is or whether it will turn out to be nothing.  It's not
   *    until the `[` that we know it's a _pointer to array 2 of_ `int`.  (Had
   *    the `[2]` not been there, then it would have been just _pointer to_
   *    `int` (with unnecessary parentheses).
   *
   * In either case, a placeholder node is created to hold the place of the
   * "something" in the AST.
   */
  K_PLACEHOLDER             = (1u << 0),

  /**
   * Built-in type, e.g., `void`, `char`, `int`, etc.
   */
  K_BUILTIN                 = (1u << 1),

  /**
   * A `class,` `struct,` or `union`.
   */
  K_CLASS_STRUCT_UNION      = (1u << 2),

  /**
   * Name only.  It's used as the initial kind for an identifier ("name") until
   * we know its actual type (if ever).  However, it's also used for pre-
   * prototype typeless function parameters in K&R C, e.g., `double sin(x)`.
   */
  K_NAME                    = (1u << 3),

  /**
   * `typedef` type, e.g., `size_t`.
   */
  K_TYPEDEF                 = (1u << 4),

  /**
   * Variadic (`...`) function parameter.
   */
  K_VARIADIC                = (1u << 5),

  ////////// "parent" kinds ///////////////////////////////////////////////////

  /**
   * Array.
   */
  K_ARRAY                   = (1u << 6),

  /**
   * Cast.
   */
  K_CAST                    = (1u << 7),

  /**
   * An `enum`.
   *
   * @note This is a "parent" kind because `enum` in C++11 and later can be
   * "of" a fixed type.
   */
  K_ENUM                    = (1u << 8),

  /**
   * C or C++ pointer.
   */
  K_POINTER                 = (1u << 9),

  /**
   * C++ pointer-to-member.
   */
  K_POINTER_TO_MEMBER       = (1u << 10),

  /**
   * C++ reference.
   */
  K_REFERENCE               = (1u << 11),

  /**
   * C++11 rvalue reference.
   */
  K_RVALUE_REFERENCE        = (1u << 12),

  ////////// function-like "parent" kinds /////////////////////////////////////

  /**
   * C++ constructor.
   */
  K_CONSTRUCTOR             = (1u << 13),

  /**
   * C++ destructor.
   */
  K_DESTRUCTOR              = (1u << 14),

  ////////// function-like "parent" kinds that have return values /////////////

  /**
   * Block (Apple extension).
   *
   * @sa [Apple's Extensions to C](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1370.pdf)
   * @sa [Blocks Programming Topics](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks)
   */
  K_APPLE_BLOCK             = (1u << 15),

  /**
   * Function.
   */
  K_FUNCTION                = (1u << 16),

  /**
   * C++ overloaded operator.
   */
  K_OPERATOR                = (1u << 17),

  /**
   * C++ user-defined conversion operator.
   */
  K_USER_DEF_CONVERSION     = (1u << 18),

  /**
   * C++11 user-defined literal.
   */
  K_USER_DEF_LITERAL        = (1u << 19),
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
 * @ingroup c-keywords-group
 * C++ keyword contexts.  A context specifies where particular literals are
 * recognized as keywords in gibberish.  For example, `final` and `override`
 * are recognized as keywords only within C++ member function declarations.
 *
 * @note These matter only when converting gibberish to pseudo-English.
 */
enum c_keyword_ctx {
  C_KW_CTX_DEFAULT,                     ///< Default context.
  C_KW_CTX_ATTRIBUTE,                   ///< Attribute declaration.
  C_KW_CTX_MBR_FUNC                     ///< Member function declaration.
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
  C_OP_CIRC,            ///< The `^` operator.
  C_OP_CIRC_EQ,         ///< The `^=` operator.
  C_OP_PIPE,            ///< The `|` operator.
  C_OP_PIPE_EQ,         ///< The `|=` operator.
  C_OP_PIPE2,           ///< The `||` operator.
  C_OP_TILDE,           ///< The `~` operator.
};

/**
 * @ingroup c-types-group
 * For \ref c_tid_t values, the low-order 4 bits specify the type part ID and
 * thus how the value should be interpreted.
 */
enum c_tpid {
  //
  // Type part IDs start at 1 so we know a c_tid_t value has been initialized
  // properly as opposed to it being 0 by default.
  //
  C_TPID_NONE   = 0u,                   ///< No types.
  C_TPID_BASE   = (1u << 0),            ///< Base types, e.g., `int`.
  C_TPID_STORE  = (1u << 1),            ///< Storage types, e.g., `static`.
  C_TPID_ATTR   = (1u << 2)             ///< Attributes.
};

/**
 * The direction to traverse an AST using c_ast_visit().
 */
enum c_visit_dir {
  C_VISIT_DOWN,                         ///< Root to leaves.
  C_VISIT_UP                            ///< Leaf to root.
};

/**
 * The kind of **cdecl** command.
 */
enum cdecl_command_kind {
  /**
   * Command is valid _only_ within the **cdecl** language and _not_ as either
   * the command-line command (`argv[0]`) or the first word of the first
   * command-line argument (`argv[1]`):
   *
   * `cdecl>` _command_ _args_
   */
  CDECL_COMMAND_LANG_ONLY,

  /**
   * Same as \ref CDECL_COMMAND_LANG_ONLY, but command is also valid as the
   * first word of the first command-line argument (`argv[1]`):
   *
   * `$ cdecl` _command_ _args_
   */
  CDECL_COMMAND_FIRST_ARG,

  /**
   * Same as \ref CDECL_COMMAND_FIRST_ARG, but command is also valid as the
   * program name (`argv[0]`):
   *
   * `$` _command_ _args_
   */
  CDECL_COMMAND_PROG_NAME,
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

/**
 * @ingroup set-options-group
 * **cdecl** `set` option kind.
 */
enum set_option_kind {
  SET_OPTION_TOGGLE,                    ///< Toggle, e.g., `foo` & `nofoo`.
  SET_OPTION_AFF_ONLY,                  ///< Affirmative only, e.g., `foo`.
  SET_OPTION_NEG_ONLY                   ///< Negative only, e.g., `nofoo`.
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
typedef enum   c_ast_kind         c_ast_kind_t;
typedef slist_t                   c_ast_list_t;   ///< AST list.
typedef struct c_ast_pair         c_ast_pair_t;
typedef int                       c_ast_sid_t;    ///< Signed \ref c_ast_id_t.
typedef struct c_bit_field_ast    c_bit_field_ast_t;
typedef struct c_builtin_ast      c_builtin_ast_t;
typedef struct c_cast_ast         c_cast_ast_t;
typedef enum   c_cast_kind        c_cast_kind_t;
typedef struct c_constructor_ast  c_constructor_ast_t;
typedef struct c_csu_ast          c_csu_ast_t;
typedef struct c_enum_ast         c_enum_ast_t;
typedef struct c_function_ast     c_function_ast_t;
typedef enum   c_graph            c_graph_t;
typedef struct c_keyword          c_keyword_t;
typedef enum   c_keyword_ctx      c_keyword_ctx_t;
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
typedef enum   c_tpid             c_tpid_t;
typedef struct c_typedef          c_typedef_t;
typedef struct c_typedef_ast      c_typedef_ast_t;
typedef struct c_type             c_type_t;
typedef struct c_udef_conv_ast    c_udef_conv_ast_t;
typedef struct c_udef_lit_ast     c_udef_lit_ast_t;
typedef enum   c_visit_dir        c_visit_dir_t;
typedef struct cdecl_command      cdecl_command_t;
typedef enum   cdecl_command_kind cdecl_command_kind_t;
typedef enum   cdecl_help         cdecl_help_t;
typedef struct cdecl_keyword      cdecl_keyword_t;
typedef enum   cdecl_mode         cdecl_mode_t;
typedef struct print_params       print_params_t;
typedef struct set_option         set_option_t;
typedef struct set_option_fn_args set_option_fn_args_t;
typedef enum   set_option_kind    set_option_kind_t;

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

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_types_H */
/* vim:set et sw=2 ts=2: */
