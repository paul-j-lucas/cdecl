/*
**      cdecl -- C gibberish translator
**      src/cdeck_keyword.cpp
**
**      Copyright (C) 2021-2023  Paul J. Lucas
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
 * Defined types, data, and functions for looking up **cdecl** keyword
 * information.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_lang.h"
#include "cdecl_keyword.h"
#include "literals.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * For \ref CDECL_KEYWORDS, used to specify which keywords to autocomplete next
 * (after the current keyword).
 *
 * @param ... A comma-separated list of keywords to autocomplete next.
 *
 * @sa CDECL_KEYWORDS
 * @sa AC_NO_NEXT_KEYWORDS
 */
#define AC_NEXT_KEYWORDS(...)     (char const *const[]){ __VA_ARGS__, NULL }

/**
 * For \ref CDECL_KEYWORDS, used to specify that the current keyword has no
 * next autocomplete keywords.
 *
 * @sa AC_NEXT_KEYWORDS
 * @sa CDECL_KEYWORDS
 */
#define AC_NO_NEXT_KEYWORDS       NULL

/**
 * Languages the **cdecl** `automatic` keyword is supported in (which is the
 * union of the languages the `auto` C/C++ keyword is supported in as either
 * a storage class or a type.
 */
#define LANG_automatic            (LANG_auto_STORAGE | LANG_auto_TYPE)

/**
 * Specify that the previosuly given keyword is a synonym for the given
 * language-specific keywords.
 *
 * @param ALWAYS_FIND If `true`, always find this synonym even when explaining
 * C/C++; if `false`, find only when composing C/C++.
 * @param ... Array of c_lang_lit.
 *
 * @sa #CDECL_KEYWORDS for examples.
 * @sa #SYNONYM()
 * @sa #TOKEN()
 */
#define SYNONYMS(ALWAYS_FIND,...) \
  (ALWAYS_FIND), /*y_token_id=*/0, C_LANG_LIT( __VA_ARGS__ )

/**
 * Special-case of #SYNONYMS when there is only one language(s)/literal pair.
 *
 * @param ALWAYS_FIND If `true`, always find this synonym even when explaining
 * C/C++; if `false`, find only when composing C/C++.
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 *
 * @sa #CDECL_KEYWORDS for examples.
 * @sa #SYNONYMS()
 * @sa #TOKEN()
 */
#define SYNONYM(ALWAYS_FIND,C_KEYWORD) \
  SYNONYMS( (ALWAYS_FIND), { LANG_ANY, (C_KEYWORD) } )

/**
 * Specify that the previosuly given keyword maps to Bison tokan \a Y_ID.
 *
 * @param Y_ID The Bison token ID (`Y_xxx`).
 *
 * @sa #CDECL_KEYWORDS for examples.
 * @sa #SYNONYM()
 * @sa #SYNONYMS()
 */
#define TOKEN(Y_ID) \
  FIND_IN_ENGLISH_ONLY, (Y_ID), /*lang_syn=*/NULL

///////////////////////////////////////////////////////////////////////////////

/// Mnemonic value to specify \ref cdecl_keyword::always_find as `true`.
static bool const ALWAYS_FIND           =  true;

/// Mnemonic value to specify \ref cdecl_keyword::always_find as `false`.
static bool const FIND_IN_ENGLISH_ONLY  = false;

