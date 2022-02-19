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
 * Defines helper macros, data structures, variables, functions, and the
 * tokenizer for C/C++ declarations.
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
  (ALWAYS_FIND), /*y_token_id=*/0, (c_lang_lit_t const[]){ __VA_ARGS__ }, LANG

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
 * All cdecl keywords that are (mostly) _not_ C/C++ keywords.
 * Exceptions are `bool`, `complex`, `const`, and `volatile` that are included
 * here as cdecl keywords so each maps to its language-specific literal.
 *
 * ## Initialization Macros
 *
 * The #C_SYN, #C_SYA, #C_SYE, and #TOKEN macros are used to initialize entries
 * in the array as follows.
 *
 * To have a literal for a cdecl keyword map to its corresponding token, use
 * #TOKEN:
 *
 *      // The "aligned" literal maps to the Y_ALIGNED token:
 *      { L_ALIGNED,        TOKEN( Y_ALIGNED, AC_LANG(NONE)  ) }
 *
 * To have a literal that is a synonym for another literal for a cdecl keyword
 * map to the other literal's same token, use #TOKEN with the other literal's
 * token:
 *
 *      // The "align" literal synonym also maps to the Y_ALIGNED token:
 *      { L_ALIGN,          TOKEN( Y_ALIGNED, AC_LANG(C_CPP_MIN(11,11)) ) },
 *
 * To have a literal that is pseudo-English be a synonym for exactly one
 * corresponding C/C++ keyword literal, but only when converting pseudo-English
 * to gibberish, use #C_SYE:
 *
 *      // The "atomic" literal is a synonym for the "_Atomic" literal, but
 *      // only when converting from pseudo-English to gibberish:
 *      { L_ATOMIC,         C_SYE( L__ATOMIC, AC_LANG(C_MIN(11)) ) },
 *
 * To do the same, but allow the literal at any time (i.e., also when
 * converting gibberish to pseudo-English), use #C_SYA:
 *
 *      // The "WINAPI" literal is always a synonym for the "__stdcall"
 *      // literal.
 *      { L_MSC_WINAPI,     C_SYA( L_MSC___STDCALL, AC_LANG(MIN(C_89)) ) },
 *
 * To have a literal that is pseudo-English be a synonym for more than one
 * corresponding C/C++ keyword depending on the current language, use #C_SYN
 * with the last row always containing #LANG_ANY:
 *
 *      // The "noreturn" literal is a synonym for the "_Noreturn" literal only
 *      // in C11 and later.
 *      { L_NORETURN,       C_SYN( true, AC_LANG(C_CPP_MIN(11,11)),
 *                            { { LANG_C_MIN(11), L__NORETURN },
 *                              { LANG_ANY,       L_NORETURN  } } ) },
 *
 * ## Autocompletion
 *
 * The #AC_LANG macro is used to specify the language(s) that a keyword should
 * be auto-completed in.  A keyword is auto-completable _unless_ it:
 *
 * 1. Is a synonym for a preferred cdecl token, e.g., `conversion` is auto-
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
  { L_ADDRESS,        TOKEN( Y_ADDRESS,           AC_LANG(CPP_MIN(20))      ) },
  { L_ALIGN,          TOKEN( Y_ALIGNED,           AC_LANG(NONE)             ) },
  { L_ALIGNED,        TOKEN( Y_ALIGNED,           AC_LANG(C_CPP_MIN(11,11)) ) },
  { L_ALL,            TOKEN( Y_ALL,               AC_LANG(NONE)             ) },
  { L_APPLE_BLOCK,    TOKEN( Y_APPLE_BLOCK,       AC_LANG(ANY)              ) },
  { L_ARRAY,          TOKEN( Y_ARRAY,             AC_LANG(ANY)              ) },
  { L_AS,             TOKEN( Y_AS,                AC_LANG(NONE)             ) },
  { L_ATOMIC,         C_SYA( L__ATOMIC,           AC_LANG(C_MIN(11))        ) },
  { L_AUTOMATIC,      C_SYE( L_AUTO,              AC_LANG(NONE)             ) },
  { L_BITS,           TOKEN( Y_BITS,              AC_LANG(ANY)              ) },
  { L_BOOL,           C_SYN( true,                AC_LANG(NONE),
                        { LANG_C_MIN(99), L__BOOL },
                        { LANG_ANY,       L_BOOL  }                         ) },
  { L_BYTES,          TOKEN( Y_BYTES,             AC_LANG(C_CPP_MIN(11,11)) ) },
  { L_CARRIES,        TOKEN( Y_CARRIES,           AC_LANG(NONE)             ) },
  { H_CARRIES_DEPENDENCY,
                      C_SYE( L_CARRIES_DEPENDENCY,
                                                  AC_LANG(NONE)             ) },
  { L_CAST,           TOKEN( Y_CAST,              AC_LANG(ANY)              ) },
  { L_CHARACTER,      C_SYE( L_CHAR,              AC_LANG(NONE)             ) },
  { L_COMPLEX,        C_SYN( true,                AC_LANG(NONE),
                        { LANG_C_MAX(95), L_GNU___COMPLEX },
                        { LANG_C_MIN(99), L__COMPLEX      },
                        { LANG_ANY,       L_COMPLEX       }                 ) },
  { L_COMMAND,        TOKEN( Y_COMMANDS,          AC_LANG(NONE)             ) },
  { L_COMMANDS,       TOKEN( Y_COMMANDS,          AC_LANG(ANY)              ) },
  { L_CONST,          C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_KNR, L_GNU___CONST },
                        { LANG_ANY,   L_CONST       }                       ) },
  { L_CONSTANT,       C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_KNR, L_GNU___CONST },
                        { LANG_ANY,   L_CONST       }                       ) },
  { L_CONSTRUCTOR,    TOKEN( Y_CONSTRUCTOR,       AC_LANG(CPP_ANY)          ) },
  { L_CONV,           TOKEN( Y_CONVERSION,        AC_LANG(NONE)             ) },
  { L_CONVERSION,     TOKEN( Y_CONVERSION,        AC_LANG(CPP_ANY)          ) },
  { L_CTOR,           TOKEN( Y_CONSTRUCTOR,       AC_LANG(NONE)             ) },
  { L_DECLARE,        TOKEN( Y_DECLARE,           AC_LANG(ANY)              ) },
  { L_DEFAULTED,      C_SYE( L_DEFAULT,           AC_LANG(NONE)             ) },
  { L_DEFINE,         TOKEN( Y_DEFINE,            AC_LANG(ANY)              ) },
  { L_DELETED,        C_SYE( L_DELETE,            AC_LANG(NONE)             ) },
  { L_DEPENDENCY,     TOKEN( Y_DEPENDENCY,        AC_LANG(CPP_MIN(11))      ) },
  { L_DESTRUCTOR,     TOKEN( Y_DESTRUCTOR,        AC_LANG(CPP_ANY)          ) },
  { L_DISCARD,        TOKEN( Y_DISCARD,           AC_LANG(CPP_MIN(17))      ) },
  { H_DOUBLE_PRECISION,
                      C_SYE( L_DOUBLE,            AC_LANG(NONE)             ) },
  { L_DTOR,           TOKEN( Y_DESTRUCTOR,        AC_LANG(CPP_ANY)          ) },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC,           AC_LANG(NONE)             ) },
  { L_ENGLISH,        TOKEN( Y_ENGLISH,           AC_LANG(ANY)              ) },
  { L_ENUMERATION,    C_SYE( L_ENUM,              AC_LANG(NONE)             ) },
  { L_EVAL,           TOKEN( Y_EVALUATION,        AC_LANG(NONE)             ) },
  { L_EVALUATION,     TOKEN( Y_EVALUATION,        AC_LANG(CPP_MIN(20))      ) },
  { L_EXCEPT,         TOKEN( Y_EXCEPT,            AC_LANG(CPP_MIN(11))      ) },
  { L_EXIT,           TOKEN( Y_QUIT,              AC_LANG(ANY)              ) },
  { L_EXPLAIN,        TOKEN( Y_EXPLAIN,           AC_LANG(ANY)              ) },
  { L_EXPORTED,       C_SYE( L_EXPORT,            AC_LANG(NONE)             ) },
  { L_EXPR,           TOKEN( Y_EXPRESSION,        AC_LANG(NONE)             ) },
  { L_EXPRESSION,     TOKEN( Y_EXPRESSION,        AC_LANG(CPP_MIN(11))      ) },
  { L_EXTERNAL,       C_SYE( L_EXTERN,            AC_LANG(NONE)             ) },
  { H_FLOATING_POINT, C_SYE( L_FLOAT,             AC_LANG(NONE)             ) },
  { L_FUNC,           TOKEN( Y_FUNCTION,          AC_LANG(NONE)             ) },
  { L_FUNCTION,       TOKEN( Y_FUNCTION,          AC_LANG(ANY)              ) },
  { L_HELP,           TOKEN( Y_HELP,              AC_LANG(ANY)              ) },
  { L_IMAGINARY,      C_SYN( true,                AC_LANG(C_MIN(99)),
                        { LANG_C_MIN(99), L__IMAGINARY },
                        { LANG_ANY,       L_IMAGINARY  }                    ) },
  { L_INIT,           TOKEN( Y_INITIALIZATION,    AC_LANG(NONE)             ) },
  { L_INITIALIZATION, TOKEN( Y_INITIALIZATION,    AC_LANG(CPP_MIN(20))      ) },
  { L_INTEGER,        C_SYE( L_INT,               AC_LANG(NONE)             ) },
  { L_INTO,           TOKEN( Y_INTO,              AC_LANG(ANY)              ) },
  { L_LEN,            TOKEN( Y_LENGTH,            AC_LANG(NONE)             ) },
  { L_LENGTH,         TOKEN( Y_LENGTH,            AC_LANG(C_MIN(99))        ) },
  { L_LINKAGE,        TOKEN( Y_LINKAGE,           AC_LANG(CPP_ANY)          ) },
  { L_LITERAL,        TOKEN( Y_LITERAL,           AC_LANG(CPP_MIN(11))      ) },
  { L_LOCAL,          TOKEN( Y_LOCAL,             AC_LANG(C_CPP_MIN(11,11)) ) },
  { L_MAYBE,          TOKEN( Y_MAYBE,             AC_LANG(NONE)             ) },
  { H_MAYBE_UNUSED,   C_SYE( L_MAYBE_UNUSED,      AC_LANG(NONE)             ) },
  { L_MBR,            TOKEN( Y_MEMBER,            AC_LANG(NONE)             ) },
  { L_MEMBER,         TOKEN( Y_MEMBER,            AC_LANG(CPP_ANY)          ) },
  { L_NO,             TOKEN( Y_NO,                AC_LANG(NONE)             ) },
  { H_NO_DISCARD,     C_SYE( L_NODISCARD,         AC_LANG(NONE)             ) },
  { H_NO_EXCEPT,      C_SYE( L_NOEXCEPT,          AC_LANG(NONE)             ) },
  { H_NO_EXCEPTION,   C_SYE( L_NOEXCEPT,          AC_LANG(NONE)             ) },
  { H_NO_RETURN,      C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_MIN(11), L__NORETURN },
                        { LANG_ANY,       L_NORETURN  }                     ) },
  { H_NON_DISCARDABLE,
                      C_SYE( L_NODISCARD,         AC_LANG(NONE)             ) },
  { H_NON_MBR,        TOKEN( Y_NON_MEMBER,        AC_LANG(NONE)             ) },
  { H_NON_MEMBER,     TOKEN( Y_NON_MEMBER,        AC_LANG(CPP_ANY)          ) },
  { H_NON_RETURNING,  C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_MIN(11), L__NORETURN },
                        { LANG_ANY,       L_NORETURN  }                     ) },
  { H_NON_THROWING,   C_SYE( L_THROW,             AC_LANG(CPP_ANY)          ) },
  { H_NO_UNIQUE_ADDRESS,
                      C_SYE( L_NO_UNIQUE_ADDRESS, AC_LANG(NONE)             ) },
  { H_NON_UNIQUE_ADDRESS,
                      C_SYE( L_NO_UNIQUE_ADDRESS, AC_LANG(NONE)             ) },
  { L_NORETURN,       C_SYN( true,                AC_LANG(C_CPP_MIN(11,11)),
                        { LANG_C_MIN(11), L__NORETURN },
                        { LANG_ANY,       L_NORETURN  }                     ) },
  { L_OF,             TOKEN( Y_OF,                AC_LANG(NONE)             ) },
  { L_OPER,           TOKEN( Y_OPERATOR,          AC_LANG(NONE)             ) },
  { L_OPTIONS,        TOKEN( Y_OPTIONS,           AC_LANG(ANY)              ) },
  { L_OVERRIDDEN,     C_SYE( L_OVERRIDE,          AC_LANG(NONE)             ) },
  { L_POINTER,        TOKEN( Y_POINTER,           AC_LANG(NONE)             ) },
  { L_PREDEF,         TOKEN( Y_PREDEFINED,        AC_LANG(NONE)             ) },
  { L_PREDEFINED,     TOKEN( Y_PREDEFINED,        AC_LANG(ANY)              ) },
  { L_PTR,            TOKEN( Y_POINTER,           AC_LANG(NONE)             ) },
  { L_PURE,           TOKEN( Y_PURE,              AC_LANG(CPP_ANY)          ) },
  { L_Q,              TOKEN( Y_QUIT,              AC_LANG(NONE)             ) },
  { L_QUIT,           TOKEN( Y_QUIT,              AC_LANG(ANY)              ) },
  { L_REF,            TOKEN( Y_REFERENCE,         AC_LANG(NONE)             ) },
  { L_REFERENCE,      TOKEN( Y_REFERENCE,         AC_LANG(CPP_ANY)          ) },
  { L_REINTERPRET,    TOKEN( Y_REINTERPRET,       AC_LANG(CPP_ANY)          ) },
  { L_RESTRICTED,     C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_CPP_MAX(95,NEW), L_GNU___RESTRICT },
                        { LANG_ANY,               L_RESTRICT }              ) },
  { L_RET,            TOKEN( Y_RETURNING,         AC_LANG(NONE)             ) },
  { L_RETURNING,      TOKEN( Y_RETURNING,         AC_LANG(ANY)              ) },
  { L_RVALUE,         TOKEN( Y_RVALUE,            AC_LANG(CPP_MIN(11))      ) },
  { L_SCOPE,          TOKEN( Y_SCOPE,             AC_LANG(ANY)              ) },
  { L_SET_COMMAND,    TOKEN( Y_SET,               AC_LANG(ANY)              ) },
  { L_SHOW,           TOKEN( Y_SHOW,              AC_LANG(ANY)              ) },
  { L_STRUCTURE,      C_SYE( L_STRUCT,            AC_LANG(NONE)             ) },
  { L_THREAD_LOCAL,   C_SYN( true,                AC_LANG(C_CPP_MIN(11,11)),
                        { LANG_C_MIN(11), L__THREAD_LOCAL },
                        { LANG_ANY,       L_THREAD_LOCAL  }                 ) },
  { L_THREAD,         TOKEN( Y_THREAD,            AC_LANG(NONE)             ) },
  { H_THREAD_LOCAL,   C_SYN( true, AC_LANG(NONE),
                        { LANG_C_CPP_MAX(99,03), L_GNU___THREAD  },
                        { LANG_C_MIN(11),        L__THREAD_LOCAL },
                        { LANG_ANY,              L_THREAD_LOCAL  }          ) },
  { L_TO,             TOKEN( Y_TO,                AC_LANG(NONE)             ) },
  { L_TYPE,           C_SYE( L_TYPEDEF,           AC_LANG(NONE)             ) },
  { L_UNIQUE,         TOKEN( Y_UNIQUE,            AC_LANG(CPP_MIN(20))      ) },
  { L_UNUSED,         TOKEN( Y_UNUSED,            AC_LANG(C_CPP_MIN(2X,17)) ) },
  { L_USER,           TOKEN( Y_USER,              AC_LANG(NONE)             ) },
  { H_USER_DEF,       TOKEN( Y_USER_DEFINED,      AC_LANG(NONE)             ) },
  { H_USER_DEFINED,   TOKEN( Y_USER_DEFINED,      AC_LANG(CPP_ANY)          ) },
  { L_VAR,            TOKEN( Y_VARIABLE,          AC_LANG(NONE)             ) },
  { L_VARARGS,        TOKEN( Y_ELLIPSIS,          AC_LANG(NONE)             ) },
  { L_VARIABLE,       TOKEN( Y_VARIABLE,          AC_LANG(C_MIN(99))        ) },
  { L_VARIADIC,       TOKEN( Y_ELLIPSIS,          AC_LANG(MIN(C_89))        ) },
  { L_VECTOR,         TOKEN( Y_ARRAY,             AC_LANG(ANY)              ) },
  { L_VOLATILE,       C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_KNR, L_GNU___VOLATILE },
                        { LANG_ANY,   L_VOLATILE       }                    ) },
  { L_WIDTH,          TOKEN( Y_WIDTH,             AC_LANG(ANY)              ) },

  // Embedded C extensions
  { L_EMC_ACCUM,      C_SYN( true,                AC_LANG(C_99),
                        { LANG_C_99, L_EMC__ACCUM },
                        { LANG_ANY,  NULL         }                         ) },
  { L_EMC_FRACT,      C_SYN( true,                AC_LANG(C_99),
                        { LANG_C_99, L_EMC__FRACT },
                        { LANG_ANY,  NULL         }                         ) },
  { L_EMC_SAT,        C_SYN( false,               AC_LANG(NONE),
                        { LANG_C_99, L_EMC__SAT   },
                        { LANG_ANY,  NULL         }                         ) },
  { L_EMC_SATURATED,  C_SYN( true,                AC_LANG(NONE),
                        { LANG_C_99, L_EMC__SAT   },
                        { LANG_ANY,  NULL         }                         ) },

  // Microsoft extensions
  { L_MSC_CDECL,      C_SYE( L_MSC___CDECL,       AC_LANG(MIN(C_89))        ) },
  { L_MSC_CLRCALL,    C_SYE( L_MSC___CLRCALL,     AC_LANG(MIN(C_89))        ) },
  { L_MSC_FASTCALL,   C_SYE( L_MSC___FASTCALL,    AC_LANG(MIN(C_89))        ) },
  { L_MSC_STDCALL,    C_SYE( L_MSC___STDCALL,     AC_LANG(MIN(C_89))        ) },
  { L_MSC_THISCALL,   C_SYE( L_MSC___THISCALL,    AC_LANG(MIN(C_89))        ) },
  { L_MSC_VECTORCALL, C_SYE( L_MSC___VECTORCALL,  AC_LANG(MIN(C_89))        ) },
  { L_MSC_WINAPI,     C_SYA( L_MSC___STDCALL,     AC_LANG(MIN(C_89))        ) },

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
