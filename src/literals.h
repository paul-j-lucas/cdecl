/*
**      cdecl -- C gibberish translator
**      src/literals.h
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

#ifndef cdecl_literals_H
#define cdecl_literals_H

/**
 * @file
 * Declares constants for **cdecl** and C/C++ language literals.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_lang.h"

/// @cond DOXYGEN_IGNORE

_GL_INLINE_HEADER_BEGIN
#ifndef C_LITERALS_H_INLINE
# define C_LITERALS_H_INLINE _GL_INLINE
#endif /* C_LITERALS_H_INLINE */

/// @endcond

/**
 * @defgroup literals-group Literal Strings
 * Constants for **cdecl** and C/C++ language literals.
 * @{
 */

////////// literals ///////////////////////////////////////////////////////////

// A literal starting with "H_" is pseudo-English for the same literal starting
// with "L_" and are spelled the same except that '-' replaces '_' ("H_" for
// "hyphen").

// If you add a new literal and it is:
//
//  + A cdecl command only, update CDECL_KEYWORDS in cdecl_keyword.c and
//    CDECL_COMMANDS in cdecl_command.c.
//
//  + A cdecl command that is also a C/C++ keyword -OR- a C/C++ keyword only,
//    update C_KEYWORDS in c_keywords.c.

// english
extern char const L_align[];
extern char const L_aligned[];
extern char const L_all[];
extern char const L_array[];
extern char const L_as[];
extern char const L_bit[], L_precise[];
extern char const H_bit_precise[];
extern char const L_bits[];
extern char const L_boolean[];
extern char const L_Boolean[];
extern char const L_by[];
extern char const L_bytes[];
extern char const L_cast[];
extern char const L_command[];            // synonym for "commands"
extern char const L_commands[];
extern char const L_copy[];
extern char const L_declare[];
extern char const L_defaulted[];          // synonym for "default"
extern char const L_define[];
extern char const L_defined[];
extern char const L_deleted[];            // synonym for "delete"
extern char const L_english[];
extern char const L_eval[];               // synonym for "evaluation"
extern char const L_evaluation[];
extern char const L_exit[];
extern char const L_expand[];
extern char const L_explain[];
extern char const L_expr[];               // synonym for "expression"
extern char const L_expression[];
extern char const L_floating[];           // "floating point"
extern char const L_func[];               // synonym for "function"
extern char const L_function[];
extern char const L_help[];
extern char const L_include[];
extern char const L_init[];               // synonym for "initialization"
extern char const L_initialization[];
extern char const L_into[];
extern char const L_linkage[];
extern char const L_macros[];
extern char const L_mbr[];                // synonym for "member"
extern char const L_member[];
extern char const L_no[];
extern char const H_non_empty[];
extern char const H_non_mbr[];            // synonym for "non-member"
extern char const H_non_member[];
extern char const L_of[];
extern char const L_options[];
extern char const L_point[];
extern char const L_pointer[];
extern char const L_precision[];
extern char const L_predef[];             // synonym for "predefined"
extern char const L_predefined[];
extern char const L_ptr[];                // synonym for "pointer"
extern char const L_quit[];
extern char const L_ref[];                // synonym for "reference"
extern char const L_reference[];
extern char const L_ret[];                // synonym for "returning"
extern char const L_returning[];
extern char const L_rvalue[];
extern char const L_set[];
extern char const L_show[];
extern char const L_to[];
extern char const L_user[];
extern char const L_vector[];             // synonym for "array"
extern char const L_wide[];               // "wide character"
extern char const L_width[];

// C Preprocessor
extern char const PL_define[];
extern char const PL_elif[];
extern char const PL_else[];
extern char const PL_error[];
extern char const PL_if[];
extern char const PL_ifdef[];
extern char const PL_ifndef[];
//     char const PL_include[];           // handled within the lexer
extern char const PL_line[];
extern char const PL_undef[];

extern char const PL_P_define[];          // combined "#define"
extern char const PL_P_include[];         // combined "#include"
extern char const PL_P_undef[];           // combined "#undef"

// C99 Preprocessor
extern char const PL_pragma[];

// C23 Preprocessor
extern char const PL_elifdef[];
extern char const PL_elifndef[];
extern char const PL_embed[];
extern char const PL_warning[];


// K&R
extern char const L_auto[];
extern char const L_automatic[];          // synonym for "auto", "__auto_type"
extern char const L_break[];
extern char const L_case[];
extern char const L_char[];
extern char const L_character[];          // English for "char"
extern char const L_continue[];
extern char const L_default[];
extern char const L_do[];
extern char const L_double[];
extern char const H_double_precision[];   // English for "double"
extern char const L_else[];
extern char const L_extern[];
extern char const L_external[];           // English for "extern"
extern char const L_float[];
extern char const H_floating_point[];     // English for "float"
extern char const L_for[];
extern char const L_goto[];
extern char const L_if[];
extern char const L_int[];
extern char const L_integer[];            // English for "int"
extern char const L_long[];
extern char const L_register[];
extern char const L_return[];
extern char const L_short[];
extern char const L_sizeof[];
extern char const L_static[];
extern char const L_struct[];
extern char const L_structure[];          // English for "struct"
extern char const L_switch[];
extern char const L_type[];               // English for "typedef"
extern char const L_typedef[];
extern char const L_union[];
extern char const L_unsigned[];
extern char const L_while[];

