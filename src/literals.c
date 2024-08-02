/*
**      cdecl -- C gibberish translator
**      src/literals.c
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

/**
 * @file
 * Defines constants for **cdecl** and C/C++ language literals.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_LITERALS_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "literals.h"

///////////////////////////////////////////////////////////////////////////////

// english
char const L_align[]                    = "align";
char const L_aligned[]                  = "aligned";
char const L_all[]                      = "all";
char const L_array[]                    = "array";
char const L_as[]                       = "as";
char const L_bit[]                      = "bit", L_precise[] = "precise";
char const H_bit_precise[]              = "bit-precise";
char const L_bits[]                     = "bits";
char const L_boolean[]                  = "boolean";
char const L_Boolean[]                  = "Boolean";
char const L_by[]                       = "by";
char const L_bytes[]                    = "bytes";
char const L_cast[]                     = "cast";
char const L_command[]                  = "command";
char const L_commands[]                 = "commands";
char const L_copy[]                     = "copy";
char const L_declare[]                  = "declare";
char const L_defaulted[]                = "defaulted";
char const L_define[]                   = "define";
char const L_defined[]                  = "defined";
char const L_deleted[]                  = "deleted";
char const L_english[]                  = "english";
char const L_eval[]                     = "eval";
char const L_evaluation[]               = "evaluation";
char const L_exit[]                     = "exit";
char const L_expand[]                   = "expand";
char const L_explain[]                  = "explain";
char const L_expr[]                     = "expr";
char const L_expression[]               = "expression";
char const L_floating[]                 = "floating";
char const L_func[]                     = "func";
char const L_function[]                 = "function";
char const L_help[]                     = "help";
char const L_include[]                  = "include";
char const L_init[]                     = "init";
char const L_initialization[]           = "initialization";
char const L_into[]                     = "into";
char const L_linkage[]                  = "linkage";
char const L_macros[]                   = "macros";
char const L_mbr[]                      = "mbr";
char const L_member[]                   = "member";
char const L_no[]                       = "no";
char const H_non_empty[]                = "non-empty";
char const H_non_mbr[]                  = "non-mbr";
char const H_non_member[]               = "non-member";
char const L_of[]                       = "of";
char const L_options[]                  = "options";
char const L_point[]                    = "point";
char const L_pointer[]                  = "pointer";
char const L_precision[]                = "precision";
char const L_predef[]                   = "predef";
char const L_predefined[]               = "predefined";
char const L_ptr[]                      = "ptr";
char const L_quit[]                     = "quit";
char const L_ref[]                      = "ref";
char const L_reference[]                = "reference";
char const L_ret[]                      = "ret";
char const L_returning[]                = "returning";
char const L_rvalue[]                   = "rvalue";
char const L_set[]                      = "set";
char const L_show[]                     = "show";
char const L_to[]                       = "to";
char const L_user[]                     = "user";
char const L_vector[]                   = "vector";
char const L_wide[]                     = "wide";
char const L_width[]                    = "width";

// options
char const L_alt_tokens[]               = "alt-tokens";
#ifdef ENABLE_BISON_DEBUG
char const L_bison_debug[]              = "bison-debug";
#endif /* ENABLE_BISON_DEBUG */
char const L_color[]                    = "color";
char const L_config[]                   = "config";
char const L_debug[]                    = "debug";
char const L_digraphs[]                 = "digraphs";
char const L_east_const[]               = "east-const";
char const L_echo_commands[]            = "echo-commands";
char const L_english_types[]            = "english-types";
char const L_explicit_ecsu[]            = "explicit-ecsu";
char const L_explicit_int[]             = "explicit-int";
char const L_file[]                     = "file";
#ifdef ENABLE_FLEX_DEBUG
char const L_flex_debug[]               = "flex-debug";
#endif /* ENABLE_FLEX_DEBUG */
char const L_infer_command[]            = "infer-command";
char const L_language[]                 = "language";
char const L_lineno[]                   = "lineno";
char const L_output[]                   = "output";
char const L_permissive_types[]         = "permissive-types";
char const L_prompt[]                   = "prompt";
char const L_semicolon[]                = "semicolon";
char const L_trailing_return[]          = "trailing-return";
char const L_trigraphs[]                = "trigraphs";
char const L_version[]                  = "version";
char const L_west_decl[]                = "west-decl";

