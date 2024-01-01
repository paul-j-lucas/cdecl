/*
**      cdecl -- C gibberish translator
**      src/cdeck_keyword.cpp
**
**      Copyright (C) 2021-2024  Paul J. Lucas
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
#include "cdecl_keyword.h"
#include "c_lang.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <stdlib.h>
#include <string.h>

/// @endcond

/**
 * @addtogroup cdecl-keywords-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

#ifdef WITH_READLINE

/**
 * Within \ref CDECL_KEYWORDS, used to specify which keywords to autocomplete
 * next (after the current keyword).
 *
 * @param ... A comma-separated list of keywords to autocomplete next.
 *
 * @note The keywords _must_ be in sorted order.
 *
 * @sa #AC_NO_NEXT_KEYWORDS
 * @sa CDECL_KEYWORDS for examples.
 */
#define AC_NEXT_KEYWORDS(...)     (char const *const[]){ __VA_ARGS__, NULL }

/**
 * Within \ref CDECL_KEYWORDS, used to specify that the current keyword has no
 * next autocomplete keywords.
 *
 * @sa #AC_NEXT_KEYWORDS
 * @sa CDECL_KEYWORDS for examples.
 */
#define AC_NO_NEXT_KEYWORDS       NULL

/**
 * Within \ref CDECL_KEYWORDS, expands into the given arguments only if GNU
 * **readline**(3) is compiled in (hence, autocompletion is enabled); nothing
 * if not.
 *
 * @remarks This macro exists because using it within \ref CDECL_KEYWORDS is
 * not as visually jarring as having <code>\#ifdef&nbsp;WITH_READLINE</code>
 * all over the place.
 *
 * @sa CDECL_KEYWORDS for examples.
 */
#define AC_SETTINGS(...)          __VA_ARGS__

#else /* WITH_READLINE */
# define AC_SETTINGS(...)         /* nothing */
#endif /* WITH_READLINE */

/**
 * Within \ref CDECL_KEYWORDS, a mnemonic value to specify \ref
 * cdecl_keyword::always_find as `true`.
 *
 * @sa CDECL_KEYWORDS for examples.
 * @sa #FIND_IN_ENGLISH_ONLY
 */
#define ALWAYS_FIND               true

/**
 * Within \ref CDECL_KEYWORDS, a mnemonic value to specify \ref
 * cdecl_keyword::always_find as `false`.
 *
 * @sa CDECL_KEYWORDS for examples.
 * @sa #ALWAYS_FIND
 */
#define FIND_IN_ENGLISH_ONLY      false

/**
 * Within \ref CDECL_KEYWORDS, specify that the previously given keyword is a
 * synonym for the given language-specific keywords.
 *
 * @param FIND_WHEN Either #ALWAYS_FIND or #FIND_IN_ENGLISH_ONLY.
 *
 * @sa #ALWAYS_FIND
 * @sa CDECL_KEYWORDS for examples.
 * @sa #FIND_IN_ENGLISH_ONLY
 * @sa #SYNONYM()
 * @sa #TOKEN()
 */
#define SYNONYMS(FIND_WHEN,...) \
  (FIND_WHEN), .y_token_id = 0, C_LANG_LIT( __VA_ARGS__ )

/**
 * Within \ref CDECL_KEYWORDS, a special-case of #SYNONYMS when there is only
 * one language(s)/literal pair.
 *
 * @param FIND_WHEN Either #ALWAYS_FIND or #FIND_IN_ENGLISH_ONLY.
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 *
 * @sa #ALWAYS_FIND
 * @sa CDECL_KEYWORDS for examples.
 * @sa #FIND_IN_ENGLISH_ONLY
 * @sa #SYNONYMS()
 * @sa #TOKEN()
 */
#define SYNONYM(FIND_WHEN,C_KEYWORD) \
  SYNONYMS( (FIND_WHEN), { LANG_ANY, (C_KEYWORD) } )

/**
 * Within \ref CDECL_KEYWORDS, specify that the previously given keyword maps
 * to Bison tokan \a Y_ID.
 *
 * @param Y_ID The Bison token ID (`Y_xxx`).
 *
 * @sa CDECL_KEYWORDS for examples.
 * @sa #SYNONYM()
 * @sa #SYNONYMS()
 */
#define TOKEN(Y_ID) \
  FIND_IN_ENGLISH_ONLY, (Y_ID), .lang_syn = NULL

///////////////////////////////////////////////////////////////////////////////