// C89
extern char const L_asm[];
extern char const L_const[];
extern char const L_constant[];           // English for "const"
extern char const L_ellipsis[];           // ...
extern char const L_enum[];
extern char const L_enumeration[];        // English for "enum"
extern char const L_signed[];
extern char const L_varargs[];            // synonym for "..."
extern char const L_variadic[];           // synonym for "..."
extern char const L_void[];
extern char const L_volatile[];

// C99
extern char const L__Bool[];
extern char const L__Complex[];
extern char const L_complex[];            // synonym for "_Complex"
extern char const L__Imaginary[];
extern char const L_imaginary[];          // synonym for "_Imaginary"
extern char const L_inline[];
extern char const L_len[];                // synonym for "length"
extern char const L_length[];             // for "variable [length] array"
extern char const L_restrict[];
extern char const L_restricted[];         // synonym for "restrict"
extern char const L_var[];                // synonym for "variable"
extern char const L_variable[];           // for "variable [length] array"
extern char const L_wchar_t[];

// C99 Preprocessor
extern char const PL___VA_ARGS__[];

// C11
extern char const L__Alignas[];
extern char const L__Alignof[];
extern char const L__Atomic[];
extern char const L_atomic[];             // synonym for "_Atomic"
extern char const L__Generic[];
extern char const L__Noreturn[];
extern char const H_no_return[];          // English for "_Noreturn"
extern char const H_non_returning[];      // English for "_Noreturn"
extern char const L__Static_assert[];
extern char const L__Thread_local[];
extern char const L_thread[], L_local[];
extern char const H_thread_local[];       // English for "_Thread_local"

// C++
extern char const L_bool[];
extern char const L_catch[];
extern char const L_class[];
extern char const L_constructor[];
extern char const L_const_cast[];
extern char const L_conv[];               // synonym for "conversion"
extern char const L_conversion[];
extern char const L_ctor[];               // synonym for "constructor"
extern char const L_delete[];
extern char const L_destructor[];
extern char const L_dtor[];               // synonym for "destructor"
extern char const L_dynamic[];
extern char const L_dynamic_cast[];
extern char const L_explicit[];
extern char const L_false[];
extern char const L_friend[];
extern char const L_mutable[];
extern char const L_namespace[];
extern char const L_new[];
extern char const H_non_throwing[];       // English for "throw"
extern char const L_oper[];               // synonym for "operator"
extern char const L_operator[];
extern char const L_private[];
extern char const L_protected[];
extern char const L_public[];
extern char const L_pure[];
extern char const L_reinterpret[];
extern char const L_reinterpret_cast[];
extern char const L_scope[];
extern char const L_static_cast[];
extern char const L_template[];
extern char const L_this[];
extern char const L_throw[];
extern char const L_true[];
extern char const L_try[];
extern char const L_typeid[];
extern char const L_typename[];
extern char const L_using[];
extern char const L_virtual[];

// C++11
extern char const L_alignas[];
extern char const L_alignof[];
extern char const L_capture[];            // synonym for "capturing"
extern char const L_captures[];           // synonym for "capturing"
extern char const L_capturing[];
extern char const L_carries_dependency[];
extern char const L_carries[], L_dependency[];
extern char const H_carries_dependency[]; // English for "carries_dependency"
extern char const L_decltype[];
extern char const L_final[];
extern char const L_lambda[];
extern char const L_noexcept[];
extern char const L_except[];
extern char const L_noreturn[];
extern char const L_literal[];
extern char const H_no_except[];          // English for "noexcept"
extern char const H_no_exception[];       // English for "noexcept"
extern char const L_override[];
extern char const L_overridden[];         // English for "override"
extern char const H_user_def[];           // synonym for "user-defined"
extern char const H_user_defined[];

// C11 & C++11
extern char const L_char16_t[];
extern char const L_char32_t[];

// C23
extern char const L__BitInt[];
extern char const L_reproducible[];
extern char const L_typeof[];
extern char const L_typeof_unqual[];
extern char const L_unsequenced[];

// C23 & C++11
extern char const L_constexpr[];
extern char const H_const_expr[];
extern char const H_constant_expression[];
extern char const L_nullptr[];
extern char const L_static_assert[];
extern char const L_thread_local[];

// C23 & C++14
extern char const L_deprecated[];
extern char const L___deprecated__[];

