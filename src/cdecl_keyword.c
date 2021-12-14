/*
**      cdecl -- C gibberish translator
**      src/cdeck_keyword.cpp
**
**      Copyright (C) 2021  Paul J. Lucas
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
 * @param ... Array of `c_lang_lit`.
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define C_SYN(ALWAYS_FIND,...) \
  (ALWAYS_FIND), 0, (c_lang_lit_t const[]){ __VA_ARGS__ }

/**
 * Special-case of #C_SYN when there is only one language(s)/literal pair.
 *
 * @param ALWAYS_FIND If `true`, always find this synonym, even when explaining
 * C/C++.
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define C_SY1(ALWAYS_FIND,C_KEYWORD) \
  C_SYN( (ALWAYS_FIND), { LANG_ANY, C_KEYWORD } )

/**
 * Specify that the previosuly given literal maps to Bison tokan \a Y_ID.
 *
 * @param Y_ID The Bison token ID (`Y_xxx`).
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define TOKEN(Y_ID)               false, (Y_ID), NULL

/**
 * All cdecl keywords that are (mostly) _not_ C/C++ keywords.
 *
 * To have a literal for a cdecl keyword map to its corresponding token, use
 * #TOKEN:
 *
 *      // The "aligned" literal maps to the Y_ALIGNED token:
 *      { L_ALIGNED,        TOKEN( Y_ALIGNED            ) }
 *
 * To have a literal that is a synonym for another literal for a cdecl keyword
 * map to the other literal's same token, use #TOKEN with the other literal's
 * token:
 *
 *      // The "align" literal synonym also maps to the Y_ALIGNED token:
 *      { L_ALIGN,          TOKEN( Y_ALIGNED            ) },
 *
 * To have a literal that is pseudo-English be a synonym for exactly one
 * corresponding C/C++ keyword literal, but only when converting pseudo-English
 * to gibberish, use #C_SY1 with `false`:
 *
 *      // The "atomic" literal is a synonym for the "_Atomic" literal, but
 *      // only when converting from pseudo-English to gibberish:
 *      { L_ATOMIC,         C_SY1( false, L__ATOMIC     ) },
 *
 * To do the same, but allow the literal at any time (i.e., also when
 * converting gibberish to pseudo-English), use #C_SY1 with `true`:
 *
 *      // The "imaginary" literal is always a synonym for the "_Imaginary"
 *      // literal.
 *      { L_IMAGINARY,      C_SY1( true,  L__IMAGINARY  ) },
 *
 * To have a literal that is pseudo-English be a synonym for more than one
 * corresponding C/C++ keyword depending on the current language, use #C_SYN
 * with the last row always containing #LANG_ANY:
 *
 *      // The "noreturn" literal is a synonym for the "_Noreturn" literal only
 *      // in C11 and later.
 *      { L_NORETURN,       C_SYN( true,
 *                            { { LANG_C_MIN(11), L__NORETURN },
 *                              { LANG_ANY,       L_NORETURN  } } ) },
 *
 * Exceptions are `bool`, `complex`, `const`, and `volatile` that are included
 * here as cdecl keywords so each maps to its language-specific literal.
 *
 * @sa AC_CDECL_KEYWORDS
 * @sa CDECL_COMMANDS
 * @sa C_KEYWORDS
 */
