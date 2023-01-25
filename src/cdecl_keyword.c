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

#ifdef WITH_READLINE

/**
 * For \ref CDECL_KEYWORDS, used to specify which keywords to autocomplete next
 * (after the current keyword).
 *
 * @param ... A comma-separated list of keywords to autocomplete next.
 *
 * @sa #AC_NO_NEXT_KEYWORDS
 * @sa CDECL_KEYWORDS
 */
#define AC_NEXT_KEYWORDS(...)     (char const *const[]){ __VA_ARGS__, NULL }

/**
 * For \ref CDECL_KEYWORDS, used to specify that the current keyword has no
 * next autocomplete keywords.
 *
 * @sa #AC_NEXT_KEYWORDS
 * @sa CDECL_KEYWORDS
 */
#define AC_NO_NEXT_KEYWORDS       NULL

/**
 * For \ref CDECL_KEYWORDS, expands into the given arguments only if GNU
 * **readline**(3) is compiled in (hence, autocompletion is enabled); nothing
 * if not.
 *
 * @remarks This macro exists because using it within \ref CDECL_KEYWORDS is
 * not as visually jarring as having <code>\#ifdef&nbsp;WITH_READLINE</code>
 * all over the place.
 *
 * @sa CDECL_KEYWORDS
 */
#define AC_SETTINGS(...)          __VA_ARGS__

#else /* WITH_READLINE */
# define AC_SETTINGS(...)         /* nothing */
#endif /* WITH_READLINE */