/**
 * All **cdecl** keywords that are (mostly) _not_ C/C++ keywords.
 *
 * @note Exceptions are:
 *  + `alignas`, `bool`, `complex`, `const`, and `volatile` so they can map to
 *    their language-specific literals, e.g., `_Alignas` in C vs. `alignas` in
 *    C++.
 *  + `double` so it can specify the #AC_NEXT_KEYWORDS() of `precision`.
 *
 * @note This is not declared `const` because it's sorted once.
 *
 * ## Initialization Macros
 *
 * The #SYNONYM, #SYNONYMS, and #TOKEN macros are used to initialize entries in
 * the array as follows:
 *
 * + To have a **cdecl** keyword literal map to its corresponding Bison token,
 *   use #TOKEN:
 *
 *          // The "aligned" literal maps to the Y_aligned token:
 *          { L_aligned,
 *            TOKEN( Y_aligned ),
 *            ...
 *          }
 *
 * + To have a **cdecl** keyword literal that is a synonym for another
 *   **cdecl** keyword literal map to the other literal's same Bison token, use
 *   #TOKEN with the other literal's token:
 *
 *          // The "align" literal synonym also maps to the Y_aligned token:
 *          { L_align,
 *            LANG_ALIGNMENT,
 *            TOKEN( Y_aligned ),
 *            ...
 *          }
 *
 * + To have a **cdecl** keyword literal be a synonym for exactly one
 *   corresponding C/C++ keyword literal, but only when converting pseudo-
 *   English to gibberish, use #SYNONYM with #FIND_IN_ENGLISH_ONLY:
 *
 *          // The "automatic" literal is a synonym for the "auto" literal, but
 *          // only when converting from pseudo-English to gibberish:
 *          { L_automatic,
 *            LANG_auto_STORAGE | LANG_auto_TYPE,
 *            SYNONYM( FIND_IN_ENGLISH_ONLY, L_auto ),
 *            ...
 *          }
 *
 * + To do the same, but allow the literal at any time (i.e., also when
 *   converting gibberish to pseudo-English), use with #ALWAYS_FIND:
 *
 *          // The "WINAPI" literal is always a synonym for the "__stdcall"
 *          // literal.
 *          { L_MSC_WINAPI,
 *            LANG_MSC_EXTENSIONS,
 *            SYNONYM( ALWAYS_FIND, L_MSC___stdcall ),
 *            ...
 *          }
 *
 * + To have a **cdecl** keyword literal be a synonym for more than one
 *   corresponding C/C++ keyword literal depending on the current language, use
 *   #SYNONYMS with the last row always containing #LANG_ANY:
 *
 *          // In languages that do not support the `noreturn` keyword/macro,
 *          // it's a synonym for `_Noreturn`; otherwise it just maps to itself.
 *          { L_noreturn,
 *            LANG_NONRETURNING_FUNCS,
 *            SYNONYMS( ALWAYS_FIND,
 *              { ~LANG_noreturn, L__Noreturn },
 *              { LANG_ANY,       L_noreturn  } ),
 *            ...
 *          }
 *
 * ## Autocompletion
 *
 * Within #AC_SETTINGS() are the autocompletion settings for a **cdecl**
 * keyword.  They are:
 *
 * 1. The autocompletion policy.
 *
 * 2. The \ref cdecl_keyword::ac_next_keywords "ac_next_keywords" using either
 *    the #AC_NEXT_KEYWORDS() or #AC_NO_NEXT_KEYWORDS macro.
 *
 * @sa #AC_NEXT_KEYWORDS()
 * @sa #AC_NO_NEXT_KEYWORDS
 * @sa ac_policy
 * @sa #AC_SETTINGS()
 * @sa #ALWAYS_FIND
 * @sa C_KEYWORDS
 * @sa CDECL_COMMANDS
 * @sa #FIND_IN_ENGLISH_ONLY
 */