// C23 & C++20 Preprocessor
extern char const PL___VA_OPT__[];

// C++17
extern char const L_maybe_unused[];
extern char const L_maybe[], L_unused[];
extern char const H_maybe_unused[];       // English for "maybe_unused"
extern char const L___maybe_unused__[];
extern char const L_nodiscard[];
extern char const L_discard[];
extern char const H_no_discard[];         // English for "nodiscard"
extern char const L___nodiscard__[];
extern char const H_non_discardable[];    // English for "nodiscard"

// C++20
extern char const L_char8_t[];
extern char const L_concept[];
extern char const L_consteval[];
extern char const H_const_eval[];
extern char const H_constant_evaluation[];
extern char const L_constinit[];
extern char const H_const_init[];
extern char const H_constant_initialization[];
extern char const L_co_await[];
extern char const L_co_return[];
extern char const L_co_yield[];
extern char const L_export[];
extern char const L_exported[];           // English for "export"
extern char const L_no_unique_address[];
extern char const L_unique[], L_address[];
extern char const H_no_unique_address[];  // English for "no_unique_address"
extern char const H_non_unique_address[]; // English for "no_unique_address"
extern char const L_requires[];

// Alternative tokens
extern char const L_and[];                // &&
extern char const L_and_eq[];             // &=
extern char const L_bitand[];             // &
extern char const L_bitor[];              // |
extern char const L_compl[];              // ~
extern char const L_not[];                // !
extern char const L_not_eq[];             // !=
extern char const L_or[];                 // ||
extern char const L_or_eq[];              // |=
extern char const L_xor[];                // ^
extern char const L_xor_eq[];             // ^=

// Embedded C extensions
extern char const L_EMC__Accum[];
extern char const L_EMC_accum[];          // synonym for "_Accum"
extern char const L_EMC__Fract[];
extern char const L_EMC_fract[];          // synonym for "_Fract"
extern char const L_EMC__Sat[];
extern char const L_EMC_sat[];            // synonym for "_Sat"
extern char const L_EMC_saturated[];      // English for "_Sat"

// Unified Parallel C extensions
extern char const L_UPC_relaxed[];
extern char const L_UPC_shared[];
extern char const L_UPC_strict[];

// GNU extensions
extern char const L_GNU___attribute__[];
extern char const L_GNU___auto_type[];
extern char const L_GNU___complex[];
extern char const L_GNU___complex__[];
extern char const L_GNU___const[];
extern char const L_GNU___inline[];
extern char const L_GNU___inline__[];
extern char const L_GNU___restrict[];
extern char const L_GNU___restrict__[];
extern char const L_GNU___signed[];
extern char const L_GNU___signed__[];
extern char const L_GNU___thread[];
extern char const L_GNU___typeof__[];
extern char const L_GNU___volatile[];
extern char const L_GNU___volatile__[];

// Apple extensions
extern char const L_Apple___block[];      // storage class
extern char const L_Apple_block[];        // Engligh for '^'

// Microsoft extensions
//
// Only some of these keywords have both two and one leading underscore; see
// <https://docs.microsoft.com/en-us/cpp/cpp/keywords-cpp?#microsoft-specific-c-keywords>.
extern char const L_MSC__asm[];
extern char const L_MSC___asm[];
extern char const L_MSC__cdecl[];
extern char const L_MSC___cdecl[];
extern char const L_MSC_cdecl[];          // English for "__cdecl"
extern char const L_MSC___clrcall[];
extern char const L_MSC_clrcall[];        // English for "__clrcall"
extern char const L_MSC__declspec[];
extern char const L_MSC___declspec[];
extern char const L_MSC__fastcall[];
extern char const L_MSC___fastcall[];
extern char const L_MSC_fastcall[];       // English for "__fastcall"
extern char const L_MSC__forceinline[];
extern char const L_MSC___forceinline[];
extern char const L_MSC__inline[];
extern char const L_MSC__restrict[];
extern char const L_MSC__stdcall[];
extern char const L_MSC___stdcall[];
extern char const L_MSC_stdcall[];        // English for "__stdcall"
extern char const L_MSC___thiscall[];
extern char const L_MSC_thiscall[];       // English for "__thiscall"
extern char const L_MSC__vectorcall[];
extern char const L_MSC___vectorcall[];
extern char const L_MSC_vectorcall[];     // English for "__vectorcall"
extern char const L_MSC_WINAPI[];         // synonym for "__stdcall"

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the alignas literal for the current language.
 *
 * @return Returns either `_Alignas` (for C17 or earlier) or `alignas` (for C23
 * or later, or C++).
 */
NODISCARD C_LITERALS_H_INLINE
char const* alignas_name( void ) {
  return OPT_LANG_IS( alignas ) ? L_alignas : L__Alignas;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_literals_H */
/* vim:set et sw=2 ts=2: */