/**
 * Specify that the previosuly given keyword is a synonym for the given
 * language-specific keywords.
 *
 * @param ALWAYS_FIND If `true`, always find this synonym even when explaining
 * C/C++; if `false`, find only when composing C/C++.
 * @param ... Array of c_lang_lit.
 *
 * @sa CDECL_KEYWORDS for examples.
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
 * @sa CDECL_KEYWORDS for examples.
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
 * @sa CDECL_KEYWORDS for examples.
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
 * Within #AC_SETTINGS() are the autocompletion settings for a **cdecl**
 * keyword.  They are:
 *
 * 1. The bitwise-or of languages it should be autocompleted in.  A keyword
 *    should be autocompletable _unless_ it:
 *
 *    + Is shorthand for a preferred keyword, e.g., `conv` is not
 *      autocompletable because `conversion` is preferred.
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
    AC_SETTINGS(
      LANG_no_unique_address,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_align,
    TOKEN( Y_aligned ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "aligned"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_alignas,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Alignas, L__Alignas },
      { LANG_ANY,      L_alignas  } ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to C's "alignas"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_aligned,
    TOKEN( Y_aligned ),
    AC_SETTINGS(
      LANG_ALIGNMENT,
      AC_POLICY_NO_OTHER,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_all,
    TOKEN( Y_all ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_Apple_block,
    TOKEN( Y_Apple_block ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_array,
    TOKEN( Y_array ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_static, L_const, L_volatile )
    )
  },

  { L_as,
    TOKEN( Y_as ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_atomic,
    SYNONYM( ALWAYS_FIND, L__Atomic ),
    AC_SETTINGS(
      LANG__Atomic,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_automatic,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_auto ),
    AC_SETTINGS(
      LANG_auto_STORAGE | LANG_auto_TYPE,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bit,
    TOKEN( Y_bit ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_precise )
    )
  },

  { H_bit_precise,
    TOKEN( Y_bit_precise ),
    AC_SETTINGS(
      LANG__BitInt,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_int )
    )
  },

  { L_bits,
    TOKEN( Y_bits ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bool,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    AC_SETTINGS(
      LANG_BOOL_TYPE,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_boolean,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    AC_SETTINGS(
      LANG_BOOL_TYPE,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_Boolean,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    AC_SETTINGS(
      LANG_BOOL_TYPE,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bytes,
    TOKEN( Y_bytes ),
    AC_SETTINGS(
      LANG_ALIGNMENT,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_carries,
    TOKEN( Y_carries ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "carries_dependency"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_dependency )
    )
  },

  { H_carries_dependency,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_carries_dependency ),
    AC_SETTINGS(
      LANG_carries_dependency,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_cast,
    TOKEN( Y_cast ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_into )
    )
  },

  { L_character,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_char ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_complex,
    SYNONYMS( ALWAYS_FIND,
      { ~LANG__Complex & LANG_C_ANY, L_GNU___complex },
      { LANG__Complex,               L__Complex      },
      { LANG_ANY,                    L_complex       } ),
    AC_SETTINGS(
      LANG__Complex,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_command,
    TOKEN( Y_commands ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "commands"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_commands,
    TOKEN( Y_commands ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_const,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_const, L_GNU___const },
      { LANG_ANY,    L_const       } ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to C's "const"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_constant,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_const, L_GNU___const },
      { LANG_ANY,    L_const       } ),
    AC_SETTINGS(
      LANG_const,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_eval,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_consteval ),
    AC_SETTINGS(
      LANG_consteval,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_evaluation,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_consteval ),
    AC_SETTINGS(
      LANG_consteval,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_expr,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constexpr ),
    AC_SETTINGS(
      LANG_constexpr,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_expression,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constexpr ),
    AC_SETTINGS(
      LANG_constexpr,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_init,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constinit ),
    AC_SETTINGS(
      LANG_constinit,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_initialization,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constinit ),
    AC_SETTINGS(
      LANG_constinit,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_constructor,
    TOKEN( Y_constructor ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_conv,
    TOKEN( Y_conversion ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "conversion"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_conversion,
    TOKEN( Y_conversion ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_operator, L_returning )
    )
  },

  { L_ctor,
    TOKEN( Y_constructor ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_declare,
    TOKEN( Y_declare ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_defaulted,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_default ),
    AC_SETTINGS(
      LANG_default_delete_FUNC,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_define,
    TOKEN( Y_define ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_defined,
    TOKEN( Y_defined ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { L_deleted,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_delete ),
    AC_SETTINGS(
      LANG_default_delete_FUNC,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dependency,
    TOKEN( Y_dependency ),
    AC_SETTINGS(
      LANG_carries_dependency,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_destructor,
    TOKEN( Y_destructor ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_discard,
    TOKEN( Y_discard ),
    AC_SETTINGS(
      LANG_nodiscard,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_double_precision,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_double ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dtor,
    TOKEN( Y_destructor ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dynamic,
    TOKEN( Y_dynamic ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_cast )
    )
  },

  { L_english,
    TOKEN( Y_english ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_enumeration,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_enum ),
    AC_SETTINGS(
      LANG_enum,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_eval,
    TOKEN( Y_evaluation ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "evaluation"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_evaluation,
    TOKEN( Y_evaluation ),
    AC_SETTINGS(
      LANG_consteval,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_except,
    TOKEN( Y_except ),
    AC_SETTINGS(
      LANG_noexcept,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_exit,
    TOKEN( Y_quit ),
    AC_SETTINGS(
      LANG_NONE,                        // only do this once
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_explain,
    TOKEN( Y_explain ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_exported,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_export ),
    AC_SETTINGS(
      LANG_export,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_expr,
    TOKEN( Y_expression ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "expression"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_expression,
    TOKEN( Y_expression ),
    AC_SETTINGS(
      LANG_constexpr,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_external,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_extern ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_floating,
    TOKEN( Y_floating ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "floating-point"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_point )
    )
  },

  { H_floating_point,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_float ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_func,
    TOKEN( Y_function ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "function"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_function,
    TOKEN( Y_function ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_help,
    TOKEN( Y_help ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_imaginary,
    SYNONYMS( ALWAYS_FIND,
        { LANG__Imaginary, L__Imaginary },
        { LANG_ANY,        L_imaginary  } ),
    AC_SETTINGS(
      LANG__Imaginary,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_init,
    TOKEN( Y_initialization ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "initialization"
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_initialization,
    TOKEN( Y_initialization ),
    AC_SETTINGS(
      LANG_constinit,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_integer,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_int ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_into,
    TOKEN( Y_into ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_len,
    TOKEN( Y_length ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "length"
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_array )
    )
  },

  { L_length,
    TOKEN( Y_length ),
    AC_SETTINGS(
      LANG_VLA,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_array )
    )
  },

  { L_linkage,
    TOKEN( Y_linkage ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_literal,
    TOKEN( Y_literal ),
    AC_SETTINGS(
      LANG_USER_DEFINED_LITERAL,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_local,
    TOKEN( Y_local ),
    AC_SETTINGS(
      LANG_THREAD_LOCAL_STORAGE,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_maybe,
    TOKEN( Y_maybe ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "maybe_unused"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_unused )
    )
  },

  { H_maybe_unused,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_maybe_unused ),
    AC_SETTINGS(
      LANG_maybe_unused,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_mbr,
    TOKEN( Y_member ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_member,
    TOKEN( Y_member ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_no,
    TOKEN( Y_no ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_discard, L_except, L_return, L_unique )
    )
  },

  { H_no_discard,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_nodiscard ),
    AC_SETTINGS(
      LANG_nodiscard,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_except,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_noexcept ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "no-exception"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_exception,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_noexcept ),
    AC_SETTINGS(
      LANG_noexcept,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_return,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_noreturn, L__Noreturn },
        { LANG_ANY,       L_noreturn  } ),
    AC_SETTINGS(
      LANG_noreturn,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_discardable,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_nodiscard ),
    AC_SETTINGS(
      LANG_nodiscard,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_mbr,
    TOKEN( Y_non_member ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "non-member"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_member,
    TOKEN( Y_non_member ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_returning,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_noreturn, L__Noreturn },
        { LANG_ANY,       L_noreturn  } ),
    AC_SETTINGS(
      LANG_noreturn,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_throwing,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_throw ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_unique_address,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_no_unique_address ),
    AC_SETTINGS(
      LANG_no_unique_address,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_unique_address,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_no_unique_address ),
    AC_SETTINGS(
      LANG_no_unique_address,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_noreturn,
    SYNONYMS( ALWAYS_FIND,
        { ~LANG_noreturn, L__Noreturn },
        { LANG_ANY,       L_noreturn  } ),
    AC_SETTINGS(
      LANG_NONRETURNING_FUNC,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_of,
    TOKEN( Y_of ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_oper,
    TOKEN( Y_operator ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "operator"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_options,
    TOKEN( Y_options ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_overridden,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_override ),
    AC_SETTINGS(
      LANG_override,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_point,
    TOKEN( Y_point ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_pointer,
    TOKEN( Y_pointer ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_precise,
    TOKEN( Y_precise ),
    AC_SETTINGS(
      LANG__BitInt,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_int )
    )
  },

  { L_precision,
    TOKEN( Y_precision ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_predef,
    TOKEN( Y_predefined ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "predefined"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_predefined,
    TOKEN( Y_predefined ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ptr,
    TOKEN( Y_pointer ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_pure,
    TOKEN( Y_pure ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_virtual )
    )
  },

  { L_q,
    TOKEN( Y_quit ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "quit"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_quit,
    TOKEN( Y_quit ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ref,
    TOKEN( Y_reference ),
    AC_SETTINGS(
      LANG_NONE,                      // defer to "reference"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_reference,
    TOKEN( Y_reference ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_reinterpret,
    TOKEN( Y_reinterpret ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_cast )
    )
  },

  { L_restricted,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_restrict, L_GNU___restrict },
        { LANG_ANY,       L_restrict       } ),
    AC_SETTINGS(
      LANG_restrict,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ret,
    TOKEN( Y_returning ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "return"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_returning,
    TOKEN( Y_returning ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_rvalue,
    TOKEN( Y_rvalue ),
    AC_SETTINGS(
      LANG_RVALUE_REFERENCE,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_reference )
    )
  },

  { L_scope,
    TOKEN( Y_scope ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_set,
    TOKEN( Y_set ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_show,
    TOKEN( Y_show ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_structure,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_struct ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_thread,
    TOKEN( Y_thread ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "thread_local"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_local )
    )
  },

  { L_thread_local,
    SYNONYMS( ALWAYS_FIND,
        //
        // Unlike H_thread_local (below), this row:
        //
        //     { ~LANG_THREAD_LOCAL_STORAGE, L_GNU___thread },
        //
        // isn't here because `thread_local` is either a C/C++ keyword or macro
        // and so should only be a synonym for the actual language-specific
        // keywords in C/C++, not as a synonym for "thread local" as a concept.
        // For that, `thread-local` should be used.
        //
        // If this row were included here, then the following:
        //
        //     cdecl> set c99
        //     cdecl> explain thread_local int x
        //     declare x as thread local integer
        //
        // would be legal (becasuse `thread_local` would map to GNU C's
        // `__thread` that's legal in all languages) when it shouldn't be
        // (because neither the C macro `thread_local` nor the C keyword
        // `_Thread_local` were supported until C11).
        //
        { LANG__Thread_local, L__Thread_local },
        { LANG_ANY,           L_thread_local  } ),
    AC_SETTINGS(
      LANG_THREAD_LOCAL_STORAGE,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_thread_local,
    SYNONYMS( ALWAYS_FIND,
        { ~LANG_THREAD_LOCAL_STORAGE, L_GNU___thread  },
        { LANG__Thread_local,         L__Thread_local },
        { LANG_ANY,                   L_thread_local  } ),
    AC_SETTINGS(
      LANG_THREAD_LOCAL_STORAGE,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_to,
    TOKEN( Y_to ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_type,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_typedef ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "typedef"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_unique,
    TOKEN( Y_unique ),
    AC_SETTINGS(
      LANG_no_unique_address,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_address )
    )
  },

  { L_unused,
    TOKEN( Y_unused ),
    AC_SETTINGS(
      LANG_maybe_unused,
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_user,
    TOKEN( Y_user ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "user-defined"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_defined )
    )
  },

  { H_user_def,
    TOKEN( Y_user_defined ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to "user-defined"
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { H_user_defined,
    TOKEN( Y_user_defined ),
    AC_SETTINGS(
      LANG_CPP_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { L_var,
    TOKEN( Y_variable ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_length, L_array )
    )
  },

  { L_varargs,
    TOKEN( Y_ELLIPSIS ),
    AC_SETTINGS(
      LANG_PROTOTYPES,
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_variable,
    TOKEN( Y_variable ),
    AC_SETTINGS(
      LANG_VLA,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_length, L_array )
    )
  },

  { L_variadic,
    TOKEN( Y_ELLIPSIS ),
    AC_SETTINGS(
      LANG_PROTOTYPES,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_vector,
    TOKEN( Y_array ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_volatile,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { ~LANG_volatile, L_GNU___volatile },
        { LANG_ANY,       L_volatile       } ),
    AC_SETTINGS(
      LANG_NONE,                        // defer to C's "volatile"
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_wide,
    TOKEN( Y_wide ),
    AC_SETTINGS(
      LANG_wchar_t,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_char )
    )
  },

  { L_width,
    TOKEN( Y_width ),
    AC_SETTINGS(
      LANG_ANY,
      AC_POLICY_NONE,
      AC_NEXT_KEYWORDS( L_bits )
    )
  },

  // Embedded C extensions
  { L_EMC_accum,
    SYNONYMS( ALWAYS_FIND,
        { LANG_C_99, L_EMC__Accum },
        { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      LANG_C_99,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_fract,
    SYNONYMS( ALWAYS_FIND,
        { LANG_C_99, L_EMC__Fract },
        { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      LANG_C_99,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_sat,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
        { LANG_C_99, L_EMC__Sat   },
        { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      LANG_NONE,                        // too short
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_saturated,
    SYNONYMS( ALWAYS_FIND,
        { LANG_C_99, L_EMC__Sat   },
        { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      LANG_C_99,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  // Microsoft extensions
  { L_MSC_cdecl,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___cdecl ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_clrcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___clrcall ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_fastcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___fastcall ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_stdcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___stdcall ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_thiscall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___thiscall ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_vectorcall,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___vectorcall ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_WINAPI,
    SYNONYM( ALWAYS_FIND, L_MSC___stdcall ),
    AC_SETTINGS(
      LANG_MSC_EXTENSIONS,
      AC_POLICY_NONE,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { NULL,
    TOKEN( 0 ),
    AC_SETTINGS(
      LANG_NONE,
      AC_POLICY_NONE,
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