static cdecl_keyword_t CDECL_KEYWORDS[] = {
  { L_address,
    LANG_no_unique_address,
    TOKEN( Y_address ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_align,
    LANG_ALIGNMENT,
    TOKEN( Y_aligned ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "aligned"
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_alignas,
    LANG_ALIGNMENT,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Alignas, L__Alignas },
      { LANG_ANY,      L_alignas  } ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "aligned"
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_aligned,
    LANG_ALIGNMENT,
    TOKEN( Y_aligned ),
    AC_SETTINGS(
      AC_POLICY_NO_OTHER,
      AC_NEXT_KEYWORDS( L_bytes )
    )
  },

  { L_all,
    LANG_ANY,
    TOKEN( Y_all ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_array,
    LANG_ANY,
    TOKEN( Y_array ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_as,
    LANG_ANY,
    TOKEN( Y_as ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_atomic,
    LANG__Atomic,
    SYNONYM( ALWAYS_FIND, L__Atomic ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_automatic,
    LANG_auto_STORAGE | LANG_auto_TYPE,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_auto ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bit,
    LANG__BitInt,
    TOKEN( Y_bit ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "bit-precise"
      AC_NEXT_KEYWORDS( L_precise )
    )
  },

  { H_bit_precise,
    LANG__BitInt,
    TOKEN( Y_bit_precise ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_integer )
    )
  },

  { L_bits,
    LANG_ANY,
    TOKEN( Y_bits ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_Apple_block,
    LANG_ANY,
    TOKEN( Y_Apple_block ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_bool,
    LANG_BOOLEAN,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_boolean,
    LANG_BOOLEAN,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    AC_SETTINGS(
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_Boolean,
    LANG_BOOLEAN,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Bool, L__Bool },
      { LANG_ANY,   L_bool  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_by,
    LANG_LAMBDAS,
    TOKEN( Y_by ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_default )
    )
  },

  { L_bytes,
    LANG_ALIGNMENT,
    TOKEN( Y_bytes ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_capture,
    LANG_LAMBDAS,
    TOKEN( Y_capturing ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "capturing"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_captures,
    LANG_LAMBDAS,
    TOKEN( Y_capturing ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "capturing"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_capturing,
    LANG_LAMBDAS,
    TOKEN( Y_capturing ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_copy, L_reference )
    )
  },

  { L_carries,
    LANG_carries_dependency,
    TOKEN( Y_carries ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "carries_dependency"
      AC_NEXT_KEYWORDS( L_dependency )
    )
  },

  { H_carries_dependency,
    LANG_carries_dependency,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_carries_dependency ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_cast,
    LANG_ANY,
    TOKEN( Y_cast ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_into )
    )
  },

  { L_character,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_char ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_command,
    LANG_ANY,
    TOKEN( Y_commands ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "commands"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_commands,
    LANG_ANY,
    TOKEN( Y_commands ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_complex,
    LANG__Complex,
    SYNONYMS( ALWAYS_FIND,
      { ~LANG__Complex & LANG_C_ANY, L_GNU___complex },
      { LANG__Complex,               L__Complex      },
      { LANG_ANY,                    L_complex       } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_const,
    LANG_ANY,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_const, L_GNU___const },
      { LANG_ANY,    L_const       } ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to C keyword
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_constant,
    LANG_ANY,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_const, L_GNU___const },
      { LANG_ANY,    L_const       } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_eval,
    LANG_consteval,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_consteval ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_evaluation,
    LANG_consteval,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_consteval ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_expr,
    LANG_constexpr,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constexpr ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_expression,
    LANG_constexpr,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constexpr ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_const_init,
    LANG_constinit,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constinit ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_constant_initialization,
    LANG_constinit,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_constinit ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_constructor,
    LANG_CONSTRUCTORS,
    TOKEN( Y_constructor ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_conv,
    LANG_CPP_ANY,
    TOKEN( Y_conversion ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "conversion"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_conversion,
    LANG_CPP_ANY,
    TOKEN( Y_conversion ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_operator, L_returning )
    )
  },

  { L_copy,
    LANG_LAMBDAS,
    TOKEN( Y_copy ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_by )
    )
  },

  { L_ctor,
    LANG_CONSTRUCTORS,
    TOKEN( Y_constructor ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_declare,
    LANG_ANY,
    TOKEN( Y_declare ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_defaulted,
    LANG_default_delete_FUNCS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_default ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_define,
    LANG_ANY,
    TOKEN( Y_define ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_defined,
    LANG_CPP_ANY,
    TOKEN( Y_defined ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { L_deleted,
    LANG_default_delete_FUNCS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_delete ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dependency,
    LANG_carries_dependency,
    TOKEN( Y_dependency ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_destructor,
    LANG_CONSTRUCTORS,
    TOKEN( Y_destructor ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_discard,
    LANG_nodiscard,
    TOKEN( Y_discard ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_double,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_double ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to C keyword
      AC_NEXT_KEYWORDS( L_precision )
    )
  },

  { H_double_precision,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_double ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dtor,
    LANG_CONSTRUCTORS,
    TOKEN( Y_destructor ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_dynamic,
    LANG_NEW_STYLE_CASTS,
    TOKEN( Y_dynamic ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_cast )
    )
  },

  { L_english,
    LANG_ANY,
    TOKEN( Y_english ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_enumeration,
    LANG_enum,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_enum ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_eval,
    LANG_consteval,
    TOKEN( Y_evaluation ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "evaluation"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_evaluation,
    LANG_consteval,
    TOKEN( Y_evaluation ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_except,
    LANG_noexcept,
    TOKEN( Y_except ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_exit,
    LANG_ANY,
    TOKEN( Y_quit ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_explain,
    LANG_ANY,
    TOKEN( Y_explain ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_exported,
    LANG_export,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_export ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_expr,
    LANG_constexpr,
    TOKEN( Y_expression ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "expression"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_expression,
    LANG_constexpr,
    TOKEN( Y_expression ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_external,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_extern ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_floating,
    LANG_ANY,
    TOKEN( Y_floating ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "floating-point"
      AC_NEXT_KEYWORDS( L_point )
    )
  },

  { H_floating_point,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_float ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_func,
    LANG_ANY,
    TOKEN( Y_function ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "function"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_function,
    LANG_ANY,
    TOKEN( Y_function ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_help,
    LANG_ANY,
    TOKEN( Y_help ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_imaginary,
    LANG__Imaginary,
    SYNONYMS( ALWAYS_FIND,
      { LANG__Imaginary, L__Imaginary },
      { LANG_ANY,        L_imaginary  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_init,
    LANG_constinit,
    TOKEN( Y_initialization ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_initialization,
    LANG_constinit,
    TOKEN( Y_initialization ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_integer,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_int ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_into,
    LANG_ANY,
    TOKEN( Y_into ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  {
    L_lambda,
    LANG_LAMBDAS,
    TOKEN( Y_lambda ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_capturing, L_copy, L_reference, L_returning )
    )
  },

  { L_len,
    LANG_VLAS,
    TOKEN( Y_length ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_array )
    )
  },

  { L_length,
    LANG_VLAS,
    TOKEN( Y_length ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_array )
    )
  },

  { L_linkage,
    LANG_LINKAGE_DECLS,
    TOKEN( Y_linkage ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_literal,
    LANG_USER_DEF_LITS,
    TOKEN( Y_literal ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_local,
    LANG_THREAD_LOCAL_STORAGE,
    TOKEN( Y_local ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_maybe,
    LANG_maybe_unused,
    TOKEN( Y_maybe ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "maybe-unused"
      AC_NEXT_KEYWORDS( L_unused )
    )
  },

  { H_maybe_unused,
    LANG_maybe_unused,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_maybe_unused ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_mbr,
    LANG_CPP_ANY,
    TOKEN( Y_member ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_member,
    LANG_CPP_ANY,
    TOKEN( Y_member ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_no,
    LANG_nodiscard | LANG_noexcept | LANG_noreturn | LANG_no_unique_address,
    TOKEN( Y_no ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NEXT_KEYWORDS( L_discard, L_except, L_return, L_unique )
    )
  },

  { H_no_discard,
    LANG_nodiscard,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_nodiscard ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_except,
    LANG_noexcept,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_noexcept ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "no-exception"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_exception,
    LANG_noexcept,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_noexcept ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_return,
    LANG_noreturn,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_noreturn, L__Noreturn },
      { LANG_ANY,       L_noreturn  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_discardable,
    LANG_nodiscard,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_nodiscard ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  {
    H_non_empty,
    LANG_QUALIFIED_ARRAYS,
    TOKEN( Y_non_empty ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_array, L_const, L_restrict, L_variable, L_volatile )
    )
  },

  { H_non_mbr,
    LANG_CPP_ANY,
    TOKEN( Y_non_member ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "non-member"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_member,
    LANG_CPP_ANY,
    TOKEN( Y_non_member ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_returning,
    LANG_NONRETURNING_FUNCS,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_noreturn, L__Noreturn },
      { LANG_ANY,       L_noreturn  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_throwing,
    LANG_CPP_ANY,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { LANG_noexcept, L_noexcept },
      { LANG_ANY,      L_throw    } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_no_unique_address,
    LANG_no_unique_address,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_no_unique_address ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_non_unique_address,
    LANG_no_unique_address,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_no_unique_address ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_noreturn,
    LANG_NONRETURNING_FUNCS,
    SYNONYMS( ALWAYS_FIND,
      { ~LANG_noreturn, L__Noreturn },
      { LANG_ANY,       L_noreturn  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_of,
    LANG_ANY,
    TOKEN( Y_of ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_oper,
    LANG_operator,
    TOKEN( Y_operator ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "operator"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_options,
    LANG_ANY,
    TOKEN( Y_options ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_overridden,
    LANG_override,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_override ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_point,
    LANG_ANY,
    TOKEN( Y_point ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_pointer,
    LANG_ANY,
    TOKEN( Y_pointer ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_precise,
    LANG__BitInt,
    TOKEN( Y_precise ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_int )
    )
  },

  { L_precision,
    LANG_ANY,
    TOKEN( Y_precision ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_predef,
    LANG_ANY,
    TOKEN( Y_predefined ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "predefined"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_predefined,
    LANG_ANY,
    TOKEN( Y_predefined ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ptr,
    LANG_ANY,
    TOKEN( Y_pointer ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NEXT_KEYWORDS( L_to )
    )
  },

  { L_pure,
    LANG_virtual,
    TOKEN( Y_pure ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_virtual )
    )
  },

  { L_quit,
    LANG_ANY,
    TOKEN( Y_quit ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ref,
    LANG_REFERENCES,
    TOKEN( Y_reference ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "reference"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_reference,
    LANG_REFERENCES,
    TOKEN( Y_reference ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_by, L_to )
    )
  },

  { L_reinterpret,
    LANG_NEW_STYLE_CASTS,
    TOKEN( Y_reinterpret ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_cast )
    )
  },

  { L_restricted,
    LANG_restrict,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_restrict, L_GNU___restrict },
      { LANG_ANY,       L_restrict       } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_ret,
    LANG_ANY,
    TOKEN( Y_returning ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "return"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_returning,
    LANG_ANY,
    TOKEN( Y_returning ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_rvalue,
    LANG_RVALUE_REFERENCES,
    TOKEN( Y_rvalue ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_reference )
    )
  },

  { L_scope,
    LANG_SCOPED_NAMES,
    TOKEN( Y_scope ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_set,
    LANG_ANY,
    TOKEN( Y_set ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_show,
    LANG_ANY,
    TOKEN( Y_show ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS               // see command_ac_keywords()
    )
  },

  { L_structure,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_struct ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_thread,
    LANG_THREAD_LOCAL_STORAGE,
    TOKEN( Y_thread ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "thread_local"
      AC_NEXT_KEYWORDS( L_local )
    )
  },

  { L_thread_local,
    LANG_THREAD_LOCAL_STORAGE,
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
      // would be legal (because `thread_local` would map to GNU C's
      // `__thread` that's legal in all languages) when it shouldn't be
      // (because neither the C macro `thread_local` nor the C keyword
      // `_Thread_local` were supported until C11).
      //
      { LANG__Thread_local, L__Thread_local },
      { LANG_ANY,           L_thread_local  } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { H_thread_local,
    LANG_THREAD_LOCAL_STORAGE,
    SYNONYMS( ALWAYS_FIND,
      { ~LANG_THREAD_LOCAL_STORAGE, L_GNU___thread  },
      { LANG__Thread_local,         L__Thread_local },
      { LANG_ANY,                   L_thread_local  } ),
    AC_SETTINGS(
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_to,
    LANG_ANY,
    TOKEN( Y_to ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_type,
    LANG_ANY,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_typedef ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "typedef"
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_unique,
    LANG_no_unique_address,
    TOKEN( Y_unique ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NEXT_KEYWORDS( L_address )
    )
  },

  { L_unused,
    LANG_maybe_unused,
    TOKEN( Y_unused ),
    AC_SETTINGS(
      AC_POLICY_IN_NEXT_ONLY,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_user,
    LANG_CPP_ANY,
    TOKEN( Y_user ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NEXT_KEYWORDS( L_defined )
    )
  },

  { H_user_def,
    LANG_CPP_ANY,
    TOKEN( Y_user_defined ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "user-defined"
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { H_user_defined,
    LANG_CPP_ANY,
    TOKEN( Y_user_defined ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_conversion, L_literal )
    )
  },

  { L_var,
    LANG_VLAS,
    TOKEN( Y_variable ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to "variable"
      AC_NEXT_KEYWORDS( L_array, L_length )
    )
  },

  { L_varargs,
    LANG_PROTOTYPES,
    TOKEN( Y_DOT3 ),
    AC_SETTINGS(
      AC_POLICY_NO_OTHER,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_variable,
    LANG_VLAS,
    TOKEN( Y_variable ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_array, L_length )
    )
  },

  { L_variadic,
    LANG_PROTOTYPES,
    TOKEN( Y_DOT3 ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_vector,
    LANG_ANY,
    TOKEN( Y_array ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_volatile,
    LANG_ANY,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { ~LANG_volatile, L_GNU___volatile },
      { LANG_ANY,       L_volatile       } ),
    AC_SETTINGS(
      AC_POLICY_DEFER,                  // to C keyword
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_wide,
    LANG_wchar_t,
    TOKEN( Y_wide ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_char )
    )
  },

  { L_width,
    LANG_ANY,
    TOKEN( Y_width ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NEXT_KEYWORDS( L_bits )
    )
  },

  // Embedded C extensions
  { L_EMC_accum,
    LANG_C_99,
    SYNONYMS( ALWAYS_FIND,
      { LANG_C_99, L_EMC__Accum },
      { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_fract,
    LANG_C_99,
    SYNONYMS( ALWAYS_FIND,
      { LANG_C_99, L_EMC__Fract },
      { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_sat,
    LANG_C_99,
    SYNONYMS( FIND_IN_ENGLISH_ONLY,
      { LANG_C_99, L_EMC__Sat   },
      { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      AC_POLICY_TOO_SHORT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_EMC_saturated,
    LANG_C_99,
    SYNONYMS( ALWAYS_FIND,
      { LANG_C_99, L_EMC__Sat   },
      { LANG_ANY,  NULL         } ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  // Microsoft extensions
  { L_MSC_cdecl,
    LANG_MSC_EXTENSIONS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___cdecl ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_clrcall,
    LANG_MSC_EXTENSIONS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___clrcall ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_fastcall,
    LANG_MSC_EXTENSIONS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___fastcall ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_stdcall,
    LANG_MSC_EXTENSIONS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___stdcall ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_thiscall,
    LANG_MSC_EXTENSIONS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___thiscall ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_vectorcall,
    LANG_MSC_EXTENSIONS,
    SYNONYM( FIND_IN_ENGLISH_ONLY, L_MSC___vectorcall ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { L_MSC_WINAPI,
    LANG_MSC_EXTENSIONS,
    SYNONYM( ALWAYS_FIND, L_MSC___stdcall ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  },

  { NULL,
    LANG_NONE,
    TOKEN( 0 ),
    AC_SETTINGS(
      AC_POLICY_DEFAULT,
      AC_NO_NEXT_KEYWORDS
    )
  }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Compares two \ref cdecl_keyword objects.
 *
 * @param i_cdk The first \ref cdecl_keyword to compare.
 * @param j_cdk The second \ref cdecl_keyword to compare.
 * @return @return Returns a number less than 0, 0, or greater than 0 if \a
 * i_cdk is less than, equal to, or greater than \a j_cdk, respectively.
 */
NODISCARD
static int cdecl_keyword_cmp( cdecl_keyword_t const *i_cdk,
                              cdecl_keyword_t const *j_cdk ) {
  return strcmp( i_cdk->literal, j_cdk->literal );
}

////////// extern functions ///////////////////////////////////////////////////

cdecl_keyword_t const* cdecl_keyword_find( char const *s ) {
  assert( s != NULL );

  // the list is small, so linear search is good enough
  for ( cdecl_keyword_t const *cdk = CDECL_KEYWORDS; cdk->literal != NULL;
        ++cdk ) {
    int const cmp = strcmp( s, cdk->literal );
    if ( cmp > 0 )
      continue;
    if ( cmp < 0 )                      // the array is sorted
      break;
    return cdk;
  } // for

  return NULL;
}

void cdecl_keywords_init( void ) {
  ASSERT_RUN_ONCE();
  qsort(                                // don't rely on manual sorting above
    CDECL_KEYWORDS, ARRAY_SIZE( CDECL_KEYWORDS ) - 1/*NULL*/,
    sizeof( cdecl_keyword_t ),
    POINTER_CAST( qsort_cmp_fn_t, &cdecl_keyword_cmp )
  );
}

cdecl_keyword_t const* cdecl_keyword_next( cdecl_keyword_t const *cdk ) {
  return cdk == NULL ? CDECL_KEYWORDS : (++cdk)->literal == NULL ? NULL : cdk;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