// C Preprocessor
char const L_PRE_define[]               = "define";
char const L_PRE_elif[]                 = "elif";
char const L_PRE_else[]                 = "else";
char const L_PRE_error[]                = "error";
char const L_PRE_if[]                   = "if";
char const L_PRE_ifdef[]                = "ifdef";
char const L_PRE_ifndef[]               = "ifndef";
//         L_PRE_include[]              // handled within the lexer
char const L_PRE_line[]                 = "line";
char const L_PRE_undef[]                = "undef";

char const L_PRE_P_define[]             = "#define";
char const L_PRE_P_include[]            = "#include";
char const L_PRE_P_undef[]              = "#undef";

// C99 Preprocessor
char const L_PRE_pragma[]               = "pragma";

// C23 Preprocessor
char const L_PRE_elifdef[]              = "elifdef";
char const L_PRE_elifndef[]             = "elifndef";
char const L_PRE_embed[]                = "embed";
char const L_PRE_warning[]              = "warning";

// K&R
char const L_auto[]                     = "auto";
char const L_automatic[]                = "automatic";
char const L_break[]                    = "break";
char const L_case[]                     = "case";
char const L_char[]                     = "char";
char const L_character[]                = "character";
char const L_continue[]                 = "continue";
char const L_default[]                  = "default";
char const L_do[]                       = "do";
char const L_double[]                   = "double";
char const H_double_precision[]         = "double-precision";
char const L_else[]                     = "else";
char const L_extern[]                   = "extern";
char const L_external[]                 = "external";
char const L_float[]                    = "float";
char const H_floating_point[]           = "floating-point";
char const L_for[]                      = "for";
char const L_goto[]                     = "goto";
char const L_if[]                       = "if";
char const L_int[]                      = "int";
char const L_integer[]                  = "integer";
char const L_long[]                     = "long";
char const L_register[]                 = "register";
char const L_return[]                   = "return";
char const L_short[]                    = "short";
char const L_sizeof[]                   = "sizeof";
char const L_static[]                   = "static";
char const L_struct[]                   = "struct";
char const L_structure[]                = "structure";
char const L_switch[]                   = "switch";
char const L_type[]                     = "type";
char const L_typedef[]                  = "typedef";
char const L_union[]                    = "union";
char const L_unsigned[]                 = "unsigned";
char const L_while[]                    = "while";

// C89
char const L_asm[]                      = "asm";
char const L_const[]                    = "const";
char const L_constant[]                 = "constant";
char const L_ELLIPSIS[]                 = "...";
char const L_enum[]                     = "enum";
char const L_enumeration[]              = "enumeration";
char const L_signed[]                   = "signed";
char const L_varargs[]                  = "varargs";
char const L_variadic[]                 = "variadic";
char const L_void[]                     = "void";
char const L_volatile[]                 = "volatile";

// C99
char const L__Bool[]                    = "_Bool";
char const L__Complex[]                 = "_Complex";
char const L_complex[]                  = "complex";
char const L__Imaginary[]               = "_Imaginary";
char const L_imaginary[]                = "imaginary";
char const L_inline[]                   = "inline";
char const L_len[]                      = "len";
char const L_length[]                   = "length";
char const L_restrict[]                 = "restrict";
char const L_restricted[]               = "restricted";
char const L_var[]                      = "var";
char const L_variable[]                 = "variable";
char const L_wchar_t[]                  = "wchar_t";

// C99 Preprocessor
char const L_PRE___VA_ARGS__[]          = "__VA_ARGS__";

// C11
char const L__Alignas[]                 = "_Alignas";
char const L__Alignof[]                 = "_Alignof";
char const L__Atomic[]                  = "_Atomic";
char const L_atomic[]                   = "atomic";
char const L__Generic[]                 = "_Generic";
char const L__Noreturn[]                = "_Noreturn";
char const H_no_return[]                = "no-return";
char const H_non_returning[]            = "non-returning";
char const L__Static_assert[]           = "_Static_assert";
char const L__Thread_local[]            = "_Thread_local";
char const L_thread[]                   = "thread", L_local[] = "local";
char const H_thread_local[]             = "thread-local";

