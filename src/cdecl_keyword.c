/*
**      cdecl -- C gibberish translator
**      src/cdeck_keyword.cpp
**
**      Copyright (C) 2021-2022  Paul J. Lucas
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
 * Specify that the previosuly given literal is a synonym for the given
 * language-specific literals.
 *
 * @param ALWAYS_FIND If `true`, always find this synonym, even when explaining
 * C/C++.
 * @param LANG The #AC_LANG macro to use.  It _must not_ be enclosed in `()`
 * since it _may_ expand into nothing.
 * @param ... Array of c_lang_lit.
 *
 * @sa #CDECL_KEYWORDS for examples.
 * @sa #C_SYA()
 * @sa #C_SYE()
 */
#define C_SYN(ALWAYS_FIND,LANG,...) \
  (ALWAYS_FIND), /*y_token_id=*/0, C_LANG_LIT( __VA_ARGS__ ), LANG

/**
 * Special-case of #C_SYN when there is only one language(s)/literal pair _and_
 * always find this synonym, even when explaining C/C++.
 *
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 * @param LANG The #AC_LANG macro to use.  It _must not_ be enclosed in `()`
 * since it _may_ expand into nothing.
 *
 * @sa #CDECL_KEYWORDS for examples.
 * @sa #C_SYE()
 * @sa #C_SYN()
 */
#define C_SYA(C_KEYWORD,LANG) \
  C_SYN( /*always_find=*/true, LANG, { LANG_ANY, C_KEYWORD } )

/**
 * Special-case of #C_SYN when there is only one language(s)/literal pair _but_
 * find this synonym only when converting from pseudo-English to gibberish.
 *
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 * @param LANG The #AC_LANG macro to use.  It _must not_ be enclosed in `()`
 * since it _may_ expand into nothing.
 *
 * @sa #CDECL_KEYWORDS for examples.
 * @sa #C_SYE()
 * @sa #C_SYN()
 */
#define C_SYE(C_KEYWORD,LANG) \
  C_SYN( /*always_find=*/false, LANG, { LANG_ANY, C_KEYWORD } )

/**
 * Specify that the previosuly given literal maps to Bison tokan \a Y_ID.
 *
 * @param Y_ID The Bison token ID (`Y_xxx`).
 * @param LANG The #AC_LANG macro to use.  It _must not_ be enclosed in `()`
 * since it _may_ expand into nothing.
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define TOKEN(Y_ID,LANG) \
  /*always_find=*/false, (Y_ID), /*lang_syn=*/NULL, LANG

/**
 * All **cdecl** keywords that are (mostly) _not_ C/C++ keywords.  Exceptions
 * are `alignas`, `bool`, `complex`, `const`, and `volatile` that are included
 * here as **cdecl** keywords so each maps to its language-specific literal.
 *
 * ## Initialization Macros
 *
 * The #C_SYN, #C_SYA, #C_SYE, and #TOKEN macros are used to initialize entries
 * in the array as follows.
 *
 * To have a literal for a **cdecl** keyword map to its corresponding token,
 * use #TOKEN:
 *
 *      // The "aligned" literal maps to the Y_aligned token:
 *      { L_aligned,        TOKEN( Y_aligned, AC_LANG(ALIGNMENT)  ) }
 *
 * To have a literal that is a synonym for another literal for a **cdecl**
 * keyword map to the other literal's same token, use #TOKEN with the other
 * literal's token:
 *
 *      // The "align" literal synonym also maps to the Y_aligned token:
 *      { L_align,          TOKEN( Y_aligned, AC_LANG(NONE) ) },
 *
 * Note that synonyms should _not_ be auto-completable because they'd be
 * ambiguous with each other.
 *
 * To have a literal that is pseudo-English be a synonym for exactly one
 * corresponding C/C++ keyword literal, but only when converting pseudo-English
 * to gibberish, use #C_SYE:
 *
 *      // The "atomic" literal is a synonym for the "_Atomic" literal, but
 *      // only when converting from pseudo-English to gibberish:
 *      { L_atomic,         C_SYE( L__Atomic, AC_LANG(_Atomic) ) },
 *
 * To do the same, but allow the literal at any time (i.e., also when
 * converting gibberish to pseudo-English), use #C_SYA:
 *
 *      // The "WINAPI" literal is always a synonym for the "__stdcall"
 *      // literal.
 *      { L_MSC_WINAPI,     C_SYA( L_MSC___stdcall, AC_LANG(MSC_EXTENSIONS) ) },
 *
 * To have a literal that is pseudo-English be a synonym for more than one
 * corresponding C/C++ keyword depending on the current language, use #C_SYN
 * with the last row always containing #LANG_ANY:
 *
 *      // The "noreturn" literal is a synonym for the "_Noreturn" literal only
 *      // in C11 and later.
 *      { L_noreturn,       C_SYN( true, AC_LANG(NONRETURNING_FUNC),
 *                            { { ~LANG_noreturn, L__Noreturn },
 *                              { LANG_ANY,       L_noreturn  } } ) },
 *
 * ## Autocompletion
 *
 * The #AC_LANG macro is used to specify the language(s) that a keyword should
 * be auto-completed in.  A keyword is auto-completable _unless_ it:
 *
 * 1. Is a synonym for a preferred **cdecl** token, e.g., `conversion` is auto-
 *    completable, but `conv` is not.
 *
 * 2. Is a synonym for a C/C++ token, e.g., `enum` is auto-completable (via
 *    \ref C_KEYWORDS), but `enumeration` is not.
 *
 * 3. Is a hyphenated token (`H_`).  (The non-hyphenated one is preferred.)
 *
 * 4. Is short, e.g, `all`, `as`, `no`, `of`, and `to` are not auto-
 *    completable.
 *
 * @sa CDECL_COMMANDS
 * @sa C_KEYWORDS
 */
