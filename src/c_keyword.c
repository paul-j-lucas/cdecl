/*
**      cdecl -- C gibberish translator
**      src/c_keyword.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
 * Defines functions for looking up C/C++ keyword information.
 */

// local
#include "config.h"                     /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_type.h"
#include "literals.h"
#include "options.h"
#include "parser.h"                     /* must go last */

// standard
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C/C++ keywords (relevant for declarations).
 *
 * @note The \c ok_langs columns is really only here for the sake of \c auto.
 * Since \c auto has two different meanings (one as a storage class in C and
 * C++ up to C++03, and the other as an automatically deduced type in C++11 and
 * later), there have to be two rows in the table for it.
 *
 * However, there is only one literal, the \c ok_langs column says which
 * language(s) the row is a valid entry for: when searching for a literal
 * match, if the current language is not among \c ok_langs, the current row is
 * rejected and the search continues.
 *
 * The remaining rows are all LANG_ALL rather than the language(s) the keyword
 * is valid in because it results in better error messages by allowing the AST
 * to be fully constructed and semantic errors issued afterwards rather than by
 * the parser that can only give "unexpected token."
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,             Y_AUTO_C,           T_AUTO_C,        LANG_MAX(CPP_03) },
  { L_CHAR,             Y_CHAR,             T_CHAR,          LANG_ALL         },
  { L_DOUBLE,           Y_DOUBLE,           T_DOUBLE,        LANG_ALL         },
  { L_EXTERN,           Y_EXTERN,           T_EXTERN,        LANG_ALL         },
  { L_FLOAT,            Y_FLOAT,            T_FLOAT,         LANG_ALL         },
  { L_INT,              Y_INT,              T_INT,           LANG_ALL         },
  { L_LONG,             Y_LONG,             T_LONG,          LANG_ALL         },
  { L_REGISTER,         Y_REGISTER,         T_REGISTER,      LANG_ALL         },
  { L_SHORT,            Y_SHORT,            T_SHORT,         LANG_ALL         },
  { L_STATIC,           Y_STATIC,           T_STATIC,        LANG_ALL         },
  { L_STRUCT,           Y_STRUCT,           T_STRUCT,        LANG_ALL         },
  { L_TYPEDEF,          Y_TYPEDEF,          T_TYPEDEF,       LANG_ALL         },
  { L_UNION,            Y_UNION,            T_UNION,         LANG_ALL         },
  { L_UNSIGNED,         Y_UNSIGNED,         T_UNSIGNED,      LANG_ALL         },

  // C89
  { L_CONST,            Y_CONST,            T_CONST,         LANG_ALL         },
  { L_ELLIPSIS,         Y_ELLIPSIS,         T_NONE,          LANG_ALL         },
  { L_ENUM,             Y_ENUM,             T_ENUM,          LANG_ALL         },
  { L_SIGNED,           Y_SIGNED,           T_SIGNED,        LANG_ALL         },
  { L_VOID,             Y_VOID,             T_VOID,          LANG_ALL         },
  { L_VOLATILE,         Y_VOLATILE,         T_VOLATILE,      LANG_ALL         },

  // C99
  { L_BOOL,             Y_BOOL,             T_BOOL,          LANG_ALL         },
  { L__COMPLEX,         Y__COMPLEX,         T_COMPLEX,       LANG_ALL         },
  { L__IMAGINARY,       Y__IMAGINARY,       T_IMAGINARY,     LANG_ALL         },
  { L_INLINE,           Y_INLINE,           T_INLINE,        LANG_ALL         },
  { L_RESTRICT,         Y_RESTRICT,         T_RESTRICT,      LANG_ALL         },
  { L_WCHAR_T,          Y_WCHAR_T,          T_WCHAR_T,       LANG_ALL         },

  // C11
  { L__ATOMIC,          Y_ATOMIC_QUAL,      T_ATOMIC,        LANG_ALL         },
  { L__NORETURN,        Y__NORETURN,        T_NORETURN,      LANG_ALL         },

  // C++
  { L_CLASS,            Y_CLASS,            T_CLASS,         LANG_ALL         },
  { L_CONST_CAST,       Y_CONST_CAST,       T_NONE,          LANG_ALL         },
  { L_DYNAMIC_CAST,     Y_DYNAMIC_CAST,     T_NONE,          LANG_ALL         },
  { L_FALSE,            Y_FALSE,            T_NONE,          LANG_ALL         },
  { L_FRIEND,           Y_FRIEND,           T_FRIEND,        LANG_ALL         },
  { L_MUTABLE,          Y_MUTABLE,          T_MUTABLE,       LANG_ALL         },
  { L_REINTERPRET_CAST, Y_REINTERPRET_CAST, T_NONE,          LANG_ALL         },
  { L_STATIC_CAST,      Y_STATIC_CAST,      T_NONE,          LANG_ALL         },
  { L_TRUE,             Y_TRUE,             T_NOEXCEPT,      LANG_ALL         },
  { L_THROW,            Y_THROW,            T_THROW,         LANG_ALL         },
  { L_VIRTUAL,          Y_VIRTUAL,          T_VIRTUAL,       LANG_ALL         },

  // C++11
  { L_AUTO,             Y_AUTO_CPP_11,      T_AUTO_CPP_11,   LANG_MIN(CPP_11) },
  { L_CONSTEXPR,        Y_CONSTEXPR,        T_CONSTEXPR,     LANG_ALL         },
  { L_FINAL,            Y_FINAL,            T_FINAL,         LANG_ALL         },
  { L_NOEXCEPT,         Y_NOEXCEPT,         T_NOEXCEPT,      LANG_ALL         },
  { L_OVERRIDE,         Y_OVERRIDE,         T_OVERRIDE,      LANG_ALL         },

  // C11 & C++11
  { L_CHAR16_T,         Y_CHAR16_T,         T_CHAR16_T,      LANG_ALL         },
  { L_CHAR32_T,         Y_CHAR32_T,         T_CHAR32_T,      LANG_ALL         },
  { L_THREAD_LOCAL,     Y_THREAD_LOCAL,     T_THREAD_LOCAL,  LANG_ALL         },

  // Apple extension
  { L___BLOCK,          Y___BLOCK,          T_BLOCK,         LANG_ALL         },

  { NULL,               0,                  T_NONE,          LANG_NONE        }
};

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_keyword_find( char const *literal ) {
  for ( c_keyword_t const *k = C_KEYWORDS; k->literal != NULL; ++k ) {
    if ( (k->ok_langs & opt_lang) == LANG_NONE )
      continue;
    if ( strcmp( literal, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
