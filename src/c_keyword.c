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
#include "parser.h"                     /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C++ attributes.
 *
 * @hideinitializer
 */
static c_keyword_t const C_ATTRIBUTES[] = {
  // C++11
  { L_CARRIES_DEPENDENCY,
                    Y_CARRIES_DEPENDENCY,
                                    T_CARRIES_DEPENDENCY,
                                                    LANG_MIN(CPP_11)  },
  { L_NORETURN,     Y_NORETURN,     T_NORETURN,     LANG_MIN(CPP_11)  },

  // C++14
  { L_DEPRECATED,   Y_DEPRECATED,   T_DEPRECATED,   LANG_MIN(CPP_14)  },

  // C++17
  { L_MAYBE_UNUSED, Y_MAYBE_UNUSED, T_MAYBE_UNUSED, LANG_MIN(CPP_17)  },
  { L_NODISCARD,    Y_NODISCARD,    T_NODISCARD,    LANG_MIN(CPP_17)  },

  { NULL,           0,              T_NONE,         LANG_NONE         }
};

/**
 * Array of all C/C++ keywords.
 *
 * @note There are two rows for `auto` since it has two meanings (one as a
 * storage class in C and C++ up to C++03 and the other as an automatically
 * deduced type in C++11 and later).
 *
 * @hideinitializer
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,             Y_AUTO_C,           T_AUTO_C,       LANG_MAX(CPP_03)  },
  { L_BREAK,            Y_BREAK,            T_NONE,         LANG_ALL          },
  { L_CASE,             Y_CASE,             T_NONE,         LANG_ALL          },
  { L_CHAR,             Y_CHAR,             T_CHAR,         LANG_ALL          },
  { L_CONTINUE,         Y_CONTINUE,         T_NONE,         LANG_ALL          },
  { L_DEFAULT,          Y_DEFAULT,          T_NONE,         LANG_ALL          },
  { L_DO,               Y_DO,               T_DOUBLE,       LANG_ALL          },
  { L_DOUBLE,           Y_DOUBLE,           T_DOUBLE,       LANG_ALL          },
  { L_ELSE,             Y_ELSE,             T_NONE,         LANG_ALL          },
  { L_EXTERN,           Y_EXTERN,           T_EXTERN,       LANG_ALL          },
  { L_FLOAT,            Y_FLOAT,            T_FLOAT,        LANG_ALL          },
  { L_FOR,              Y_FOR,              T_NONE,         LANG_ALL          },
  { L_GOTO,             Y_GOTO,             T_NONE,         LANG_ALL          },
  { L_IF,               Y_IF,               T_NONE,         LANG_ALL          },
  { L_INT,              Y_INT,              T_INT,          LANG_ALL          },
  { L_LONG,             Y_LONG,             T_LONG,         LANG_ALL          },
  { L_REGISTER,         Y_REGISTER,         T_REGISTER,     LANG_ALL          },
  { L_RETURN,           Y_RETURN,           T_NONE,         LANG_ALL          },
  { L_SHORT,            Y_SHORT,            T_SHORT,        LANG_ALL          },
  { L_SIZEOF,           Y_SIZEOF,           T_NONE,         LANG_ALL          },
  { L_STATIC,           Y_STATIC,           T_STATIC,       LANG_ALL          },
  { L_STRUCT,           Y_STRUCT,           T_STRUCT,       LANG_ALL          },
  { L_SWITCH,           Y_SWITCH,           T_NONE,         LANG_ALL          },
  { L_TYPEDEF,          Y_TYPEDEF,          T_TYPEDEF,      LANG_ALL          },
  { L_UNION,            Y_UNION,            T_UNION,        LANG_ALL          },
  { L_UNSIGNED,         Y_UNSIGNED,         T_UNSIGNED,     LANG_ALL          },
  { L_WHILE,            Y_WHILE,            T_NONE,         LANG_ALL          },

  // C89
  { L_CONST,            Y_CONST,            T_CONST,        LANG_MIN(C_89)    },
  { L_ELLIPSIS,         Y_ELLIPSIS,         T_NONE,         LANG_MIN(C_89)    },
  { L_ENUM,             Y_ENUM,             T_ENUM,         LANG_MIN(C_89)    },
  { L_SIGNED,           Y_SIGNED,           T_SIGNED,       LANG_MIN(C_89)    },
  { L_VOID,             Y_VOID,             T_VOID,         LANG_MIN(C_89)    },
  { L_VOLATILE,         Y_VOLATILE,         T_VOLATILE,     LANG_MIN(C_89)    },

  // C99
  { L_BOOL,             Y_BOOL,             T_BOOL,         LANG_MIN(C_99)    },
  { L__COMPLEX,         Y__COMPLEX,         T_COMPLEX,      LANG_MIN(C_99)    },
  { L__IMAGINARY,       Y__IMAGINARY,       T_IMAGINARY,    LANG_MIN(C_99)    },
  { L_INLINE,           Y_INLINE,           T_INLINE,       LANG_MIN(C_99)    },
  { L_RESTRICT,         Y_RESTRICT,         T_RESTRICT,     LANG_MIN(C_99)    },
  { L_WCHAR_T,          Y_WCHAR_T,          T_WCHAR_T,      LANG_MIN(C_99)    },

  // C11
  { L__ALIGNAS,         Y__ALIGNAS,         T_NONE,         LANG_MIN(C_11)    },
  { L__ALIGNOF,         Y__ALIGNOF,         T_NONE,         LANG_MIN(C_11)    },
  { L__ATOMIC,          Y_ATOMIC_QUAL,      T_ATOMIC,       LANG_MIN(C_11)    },
  { L__GENERIC,         Y__GENERIC,         T_NONE,         LANG_MIN(C_11)    },
  { L__NORETURN,        Y__NORETURN,        T_NORETURN,     LANG_MIN(C_11)    },
  { L__STATIC_ASSERT ,  Y__STATIC_ASSERT,   T_NONE,         LANG_MIN(C_11)    },

  // C++
  { L_AND,              Y_AND,              T_NONE,         LANG_MIN(CPP_MIN) },
  { L_AND_EQ,           Y_AND_EQ,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_BITAND,           Y_BITAND,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_BITOR,            Y_BITOR,            T_NONE,         LANG_MIN(CPP_MIN) },
  { L_CATCH,            Y_CATCH,            T_NONE,         LANG_MIN(CPP_MIN) },
  { L_CLASS,            Y_CLASS,            T_CLASS,        LANG_MIN(CPP_MIN) },
  { L_COMPL,            Y_COMPL,            T_NONE,         LANG_MIN(CPP_MIN) },
  { L_CONST_CAST,       Y_CONST_CAST,       T_NONE,         LANG_MIN(CPP_MIN) },
  { L_DELETE,           Y_DELETE,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_DYNAMIC_CAST,     Y_DYNAMIC_CAST,     T_NONE,         LANG_MIN(CPP_MIN) },
  { L_EXPLICIT,         Y_EXPLICIT,         T_NONE,         LANG_MIN(CPP_MIN) },
  { L_EXPORT,           Y_EXPORT,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_FALSE,            Y_FALSE,            T_NONE,         LANG_MIN(CPP_MIN) },
  { L_FRIEND,           Y_FRIEND,           T_FRIEND,       LANG_MIN(CPP_MIN) },
  { L_MUTABLE,          Y_MUTABLE,          T_MUTABLE,      LANG_MIN(CPP_MIN) },
  { L_NAMESPACE,        Y_NAMESPACE,        T_NONE,         LANG_MIN(CPP_MIN) },
  { L_NEW,              Y_NEW,              T_NONE,         LANG_MIN(CPP_MIN) },
  { L_NOT,              Y_NOT,              T_NONE,         LANG_MIN(CPP_MIN) },
  { L_NOT_EQ,           Y_NOT_EQ,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_OPERATOR,         Y_OPERATOR,         T_NONE,         LANG_MIN(CPP_MIN) },
  { L_OR,               Y_OR,               T_NONE,         LANG_MIN(CPP_MIN) },
  { L_OR_EQ,            Y_OR_EQ,            T_NONE,         LANG_MIN(CPP_MIN) },
  { L_PRIVATE,          Y_PRIVATE,          T_NONE,         LANG_MIN(CPP_MIN) },
  { L_PROTECTED,        Y_PROTECTED,        T_NONE,         LANG_MIN(CPP_MIN) },
  { L_PUBLIC,           Y_PUBLIC,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_REINTERPRET_CAST, Y_REINTERPRET_CAST, T_NONE,         LANG_MIN(CPP_MIN) },
  { L_STATIC_CAST,      Y_STATIC_CAST,      T_NONE,         LANG_MIN(CPP_MIN) },
  { L_TEMPLATE,         Y_TEMPLATE,         T_NONE,         LANG_MIN(CPP_MIN) },
  { L_THIS,             Y_THIS,             T_NONE,         LANG_MIN(CPP_MIN) },
  { L_THROW,            Y_THROW,            T_THROW,        LANG_MIN(CPP_MIN) },
  { L_TRUE,             Y_TRUE,             T_NOEXCEPT,     LANG_MIN(CPP_MIN) },
  { L_TRY,              Y_TRY,              T_NONE,         LANG_MIN(CPP_MIN) },
  { L_TRY,              Y_TRY,              T_NONE,         LANG_MIN(CPP_MIN) },
  { L_TYPEID,           Y_TYPEID,           T_NONE,         LANG_MIN(CPP_MIN) },
  { L_TYPENAME,         Y_TYPENAME,         T_NONE,         LANG_MIN(CPP_MIN) },
  { L_USING,            Y_USING,            T_TYPEDEF,      LANG_MIN(CPP_MIN) },
  { L_VIRTUAL,          Y_VIRTUAL,          T_VIRTUAL,      LANG_MIN(CPP_MIN) },
  { L_XOR,              Y_XOR,              T_NONE,         LANG_MIN(CPP_MIN) },
  { L_XOR_EQ,           Y_XOR_EQ,           T_NONE,         LANG_MIN(CPP_MIN) },

  // C++11
  { L_ALIGNAS,          Y_ALIGNAS,          T_NONE,         LANG_MIN(CPP_11)  },
  { L_ALIGNOF,          Y_ALIGNOF,          T_NONE,         LANG_MIN(CPP_11)  },
  { L_AUTO,             Y_AUTO_CPP_11,      T_AUTO_CPP_11,  LANG_MIN(CPP_11)  },
  { L_CONSTEXPR,        Y_CONSTEXPR,        T_CONSTEXPR,    LANG_MIN(CPP_11)  },
  { L_DECLTYPE,         Y_DECLTYPE,         T_NONE,         LANG_MIN(CPP_11)  },
  { L_FINAL,            Y_FINAL,            T_FINAL,        LANG_MIN(CPP_11)  },
  { L_NOEXCEPT,         Y_NOEXCEPT,         T_NOEXCEPT,     LANG_MIN(CPP_11)  },
  { L_NULLPTR,          Y_NULLPTR,          T_NONE,         LANG_MIN(CPP_11)  },
  { L_OVERRIDE,         Y_OVERRIDE,         T_OVERRIDE,     LANG_MIN(CPP_11)  },

  // C11 & C++11
  { L_CHAR16_T,         Y_CHAR16_T,         T_CHAR16_T,     LANG_C_11 |
                                                            LANG_MIN(CPP_11) },
  { L_CHAR32_T,         Y_CHAR32_T,         T_CHAR32_T,     LANG_C_11 |
                                                            LANG_MIN(CPP_11) },
  { L_THREAD_LOCAL,     Y_THREAD_LOCAL,     T_THREAD_LOCAL, LANG_C_11 |
                                                            LANG_MIN(CPP_11) },

  // Apple extension    
  { L___BLOCK,          Y___BLOCK,          T_BLOCK,        LANG_ALL          },

  { NULL,               0,                  T_NONE,         LANG_NONE         }
};

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_attribute_find( char const *literal ) {
  for ( c_keyword_t const *a = C_ATTRIBUTES; a->literal != NULL; ++a ) {
    if ( strcmp( literal, a->literal ) == 0 )
      return a;
  } // for
  return NULL;
}

c_keyword_t const* c_keyword_find( char const *literal, c_lang_id_t lang_id ) {
  for ( c_keyword_t const *k = C_KEYWORDS; k->literal != NULL; ++k ) {
    if ( (k->ok_langs & lang_id) == LANG_NONE )
      continue;
    if ( strcmp( literal, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
