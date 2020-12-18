/*
**      cdecl -- C gibberish translator
**      src/c_keyword.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include "c_typedef.h"
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
                    Y_CARRIES_DEPENDENCY,
                                    TA_CARRIES_DEPENDENCY,
                                                      LANG_CPP_MIN(11)      },
  { L_NORETURN,     Y_NORETURN,     TA_NORETURN,      LANG_CPP_MIN(11)      },

  // C2X & C++14
  { L_DEPRECATED,   Y_DEPRECATED,   TA_DEPRECATED,    LANG_C_CPP_MIN(2X,14) },

  // C2X & C++17
  { L_MAYBE_UNUSED, Y_MAYBE_UNUSED, TA_MAYBE_UNUSED,  LANG_C_CPP_MIN(2X,17) },
  { L_NODISCARD,    Y_NODISCARD,    TA_NODISCARD,     LANG_C_CPP_MIN(2X,17) },

  // C++20                // Not implemented because:
#if 0
  { L_ASSERT,             // + These use arbitrary expressions that require
  { L_ENSURES,            //   being able to parse them -- which is a lot of
  { L_EXPECTS,            //   work for little benefit.

  { L_LIKELY,             // + These are only for statements, not declarations.
  { L_UNLIKELY,           //
#endif
  { L_NO_UNIQUE_ADDRESS,
                    Y_NO_UNIQUE_ADDRESS,
                                    TA_NO_UNIQUE_ADDRESS,
                                                    LANG_CPP_MIN(20)      },

  { NULL,           0,              TX_NONE,       LANG_NONE             }
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
  { L_AUTO,             Y_AUTO_STORAGE,     TS_AUTO,        LANG_MAX(CPP_03)  },
  { L_BREAK,            Y_BREAK,            TX_NONE,        LANG_ALL          },
  { L_CASE,             Y_CASE,             TX_NONE,        LANG_ALL          },
  { L_CHAR,             Y_CHAR,             TB_CHAR,        LANG_ALL          },
  { L_CONTINUE,         Y_CONTINUE,         TX_NONE,        LANG_ALL          },
  { L_DEFAULT,          Y_DEFAULT,          TS_DEFAULT,     LANG_ALL          },
  { L_DO,               Y_DO,               TX_NONE,        LANG_ALL          },
  { L_DOUBLE,           Y_DOUBLE,           TB_DOUBLE,      LANG_ALL          },
  { L_ELSE,             Y_ELSE,             TX_NONE,        LANG_ALL          },
  { L_EXTERN,           Y_EXTERN,           TS_EXTERN,      LANG_ALL          },
  { L_FLOAT,            Y_FLOAT,            TB_FLOAT,       LANG_ALL          },
  { L_FOR,              Y_FOR,              TX_NONE,        LANG_ALL          },
  { L_GOTO,             Y_GOTO,             TX_NONE,        LANG_ALL          },
  { L_IF,               Y_IF,               TX_NONE,        LANG_ALL          },
  { L_INT,              Y_INT,              TB_INT,         LANG_ALL          },
  { L_LONG,             Y_LONG,             TB_LONG,        LANG_ALL          },
  { L_REGISTER,         Y_REGISTER,         TS_REGISTER,    LANG_ALL          },
  { L_RETURN,           Y_RETURN,           TX_NONE,        LANG_ALL          },
  { L_SHORT,            Y_SHORT,            TB_SHORT,       LANG_ALL          },
  { L_SIZEOF,           Y_SIZEOF,           TX_NONE,        LANG_ALL          },
  { L_STATIC,           Y_STATIC,           TS_STATIC,      LANG_ALL          },
  { L_STRUCT,           Y_STRUCT,           TB_STRUCT,      LANG_ALL          },
  { L_SWITCH,           Y_SWITCH,           TX_NONE,        LANG_ALL          },
  { L_TYPEDEF,          Y_TYPEDEF,          TS_TYPEDEF,     LANG_ALL          },
  { L_UNION,            Y_UNION,            TB_UNION,       LANG_ALL          },
  { L_UNSIGNED,         Y_UNSIGNED,         TB_UNSIGNED,    LANG_ALL          },
  { L_WHILE,            Y_WHILE,            TX_NONE,        LANG_ALL          },

  // C89
  { L_ASM,              Y_ASM,              TX_NONE,        LANG_MIN(C_89)    },
  { L_CONST,            Y_CONST,            TS_CONST,       LANG_MIN(C_89)    },
  { L_ENUM,             Y_ENUM,             TB_ENUM,        LANG_MIN(C_89)    },
  { L_SIGNED,           Y_SIGNED,           TB_SIGNED,      LANG_MIN(C_89)    },
  { L_VOID,             Y_VOID,             TB_VOID,        LANG_MIN(C_89)    },
  { L_VOLATILE,         Y_VOLATILE,         TS_VOLATILE,    LANG_MIN(C_89)    },

  // C99
  { L__BOOL,            Y__BOOL,            TB_BOOL,        LANG_MIN(C_99)    },
  { L__COMPLEX,         Y__COMPLEX,         TB_COMPLEX,     LANG_C_MIN(99)    },
  { L__IMAGINARY,       Y__IMAGINARY,       TB_IMAGINARY,   LANG_C_MIN(99)    },
  { L_INLINE,           Y_INLINE,           TS_INLINE,      LANG_MIN(C_99)    },
  { L_RESTRICT,         Y_RESTRICT,         TS_RESTRICT,    LANG_C_MIN(99)    },
  { L_WCHAR_T,          Y_WCHAR_T,          TB_WCHAR_T,     LANG_MIN(C_95)    },

  // C11
  { L__ALIGNAS,         Y__ALIGNAS,         TX_NONE,        LANG_C_MIN(11)    },
  { L__ALIGNOF,         Y__ALIGNOF,         TX_NONE,        LANG_C_MIN(11)    },
  { L__ATOMIC,          Y__ATOMIC_QUAL,     TS_ATOMIC,      LANG_MIN(C_11)    },
  { L__GENERIC,         Y__GENERIC,         TX_NONE,        LANG_C_MIN(11)    },
  { L__NORETURN,        Y__NORETURN,        TA_NORETURN,    LANG_C_MIN(11)    },
  { L__STATIC_ASSERT,   Y__STATIC_ASSERT,   TX_NONE,        LANG_C_MIN(11)    },
  { L__THREAD_LOCAL,    Y__THREAD_LOCAL,    TS_THREAD_LOCAL,LANG_C_MIN(11)    },

  // C++
  { L_BOOL,             Y_BOOL,             TB_BOOL,        LANG_CPP_ALL      },
  { L_CATCH,            Y_CATCH,            TX_NONE,        LANG_CPP_ALL      },
  { L_CLASS,            Y_CLASS,            TB_CLASS,       LANG_CPP_ALL      },
  { L_CONST_CAST,       Y_CONST_CAST,       TX_NONE,        LANG_CPP_ALL      },
  { L_DELETE,           Y_DELETE,           TS_DELETE,      LANG_CPP_ALL      },
  { L_DYNAMIC_CAST,     Y_DYNAMIC_CAST,     TX_NONE,        LANG_CPP_ALL      },
  { L_EXPLICIT,         Y_EXPLICIT,         TS_EXPLICIT,    LANG_CPP_ALL      },
  { L_FALSE,            Y_FALSE,            TX_NONE,        LANG_CPP_ALL      },
  { L_FRIEND,           Y_FRIEND,           TS_FRIEND,      LANG_CPP_ALL      },
  { L_MUTABLE,          Y_MUTABLE,          TS_MUTABLE,     LANG_CPP_ALL      },
  { L_NAMESPACE,        Y_NAMESPACE,        TB_NAMESPACE,   LANG_CPP_ALL      },
  { L_NEW,              Y_NEW,              TX_NONE,        LANG_CPP_ALL      },
  { L_OPERATOR,         Y_OPERATOR,         TX_NONE,        LANG_CPP_ALL      },
  { L_PRIVATE,          Y_PRIVATE,          TX_NONE,        LANG_CPP_ALL      },
  { L_PROTECTED,        Y_PROTECTED,        TX_NONE,        LANG_CPP_ALL      },
  { L_PUBLIC,           Y_PUBLIC,           TX_NONE,        LANG_CPP_ALL      },
  { L_REINTERPRET_CAST, Y_REINTERPRET_CAST, TX_NONE,        LANG_CPP_ALL      },
  { L_STATIC_CAST,      Y_STATIC_CAST,      TX_NONE,        LANG_CPP_ALL      },
  { L_TEMPLATE,         Y_TEMPLATE,         TX_NONE,        LANG_CPP_ALL      },
  { L_THIS,             Y_THIS,             TX_NONE,        LANG_CPP_ALL      },
  { L_THROW,            Y_THROW,            TS_THROW,       LANG_CPP_ALL      },
  { L_TRUE,             Y_TRUE,             TS_NOEXCEPT,    LANG_CPP_ALL      },
  { L_TRY,              Y_TRY,              TX_NONE,        LANG_CPP_ALL      },
  { L_TYPEID,           Y_TYPEID,           TX_NONE,        LANG_CPP_ALL      },
  { L_TYPENAME,         Y_TYPENAME,         TX_NONE,        LANG_CPP_ALL      },
  { L_USING,            Y_USING,            TS_TYPEDEF,     LANG_CPP_ALL      },
  { L_VIRTUAL,          Y_VIRTUAL,          TS_VIRTUAL,     LANG_CPP_ALL      },

  // C++11
  { L_ALIGNAS,          Y_ALIGNAS,          TX_NONE,        LANG_CPP_MIN(11)  },
  { L_ALIGNOF,          Y_ALIGNOF,          TX_NONE,        LANG_CPP_MIN(11)  },
  { L_AUTO,             Y_AUTO_TYPE,        TB_AUTO,        LANG_CPP_MIN(11)  },
  { L_CONSTEXPR,        Y_CONSTEXPR,        TS_CONSTEXPR,   LANG_CPP_MIN(11)  },
  { L_DECLTYPE,         Y_DECLTYPE,         TX_NONE,        LANG_CPP_MIN(11)  },
  { L_FINAL,            Y_FINAL,            TS_FINAL,       LANG_CPP_MIN(11)  },
  { L_NOEXCEPT,         Y_NOEXCEPT,         TS_NOEXCEPT,    LANG_CPP_MIN(11)  },
  { L_NULLPTR,          Y_NULLPTR,          TX_NONE,        LANG_CPP_MIN(11)  },
  { L_OVERRIDE,         Y_OVERRIDE,         TS_OVERRIDE,    LANG_CPP_MIN(11)  },
  { L_STATIC_ASSERT,    Y_STATIC_ASSERT,    TX_NONE,        LANG_CPP_MIN(11)  },
  { L_THREAD_LOCAL,     Y_THREAD_LOCAL,     TS_THREAD_LOCAL,LANG_CPP_MIN(11)  },

  // C11 & C++11
  { L_CHAR16_T,         Y_CHAR16_T,         TB_CHAR16_T,    LANG_C_CPP_11_MIN },
  { L_CHAR32_T,         Y_CHAR32_T,         TB_CHAR32_T,    LANG_C_CPP_11_MIN },

  // C2X & C++20
  { L_CHAR8_T,          Y_CHAR8_T,          TB_CHAR8_T, LANG_C_CPP_MIN(2X,20) },

  // C++20
  { L_CONCEPT,          Y_CONCEPT,          TX_NONE,        LANG_CPP_MIN(20)  },
  { L_CONSTEVAL,        Y_CONSTEVAL,        TS_CONSTEVAL,   LANG_CPP_MIN(20)  },
  { L_CONSTINIT,        Y_CONSTINIT,        TS_CONSTINIT,   LANG_CPP_MIN(20)  },
  { L_CO_AWAIT,         Y_CO_AWAIT,         TX_NONE,        LANG_CPP_MIN(20)  },
  { L_CO_RETURN,        Y_CO_RETURN,        TX_NONE,        LANG_CPP_MIN(20)  },
  { L_CO_YIELD,         Y_CO_YIELD,         TX_NONE,        LANG_CPP_MIN(20)  },
  { L_EXPORT,           Y_EXPORT,           TS_EXPORT,      LANG_CPP_MIN(20)  },
  { L_REQUIRES,         Y_REQUIRES,         TX_NONE,        LANG_CPP_MIN(20)  },

  // Alternative tokens
  { L_AND,              Y_AMPER2,           TX_NONE,       LANG_MIN(C_89)     },
  { L_AND_EQ,           Y_AMPER_EQ,         TX_NONE,       LANG_MIN(C_89)     },
  { L_BITAND,           Y_AMPER,            TX_NONE,       LANG_MIN(C_89)     },
  { L_BITOR,            Y_PIPE,             TX_NONE,       LANG_MIN(C_89)     },
  { L_COMPL,            Y_TILDE,            TX_NONE,       LANG_MIN(C_89)     },
  { L_NOT,              Y_EXCLAM,           TX_NONE,       LANG_MIN(C_89)     },
  { L_NOT_EQ,           Y_EXCLAM_EQ,        TX_NONE,       LANG_MIN(C_89)     },
  { L_OR,               Y_PIPE2,            TX_NONE,       LANG_MIN(C_89)     },
  { L_OR_EQ,            Y_PIPE_EQ,          TX_NONE,       LANG_MIN(C_89)     },
  { L_XOR,              Y_CIRC,             TX_NONE,       LANG_MIN(C_89)     },
  { L_XOR_EQ,           Y_CIRC_EQ,          TX_NONE,       LANG_MIN(C_89)     },

  // Embedded C extensions
  { L_EMC__ACCUM,       Y_EMC__ACCUM,       TB_EMC_ACCUM,   LANG_C_99_EMC     },
  { L_EMC__FRACT,       Y_EMC__FRACT,       TB_EMC_FRACT,   LANG_C_99_EMC     },
  { L_EMC__SAT,         Y_EMC__SAT,         TB_EMC_SAT,     LANG_C_99_EMC     },

  // Unified Parallel C extensions
  { L_UPC_RELAXED,      Y_UPC_RELAXED,      TS_UPC_RELAXED, LANG_C_99_UPC     },
  { L_UPC_SHARED,       Y_UPC_SHARED,       TS_UPC_SHARED,  LANG_C_99_UPC     },
  { L_UPC_STRICT,       Y_UPC_STRICT,       TS_UPC_STRICT,  LANG_C_99_UPC     },

  // GNU extensions
  { L_GNU___AUTO_TYPE,  Y_AUTO_TYPE,        TB_AUTO,        LANG_ALL          },
  { L_GNU___COMPLEX,    Y__COMPLEX,         TB_COMPLEX,     LANG_ALL          },
  { L_GNU___COMPLEX__,  Y__COMPLEX,         TB_COMPLEX,     LANG_ALL          },
  { L_GNU___CONST,      Y_CONST,            TS_CONST,       LANG_ALL          },
  { L_GNU___INLINE,     Y_INLINE,           TS_INLINE,      LANG_ALL          },
  { L_GNU___INLINE__,   Y_INLINE,           TS_INLINE,      LANG_ALL          },
  { L_GNU___RESTRICT,   Y_GNU___RESTRICT,   TS_RESTRICT,    LANG_ALL          },
  { L_GNU___RESTRICT__, Y_GNU___RESTRICT,   TS_RESTRICT,    LANG_ALL          },
  { L_GNU___SIGNED,     Y_SIGNED,           TB_SIGNED,      LANG_ALL          },
  { L_GNU___SIGNED__,   Y_SIGNED,           TB_SIGNED,      LANG_ALL          },
  { L_GNU___THREAD,     Y_THREAD_LOCAL,     TS_THREAD_LOCAL,LANG_ALL          },
  { L_GNU___VOLATILE,   Y_VOLATILE,         TS_VOLATILE,    LANG_ALL          },
  { L_GNU___VOLATILE__, Y_VOLATILE,         TS_VOLATILE,    LANG_ALL          },

  // Apple extensions
  { L_APPLE___BLOCK,    Y_APPLE___BLOCK,    TS_APPLE_BLOCK, LANG_ALL          },

  { NULL,               0,                  TX_NONE,        LANG_NONE         }
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