/**
 * All **cdecl** keywords that are (mostly) _not_ C/C++ keywords.  Exceptions
 * are `alignas`, `bool`, `complex`, `const`, and `volatile` that are included
 * here as **cdecl** keywords so each maps to its language-specific literal,
 * e.g., `_Alignas` in C vs. `alignas` in C++.
 *
 * ## Initialization Macros
 *
 * The #SYNONYM, #SYNONYMS, and #TOKEN macros are used to initialize entries in
 * the array as follows.
 *
 * To have a **cdecl** keyword literal map to its corresponding Bison token,
 * use #TOKEN:
 *
 *      // The "aligned" literal maps to the Y_aligned token:
 *      { L_aligned,
 *        TOKEN( Y_aligned ),
 *        ...
 *      }
 *
 * To have a **cdecl** keyword literal that is a synonym for another **cdecl**
 * keyword literal map to the other literal's same Bison token, use #TOKEN with
 * the other literal's token:
 *
 *      // The "align" literal synonym also maps to the Y_aligned token:
 *      { L_align,
 *        TOKEN( Y_aligned ),
 *        ...
 *      }
 *
 * To have a **cdecl** keyword literal be a synonym for exactly one
 * corresponding C/C++ keyword literal, but only when converting pseudo-English
 * to gibberish, use #SYNONYM with FIND_IN_ENGLISH_ONLY:
 *
 *      // The "automatic" literal is a synonym for the "auto" literal, but
 *      // only when converting from pseudo-English to gibberish:
 *      { L_automatic,
 *        SYNONYM( FIND_IN_ENGLISH_ONLY, L_auto ),
 *        ...
 *      }
 *
 * To do the same, but allow the literal at any time (i.e., also when
 * converting gibberish to pseudo-English), use with ALWAYS_FIND:
 *
 *      // The "WINAPI" literal is always a synonym for the "__stdcall"
 *      // literal.
 *      { L_MSC_WINAPI,
 *        SYNONYM( ALWAYS_FIND, L_MSC___stdcall ),
 *        ...
 *      }
 *
 * To have a **cdecl** keyword literal be a synonym for more than one
 * corresponding C/C++ keyword literal depending on the current language, use
 * #SYNONYMS with the last row always containing #LANG_ANY:
 *
 *      // In languages that do not support the `noreturn` keyword/macro, it's
 *      // a synonym for `_Noreturn`; otherwise it just maps to itself.
 *      { L_noreturn,
 *        SYNONYMS( ALWAYS_FIND,
 *          { ~LANG_noreturn, L__Noreturn },
 *          { LANG_ANY,       L_noreturn  } ),
 *        ...
 *      }
 *
 * ## Autocompletion
 *
 * Within #IF_AC() are the autocompletion settings for a **cdecl** keyword.
 * They are:
 *
 * 1. The bitwise-or of languages it should be autocompleted in.  A keyword
 *    should be autocompletable _unless_ it:
 *
 *    + Is shorthand for a more-preferred keyword, e.g., `conv` is not
 *      autocompletable because `conversion` is more-preferred.
 *
 *    + Is too short, e.g, `all`, `as`, `bit`, `exit`, `mbr`, `no`, `of`,
 *      `ptr`, and `to` are not autocompletable.
 *
 * 2. The autocompletion policy.
 *
 * 3. The \ref cdecl_keyword::ac_next_keywords "ac_next_keywords" using either
 *    the #AC_NEXT_KEYWORDS() or #AC_NO_NEXT_KEYWORDS macro.
 *
 * @sa ac_policy
 * @sa CDECL_COMMANDS
 * @sa C_KEYWORDS
 */
