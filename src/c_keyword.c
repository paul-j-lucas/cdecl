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
 * Defines functions for looking up C/C++ keyword or C23/C++11 (or later)
 * attribute information.
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
 * Array of all C/C++ keywords or C23/C++11 (or later) attributes.
 *
 * @note There are two rows for `auto` since it has two meanings (one as a
 * storage class in C and C++ up to C++03 and the other as an automatically
 * deduced type in C++11 and later).
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_auto,                 Y_auto_STORAGE,       KC__, TS_AUTO,
    LANG_AUTO_STORAGE,      AC_LANG(AUTO_STORAGE)                         },
  { L_break,                Y_break,              KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_case,                 Y_case,               KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_char,                 Y_char,               KC__, TB_CHAR,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_continue,             Y_continue,           KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  // Allow "default" in any language version since it's a keyword, but only
  // make it auto-completable in languages where it's allowed in declarations.
  { L_default,              Y_default,            KC__, TS_DEFAULT,
    LANG_ANY,               AC_LANG(DEFAULT_DELETE_FUNC)                  },
  { L_do,                   Y_do,                 KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_double,               Y_double,             KC__, TB_DOUBLE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_else,                 Y_else,               KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_extern,               Y_extern,             KC__, TS_EXTERN,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_float,                Y_float,              KC__, TB_FLOAT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_for,                  Y_for,                KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_goto,                 Y_goto,               KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_if,                   Y_if,                 KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_int,                  Y_int,                KC__, TB_INT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_long,                 Y_long,               KC__, TB_LONG,
    LANG_ANY,               AC_LANG(ANY)                                  },
  // Allow "register" in any language since it's (still) a keyword, but only
  // make it auto-completable in languages where it's allowed in declarations.
  { L_register,             Y_register,           KC__, TS_REGISTER,
    LANG_ANY,               AC_LANG(REGISTER)                             },
  { L_return,               Y_return,             KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_short,                Y_short,              KC__, TB_SHORT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_sizeof,               Y_sizeof,             KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_static,               Y_static,             KC__, TS_STATIC,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_struct,               Y_struct,             KC__, TB_STRUCT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_switch,               Y_switch,             KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_typedef,              Y_typedef,            KC__, TS_TYPEDEF,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_union,                Y_union,              KC__, TB_UNION,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_unsigned,             Y_unsigned,           KC__, TB_UNSIGNED,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_while,                Y_while,              KC__, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },

  // C89
  { L_asm,                  Y_asm,                KC__, TX_NONE,
    LANG_ASM,               AC_LANG(NONE)                                 },
  { L_const,                Y_const,              KC__, TS_CONST,
    LANG_CONST,             AC_LANG(CONST)                                },
  { L_enum,                 Y_enum,               KC__, TB_ENUM,
    LANG_ENUM,              AC_LANG(ENUM)                                 },
  { L_signed,               Y_signed,             KC__, TB_SIGNED,
    LANG_SIGNED,            AC_LANG(SIGNED)                               },
  { L_void,                 Y_void,               KC__, TB_VOID,
    LANG_VOID,              AC_LANG(VOID)                                 },
  { L_volatile,             Y_volatile,           KC__, TS_VOLATILE,
    LANG_VOLATILE,          AC_LANG(VOLATILE)                             },

  // C99
  { L__Bool,                Y__Bool,              KC__, TB_BOOL,
    LANG__BOOL,             AC_LANG(_BOOL)                                },
  { L__Complex,             Y__Complex,           KC__, TB_COMPLEX,
    LANG__COMPLEX,          AC_LANG(_COMPLEX)                             },
  { L__Imaginary,           Y__Imaginary,         KC__, TB_IMAGINARY,
    LANG__IMAGINARY,        AC_LANG(_IMAGINARY)                           },
  { L_inline,               Y_inline,             KC__, TS_INLINE,
    LANG_INLINE,            AC_LANG(INLINE)                               },
  // Allow "restrict" to be recognized in C++ also so the parser can give a
  // better error messsage -- see "restrict_qualifier_c_tid" in parser.y.
  { L_restrict,             Y_restrict,           KC__, TS_RESTRICT,
    LANG_RESTRICT | LANG_CPP_ANY,
                            AC_LANG(RESTRICT)                             },
  { L_wchar_t,              Y_wchar_t,            KC__, TB_WCHAR_T,
    LANG_WCHAR_T,           AC_LANG(WCHAR_T)                              },

  // C11
  { L__Alignas,             Y__Alignas,           KC__, TX_NONE,
    LANG__ALIGNAS,          AC_LANG(_ALIGNAS)                             },
  { L__Alignof,             Y__Alignof,           KC__, TX_NONE,
    LANG__ALIGNOF,          AC_LANG(NONE)                                 },
  { L__Atomic,              Y__Atomic_QUAL,       KC__, TS_ATOMIC,
    LANG__ATOMIC,           AC_LANG(_ATOMIC)                              },
  { L__Generic,             Y__Generic,           KC__, TX_NONE,
    LANG__GENERIC,          AC_LANG(NONE)                                 },
  { L__Noreturn,            Y__Noreturn,          KC__, TA_NORETURN,
    LANG___NORETURN__,      AC_LANG(__NORETURN__)                         },
  { L__Static_assert,       Y__Static_assert,     KC__, TX_NONE,
    LANG__STATIC_ASSERT,    AC_LANG(NONE)                                 },
  { L__Thread_local,        Y__Thread_local,      KC__, TS_THREAD_LOCAL,
    LANG__THREAD_LOCAL,     AC_LANG(_THREAD_LOCAL)                        },

  // C++
  { L_bool,                 Y_bool,               KC__, TB_BOOL,
    LANG_CPP_ANY,           AC_LANG(BOOL)                                 },
  { L_catch,                Y_catch,              KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_class,                Y_class,              KC__, TB_CLASS,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_const_cast,           Y_const_cast,         KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  // Allow "delete" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where it's allowed in declarations.
  { L_delete,               Y_delete,             KC__, TS_DELETE,
    LANG_CPP_ANY,           AC_LANG(DEFAULT_DELETE_FUNC)                  },
  { L_dynamic_cast,         Y_dynamic_cast,       KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_explicit,             Y_explicit,           KC__, TS_EXPLICIT,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  // Allow "false" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where "noexcept" is supported.
  { L_false,                Y_false,              KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NOEXCEPT)                             },
  { L_friend,               Y_friend,             KC__, TS_FRIEND,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_mutable,              Y_mutable,            KC__, TS_MUTABLE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_namespace,            Y_namespace,          KC__, TB_NAMESPACE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_new,                  Y_new,                KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_operator,             Y_operator,           KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_private,              Y_private,            KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_protected,            Y_protected,          KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_public,               Y_public,             KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_reinterpret_cast,     Y_reinterpret_cast,   KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_static_cast,          Y_static_cast,        KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_template,             Y_template,           KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_this,                 Y_this,               KC__, TS_THIS,
    LANG_CPP_ANY,           AC_LANG(EXPLICIT_OBJ_PARAM_DECL)              },
  { L_throw,                Y_throw,              KC__, TS_THROW,
    LANG_CPP_ANY,           AC_LANG(THROW)                                },
  // Allow "true" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where "noexcept" is supported.
  { L_true,                 Y_true,               KC__, TS_NOEXCEPT,
    LANG_CPP_ANY,           AC_LANG(NOEXCEPT)                             },
  { L_try,                  Y_try,                KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_typeid,               Y_typeid,             KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_typename,             Y_typename,           KC__, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_using,                Y_using,              KC__, TS_TYPEDEF,
    LANG_CPP_ANY,           AC_LANG(USING_DECLARATION)                    },
  { L_virtual,              Y_virtual,            KC__, TS_VIRTUAL,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },

  // C++11
  { L_alignas,              Y_alignas,            KC__, TX_NONE,
    LANG_ALIGNAS,           AC_LANG(ALIGNMENT)                            },
  { L_alignof,              Y_alignof,            KC__, TX_NONE,
    LANG_ALIGNOF,           AC_LANG(NONE)                                 },
  { L_auto,                 Y_auto_TYPE,          KC__, TB_AUTO,
    LANG_AUTO_TYPE,         AC_LANG(AUTO_TYPE)                            },
  { L_constexpr,            Y_constexpr,          KC__, TS_CONSTEXPR,
    LANG_CONSTEXPR,         AC_LANG(CONSTEXPR)                            },
  { L_decltype,             Y_decltype,           KC__, TX_NONE,
    LANG_DECLTYPE,          AC_LANG(NONE)                                 },
  { L_final,                Y_final,              KC_F, TS_FINAL,
    LANG_FINAL,             AC_LANG(FINAL)                                },
  { L_noexcept,             Y_noexcept,           KC__, TS_NOEXCEPT,
    LANG_NOEXCEPT,          AC_LANG(NOEXCEPT)                             },
  { L_nullptr,              Y_nullptr,            KC__, TX_NONE,
    LANG_NULLPTR,           AC_LANG(NONE)                                 },
  { L_override,             Y_override,           KC_F, TS_OVERRIDE,
    LANG_OVERRIDE,          AC_LANG(OVERRIDE)                             },
  { L_static_assert,        Y_static_assert,      KC__, TX_NONE,
    LANG_STATIC_ASSERT,     AC_LANG(NONE)                                 },
  { L_thread_local,         Y_thread_local,       KC__, TS_THREAD_LOCAL,
    LANG_THREAD_LOCAL,      AC_LANG(THREAD_LOCAL_STORAGE)                 },

  // C11 & C++11
  { L_char16_t,             Y_char16_t,           KC__, TB_CHAR16_T,
    LANG_CHAR16_32_T,       AC_LANG(CHAR16_32_T)                          },
  { L_char32_t,             Y_char32_t,           KC__, TB_CHAR32_T,
    LANG_CHAR16_32_T,       AC_LANG(CHAR16_32_T)                          },

  // C23 & C++20
  { L_char8_t,              Y_char8_t,            KC__, TB_CHAR8_T,
    LANG_CHAR8_T,           AC_LANG(CHAR8_T)                              },

  // C++20
  { L_concept,              Y_concept,            KC__, TX_NONE,
    LANG_CONCEPTS,          AC_LANG(NONE)                                 },
  { L_consteval,            Y_consteval,          KC__, TS_CONSTEVAL,
    LANG_CONSTEVAL,         AC_LANG(CONSTEVAL)                            },
  { L_constinit,            Y_constinit,          KC__, TS_CONSTINIT,
    LANG_CONSTINIT,         AC_LANG(CONSTINIT)                            },
  { L_co_await,             Y_co_await,           KC__, TX_NONE,
    LANG_COROUTINES,        AC_LANG(COROUTINES)                           },
  { L_co_return,            Y_co_return,          KC__, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_co_yield,             Y_co_yield,           KC__, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_export,               Y_export,             KC__, TS_EXPORT,
    LANG_EXPORT,            AC_LANG(EXPORT)                               },
  { L_requires,             Y_requires,           KC__, TX_NONE,
    LANG_CONCEPTS,          AC_LANG(NONE)                                 },

  // Alternative tokens
  { L_and,                  Y_AMPER2,             KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_and_eq,               Y_AMPER_EQ,           KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_bitand,               Y_AMPER,              KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_bitor,                Y_PIPE,               KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_compl,                Y_TILDE,              KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_not,                  Y_EXCLAM,             KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_not_eq,               Y_EXCLAM_EQ,          KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_or,                   Y_PIPE2,              KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_or_eq,                Y_PIPE_EQ,            KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_xor,                  Y_CIRC,               KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_xor_eq,               Y_CIRC_EQ,            KC__, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },

  // C++11 attributes
  { L_carries_dependency,   Y_carries_dependency, KC_A, TA_CARRIES_DEPENDENCY,
    LANG_CARRIES_DEPENDENCY,AC_LANG(CARRIES_DEPENDENCY)                   },
  { L_noreturn,             Y_noreturn,           KC_A, TA_NORETURN,
    LANG_NORETURN,          AC_LANG(NORETURN)                             },

  // C23 & C++14 attributes
  { L_deprecated,           Y_deprecated,         KC_A, TA_DEPRECATED,
    LANG_DEPRECATED,        AC_LANG(DEPRECATED)                           },
  { L___deprecated__,       Y_deprecated,         KC_A, TA_DEPRECATED,
    LANG___DEPRECATED__,    AC_LANG(__DEPRECATED__)                       },

  // C23 & C++17 attributes
  { L_maybe_unused,         Y_maybe_unused,       KC_A, TA_MAYBE_UNUSED,
    LANG_MAYBE_UNUSED,      AC_LANG(MAYBE_UNUSED)                         },
  { L___maybe_unused__,     Y_maybe_unused,       KC_A, TA_MAYBE_UNUSED,
    LANG___MAYBE_UNUSED__,  AC_LANG(__MAYBE_UNUSED__)                     },
  { L_nodiscard,            Y_nodiscard,          KC_A, TA_NODISCARD,
    LANG_NODISCARD,         AC_LANG(NODISCARD)                            },
  { L___nodiscard__,        Y_nodiscard,          KC_A, TA_NODISCARD,
    LANG___NODISCARD__,     AC_LANG(__NODISCARD__)                        },

  // C++20 attributes
#if 0                       // Not implemented because:
  { L_ASSERT,               // + These use arbitrary expressions that require
  { L_ENSURES,              //   being able to parse them -- which is a lot of
  { L_EXPECTS,              //   work for little benefit.

  { L_LIKELY,               // + These are only for statements, not
  { L_UNLIKELY,             //   declarations.
#endif
  { L_no_unique_address,    Y_no_unique_address,  KC_A, TA_NO_UNIQUE_ADDRESS,
    LANG_NO_UNIQUE_ADDRESS, AC_LANG(NO_UNIQUE_ADDRESS)                    },

  // Embedded C extensions
  { L_EMC__Accum,           Y_EMC__Accum,         KC__, TB_EMC_ACCUM,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },
  { L_EMC__Fract,           Y_EMC__Fract,         KC__, TB_EMC_FRACT,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },
  { L_EMC__Sat,             Y_EMC__Sat,           KC__, TB_EMC_SAT,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },

  // Unified Parallel C extensions
  { L_UPC_relaxed,          Y_UPC_relaxed,        KC__, TS_UPC_RELAXED,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },
  { L_UPC_shared,           Y_UPC_shared,         KC__, TS_UPC_SHARED,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },
  { L_UPC_strict,           Y_UPC_strict,         KC__, TS_UPC_STRICT,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },

  // GNU extensions
  { L_GNU___attribute__,    Y_GNU___attribute__,  KC__, TX_NONE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___auto_type,      Y_auto_TYPE,          KC__, TB_AUTO,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___complex,        Y__Complex,           KC__, TB_COMPLEX,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___complex__,      Y__Complex,           KC__, TB_COMPLEX,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___const,          Y_const,              KC__, TS_CONST,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___inline,         Y_inline,             KC__, TS_INLINE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___inline__,       Y_inline,             KC__, TS_INLINE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___restrict,       Y_GNU___restrict,     KC__, TS_RESTRICT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___restrict__,     Y_GNU___restrict,     KC__, TS_RESTRICT,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___signed,         Y_signed,             KC__, TB_SIGNED,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___signed__,       Y_signed,             KC__, TB_SIGNED,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___thread,         Y_thread_local,       KC__, TS_THREAD_LOCAL,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___volatile,       Y_volatile,           KC__, TS_VOLATILE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___volatile__,     Y_volatile,           KC__, TS_VOLATILE,
    LANG_ANY,               AC_LANG(ANY)                                  },

  // Apple extensions
  { L_Apple___block,        Y_Apple___block,      KC__, TS_APPLE_BLOCK,
    LANG_APPLE___BLOCK,     AC_LANG(APPLE___BLOCK)                        },

  // Microsoft extensions
  { L_MSC__asm,             Y_asm,                KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(NONE)                                 },
  { L_MSC___asm,            Y_asm,                KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(NONE)                                 },
  { L_MSC__cdecl,           Y_MSC___cdecl,        KC__, TA_MSC_CDECL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___cdecl,          Y_MSC___cdecl,        KC__, TA_MSC_CDECL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___clrcall,        Y_MSC___clrcall,      KC__, TA_MSC_CLRCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__declspec,        Y_MSC___declspec,     KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___declspec,       Y_MSC___declspec,     KC__, TX_NONE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__fastcall,        Y_MSC___fastcall,     KC__, TA_MSC_FASTCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___fastcall,       Y_MSC___fastcall,     KC__, TA_MSC_FASTCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__forceinline,     Y_inline,             KC__, TS_INLINE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___forceinline,    Y_inline,             KC__, TS_INLINE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__inline,          Y_inline,             KC__, TS_INLINE,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__restrict,        Y_restrict,           KC__, TS_RESTRICT,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__stdcall,         Y_MSC___stdcall,      KC__, TA_MSC_STDCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___stdcall,        Y_MSC___stdcall,      KC__, TA_MSC_STDCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___thiscall,       Y_MSC___thiscall,     KC__, TA_MSC_THISCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC__vectorcall,      Y_MSC___vectorcall,   KC__, TA_MSC_VECTORCALL,
    LANG_MSC_EXTENSIONS,    AC_LANG(MSC_EXTENSIONS)                       },
  { L_MSC___vectorcall,     Y_MSC___vectorcall,   KC__, TA_MSC_VECTORCALL,
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