static cdecl_keyword_t const CDECL_KEYWORDS[] = {
  { L_ADDRESS,        TOKEN( Y_ADDRESS                    ) },
  { L_ALIGN,          TOKEN( Y_ALIGNED                    ) },
  { L_ALIGNED,        TOKEN( Y_ALIGNED                    ) },
  { L_ALL,            TOKEN( Y_ALL                        ) },
  { L_APPLE_BLOCK,    TOKEN( Y_APPLE_BLOCK                ) },
  { L_ARRAY,          TOKEN( Y_ARRAY                      ) },
  { L_AS,             TOKEN( Y_AS                         ) },
  { L_ATOMIC,         C_SY1( false, L__ATOMIC             ) },
  { L_AUTOMATIC,      C_SY1( false, L_AUTO                ) },
  { L_BITS,           TOKEN( Y_BITS                       ) },
  { L_BOOL,           C_SYN( true,
                        { LANG_C_MIN(99), L__BOOL },
                        { LANG_ANY,       L_BOOL  } ) },
  { L_BYTES,          TOKEN( Y_BYTES                      ) },
  { L_CARRIES,        TOKEN( Y_CARRIES                    ) },
  { H_CARRIES_DEPENDENCY,
                      C_SY1( false, L_CARRIES_DEPENDENCY  ) },
  { L_CAST,           TOKEN( Y_CAST                       ) },
  { L_CHARACTER,      C_SY1( false, L_CHAR                ) },
  { L_COMPLEX,        C_SYN( true,
                        { LANG_C_MAX(95), L_GNU___COMPLEX },
                        { LANG_C_MIN(99), L__COMPLEX      },
                        { LANG_ANY,       L_COMPLEX       } ) },
  { L_COMMAND,        TOKEN( Y_COMMANDS                   ) },
  { L_COMMANDS,       TOKEN( Y_COMMANDS                   ) },
  { L_CONST,          C_SYN( false,
                        { LANG_C_KNR, L_GNU___CONST },
                        { LANG_ANY,   L_CONST       } ) },
  { L_CONSTANT,       C_SYN( false,
                        { LANG_C_KNR, L_GNU___CONST },
                        { LANG_ANY,   L_CONST       } ) },
  { L_CONSTRUCTOR,    TOKEN( Y_CONSTRUCTOR                ) },
  { L_CONV,           TOKEN( Y_CONVERSION                 ) },
  { L_CONVERSION,     TOKEN( Y_CONVERSION                 ) },
  { L_CTOR,           TOKEN( Y_CONSTRUCTOR                ) },
  { L_DECLARE,        TOKEN( Y_DECLARE                    ) },
  { L_DEFAULTED,      C_SY1( false, L_DEFAULT             ) },
  { L_DEFINE,         TOKEN( Y_DEFINE                     ) },
  { L_DELETED,        C_SY1( false, L_DELETE              ) },
  { L_DEPENDENCY,     TOKEN( Y_DEPENDENCY                 ) },
  { L_DESTRUCTOR,     TOKEN( Y_DESTRUCTOR                 ) },
  { L_DISCARD,        TOKEN( Y_DISCARD                    ) },
  { H_DOUBLE_PRECISION,
                      C_SY1( false, L_DOUBLE              ) },
  { L_DTOR,           TOKEN( Y_DESTRUCTOR                 ) },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC                    ) },
  { L_ENGLISH,        TOKEN( Y_ENGLISH                    ) },
  { L_ENUMERATION,    C_SY1( false, L_ENUM                ) },
  { L_EVAL,           TOKEN( Y_EVALUATION                 ) },
  { L_EVALUATION,     TOKEN( Y_EVALUATION                 ) },
  { L_EXCEPT,         TOKEN( Y_EXCEPT                     ) },
  { L_EXIT,           TOKEN( Y_QUIT                       ) },
  { L_EXPLAIN,        TOKEN( Y_EXPLAIN                    ) },
  { L_EXPORTED,       C_SY1( false, L_EXPORT              ) },
  { L_EXPR,           TOKEN( Y_EXPRESSION                 ) },
  { L_EXPRESSION,     TOKEN( Y_EXPRESSION                 ) },
  { L_EXTERNAL,       C_SY1( false, L_EXTERN              ) },
  { H_FLOATING_POINT, C_SY1( false, L_FLOAT               ) },
  { L_FUNC,           TOKEN( Y_FUNCTION                   ) },
  { L_FUNCTION,       TOKEN( Y_FUNCTION                   ) },
  { L_HELP,           TOKEN( Y_HELP                       ) },
  { L_IMAGINARY,      C_SYN( true,
                        { LANG_C_MIN(99), L__IMAGINARY },
                        { LANG_ANY,       L_IMAGINARY  } ) },
  { L_INIT,           TOKEN( Y_INITIALIZATION             ) },
  { L_INITIALIZATION, TOKEN( Y_INITIALIZATION             ) },
  { L_INTEGER,        C_SY1( false, L_INT                 ) },
  { L_INTO,           TOKEN( Y_INTO                       ) },
  { L_LEN,            TOKEN( Y_LENGTH                     ) },
  { L_LENGTH,         TOKEN( Y_LENGTH                     ) },
  { L_LINKAGE,        TOKEN( Y_LINKAGE                    ) },
  { L_LITERAL,        TOKEN( Y_LITERAL                    ) },
  { L_LOCAL,          TOKEN( Y_LOCAL                      ) },
  { L_MAYBE,          TOKEN( Y_MAYBE                      ) },
  { H_MAYBE_UNUSED,   C_SY1( false, L_MAYBE_UNUSED        ) },
  { L_MBR,            TOKEN( Y_MEMBER                     ) },
  { L_MEMBER,         TOKEN( Y_MEMBER                     ) },
  { L_NO,             TOKEN( Y_NO                         ) },
  { H_NO_DISCARD,     C_SY1( false, L_NODISCARD           ) },
  { H_NO_EXCEPT,      C_SY1( false, L_NOEXCEPT            ) },
  { H_NO_EXCEPTION,   C_SY1( false, L_NOEXCEPT            ) },
  { H_NO_RETURN,      C_SYN( false,
                        { LANG_C_MIN(11), L__NORETURN },
                        { LANG_ANY,       L_NORETURN  } ) },
  { H_NON_DISCARDABLE,
                      C_SY1( false, L_NODISCARD           ) },
  { H_NON_MBR,        TOKEN( Y_NON_MEMBER                 ) },
  { H_NON_MEMBER,     TOKEN( Y_NON_MEMBER                 ) },
  { H_NON_RETURNING,  C_SYN( false,
                        { LANG_C_MIN(11), L__NORETURN },
                        { LANG_ANY,       L_NORETURN  } ) },
  { H_NON_THROWING,   C_SY1( false, L_THROW               ) },
  { H_NO_UNIQUE_ADDRESS,
                      C_SY1( false, L_NO_UNIQUE_ADDRESS   ) },
  { H_NON_UNIQUE_ADDRESS,
                      C_SY1( false, L_NO_UNIQUE_ADDRESS   ) },
  { L_NORETURN,       C_SYN( true,
                        { LANG_C_MIN(11), L__NORETURN },
                        { LANG_ANY,       L_NORETURN  } ) },
  { L_OF,             TOKEN( Y_OF                         ) },
  { L_OPER,           TOKEN( Y_OPERATOR                   ) },
  { L_OPTIONS,        TOKEN( Y_OPTIONS                    ) },
  { L_OVERRIDDEN,     C_SY1( false, L_OVERRIDE            ) },
  { L_POINTER,        TOKEN( Y_POINTER                    ) },
  { L_PREDEF,         TOKEN( Y_PREDEFINED                 ) },
  { L_PREDEFINED,     TOKEN( Y_PREDEFINED                 ) },
  { L_PTR,            TOKEN( Y_POINTER                    ) },
  { L_PURE,           TOKEN( Y_PURE                       ) },
  { L_Q,              TOKEN( Y_QUIT                       ) },
  { L_QUIT,           TOKEN( Y_QUIT                       ) },
  { L_REF,            TOKEN( Y_REFERENCE                  ) },
  { L_REFERENCE,      TOKEN( Y_REFERENCE                  ) },
  { L_REINTERPRET,    TOKEN( Y_REINTERPRET                ) },
  { L_RESTRICTED,     C_SYN( false,
                        { LANG_C_MAX(95) | LANG_CPP_ANY, L_GNU___RESTRICT },
                        { LANG_ANY,                      L_RESTRICT } ) },
  { L_RET,            TOKEN( Y_RETURNING                  ) },
  { L_RETURNING,      TOKEN( Y_RETURNING                  ) },
  { L_RVALUE,         TOKEN( Y_RVALUE                     ) },
  { L_SCOPE,          TOKEN( Y_SCOPE                      ) },
  { L_SET_COMMAND,    TOKEN( Y_SET                        ) },
  { L_SHOW,           TOKEN( Y_SHOW                       ) },
  { L_STRUCTURE,      C_SY1( false, L_STRUCT              ) },
  { L_THREAD_LOCAL,   C_SYN( true,
                        { LANG_C_MIN(11), L__THREAD_LOCAL },
                        { LANG_ANY,       L_THREAD_LOCAL  } ) },
  { L_THREAD,         TOKEN( Y_THREAD                     ) },
  { H_THREAD_LOCAL,   C_SYN( true,
                        { LANG_C_CPP_MAX(99,03), L_GNU___THREAD  },
                        { LANG_C_MIN(11),        L__THREAD_LOCAL },
                        { LANG_ANY,              L_THREAD_LOCAL  } ) },
  { L_TO,             TOKEN( Y_TO                         ) },
  { L_TYPE,           C_SY1( false, L_TYPEDEF             ) },
  { L_UNIQUE,         TOKEN( Y_UNIQUE                     ) },
  { L_UNUSED,         TOKEN( Y_UNUSED                     ) },
  { L_USER,           TOKEN( Y_USER                       ) },
  { H_USER_DEF,       TOKEN( Y_USER_DEFINED               ) },
  { H_USER_DEFINED,   TOKEN( Y_USER_DEFINED               ) },
  { L_VAR,            TOKEN( Y_VARIABLE                   ) },
  { L_VARARGS,        TOKEN( Y_ELLIPSIS                   ) },
  { L_VARIABLE,       TOKEN( Y_VARIABLE                   ) },
  { L_VARIADIC,       TOKEN( Y_ELLIPSIS                   ) },
  { L_VECTOR,         TOKEN( Y_ARRAY                      ) },
  { L_VOLATILE,       C_SYN( false,
                        { LANG_C_KNR, L_GNU___VOLATILE },
                        { LANG_ANY,   L_VOLATILE       } ) },
  { L_WIDTH,          TOKEN( Y_WIDTH                      ) },

  // Embedded C extensions
  { L_EMC_ACCUM,      C_SYN( true,
                        { LANG_C_99, L_EMC__ACCUM },
                        { LANG_ANY,  NULL         } ) },
  { L_EMC_FRACT,      C_SYN( true,
                        { LANG_C_99, L_EMC__FRACT },
                        { LANG_ANY,  NULL         } ) },
  { L_EMC_SAT,        C_SYN( false,
                        { LANG_C_99, L_EMC__SAT   },
                        { LANG_ANY,  NULL         } ) },
  { L_EMC_SATURATED,  C_SYN( true,
                        { LANG_C_99, L_EMC__SAT   },
                        { LANG_ANY,  NULL         } ) },

  // Microsoft extensions
  { L_MSC_CDECL,      C_SY1( false, L_MSC___CDECL         ) },
  { L_MSC_CLRCALL,    C_SY1( false, L_MSC___CLRCALL       ) },
  { L_MSC_FASTCALL,   C_SY1( false, L_MSC___FASTCALL      ) },
  { L_MSC_STDCALL,    C_SY1( false, L_MSC___STDCALL       ) },
  { L_MSC_THISCALL,   C_SY1( false, L_MSC___THISCALL      ) },
  { L_MSC_VECTORCALL, C_SY1( false, L_MSC___VECTORCALL    ) },
  { L_MSC_WINAPI,     C_SY1( true,  L_MSC___STDCALL       ) },

  { NULL,             TOKEN( 0                            ) }
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