// C++
char const L_bool[]                     = "bool";
char const L_catch[]                    = "catch";
char const L_class[]                    = "class";
char const L_constructor[]              = "constructor";
char const L_const_cast[]               = "const_cast";
char const L_conv[]                     = "conv";
char const L_conversion[]               = "conversion";
char const L_ctor[]                     = "ctor";
char const L_delete[]                   = "delete";
char const L_destructor[]               = "destructor";
char const L_dtor[]                     = "dtor";
char const L_dynamic[]                  = "dynamic";
char const L_dynamic_cast[]             = "dynamic_cast";
char const L_explicit[]                 = "explicit";
char const L_false[]                    = "false";
char const L_friend[]                   = "friend";
char const L_mutable[]                  = "mutable";
char const L_namespace[]                = "namespace";
char const L_new[]                      = "new";
char const H_non_throwing[]             = "non-throwing";
char const L_oper[]                     = "oper";
char const L_operator[]                 = "operator";
char const L_private[]                  = "private";
char const L_protected[]                = "protected";
char const L_public[]                   = "public";
char const L_pure[]                     = "pure";
char const L_reinterpret[]              = "reinterpret";
char const L_reinterpret_cast[]         = "reinterpret_cast";
char const L_scope[]                    = "scope";
char const L_static_cast[]              = "static_cast";
char const L_template[]                 = "template";
char const L_this[]                     = "this";
char const L_throw[]                    = "throw";
char const L_true[]                     = "true";
char const L_try[]                      = "try";
char const L_typeid[]                   = "typeid";
char const L_typename[]                 = "typename";
char const L_using[]                    = "using";
char const L_virtual[]                  = "virtual";

// C++11
char const L_alignas[]                  = "alignas";
char const L_alignof[]                  = "alignof";
char const L_capture[]                  = "capture";
char const L_captures[]                 = "captures";
char const L_capturing[]                = "capturing";
char const L_carries[]                  = "carries", L_dependency[] = "dependency";
char const L_carries_dependency[]       = "carries_dependency";
char const H_carries_dependency[]       = "carries-dependency";
char const L_decltype[]                 = "decltype";
char const L_final[]                    = "final";
char const L_literal[]                  = "literal";
char const L_noexcept[]                 = "noexcept";
char const L_except[]                   = "except";
char const L_lambda[]                   = "lambda";
char const H_no_except[]                = "no-except";
char const H_no_exception[]             = "no-exception";
char const L_noreturn[]                 = "noreturn";
char const L_override[]                 = "override";
char const L_overridden[]               = "overridden";
char const H_user_def[]                 = "user-def";
char const H_user_defined[]             = "user-defined";

// C11 & C++11
char const L_char16_t[]                 = "char16_t";
char const L_char32_t[]                 = "char32_t";

// C23
char const L__BitInt[]                  = "_BitInt";
char const L_reproducible[]             = "reproducible";
char const L_typeof[]                   = "typeof";
char const L_typeof_unqual[]            = "typeof_unqual";
char const L_unsequenced[]              = "unsequenced";

// C23 & C++11
char const L_constexpr[]                = "constexpr";
char const H_const_expr[]               = "const-expr";
char const H_constant_expression[]      = "constant-expression";
char const L_nullptr[]                  = "nullptr";
char const L_static_assert[]            = "static_assert";
char const L_thread_local[]             = "thread_local";

// C23 & C++14
char const L_deprecated[]               = "deprecated";
char const L___deprecated__[]           = "__deprecated__";

// C23 & C++20 Preprocessor
char const L_PRE___VA_OPT__[]           = "__VA_OPT__";

// C++17
char const L_maybe_unused[]             = "maybe_unused";
char const L_maybe[]                    = "maybe", L_unused[] = "unused";
char const H_maybe_unused[]             = "maybe-unused";
char const L___maybe_unused__[]         = "__maybe_unused__";
char const L_nodiscard[]                = "nodiscard";
char const L_discard[]                  = "discard";
char const H_no_discard[]               = "no-discard";
char const L___nodiscard__[]            = "__nodiscard__";
char const H_non_discardable[]          = "non-discardable";
char const L_structured[]               = "structured";
char const L_binding[]                  = "binding";