static cdecl_keyword_t const CDECL_KEYWORDS[] = {
  { L_address,        TOKEN( Y_address,           AC_LANG(no_unique_address)) },
  { L_align,          TOKEN( Y_aligned,           AC_LANG(NONE)             ) },
  { L_alignas,        C_SYN( true,                AC_LANG(NONE),
                        { LANG__Alignas, L__Alignas },
                        { LANG_ANY,      L_alignas  }                       ) },
  { L_aligned,        TOKEN( Y_aligned,           AC_LANG(ALIGNMENT)        ) },
  { L_all,            TOKEN( Y_all,               AC_LANG(NONE)             ) },
  { L_Apple_block,    TOKEN( Y_Apple_BLOCK,       AC_LANG(ANY)              ) },
  { L_array,          TOKEN( Y_array,             AC_LANG(ANY)              ) },
  { L_as,             TOKEN( Y_as,                AC_LANG(NONE)             ) },
  { L_atomic,         C_SYA( L__Atomic,           AC_LANG(_Atomic)          ) },
  { L_automatic,      C_SYE( L_auto,              AC_LANG(NONE)             ) },
  { L_bits,           TOKEN( Y_bits,              AC_LANG(ANY)              ) },
  { L_bool,           C_SYN( true,                AC_LANG(NONE),
                        { LANG__Bool, L__Bool },
                        { LANG_ANY,   L_bool  }                             ) },
  { L_bytes,          TOKEN( Y_bytes,             AC_LANG(ALIGNMENT)        ) },
  { L_carries,        TOKEN( Y_carries,           AC_LANG(NONE)             ) },
  { H_carries_dependency,
                      C_SYE( L_carries_dependency,
                                                  AC_LANG(NONE)             ) },
  { L_cast,           TOKEN( Y_cast,              AC_LANG(ANY)              ) },
  { L_character,      C_SYE( L_char,              AC_LANG(NONE)             ) },
  { L_complex,        C_SYN( true,                AC_LANG(NONE),
                        { ~LANG__Complex & LANG_C_ANY, L_GNU___complex },
                        { LANG__Complex,               L__Complex      },
                        { LANG_ANY,                    L_complex       }    ) },
  { L_command,        TOKEN( Y_commands,          AC_LANG(NONE)             ) },
  { L_commands,       TOKEN( Y_commands,          AC_LANG(ANY)              ) },
  { L_const,          C_SYN( false,               AC_LANG(NONE),
                        { ~LANG_const, L_GNU___const },
                        { LANG_ANY,    L_const       }                      ) },
  { L_constant,       C_SYN( false,               AC_LANG(NONE),
                        { ~LANG_const, L_GNU___const },
                        { LANG_ANY,    L_const       }                      ) },
  { H_const_eval,     C_SYE( L_consteval,         AC_LANG(NONE)             ) },
  { H_constant_evaluation,
                      C_SYE( L_consteval,         AC_LANG(NONE)             ) },
  { H_const_expr,     C_SYE( L_constexpr,         AC_LANG(NONE)             ) },
  { H_constant_expression,
                      C_SYE( L_constexpr,         AC_LANG(NONE)             ) },
  { H_const_init,     C_SYE( L_constinit,         AC_LANG(NONE)             ) },
  { H_constant_initialization,
                      C_SYE( L_constinit,         AC_LANG(NONE)             ) },
  { L_constructor,    TOKEN( Y_constructor,       AC_LANG(CPP_ANY)          ) },
  { L_conv,           TOKEN( Y_conversion,        AC_LANG(NONE)             ) },
  { L_conversion,     TOKEN( Y_conversion,        AC_LANG(CPP_ANY)          ) },
  { L_ctor,           TOKEN( Y_constructor,       AC_LANG(NONE)             ) },
  { L_declare,        TOKEN( Y_declare,           AC_LANG(ANY)              ) },
  { L_defaulted,      C_SYE( L_default,           AC_LANG(NONE)             ) },
  { L_define,         TOKEN( Y_define,            AC_LANG(ANY)              ) },
  { L_deleted,        C_SYE( L_delete,            AC_LANG(NONE)             ) },
  { L_dependency,     TOKEN( Y_dependency,
                                                AC_LANG(carries_dependency) ) },
  { L_destructor,     TOKEN( Y_destructor,        AC_LANG(CPP_ANY)          ) },
  { L_discard,        TOKEN( Y_discard,           AC_LANG(nodiscard)        ) },
  { H_double_precision,
                      C_SYE( L_double,            AC_LANG(NONE)             ) },
  { L_dtor,           TOKEN( Y_destructor,        AC_LANG(CPP_ANY)          ) },
  { L_dynamic,        TOKEN( Y_dynamic,           AC_LANG(NONE)             ) },
  { L_english,        TOKEN( Y_english,           AC_LANG(ANY)              ) },
  { L_enumeration,    C_SYE( L_enum,              AC_LANG(NONE)             ) },
  { L_eval,           TOKEN( Y_evaluation,        AC_LANG(NONE)             ) },
  { L_evaluation,     TOKEN( Y_evaluation,        AC_LANG(consteval)        ) },
  { L_except,         TOKEN( Y_except,            AC_LANG(noexcept)         ) },
  { L_exit,           TOKEN( Y_quit,              AC_LANG(NONE)             ) },
  { L_explain,        TOKEN( Y_explain,           AC_LANG(ANY)              ) },
  { L_exported,       C_SYE( L_export,            AC_LANG(NONE)             ) },
  { L_expr,           TOKEN( Y_expression,        AC_LANG(NONE)             ) },
  { L_expression,     TOKEN( Y_expression,        AC_LANG(constexpr)        ) },
  { L_external,       C_SYE( L_extern,            AC_LANG(NONE)             ) },
  { L_floating,       TOKEN( Y_floating,          AC_LANG(ANY)              ) },
  { H_floating_point, C_SYE( L_float,             AC_LANG(NONE)             ) },
  { L_func,           TOKEN( Y_function,          AC_LANG(NONE)             ) },
  { L_function,       TOKEN( Y_function,          AC_LANG(ANY)              ) },
  { L_help,           TOKEN( Y_help,              AC_LANG(ANY)              ) },
  { L_imaginary,      C_SYN( true,                AC_LANG(_Imaginary),
                        { LANG__Imaginary, L__Imaginary },
                        { LANG_ANY,        L_imaginary  }                   ) },
  { L_init,           TOKEN( Y_initialization,    AC_LANG(NONE)             ) },
  { L_initialization, TOKEN( Y_initialization,    AC_LANG(constinit)        ) },
  { L_integer,        C_SYE( L_int,               AC_LANG(NONE)             ) },
  { L_into,           TOKEN( Y_into,              AC_LANG(ANY)              ) },
  { L_len,            TOKEN( Y_length,            AC_LANG(NONE)             ) },
  { L_length,         TOKEN( Y_length,            AC_LANG(VLA)              ) },
  { L_linkage,        TOKEN( Y_linkage,           AC_LANG(CPP_ANY)          ) },
  { L_literal,        TOKEN( Y_literal,       AC_LANG(USER_DEFINED_LITERAL) ) },
  { L_local,          TOKEN( Y_local,         AC_LANG(THREAD_LOCAL_STORAGE) ) },
  { L_maybe,          TOKEN( Y_maybe,             AC_LANG(NONE)             ) },
  { H_maybe_unused,   C_SYE( L_maybe_unused,      AC_LANG(NONE)             ) },
  { L_mbr,            TOKEN( Y_member,            AC_LANG(NONE)             ) },
  { L_member,         TOKEN( Y_member,            AC_LANG(CPP_ANY)          ) },
  { L_no,             TOKEN( Y_no,                AC_LANG(NONE)             ) },
  { H_no_discard,     C_SYE( L_nodiscard,         AC_LANG(NONE)             ) },
  { H_no_except,      C_SYE( L_noexcept,          AC_LANG(NONE)             ) },
  { H_no_exception,   C_SYE( L_noexcept,          AC_LANG(NONE)             ) },
  { H_no_return,      C_SYN( false,               AC_LANG(NONE),
                        { ~LANG_noreturn, L__Noreturn },
                        { LANG_ANY,       L_noreturn  }                     ) },
  { H_non_discardable,
                      C_SYE( L_nodiscard,         AC_LANG(NONE)             ) },
  { H_non_mbr,        TOKEN( Y_non_member,        AC_LANG(NONE)             ) },
  { H_non_member,     TOKEN( Y_non_member,        AC_LANG(CPP_ANY)          ) },
  { H_non_returning,  C_SYN( false,               AC_LANG(NONE),
                        { ~LANG_noreturn, L__Noreturn },
                        { LANG_ANY,       L_noreturn  }                     ) },
  { H_non_throwing,   C_SYE( L_throw,             AC_LANG(CPP_ANY)          ) },
  { H_no_unique_address,
                      C_SYE( L_no_unique_address, AC_LANG(NONE)             ) },
  { H_non_unique_address,
                      C_SYE( L_no_unique_address, AC_LANG(NONE)             ) },
  { L_noreturn,       C_SYN( true,                AC_LANG(NONRETURNING_FUNC),
                        { ~LANG_noreturn, L__Noreturn },
                        { LANG_ANY,       L_noreturn  }                     ) },
  { L_of,             TOKEN( Y_of,                AC_LANG(NONE)             ) },
  { L_oper,           TOKEN( Y_operator,          AC_LANG(NONE)             ) },
  { L_options,        TOKEN( Y_options,           AC_LANG(ANY)              ) },
  { L_overridden,     C_SYE( L_override,          AC_LANG(NONE)             ) },
  { L_point,          TOKEN( Y_point,             AC_LANG(ANY)              ) },
  { L_pointer,        TOKEN( Y_pointer,           AC_LANG(ANY)              ) },
  { L_precision,      TOKEN( Y_precision,         AC_LANG(ANY)              ) },
  { L_predef,         TOKEN( Y_predefined,        AC_LANG(NONE)             ) },
  { L_predefined,     TOKEN( Y_predefined,        AC_LANG(ANY)              ) },
  { L_ptr,            TOKEN( Y_pointer,           AC_LANG(NONE)             ) },
  { L_pure,           TOKEN( Y_pure,              AC_LANG(CPP_ANY)          ) },
  { L_q,              TOKEN( Y_quit,              AC_LANG(NONE)             ) },
  { L_quit,           TOKEN( Y_quit,              AC_LANG(ANY)              ) },
  { L_ref,            TOKEN( Y_reference,         AC_LANG(NONE)             ) },
  { L_reference,      TOKEN( Y_reference,         AC_LANG(CPP_ANY)          ) },
  { L_reinterpret,    TOKEN( Y_reinterpret,       AC_LANG(CPP_ANY)          ) },
  { L_restricted,     C_SYN( false,               AC_LANG(NONE),
                        { ~LANG_restrict, L_GNU___restrict },
                        { LANG_ANY,       L_restrict       }                ) },
  { L_ret,            TOKEN( Y_returning,         AC_LANG(NONE)             ) },
  { L_returning,      TOKEN( Y_returning,         AC_LANG(ANY)              ) },
  { L_rvalue,         TOKEN( Y_rvalue,            AC_LANG(RVALUE_REFERENCE) ) },
  { L_scope,          TOKEN( Y_scope,             AC_LANG(ANY)              ) },
  { L_set,            TOKEN( Y_set,               AC_LANG(ANY)              ) },
  { L_show,           TOKEN( Y_show,              AC_LANG(ANY)              ) },
  { L_structure,      C_SYE( L_struct,            AC_LANG(NONE)             ) },
  { L_thread_local,   C_SYN( true,                AC_LANG(THREAD_LOCAL_STORAGE),
                        { LANG__Thread_local, L__Thread_local },
                        { LANG_ANY,           L_thread_local  }             ) },
  { L_thread,         TOKEN( Y_thread,            AC_LANG(NONE)             ) },
  { H_thread_local,   C_SYN( true, AC_LANG(NONE),
                        { ~LANG_THREAD_LOCAL_STORAGE, L_GNU___thread  },
                        { LANG__Thread_local,         L__Thread_local },
                        { LANG_ANY,                   L_thread_local  }     ) },
  { L_to,             TOKEN( Y_to,                AC_LANG(NONE)             ) },
  { L_type,           C_SYE( L_typedef,           AC_LANG(NONE)             ) },
  { L_unique,         TOKEN( Y_unique,           AC_LANG(no_unique_address) ) },
  { L_unused,         TOKEN( Y_unused,            AC_LANG(maybe_unused)     ) },
  { L_user,           TOKEN( Y_user,              AC_LANG(NONE)             ) },
  { H_user_def,       TOKEN( Y_user_defined,      AC_LANG(NONE)             ) },
  { H_user_defined,   TOKEN( Y_user_defined,      AC_LANG(CPP_ANY)          ) },
  { L_var,            TOKEN( Y_variable,          AC_LANG(NONE)             ) },
  { L_varargs,        TOKEN( Y_ELLIPSIS,          AC_LANG(NONE)             ) },
  { L_variable,       TOKEN( Y_variable,          AC_LANG(VLA)              ) },
  { L_variadic,       TOKEN( Y_ELLIPSIS,          AC_LANG(PROTOTYPES)       ) },
  { L_vector,         TOKEN( Y_array,             AC_LANG(ANY)              ) },
  { L_volatile,       C_SYN( false,               AC_LANG(NONE),
                        { ~LANG_volatile, L_GNU___volatile },
                        { LANG_ANY,       L_volatile       }                ) },
  { L_wide,           TOKEN( Y_wide,              AC_LANG(wchar_t)          ) },
  { L_width,          TOKEN( Y_width,             AC_LANG(ANY)              ) },

  // Embedded C extensions
  { L_EMC_accum,      C_SYN( true,                AC_LANG(C_99),
                        { LANG_C_99, L_EMC__Accum },
                        { LANG_ANY,  NULL         }                         ) },
  { L_EMC_fract,      C_SYN( true,                AC_LANG(C_99),
                        { LANG_C_99, L_EMC__Fract },
                        { LANG_ANY,  NULL         }                         ) },
  { L_EMC_sat,        C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_99, L_EMC__Sat   },
                        { LANG_ANY,  NULL         }                         ) },
  { L_EMC_saturated,  C_SYN( true,                AC_LANG(NONE),
                        { LANG_C_99, L_EMC__Sat   },
                        { LANG_ANY,  NULL         }                         ) },

  // Microsoft extensions
  { L_MSC_cdecl,      C_SYE( L_MSC___cdecl,       AC_LANG(MSC_EXTENSIONS)   ) },
  { L_MSC_clrcall,    C_SYE( L_MSC___clrcall,     AC_LANG(MSC_EXTENSIONS)   ) },
  { L_MSC_fastcall,   C_SYE( L_MSC___fastcall,    AC_LANG(MSC_EXTENSIONS)   ) },
  { L_MSC_stdcall,    C_SYE( L_MSC___stdcall,     AC_LANG(MSC_EXTENSIONS)   ) },
  { L_MSC_thiscall,   C_SYE( L_MSC___thiscall,    AC_LANG(MSC_EXTENSIONS)   ) },
  { L_MSC_vectorcall, C_SYE( L_MSC___vectorcall,  AC_LANG(MSC_EXTENSIONS)   ) },
  { L_MSC_WINAPI,     C_SYA( L_MSC___stdcall,     AC_LANG(MSC_EXTENSIONS)   ) },

  { NULL,             TOKEN( 0,                   AC_LANG(NONE)             ) }
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
