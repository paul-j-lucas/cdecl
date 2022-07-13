/*
**      cdecl -- C gibberish translator
**      src/c_keyword.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
#include "cdecl.h"
#include "literals.h"
#include "cdecl_parser.h"               /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <string.h>

// shorthands
#define KC__                      C_KW_CTX_DEFAULT
#define KC_A                      C_KW_CTX_ATTRIBUTE
#define KC_F                      C_KW_CTX_MBR_FUNC

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C/C++ keywords.
 *
 * @note There are two rows for `auto` since it has two meanings (one as a
 * storage class in C and C++ up to C++03 and the other as an automatically
 * deduced type in C++11 and later).
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,                 Y_AUTO_STORAGE,       KC__, TS_AUTO,
    LANG_AUTO_STORAGE,      AC_LANG(AUTO_STORAGE)                         },
  { L_BREAK,                Y_BREAK,              KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_CASE,                 Y_CASE,               KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_CHAR,                 Y_CHAR,               KC__, TB_CHAR,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_CONTINUE,             Y_CONTINUE,           KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  // Allow "default" in any language version since it's a keyword, but only
  // make it auto-completable in languages where it's allowed in declarations.
  { L_DEFAULT,              Y_DEFAULT,            KC__, TS_DEFAULT,
    LANG_ANY,               AC_LANG(DEFAULT_DELETE_FUNC)                  },
  { L_DO,                   Y_DO,                 KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_DOUBLE,               Y_DOUBLE,             KC__, TB_DOUBLE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_ELSE,                 Y_ELSE,               KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_EXTERN,               Y_EXTERN,             KC__, TS_EXTERN,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_FLOAT,                Y_FLOAT,              KC__, TB_FLOAT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_FOR,                  Y_FOR,                KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_GOTO,                 Y_GOTO,               KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_IF,                   Y_IF,                 KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_INT,                  Y_INT,                KC__, TB_INT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_LONG,                 Y_LONG,               KC__, TB_LONG,
    LANG_ANY,               AC_LANG(ANY)                                  },
  // Allow "register" in any language since it's (still) a keyword, but only
  // make it auto-completable in languages where it's allowed in declarations.
  { L_REGISTER,             Y_REGISTER,           KC__, TS_REGISTER,
    LANG_ANY,               AC_LANG(REGISTER)                             },
  { L_RETURN,               Y_RETURN,             KC__, TX_NONE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_SHORT,                Y_SHORT,              KC__, TB_SHORT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_SIZEOF,               Y_SIZEOF,             KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_STATIC,               Y_STATIC,             KC__, TS_STATIC,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_STRUCT,               Y_STRUCT,             KC__, TB_STRUCT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_SWITCH,               Y_SWITCH,             KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_TYPEDEF,              Y_TYPEDEF,            KC__, TS_TYPEDEF,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_UNION,                Y_UNION,              KC__, TB_UNION,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_UNSIGNED,             Y_UNSIGNED,           KC__, TB_UNSIGNED,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_WHILE,                Y_WHILE,              KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },

  // C89
  { L_ASM,                  Y_ASM,                KC__, TX_NONE,
    LANG_ASM,               AC_LANG(NONE)                                 },
  { L_CONST,                Y_CONST,              KC__, TS_CONST,
    LANG_CONST,             AC_LANG(CONST)                                },
  { L_ENUM,                 Y_ENUM,               KC__, TB_ENUM,
    LANG_ENUM,              AC_LANG(ENUM)                                 },
  { L_SIGNED,               Y_SIGNED,             KC__, TB_SIGNED,
    LANG_SIGNED,            AC_LANG(SIGNED)                               },
  { L_VOID,                 Y_VOID,               KC__, TB_VOID,
    LANG_VOID,              AC_LANG(VOID)                                 },
  { L_VOLATILE,             Y_VOLATILE,           KC__, TS_VOLATILE,
    LANG_VOLATILE,          AC_LANG(VOLATILE)                             },

  // C99
  { L__BOOL,                Y__BOOL,              KC__, TB_BOOL,
    LANG__BOOL,             AC_LANG(_BOOL)                                },
  { L__COMPLEX,             Y__COMPLEX,           KC__, TB_COMPLEX,
    LANG__COMPLEX,          AC_LANG(_COMPLEX)                             },
  { L__IMAGINARY,           Y__IMAGINARY,         KC__, TB_IMAGINARY,
    LANG__IMAGINARY,        AC_LANG(_IMAGINARY)                           },
  { L_INLINE,               Y_INLINE,             KC__, TS_INLINE,
    LANG_INLINE,            AC_LANG(INLINE)                               },
  // Allow "restrict" to be recognized in C++ also so the parser can give a
  // better error messsage -- see "restrict_qualifier_c_tid" in parser.y.
  { L_RESTRICT,             Y_RESTRICT,           KC__, TS_RESTRICT,
    LANG_RESTRICT | LANG_CPP_ANY,
                            AC_LANG(RESTRICT)                             },
  { L_WCHAR_T,              Y_WCHAR_T,            KC__, TB_WCHAR_T,
    LANG_WCHAR_T,           AC_LANG(WCHAR_T)                              },

  // C11
  { L__ALIGNAS,             Y__ALIGNAS,           KC__, TX_NONE,
    LANG__ALIGNAS,          AC_LANG(_ALIGNAS)                             },
  { L__ALIGNOF,             Y__ALIGNOF,           KC__, TX_NONE,
    LANG__ALIGNOF,          AC_LANG(NONE)                                 },
  { L__ATOMIC,              Y__ATOMIC_QUAL,       KC__, TS_ATOMIC,
    LANG__ATOMIC,           AC_LANG(_ATOMIC)                              },
  { L__GENERIC,             Y__GENERIC,           KC__, TX_NONE,
    LANG__GENERIC,          AC_LANG(NONE)                                 },
  { L__NORETURN,            Y__NORETURN,          KC__, TA_NORETURN,
    LANG___NORETURN__,      AC_LANG(__NORETURN__)                         },
  { L__STATIC_ASSERT,       Y__STATIC_ASSERT,     KC__, TX_NONE,
    LANG__STATIC_ASSERT,    AC_LANG(NONE)                                 },
  { L__THREAD_LOCAL,        Y__THREAD_LOCAL,      KC__, TS_THREAD_LOCAL,  
    LANG__THREAD_LOCAL,     AC_LANG(_THREAD_LOCAL)                        },

  // C++
  { L_BOOL,                 Y_BOOL,               KC__, TB_BOOL,
    LANG_CPP_ANY,           AC_LANG(BOOL)                                 },
  { L_CATCH,                Y_CATCH,              KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_CLASS,                Y_CLASS,              KC__, TB_CLASS,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_CONST_CAST,           Y_CONST_CAST,         KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  // Allow "delete" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where it's allowed in declarations.
  { L_DELETE,               Y_DELETE,             KC__, TS_DELETE,
    LANG_CPP_ANY,           AC_LANG(DEFAULT_DELETE_FUNC)                  },
  { L_DYNAMIC_CAST,         Y_DYNAMIC_CAST,       KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_EXPLICIT,             Y_EXPLICIT,           KC__, TS_EXPLICIT,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  // Allow "false" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where "noexcept" is supported.
  { L_FALSE,                Y_FALSE,              KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NOEXCEPT)                             },
  { L_FRIEND,               Y_FRIEND,             KC__, TS_FRIEND,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_MUTABLE,              Y_MUTABLE,            KC__, TS_MUTABLE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_NAMESPACE,            Y_NAMESPACE,          KC__, TB_NAMESPACE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_NEW,                  Y_NEW,                KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_OPERATOR,             Y_OPERATOR,           KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_PRIVATE,              Y_PRIVATE,            KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_PROTECTED,            Y_PROTECTED,          KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_PUBLIC,               Y_PUBLIC,             KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_REINTERPRET_CAST,     Y_REINTERPRET_CAST,   KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_STATIC_CAST,          Y_STATIC_CAST,        KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_TEMPLATE,             Y_TEMPLATE,           KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_THIS,                 Y_THIS,               KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_THROW,                Y_THROW,              KC__, TS_THROW,
    LANG_CPP_ANY,           AC_LANG(CPP_MAX(03))                          },
  // Allow "true" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where "noexcept" is supported.
  { L_TRUE,                 Y_TRUE,               KC__, TS_NOEXCEPT,
    LANG_CPP_ANY,           AC_LANG(NOEXCEPT)                             },
  { L_TRY,                  Y_TRY,                KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_TYPEID,               Y_TYPEID,             KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_TYPENAME,             Y_TYPENAME,           KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_USING,                Y_USING,              KC__, TS_TYPEDEF,
    LANG_CPP_ANY,           AC_LANG(CPP_MIN(11))                          },
  { L_VIRTUAL,              Y_VIRTUAL,            KC__, TS_VIRTUAL,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },

  // C++11
  { L_ALIGNAS,              Y_ALIGNAS,            KC__, TX_NONE,
    LANG_ALIGNAS,           AC_LANG(ALIGNMENT)                            },
  { L_ALIGNOF,              Y_ALIGNOF,            KC__, TX_NONE,
    LANG_ALIGNOF,           AC_LANG(NONE)                                 },
  { L_AUTO,                 Y_AUTO_TYPE,          KC__, TB_AUTO,
    LANG_AUTO_TYPE,         AC_LANG(AUTO_TYPE)                            },
  { L_CONSTEXPR,            Y_CONSTEXPR,          KC__, TS_CONSTEXPR,
    LANG_CONSTEXPR,         AC_LANG(CONSTEXPR)                            },
  { L_DECLTYPE,             Y_DECLTYPE,           KC__, TX_NONE,
    LANG_DECLTYPE,          AC_LANG(NONE)                                 },
  { L_FINAL,                Y_FINAL,              KC_F, TS_FINAL,
    LANG_FINAL,             AC_LANG(FINAL)                                },
  { L_NOEXCEPT,             Y_NOEXCEPT,           KC__, TS_NOEXCEPT,
    LANG_NOEXCEPT,          AC_LANG(NOEXCEPT)                             },
  { L_NULLPTR,              Y_NULLPTR,            KC__, TX_NONE,
    LANG_NULLPTR,           AC_LANG(NONE)                                 },
  { L_OVERRIDE,             Y_OVERRIDE,           KC_F, TS_OVERRIDE,
    LANG_OVERRIDE,          AC_LANG(OVERRIDE)                             },
  { L_STATIC_ASSERT,        Y_STATIC_ASSERT,      KC__, TX_NONE,
    LANG_STATIC_ASSERT,     AC_LANG(NONE)                                 },
  { L_THREAD_LOCAL,         Y_THREAD_LOCAL,       KC__, TS_THREAD_LOCAL,
    LANG_THREAD_LOCAL,      AC_LANG(THREAD_LOCAL_STORAGE)                 },

  // C11 & C++11
  { L_CHAR16_T,             Y_CHAR16_T,           KC__, TB_CHAR16_T,
    LANG_CHAR16_32_T,       AC_LANG(CHAR16_32_T)                          },
  { L_CHAR32_T,             Y_CHAR32_T,           KC__, TB_CHAR32_T,
    LANG_CHAR16_32_T,       AC_LANG(CHAR16_32_T)                          },

  // C2X & C++20
  { L_CHAR8_T,              Y_CHAR8_T,            KC__, TB_CHAR8_T,
    LANG_CHAR8_T,           AC_LANG(CHAR8_T)                              },

  // C++20
  { L_CONCEPT,              Y_CONCEPT,            KC__, TX_NONE,
    LANG_CONCEPTS,          AC_LANG(NONE)                                 },
  { L_CONSTEVAL,            Y_CONSTEVAL,          KC__, TS_CONSTEVAL,
    LANG_CONSTEVAL,         AC_LANG(CONSTEVAL)                            },
  { L_CONSTINIT,            Y_CONSTINIT,          KC__, TS_CONSTINIT,
    LANG_CONSTINIT,         AC_LANG(CONSTINIT)                            },
  { L_CO_AWAIT,             Y_CO_AWAIT,           KC__, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_CO_RETURN,            Y_CO_RETURN,          KC__, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_CO_YIELD,             Y_CO_YIELD,           KC__, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_EXPORT,               Y_EXPORT,             KC__, TS_EXPORT,
    LANG_EXPORT,            AC_LANG(EXPORT)                               },
  { L_REQUIRES,             Y_REQUIRES,           KC__, TX_NONE,
    LANG_CONCEPTS,          AC_LANG(NONE)                                 },

  // Alternative tokens
  { L_AND,                  Y_AMPER2,             KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_AND_EQ,               Y_AMPER_EQ,           KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_BITAND,               Y_AMPER,              KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_BITOR,                Y_PIPE,               KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_COMPL,                Y_TILDE,              KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_NOT,                  Y_EXCLAM,             KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_NOT_EQ,               Y_EXCLAM_EQ,          KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_OR,                   Y_PIPE2,              KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_OR_EQ,                Y_PIPE_EQ,            KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_XOR,                  Y_CIRC,               KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_XOR_EQ,               Y_CIRC_EQ,            KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },

  // C++11 attributes
  { L_CARRIES_DEPENDENCY,   Y_CARRIES_DEPENDENCY, KC_A, TA_CARRIES_DEPENDENCY,
    LANG_CARRIES_DEPENDENCY,AC_LANG(CARRIES_DEPENDENCY)                   },
  { L_NORETURN,             Y_NORETURN,           KC_A, TA_NORETURN,
    LANG_NORETURN,          AC_LANG(NORETURN)                             },

  // C2X & C++14 attributes
  { L_DEPRECATED,           Y_DEPRECATED,         KC_A, TA_DEPRECATED,
    LANG_DEPRECATED,        AC_LANG(DEPRECATED)                           },
  { L___DEPRECATED__,       Y_DEPRECATED,         KC_A, TA_DEPRECATED,    
    LANG___DEPRECATED__,    AC_LANG(__DEPRECATED__)                       },

  // C2X & C++17 attributes
  { L_MAYBE_UNUSED,         Y_MAYBE_UNUSED,       KC_A, TA_MAYBE_UNUSED,
    LANG_MAYBE_UNUSED,      AC_LANG(MAYBE_UNUSED)                         },
  { L___MAYBE_UNUSED__,     Y_MAYBE_UNUSED,       KC_A, TA_MAYBE_UNUSED,
    LANG___MAYBE_UNUSED__,  AC_LANG(__MAYBE_UNUSED__)                     },
  { L_NODISCARD,            Y_NODISCARD,          KC_A, TA_NODISCARD,
    LANG_NODISCARD,         AC_LANG(NODISCARD)                            },
  { L___NODISCARD__,        Y_NODISCARD,          KC_A, TA_NODISCARD,
    LANG___NODISCARD__,     AC_LANG(__NODISCARD__)                        },

  // C++20 attributes
#if 0                       // Not implemented because:
  { L_ASSERT,               // + These use arbitrary expressions that require
  { L_ENSURES,              //   being able to parse them -- which is a lot of
  { L_EXPECTS,              //   work for little benefit.

  { L_LIKELY,               // + These are only for statements, not
  { L_UNLIKELY,             //   declarations.
#endif
  { L_NO_UNIQUE_ADDRESS,    Y_NO_UNIQUE_ADDRESS,  KC_A, TA_NO_UNIQUE_ADDRESS,
    LANG_NO_UNIQUE_ADDRESS, AC_LANG(NO_UNIQUE_ADDRESS)                    },

  // Embedded C extensions
  { L_EMC__ACCUM,           Y_EMC__ACCUM,         KC__, TB_EMC_ACCUM,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },
  { L_EMC__FRACT,           Y_EMC__FRACT,         KC__, TB_EMC_FRACT,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },
  { L_EMC__SAT,             Y_EMC__SAT,           KC__, TB_EMC_SAT,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },

  // Unified Parallel C extensions
  { L_UPC_RELAXED,          Y_UPC_RELAXED,        KC__, TS_UPC_RELAXED,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },
  { L_UPC_SHARED,           Y_UPC_SHARED,         KC__, TS_UPC_SHARED,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },
  { L_UPC_STRICT,           Y_UPC_STRICT,         KC__, TS_UPC_STRICT,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },

  // GNU extensions
  { L_GNU___ATTRIBUTE__,    Y_GNU___ATTRIBUTE__,  KC__, TX_NONE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___AUTO_TYPE,      Y_AUTO_TYPE,          KC__, TB_AUTO,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___COMPLEX,        Y__COMPLEX,           KC__, TB_COMPLEX,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___COMPLEX__,      Y__COMPLEX,           KC__, TB_COMPLEX,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___CONST,          Y_CONST,              KC__, TS_CONST,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___INLINE,         Y_INLINE,             KC__, TS_INLINE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___INLINE__,       Y_INLINE,             KC__, TS_INLINE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___RESTRICT,       Y_GNU___RESTRICT,     KC__, TS_RESTRICT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___RESTRICT__,     Y_GNU___RESTRICT,     KC__, TS_RESTRICT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___SIGNED,         Y_SIGNED,             KC__, TB_SIGNED,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___SIGNED__,       Y_SIGNED,             KC__, TB_SIGNED,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___THREAD,         Y_THREAD_LOCAL,       KC__, TS_THREAD_LOCAL,  
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___VOLATILE,       Y_VOLATILE,           KC__, TS_VOLATILE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___VOLATILE__,     Y_VOLATILE,           KC__, TS_VOLATILE,
    LANG_ANY,               AC_LANG(ANY)                                  },

  // Apple extensions
  { L_APPLE___BLOCK,        Y_APPLE___BLOCK,      KC__, TS_APPLE_BLOCK,
    LANG_APPLE___BLOCK,     AC_LANG(APPLE___BLOCK)                        },

  // Microsoft extensions
  { L_MSC__ASM,             Y_ASM,                KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(NONE)                                 },
  { L_MSC___ASM,            Y_ASM,                KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(NONE)                                 },
  { L_MSC__CDECL,           Y_MSC___CDECL,        KC__, TA_MSC_CDECL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___CDECL,          Y_MSC___CDECL,        KC__, TA_MSC_CDECL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___CLRCALL,        Y_MSC___CLRCALL,      KC__, TA_MSC_CLRCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__DECLSPEC,        Y_MSC___DECLSPEC,     KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___DECLSPEC,       Y_MSC___DECLSPEC,     KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__FASTCALL,        Y_MSC___FASTCALL,     KC__, TA_MSC_FASTCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___FASTCALL,       Y_MSC___FASTCALL,     KC__, TA_MSC_FASTCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__FORCEINLINE,     Y_INLINE,             KC__, TS_INLINE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___FORCEINLINE,    Y_INLINE,             KC__, TS_INLINE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__INLINE,          Y_INLINE,             KC__, TS_INLINE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__RESTRICT,        Y_RESTRICT,           KC__, TS_RESTRICT,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__STDCALL,         Y_MSC___STDCALL,      KC__, TA_MSC_STDCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___STDCALL,        Y_MSC___STDCALL,      KC__, TA_MSC_STDCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___THISCALL,       Y_MSC___THISCALL,     KC__, TA_MSC_THISCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__VECTORCALL,      Y_MSC___VECTORCALL,   KC__, TA_MSC_VECTORCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___VECTORCALL,     Y_MSC___VECTORCALL,   KC__, TA_MSC_VECTORCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },

  { NULL,                   0,                    KC__, TX_NONE,
    LANG_NONE,              AC_LANG(NONE)                                 }
};

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_keyword_find( char const *literal, c_lang_id_t lang_ids,
                                   c_keyword_ctx_t kw_ctx ) {
  assert( literal != NULL );
  assert( lang_ids != LANG_NONE );

  for ( c_keyword_t const *k = C_KEYWORDS; k->literal != NULL; ++k ) {
    if ( (k->lang_ids & lang_ids) == LANG_NONE )
      continue;

    if ( cdecl_mode == CDECL_GIBBERISH_TO_ENGLISH &&
         k->kw_ctx != C_KW_CTX_DEFAULT && kw_ctx != k->kw_ctx ) {
      //
      // Keyword contexts matter only when converting gibberish to pseudo-
      // English.  For example, we do NOT match attribute names when parsing
      // C++ because they are not reserved words.  For example:
      //
      //      [[noreturn]] void noreturn();
      //
      // is legal.
      //
      continue;
    }
    else {
      //
      // When converting pseudo-English to gibberish, we MUST match attribute
      // names because there isn't any special syntax for them, e.g.:
      //
      //      declare x as deprecated int
      //
    }

    if ( strcmp( literal, k->literal ) == 0 )
      return k;
  } // for

  return NULL;
}

c_keyword_t const* c_keyword_next( c_keyword_t const *k ) {
  return k == NULL ? C_KEYWORDS : (++k)->literal == NULL ? NULL : k;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