// C++20
char const L_char8_t[]                  = "char8_t";
char const L_concept[]                  = "concept";
char const L_consteval[]                = "consteval";
char const H_const_eval[]               = "const-eval";
char const H_constant_evaluation[]      = "constant-evaluation";
char const L_constinit[]                = "constinit";
char const H_const_init[]               = "const-init";
char const H_constant_initialization[]  = "constant-initialization";
char const L_co_await[]                 = "co_await";
char const L_co_return[]                = "co_return";
char const L_co_yield[]                 = "co_yield";
char const L_export[]                   = "export";
char const L_exported[]                 = "exported";
char const L_no_unique_address[]        = "no_unique_address";
char const L_unique[]                   = "unique", L_address[] = "address";
char const H_no_unique_address[]        = "no-unique-address";
char const H_non_unique_address[]       = "non-unique-address";
char const L_pack[]                     = "pack";
char const L_parameter[]                = "parameter";
char const L_requires[]                 = "requires";

// Alternative tokens
char const L_and[]                      = "and";
char const L_and_eq[]                   = "and_eq";
char const L_bitand[]                   = "bitand";
char const L_bitor[]                    = "bitor";
char const L_compl[]                    = "compl";
char const L_not[]                      = "not";
char const L_not_eq[]                   = "not_eq";
char const L_or[]                       = "or";
char const L_or_eq[]                    = "or_eq";
char const L_xor[]                      = "xor";
char const L_xor_eq[]                   = "xor_eq";

// Embedded C extensions
char const L_EMC__Accum[]               = "_Accum";
char const L_EMC_accum[]                = "accum";
char const L_EMC__Fract[]               = "_Fract";
char const L_EMC_fract[]                = "fract";
char const L_EMC__Sat[]                 = "_Sat";
char const L_EMC_sat[]                  = "sat";
char const L_EMC_saturated[]            = "saturated";

// Unified Parallel C extensions
char const L_UPC_relaxed[]              = "relaxed";
char const L_UPC_shared[]               = "shared";
char const L_UPC_strict[]               = "strict";

// GNU extensions
char const L_GNU___attribute__[]        = "__attribute__";
char const L_GNU___auto_type[]          = "__auto_type";
char const L_GNU___complex[]            = "__complex";
char const L_GNU___complex__[]          = "__complex__";
char const L_GNU___const[]              = "__const";
char const L_GNU___inline[]             = "__inline";
char const L_GNU___inline__[]           = "__inline__";
char const L_GNU___restrict[]           = "__restrict";
char const L_GNU___restrict__[]         = "__restrict__";
char const L_GNU___signed[]             = "__signed";
char const L_GNU___signed__[]           = "__signed__";
char const L_GNU___thread[]             = "__thread";
char const L_GNU___typeof__[]           = "__typeof__";
char const L_GNU___volatile[]           = "__volatile";
char const L_GNU___volatile__[]         = "__volatile__";

// Apple extensions
char const L_Apple___block[]            = "__block";
char const L_Apple_block[]              = "block";    // English for '^'

// Microsoft extensions
char const L_MSC__asm[]                 = "_asm";
char const L_MSC___asm[]                = "__asm";
char const L_MSC__cdecl[]               = "_cdecl";
char const L_MSC___cdecl[]              = "__cdecl";
char const L_MSC_cdecl[]                = "cdecl";
char const L_MSC___clrcall[]            = "__clrcall";
char const L_MSC_clrcall[]              = "clrcall";
char const L_MSC__declspec[]            = "_declspec";
char const L_MSC___declspec[]           = "__declspec";
char const L_MSC__fastcall[]            = "_fastcall";
char const L_MSC___fastcall[]           = "__fastcall";
char const L_MSC_fastcall[]             = "fastcall";
char const L_MSC__forceinline[]         = "_forceinline";
char const L_MSC___forceinline[]        = "__forceinline";
char const L_MSC__inline[]              = "_inline";
char const L_MSC__restrict[]            = "_restrict";
char const L_MSC__stdcall[]             = "_stdcall";
char const L_MSC___stdcall[]            = "__stdcall";
char const L_MSC_stdcall[]              = "stdcall";
char const L_MSC___thiscall[]           = "__thiscall";
char const L_MSC_thiscall[]             = "thiscall";
char const L_MSC__vectorcall[]          = "_vectorcall";
char const L_MSC___vectorcall[]         = "__vectorcall";
char const L_MSC_vectorcall[]           = "vectorcall";
char const L_MSC_WINAPI[]               = "WINAPI";

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