static cdecl_keyword_t const CDECL_KEYWORDS[] = {
  { L_address,
    TOKEN( Y_address ),
    IF_AC(
      LANG_no_unique_address,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_align,
    TOKEN( Y_aligned ),
    IF_AC(
      LANG_NONE,                        // in deference to "aligned"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_alignas,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Alignas, L__Alignas },
      { LANG_ANY,      L_alignas  } ),
    IF_AC(
      LANG_NONE,                        // in deference to "aligned"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_aligned,
    TOKEN( Y_aligned ),
    IF_AC(
      LANG_ALIGNMENT,
      AC_POLICY_NO_OTHER,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_all,
    TOKEN( Y_all ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_Apple_block,
    TOKEN( Y_Apple_block ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_array,
    TOKEN( Y_array ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_static, L_const, L_volatile )
    )
  },

  { L_as,
    TOKEN( Y_as ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_atomic,
    SYNONYM( ALWAYS_FIND, L__Atomic ),
    IF_AC(
      LANG__Atomic,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_automatic,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_auto ),
    IF_AC(
      LANG_automatic,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bit,
    TOKEN( Y_bit ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_precise )
    )
  },

  { H_bit_precise,
    TOKEN( Y_bit_precise ),
    IF_AC(
      LANG__BitInt,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_int )
    )
  },

  { L_bits,
    TOKEN( Y_bits ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bool,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    IF_AC(
      LANG_BOOL_TYPE,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_boolean,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    IF_AC(
      LANG_BOOL_TYPE,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_Boolean,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    IF_AC(
      LANG_BOOL_TYPE,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bytes,
    TOKEN( Y_bytes ),
    IF_AC(
      LANG_ALIGNMENT,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_carries,
    TOKEN( Y_carries ),
    IF_AC(
      LANG_NONE,                        // in deference to "carries_dependency"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_dependency )
    )
  },

  { H_carries_dependency,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_carries_dependency ),
    IF_AC(
      LANG_carries_dependency,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_cast,
    TOKEN( Y_cast ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_into )
    )
  },

  { L_character,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_char ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_complex,
    SYNONYMS( ALWAYS_FIND,
      { ~LANG__Complex & LANG_C_ANY, L_GNU___complex },
      { LANG__Complex,               L__Complex      },
      { LANG_ANY,                    L_complex       } ),
    IF_AC(
      LANG__Complex,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_command,
    TOKEN( Y_commands ),
    IF_AC(
      LANG_NONE,                        // in deference to "commands"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_commands,
    TOKEN( Y_commands ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_const,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_const, L_GNU___const },
      { LANG_ANY,    L_const       } ),
    IF_AC(
      LANG_const,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_constant,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_const, L_GNU___const },
      { LANG_ANY,    L_const       } ),
    IF_AC(
      LANG_const,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_eval,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_consteval ),
    IF_AC(
      LANG_consteval,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_evaluation,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_consteval ),
    IF_AC(
      LANG_consteval,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_expr,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constexpr ),
    IF_AC(
      LANG_constexpr,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_expression,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constexpr ),
    IF_AC(
      LANG_constexpr,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_init,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constinit ),
    IF_AC(
      LANG_constinit,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_initialization,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constinit ),
    IF_AC(
      LANG_constinit,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_constructor,
    TOKEN( Y_constructor ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_conv,
    TOKEN( Y_conversion ),
    IF_AC(
      LANG_NONE,                        // in deference to "conversion"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_conversion,
    TOKEN( Y_conversion ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_operator, L_returning )
    )
  },

  { L_ctor,
    TOKEN( Y_constructor ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_declare,
    TOKEN( Y_declare ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_defaulted,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_default ),
    IF_AC(
      LANG_default_delete_FUNC,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_define,
    TOKEN( Y_define ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_defined,
    TOKEN( Y_defined ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { L_deleted,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_delete ),
    IF_AC(
      LANG_default_delete_FUNC,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dependency,
    TOKEN( Y_dependency ),
    IF_AC(
      LANG_carries_dependency,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_destructor,
    TOKEN( Y_destructor ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_discard,
    TOKEN( Y_discard ),
    IF_AC(
      LANG_nodiscard,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_double_precision,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_double ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dtor,
    TOKEN( Y_destructor ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dynamic,
    TOKEN( Y_dynamic ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_cast )
    )
  },

  { L_english,
    TOKEN( Y_english ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_enumeration,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_enum ),
    IF_AC(
      LANG_enum,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_eval,
    TOKEN( Y_evaluation ),
    IF_AC(
      LANG_NONE,                        // in deference to "evaluation"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_evaluation,
    TOKEN( Y_evaluation ),
    IF_AC(
      LANG_consteval,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_except,
    TOKEN( Y_except ),
    IF_AC(
      LANG_noexcept,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_exit,
    TOKEN( Y_quit ),
    IF_AC(
      LANG_NONE,                        // only do this once
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_explain,
    TOKEN( Y_explain ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_exported,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_export ),
    IF_AC(
      LANG_export,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_expr,
    TOKEN( Y_expression ),
    IF_AC(
      LANG_NONE,                        // in deference to "expression"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_expression,
    TOKEN( Y_expression ),
    IF_AC(
      LANG_constexpr,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_external,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_extern ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_floating,
    TOKEN( Y_floating ),
    IF_AC(
      LANG_NONE,                        // in deference to "floating-point"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_point )
    )
  },

  { H_floating_point,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_float ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_func,
    TOKEN( Y_function ),
    IF_AC(
      LANG_NONE,                        // in deference to "function"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_function,
    TOKEN( Y_function ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_help,
    TOKEN( Y_help ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_imaginary,
    SYNONYMS( ALWAYS_FIND,
        { LANG__Imaginary, L__Imaginary },
        { LANG_ANY,        L_imaginary  } ),
    IF_AC(
      LANG__Imaginary,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_init,
    TOKEN( Y_initialization ),
    IF_AC(
      LANG_NONE,                        // in deference to "initialization"
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_initialization,
    TOKEN( Y_initialization ),
    IF_AC(
      LANG_constinit,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_integer,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_int ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_into,
    TOKEN( Y_into ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_len,
    TOKEN( Y_length ),
    IF_AC(
      LANG_NONE,                        // in deference to "length"
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_array )
    )
  },

  { L_length,
    TOKEN( Y_length ),
    IF_AC(
      LANG_VLA,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_array )
    )
  },

  { L_linkage,
    TOKEN( Y_linkage ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_literal,
    TOKEN( Y_literal ),
    IF_AC(
      LANG_USER_DEFINED_LITERAL,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_local,
    TOKEN( Y_local ),
    IF_AC(
      LANG_THREAD_LOCAL_STORAGE,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_maybe,
    TOKEN( Y_maybe ),
    IF_AC(
      LANG_NONE,                        // in deference to "maybe_unused"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_unused )
    )
  },

  { H_maybe_unused,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_maybe_unused ),
    IF_AC(
      LANG_maybe_unused,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_mbr,
    TOKEN( Y_member ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_member,
    TOKEN( Y_member ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_no,
    TOKEN( Y_no ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_discard, L_except, L_return, L_unique )
    )
  },

  { H_no_discard,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_nodiscard ),
    IF_AC(
      LANG_nodiscard,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_except,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_noexcept ),
    IF_AC(
      LANG_NONE,                        // in deference to "no-exception"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_exception,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_noexcept ),
    IF_AC(
      LANG_noexcept,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_return,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_noreturn, L__Noreturn },
        { LANG_ANY,       L_noreturn  } ),
    IF_AC(
      LANG_noreturn,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_discardable,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_nodiscard ),
    IF_AC(
      LANG_nodiscard,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_mbr,
    TOKEN( Y_non_member ),
    IF_AC(
      LANG_NONE,                        // in deference to "non-member"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_member,
    TOKEN( Y_non_member ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_returning,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_noreturn, L__Noreturn },
        { LANG_ANY,       L_noreturn  } ),
    IF_AC(
      LANG_noreturn,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_throwing,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_throw ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_unique_address,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_no_unique_address ),
    IF_AC(
      LANG_no_unique_address,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_unique_address,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_no_unique_address ),
    IF_AC(
      LANG_no_unique_address,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_noreturn,
    SYNONYMS( ALWAYS_FIND,
        { ~LANG_noreturn, L__Noreturn },
        { LANG_ANY,       L_noreturn  } ),
    IF_AC(
      LANG_NONRETURNING_FUNC,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_of,
    TOKEN( Y_of ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_oper,
    TOKEN( Y_operator ),
    IF_AC(
      LANG_NONE,                        // in deference to "operator"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_options,
    TOKEN( Y_options ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_overridden,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_override ),
    IF_AC(
      LANG_override,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_point,
    TOKEN( Y_point ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_pointer,
    TOKEN( Y_pointer ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_precise,
    TOKEN( Y_precise ),
    IF_AC(
      LANG__BitInt,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_int )
    )
  },

  { L_precision,
    TOKEN( Y_precision ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_predef,
    TOKEN( Y_predefined ),
    IF_AC(
      LANG_NONE,                        // in deference to "predefined"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_predefined,
    TOKEN( Y_predefined ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ptr,
    TOKEN( Y_pointer ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_pure,
    TOKEN( Y_pure ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_virtual )
    )
  },

  { L_q,
    TOKEN( Y_quit ),
    IF_AC(
      LANG_NONE,                        // in deference to "quit"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_quit,
    TOKEN( Y_quit ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ref,
    TOKEN( Y_reference ),
    IF_AC(
      LANG_NONE,                      // in deference to "reference"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_reference,
    TOKEN( Y_reference ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_reinterpret,
    TOKEN( Y_reinterpret ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_cast )
    )
  },

  { L_restricted,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_restrict, L_GNU___restrict },
        { LANG_ANY,       L_restrict       } ),
    IF_AC(
      LANG_restrict,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ret,
    TOKEN( Y_returning ),
    IF_AC(
      LANG_NONE,                        // in deference to "return"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_returning,
    TOKEN( Y_returning ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_rvalue,
    TOKEN( Y_rvalue ),
    IF_AC(
      LANG_RVALUE_REFERENCE,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_reference )
    )
  },

  { L_scope,
    TOKEN( Y_scope ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_set,
    TOKEN( Y_set ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_show,
    TOKEN( Y_show ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_structure,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_struct ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_thread,
    TOKEN( Y_thread ),
    IF_AC(
      LANG_NONE,                        // in deference to "thread_local"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_local )
    )
  },

  { L_thread_local,
    SYNONYMS( ALWAYS_FIND,
     // { ~LANG_THREAD_LOCAL_STORAGE, L_GNU___thread  },
        { LANG__Thread_local,         L__Thread_local },
        { LANG_ANY,                   L_thread_local  } ),
    IF_AC(
      LANG_THREAD_LOCAL_STORAGE,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_thread_local,
    SYNONYMS( ALWAYS_FIND,
        { ~LANG_THREAD_LOCAL_STORAGE, L_GNU___thread  },
        { LANG__Thread_local,         L__Thread_local },
        { LANG_ANY,                   L_thread_local  } ),
    IF_AC(
      LANG_THREAD_LOCAL_STORAGE,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_to,
    TOKEN( Y_to ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_type,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_typedef ),
    IF_AC(
      LANG_NONE,                        // in deference to "typedef"
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_unique,
    TOKEN( Y_unique ),
    IF_AC(
      LANG_no_unique_address,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_address )
    )
  },

  { L_unused,
    TOKEN( Y_unused ),
    IF_AC(
      LANG_maybe_unused,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_user,
    TOKEN( Y_user ),
    IF_AC(
      LANG_NONE,                        // in deference to "user-defined"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_defined )
    )
  },

  { H_user_def,
    TOKEN( Y_user_defined ),
    IF_AC(
      LANG_NONE,                        // in deference to "user-defined"
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { H_user_defined,
    TOKEN( Y_user_defined ),
    IF_AC(
      LANG_CPP_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { L_var,
    TOKEN( Y_variable ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_length, L_array )
    )
  },

  { L_varargs,
    TOKEN( Y_ELLIPSIS ),
    IF_AC(
      LANG_PROTOTYPES,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_variable,
    TOKEN( Y_variable ),
    IF_AC(
      LANG_VLA,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_length, L_array )
    )
  },

  { L_variadic,
    TOKEN( Y_ELLIPSIS ),
    IF_AC(
      LANG_PROTOTYPES,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_vector,
    TOKEN( Y_array ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_volatile,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_volatile, L_GNU___volatile },
        { LANG_ANY,       L_volatile       } ),
    IF_AC(
      LANG_volatile,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_wide,
    TOKEN( Y_wide ),
    IF_AC(
      LANG_wchar_t,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_char )
    )
  },

  { L_width,
    TOKEN( Y_width ),
    IF_AC(
      LANG_ANY,
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_bits )
    )
  },

  // Embedded C extensions
  { L_EMC_accum,
    SYNONYMS( ALWAYS_FIND,
        { LANG_C_99, L_EMC__Accum },
        { LANG_ANY,  NULL         } ),
    IF_AC(
      LANG_C_99,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_fract,
    SYNONYMS( ALWAYS_FIND,
        { LANG_C_99, L_EMC__Fract },
        { LANG_ANY,  NULL         } ),
    IF_AC(
      LANG_C_99,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_sat,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { LANG_C_99, L_EMC__Sat   },
        { LANG_ANY,  NULL         } ),
    IF_AC(
      LANG_NONE,                        // too short
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_saturated,
    SYNONYMS( ALWAYS_FIND,
        { LANG_C_99, L_EMC__Sat   },
        { LANG_ANY,  NULL         } ),
    IF_AC(
      LANG_C_99,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  // Microsoft extensions
  { L_MSC_cdecl,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___cdecl ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_clrcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___clrcall ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_fastcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___fastcall ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_stdcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___stdcall ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_thiscall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___thiscall ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_vectorcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___vectorcall ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_WINAPI,
    SYNONYM( ALWAYS_FIND, L_MSC___stdcall ),
    IF_AC(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { NULL,
    TOKEN( 0 ),
    IF_AC(
      LANG_NONE,
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  }
};

////////// extern functions ///////////////////////////////////////////////////

cdecl_keyword_t const* cdecl_keyword_find( char const *s ) {
  assert( s != NULL );
  // the list is small, so linear search is good enough
  for ( cdecl_keyword_t const *k = CDECL_KEYWORDS; k->literal != NULL; ++k ) {
    if ( strcmp( s, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

cdecl_keyword_t const* cdecl_keyword_next( cdecl_keyword_t const *k ) {
  return k == NULL ? CDECL_KEYWORDS : (++k)->literal == NULL ? NULL : k;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
