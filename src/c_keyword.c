/*
**      cdecl -- C gibberish translator
**      src/c_keyword.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
#include "pjl_config.h"                 /* must go first */
#include "c_keyword.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_lang.h"
#include "c_type.h"
#include "literals.h"
#include "parser.h"                     /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <string.h>

/// @endcond

#define LANG_C_CPP_11_MIN         LANG_C_CPP_MIN(11,11)

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C++ attributes.
 */
static c_keyword_t const C_ATTRIBUTES[] = {
  // C++11
  { L_CARRIES_DEPENDENCY,
    Y_CARRIES_DEPENDENCY, LANG_CPP_MIN(11),       TA_CARRIES_DEPENDENCY },
  { L_NORETURN,
    Y_NORETURN,           LANG_CPP_MIN(11),       TA_NORETURN           },

  // C2X & C++14
  { L_DEPRECATED,
    Y_DEPRECATED,         LANG_C_CPP_MIN(2X,14),  TA_DEPRECATED         },

  // C2X & C++17
  { L_MAYBE_UNUSED,
    Y_MAYBE_UNUSED,       LANG_C_CPP_MIN(2X,17),  TA_MAYBE_UNUSED       },
  { L_NODISCARD,
    Y_NODISCARD,          LANG_C_CPP_MIN(2X,17),  TA_NODISCARD          },

  // C++20                // Not implemented because:
#if 0
  { L_ASSERT,             // + These use arbitrary expressions that require
  { L_ENSURES,            //   being able to parse them -- which is a lot of
  { L_EXPECTS,            //   work for little benefit.

  { L_LIKELY,             // + These are only for statements, not declarations.
  { L_UNLIKELY,           //
#endif
  { L_NO_UNIQUE_ADDRESS,
    Y_NO_UNIQUE_ADDRESS,  LANG_CPP_MIN(20),       TA_NO_UNIQUE_ADDRESS  },

  { NULL,
    0,                    LANG_NONE,              TX_NONE               }
};

/**
 * Array of all C/C++ keywords.
 *
 * @note There are two rows for `auto` since it has two meanings (one as a
 * storage class in C and C++ up to C++03 and the other as an automatically
 * deduced type in C++11 and later).
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,
    Y_AUTO_STORAGE,     LANG_MAX(CPP_03),         TS_AUTO               },
  { L_BREAK,
    Y_BREAK,            LANG_ALL,                 TX_NONE               },
  { L_CASE,
    Y_CASE,             LANG_ALL,                 TX_NONE               },
  { L_CHAR,
    Y_CHAR,             LANG_ALL,                 TB_CHAR               },
  { L_CONTINUE,
    Y_CONTINUE,         LANG_ALL,                 TX_NONE               },
  { L_DEFAULT,
    Y_DEFAULT,          LANG_ALL,                 TS_DEFAULT            },
  { L_DO,
    Y_DO,               LANG_ALL,                 TX_NONE               },
  { L_DOUBLE,
    Y_DOUBLE,           LANG_ALL,                 TB_DOUBLE             },
  { L_ELSE,
    Y_ELSE,             LANG_ALL,                 TX_NONE               },
  { L_EXTERN,
    Y_EXTERN,           LANG_ALL,                 TS_EXTERN             },
  { L_FLOAT,
    Y_FLOAT,            LANG_ALL,                 TB_FLOAT              },
  { L_FOR,
    Y_FOR,              LANG_ALL,                 TX_NONE               },
  { L_GOTO,
    Y_GOTO,             LANG_ALL,                 TX_NONE               },
  { L_IF,
    Y_IF,               LANG_ALL,                 TX_NONE               },
  { L_INT,
    Y_INT,              LANG_ALL,                 TB_INT                },
  { L_LONG,
    Y_LONG,             LANG_ALL,                 TB_LONG               },
  { L_REGISTER,
    Y_REGISTER,         LANG_ALL,                 TS_REGISTER           },
  { L_RETURN,
    Y_RETURN,           LANG_ALL,                 TX_NONE               },
  { L_SHORT,
    Y_SHORT,            LANG_ALL,                 TB_SHORT              },
  { L_SIZEOF,
    Y_SIZEOF,           LANG_ALL,                 TX_NONE               },
  { L_STATIC,
    Y_STATIC,           LANG_ALL,                 TS_STATIC             },
  { L_STRUCT,
    Y_STRUCT,           LANG_ALL,                 TB_STRUCT             },
  { L_SWITCH,
    Y_SWITCH,           LANG_ALL,                 TX_NONE               },
  { L_TYPEDEF,
    Y_TYPEDEF,          LANG_ALL,                 TS_TYPEDEF            },
  { L_UNION,
    Y_UNION,            LANG_ALL,                 TB_UNION              },
  { L_UNSIGNED,
    Y_UNSIGNED,         LANG_ALL,                 TB_UNSIGNED           },
  { L_WHILE,
    Y_WHILE,            LANG_ALL,                 TX_NONE               },

  // C89
  { L_ASM,
    Y_ASM,              LANG_MIN(C_89),           TX_NONE               },
  { L_CONST,
    Y_CONST,            LANG_MIN(C_89),           TS_CONST              },
  { L_ENUM,
    Y_ENUM,             LANG_MIN(C_89),           TB_ENUM               },
  { L_SIGNED,
    Y_SIGNED,           LANG_MIN(C_89),           TB_SIGNED             },
  { L_VOID,
    Y_VOID,             LANG_MIN(C_89),           TB_VOID               },
  { L_VOLATILE,
    Y_VOLATILE,         LANG_MIN(C_89),           TS_VOLATILE           },

  // C99
  { L__BOOL,
    Y__BOOL,            LANG_MIN(C_99),           TB_BOOL               },
  { L__COMPLEX,
    Y__COMPLEX,         LANG_C_MIN(99),           TB_COMPLEX            },
  { L__IMAGINARY,
    Y__IMAGINARY,       LANG_C_MIN(99),           TB_IMAGINARY          },
  { L_INLINE,
    Y_INLINE,           LANG_MIN(C_99),           TS_INLINE             },
  { L_RESTRICT,
    Y_RESTRICT,         LANG_C_MIN(99),           TS_RESTRICT           },
  { L_WCHAR_T,
    Y_WCHAR_T,          LANG_MIN(C_95),           TB_WCHAR_T            },

  // C11
  { L__ALIGNAS,
    Y__ALIGNAS,         LANG_C_MIN(11),           TX_NONE               },
  { L__ALIGNOF,
    Y__ALIGNOF,         LANG_C_MIN(11),           TX_NONE               },
  { L__ATOMIC,
    Y__ATOMIC_QUAL,     LANG_MIN(C_11),           TS_ATOMIC             },
  { L__GENERIC,
    Y__GENERIC,         LANG_C_MIN(11),           TX_NONE               },
  { L__NORETURN,
    Y__NORETURN,        LANG_C_MIN(11),           TA_NORETURN           },
  { L__STATIC_ASSERT,
    Y__STATIC_ASSERT,   LANG_C_MIN(11),           TX_NONE               },
  { L__THREAD_LOCAL,
    Y__THREAD_LOCAL,    LANG_C_MIN(11),           TS_THREAD_LOCAL       },

  // C++
  { L_BOOL,
    Y_BOOL,             LANG_CPP_ALL,             TB_BOOL               },
  { L_CATCH,
    Y_CATCH,            LANG_CPP_ALL,             TX_NONE               },
  { L_CLASS,
    Y_CLASS,            LANG_CPP_ALL,             TB_CLASS              },
  { L_CONST_CAST,
    Y_CONST_CAST,       LANG_CPP_ALL,             TX_NONE               },
  { L_DELETE,
    Y_DELETE,           LANG_CPP_ALL,             TS_DELETE             },
  { L_DYNAMIC_CAST,
    Y_DYNAMIC_CAST,     LANG_CPP_ALL,             TX_NONE               },
  { L_EXPLICIT,
    Y_EXPLICIT,         LANG_CPP_ALL,             TS_EXPLICIT           },
  { L_FALSE,
    Y_FALSE,            LANG_CPP_ALL,             TX_NONE               },
  { L_FRIEND,
    Y_FRIEND,           LANG_CPP_ALL,             TS_FRIEND             },
  { L_MUTABLE,
    Y_MUTABLE,          LANG_CPP_ALL,             TS_MUTABLE            },
  { L_NAMESPACE,
    Y_NAMESPACE,        LANG_CPP_ALL,             TB_NAMESPACE          },
  { L_NEW,
    Y_NEW,              LANG_CPP_ALL,             TX_NONE               },
  { L_OPERATOR,
    Y_OPERATOR,         LANG_CPP_ALL,             TX_NONE               },
  { L_PRIVATE,
    Y_PRIVATE,          LANG_CPP_ALL,             TX_NONE               },
  { L_PROTECTED,
    Y_PROTECTED,        LANG_CPP_ALL,             TX_NONE               },
  { L_PUBLIC,
    Y_PUBLIC,           LANG_CPP_ALL,             TX_NONE               },
  { L_REINTERPRET_CAST,
    Y_REINTERPRET_CAST, LANG_CPP_ALL,             TX_NONE               },
  { L_STATIC_CAST,
    Y_STATIC_CAST,      LANG_CPP_ALL,             TX_NONE               },
  { L_TEMPLATE,
    Y_TEMPLATE,         LANG_CPP_ALL,             TX_NONE               },
  { L_THIS,
    Y_THIS,             LANG_CPP_ALL,             TX_NONE               },
  { L_THROW,
    Y_THROW,            LANG_CPP_ALL,             TS_THROW              },
  { L_TRUE,
    Y_TRUE,             LANG_CPP_ALL,             TS_NOEXCEPT           },
  { L_TRY,
    Y_TRY,              LANG_CPP_ALL,             TX_NONE               },
  { L_TYPEID,
    Y_TYPEID,           LANG_CPP_ALL,             TX_NONE               },
  { L_TYPENAME,
    Y_TYPENAME,         LANG_CPP_ALL,             TX_NONE               },
  { L_USING,
    Y_USING,            LANG_CPP_ALL,             TS_TYPEDEF            },
  { L_VIRTUAL,
    Y_VIRTUAL,          LANG_CPP_ALL,             TS_VIRTUAL            },

  // C++11
  { L_ALIGNAS,
    Y_ALIGNAS,          LANG_CPP_MIN(11),         TX_NONE               },
  { L_ALIGNOF,
    Y_ALIGNOF,          LANG_CPP_MIN(11),         TX_NONE               },
  { L_AUTO,
    Y_AUTO_TYPE,        LANG_CPP_MIN(11),         TB_AUTO               },
  { L_CONSTEXPR,
    Y_CONSTEXPR,        LANG_CPP_MIN(11),         TS_CONSTEXPR          },
  { L_DECLTYPE,
    Y_DECLTYPE,         LANG_CPP_MIN(11),         TX_NONE               },
  { L_FINAL,
    Y_FINAL,            LANG_CPP_MIN(11),         TS_FINAL              },
  { L_NOEXCEPT,
    Y_NOEXCEPT,         LANG_CPP_MIN(11),         TS_NOEXCEPT           },
  { L_NULLPTR,
    Y_NULLPTR,          LANG_CPP_MIN(11),         TX_NONE               },
  { L_OVERRIDE,
    Y_OVERRIDE,         LANG_CPP_MIN(11),         TS_OVERRIDE           },
  { L_STATIC_ASSERT,
    Y_STATIC_ASSERT,    LANG_CPP_MIN(11),         TX_NONE               },
  { L_THREAD_LOCAL,
    Y_THREAD_LOCAL,     LANG_CPP_MIN(11),         TS_THREAD_LOCAL       },

  // C11 & C++11
  { L_CHAR16_T,
    Y_CHAR16_T,         LANG_C_CPP_11_MIN,        TB_CHAR16_T           },
  { L_CHAR32_T,
    Y_CHAR32_T,         LANG_C_CPP_11_MIN,        TB_CHAR32_T           },

  // C2X & C++20
  { L_CHAR8_T,
    Y_CHAR8_T,          LANG_C_CPP_MIN(2X,20),    TB_CHAR8_T            },

  // C++20
  { L_CONCEPT,
    Y_CONCEPT,          LANG_CPP_MIN(20),         TX_NONE               },
  { L_CONSTEVAL,
    Y_CONSTEVAL,        LANG_CPP_MIN(20),         TS_CONSTEVAL          },
  { L_CONSTINIT,
    Y_CONSTINIT,        LANG_CPP_MIN(20),         TS_CONSTINIT          },
  { L_CO_AWAIT,
    Y_CO_AWAIT,         LANG_CPP_MIN(20),         TX_NONE               },
  { L_CO_RETURN,
    Y_CO_RETURN,        LANG_CPP_MIN(20),         TX_NONE               },
  { L_CO_YIELD,
    Y_CO_YIELD,         LANG_CPP_MIN(20),         TX_NONE               },
  { L_EXPORT,
    Y_EXPORT,           LANG_CPP_MIN(20),         TS_EXPORT             },
  { L_REQUIRES,
    Y_REQUIRES,         LANG_CPP_MIN(20),         TX_NONE               },

  // Alternative tokens
  { L_AND,
    Y_AMPER2,           LANG_MIN(C_89),           TX_NONE               },
  { L_AND_EQ,
    Y_AMPER_EQ,         LANG_MIN(C_89),           TX_NONE               },
  { L_BITAND,
    Y_AMPER,            LANG_MIN(C_89),           TX_NONE               },
  { L_BITOR,
    Y_PIPE,             LANG_MIN(C_89),           TX_NONE               },
  { L_COMPL,
    Y_TILDE,            LANG_MIN(C_89),           TX_NONE               },
  { L_NOT,
    Y_EXCLAM,           LANG_MIN(C_89),           TX_NONE               },
  { L_NOT_EQ,
    Y_EXCLAM_EQ,        LANG_MIN(C_89),           TX_NONE               },
  { L_OR,
    Y_PIPE2,            LANG_MIN(C_89),           TX_NONE               },
  { L_OR_EQ,
    Y_PIPE_EQ,          LANG_MIN(C_89),           TX_NONE               },
  { L_XOR,
    Y_CIRC,             LANG_MIN(C_89),           TX_NONE               },
  { L_XOR_EQ,
    Y_CIRC_EQ,          LANG_MIN(C_89),           TX_NONE               },

  // Embedded C extensions
  { L_EMC__ACCUM,
    Y_EMC__ACCUM,       LANG_C_99_EMC,            TB_EMC_ACCUM          },
  { L_EMC__FRACT,
    Y_EMC__FRACT,       LANG_C_99_EMC,            TB_EMC_FRACT          },
  { L_EMC__SAT,
    Y_EMC__SAT,         LANG_C_99_EMC,            TB_EMC_SAT            },

  // Unified Parallel C extensions
  { L_UPC_RELAXED,
    Y_UPC_RELAXED,      LANG_C_99_UPC,            TS_UPC_RELAXED        },
  { L_UPC_SHARED,
    Y_UPC_SHARED,       LANG_C_99_UPC,            TS_UPC_SHARED         },
  { L_UPC_STRICT,
    Y_UPC_STRICT,       LANG_C_99_UPC,            TS_UPC_STRICT         },

  // GNU extensions
  { L_GNU___AUTO_TYPE,
    Y_AUTO_TYPE,        LANG_ALL,                 TB_AUTO               },
  { L_GNU___COMPLEX,
    Y__COMPLEX,         LANG_ALL,                 TB_COMPLEX            },
  { L_GNU___COMPLEX__,
    Y__COMPLEX,         LANG_ALL,                 TB_COMPLEX            },
  { L_GNU___CONST,
    Y_CONST,            LANG_ALL,                 TS_CONST              },
  { L_GNU___INLINE,
    Y_INLINE,           LANG_ALL,                 TS_INLINE             },
  { L_GNU___INLINE__,
    Y_INLINE,           LANG_ALL,                 TS_INLINE             },
  { L_GNU___RESTRICT,
    Y_GNU___RESTRICT,   LANG_ALL,                 TS_RESTRICT           },
  { L_GNU___RESTRICT__,
    Y_GNU___RESTRICT,   LANG_ALL,                 TS_RESTRICT           },
  { L_GNU___SIGNED,
    Y_SIGNED,           LANG_ALL,                 TB_SIGNED             },
  { L_GNU___SIGNED__,
    Y_SIGNED,           LANG_ALL,                 TB_SIGNED             },
  { L_GNU___THREAD,
    Y_THREAD_LOCAL,     LANG_ALL,                 TS_THREAD_LOCAL       },
  { L_GNU___VOLATILE,
    Y_VOLATILE,         LANG_ALL,                 TS_VOLATILE           },
  { L_GNU___VOLATILE__,
    Y_VOLATILE,         LANG_ALL,                 TS_VOLATILE           },

  // Apple extensions
  { L_APPLE___BLOCK,
    Y_APPLE___BLOCK,    LANG_ALL,                 TS_APPLE_BLOCK        },

  { NULL,
    0,                  LANG_NONE,                TX_NONE               }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Given a literal, gets the `c_keyword` for the corresponding C/C++ keyword in
 * \a lang_ids.
 *
 * @param literal The literal to find.
 * @param keywords The array of `c_keyword` to look in.
 * @param lang_ids The bitwise-or of language(s) to look for the keyword in.
 * @return Returns a pointer to the corresponding `c_keyword` or null for none.
 */
PJL_WARN_UNUSED_RESULT
static c_keyword_t const* c_keyword_find_impl( char const *literal,
                                               c_keyword_t const keywords[],
                                               c_lang_id_t lang_ids ) {
  assert( literal != NULL );

  for ( c_keyword_t const *k = keywords; k->literal != NULL; ++k ) {
    if ( (k->lang_ids & lang_ids) == LANG_NONE )
      continue;
    if ( strcmp( literal, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_attribute_find( char const *literal ) {
  return c_keyword_find_impl( literal, C_ATTRIBUTES, LANG_ALL );
}

c_keyword_t const* c_keyword_find( char const *literal, c_lang_id_t lang_id ) {
  return c_keyword_find_impl( literal, C_KEYWORDS, lang_id );
}

c_keyword_t const* c_keyword_next( c_keyword_t const *k ) {
  if ( k == NULL )
    k = C_KEYWORDS;
  else if ( (++k)->literal == NULL )
    k = NULL;
  return k;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
