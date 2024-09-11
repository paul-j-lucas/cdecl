/*
**      cdecl -- C gibberish translator
**      src/c_keyword.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
#include "c_lang.h"
#include "lexer.h"
#include "literals.h"
#include "util.h"
#include "cdecl_parser.h"               /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <stdlib.h>
#include <string.h>

// shorthands
#define KC_D                      C_KW_CTX_DEFAULT
#define KC_A                      C_KW_CTX_ATTRIBUTE
#define KC_F                      C_KW_CTX_MBR_FUNC

/// @endcond

/**
 * @addtogroup c-keywords-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C/C++ keywords or C23/C++11 (or later) attributes.
 *
 * @note There are two rows for `auto` since it has two meanings (one as a
 * storage class in C and C++ up to C++03 and the other as an automatically
 * deduced type in C++11 and later).
 *
 * @note This is not declared `const` because it's sorted once.
 */
static c_keyword_t C_KEYWORDS[] = {
  // K&R C
  { L_auto,                 Y_auto_STORAGE,       KC_D, TS_auto,
    LANG_auto_STORAGE,      AC_LANG(auto_STORAGE)                         },
  { L_break,                Y_break,              KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_case,                 Y_case,               KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_char,                 Y_char,               KC_D, TB_char,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_continue,             Y_continue,           KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  // Allow "default" in any language version since it's a keyword, but only
  // make it autocompletable in languages where it's allowed in declarations.
  { L_default,              Y_default,            KC_D, TS_default,
    LANG_ANY,               AC_LANG(default_delete_FUNCS)                 },
  { L_do,                   Y_do,                 KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_double,               Y_double,             KC_D, TB_double,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_else,                 Y_else,               KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_extern,               Y_extern,             KC_D, TS_extern,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_float,                Y_float,              KC_D, TB_float,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_for,                  Y_for,                KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_goto,                 Y_goto,               KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_if,                   Y_if,                 KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_int,                  Y_int,                KC_D, TB_int,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_long,                 Y_long,               KC_D, TB_long,
    LANG_ANY,               AC_LANG(ANY)                                  },
  // Allow "register" in any language since it's (still) a keyword, but only
  // make it autocompletable in languages where it's allowed in declarations.
  { L_register,             Y_register,           KC_D, TS_register,
    LANG_ANY,               AC_LANG(register)                             },
  { L_return,               Y_return,             KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_short,                Y_short,              KC_D, TB_short,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_sizeof,               Y_sizeof,             KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_static,               Y_static,             KC_D, TS_static,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_struct,               Y_struct,             KC_D, TB_struct,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_switch,               Y_switch,             KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },
  { L_typedef,              Y_typedef,            KC_D, TS_typedef,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_union,                Y_union,              KC_D, TB_union,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_unsigned,             Y_unsigned,           KC_D, TB_unsigned,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_while,                Y_while,              KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(NONE)                                 },

  // C89
  { L_asm,                  Y_asm,                KC_D, TX_NONE,
    LANG_asm,               AC_LANG(NONE)                                 },
  { L_const,                Y_const,              KC_D, TS_const,
    LANG_const,             AC_LANG(const)                                },
  { L_enum,                 Y_enum,               KC_D, TB_enum,
    LANG_enum,              AC_LANG(enum)                                 },
  { L_signed,               Y_signed,             KC_D, TB_signed,
    LANG_signed,            AC_LANG(signed)                               },
  { L_void,                 Y_void,               KC_D, TB_void,
    LANG_void,              AC_LANG(void)                                 },
  { L_volatile,             Y_volatile,           KC_D, TS_volatile,
    LANG_volatile,          AC_LANG(volatile)                             },

  // C99
  { L__Bool,                Y__Bool,              KC_D, TB_bool,
    LANG__Bool,             AC_LANG(_Bool)                                },
  { L__Complex,             Y__Complex,           KC_D, TB__Complex,
    LANG__Complex,          AC_LANG(_Complex)                             },
  { L__Imaginary,           Y__Imaginary,         KC_D, TB__Imaginary,
    LANG__Imaginary,        AC_LANG(_Imaginary)                           },
  { L_inline,               Y_inline,             KC_D, TS_inline,
    LANG_inline,            AC_LANG(inline)                               },
  // Allow "restrict" to be recognized in C++ also so the parser can give a
  // better error message -- see "restrict_qualifier_c_tid" in parser.y.
  { L_restrict,             Y_restrict,           KC_D, TS_restrict,
    LANG_restrict | LANG_CPP_ANY,
                            AC_LANG(restrict)                             },
  { L_wchar_t,              Y_wchar_t,            KC_D, TB_wchar_t,
    LANG_wchar_t,           AC_LANG(wchar_t)                              },

  // C11
  { L__Alignas,             Y__Alignas,           KC_D, TX_NONE,
    LANG__Alignas,          AC_LANG(_Alignas)                             },
  { L__Alignof,             Y__Alignof,           KC_D, TX_NONE,
    LANG__Alignof,          AC_LANG(NONE)                                 },
  { L__Atomic,              Y__Atomic_QUAL,       KC_D, TS__Atomic,
    LANG__Atomic,           AC_LANG(_Atomic)                              },
  { L__Generic,             Y__Generic,           KC_D, TX_NONE,
    LANG__Generic,          AC_LANG(NONE)                                 },
  { L__Noreturn,            Y__Noreturn,          KC_D, TA_noreturn,
    LANG__Noreturn,         AC_LANG(_Noreturn)                            },
  { L__Static_assert,       Y__Static_assert,     KC_D, TX_NONE,
    LANG__Static_assert,    AC_LANG(NONE)                                 },
  { L__Thread_local,        Y__Thread_local,      KC_D, TS_thread_local,
    LANG__Thread_local,     AC_LANG(_Thread_local)                        },

  // C++
  { L_bool,                 Y_bool,               KC_D, TB_bool,
    LANG_CPP_ANY,           AC_LANG(bool)                                 },
  { L_catch,                Y_catch,              KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_class,                Y_class,              KC_D, TB_class,
    LANG_class,             AC_LANG(class)                                },
  { L_const_cast,           Y_const_cast,         KC_D, TX_NONE,
    LANG_NEW_STYLE_CASTS,   AC_LANG(NEW_STYLE_CASTS)                      },
  // Allow "delete" in any C++ version since it's a keyword, but make it auto-
  // completable only in languages where it's allowed in declarations.
  { L_delete,               Y_delete,             KC_D, TS_delete,
    LANG_CPP_ANY,           AC_LANG(default_delete_FUNCS)                 },
  { L_dynamic_cast,         Y_dynamic_cast,       KC_D, TX_NONE,
    LANG_NEW_STYLE_CASTS,   AC_LANG(NEW_STYLE_CASTS)                      },
  { L_explicit,             Y_explicit,           KC_D, TS_explicit,
    LANG_explicit,          AC_LANG(explicit)                             },
  { L_false,                Y_false,              KC_D, TX_NONE,
    LANG_true_false,        AC_LANG(true_false)                           },
  { L_friend,               Y_friend,             KC_D, TS_friend,
    LANG_friend,            AC_LANG(friend)                               },
  { L_mutable,              Y_mutable,            KC_D, TS_mutable,
    LANG_mutable,           AC_LANG(mutable)                              },
  { L_namespace,            Y_namespace,          KC_D, TB_namespace,
    LANG_namespace,         AC_LANG(namespace)                            },
  { L_new,                  Y_new,                KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_operator,             Y_operator,           KC_D, TX_NONE,
    LANG_operator,          AC_LANG(operator)                             },
  { L_private,              Y_private,            KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_protected,            Y_protected,          KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_public,               Y_public,             KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_reinterpret_cast,     Y_reinterpret_cast,   KC_D, TX_NONE,
    LANG_NEW_STYLE_CASTS,   AC_LANG(NEW_STYLE_CASTS)                      },
  { L_static_cast,          Y_static_cast,        KC_D, TX_NONE,
    LANG_NEW_STYLE_CASTS,   AC_LANG(NEW_STYLE_CASTS)                      },
  { L_template,             Y_template,           KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_this,                 Y_this,               KC_D, TS_this,
    LANG_CPP_ANY,           AC_LANG(EXPLICIT_OBJ_PARAM_DECLS)             },
  { L_throw,                Y_throw,              KC_D, TS_throw,
    LANG_CPP_ANY,           AC_LANG(throw)                                },
  { L_true,                 Y_true,               KC_D, TS_noexcept,
    LANG_true_false,        AC_LANG(true_false)                           },
  { L_try,                  Y_try,                KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_typeid,               Y_typeid,             KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(NONE)                                 },
  { L_typename,             Y_typename,           KC_D, TX_NONE,
    LANG_CPP_ANY,           AC_LANG(CPP_ANY)                              },
  { L_using,                Y_using,              KC_D, TS_typedef,
    LANG_CPP_ANY,           AC_LANG(using_DECLS)                          },
  { L_virtual,              Y_virtual,            KC_D, TS_virtual,
    LANG_virtual,           AC_LANG(virtual)                              },

  // C++11
  { L_alignas,              Y_alignas,            KC_D, TX_NONE,
    LANG_alignas,           AC_LANG(ALIGNMENT)                            },
  { L_alignof,              Y_alignof,            KC_D, TX_NONE,
    LANG_alignof,           AC_LANG(NONE)                                 },
  { L_auto,                 Y_auto_TYPE,          KC_D, TB_auto,
    LANG_auto_TYPE,         AC_LANG(auto_TYPE)                            },
  { L_constexpr,            Y_constexpr,          KC_D, TS_constexpr,
    LANG_constexpr,         AC_LANG(constexpr)                            },
  { L_decltype,             Y_decltype,           KC_D, TX_NONE,
    LANG_decltype,          AC_LANG(NONE)                                 },
  { L_final,                Y_final,              KC_F, TS_final,
    LANG_final,             AC_LANG(final)                                },
  { L_noexcept,             Y_noexcept,           KC_D, TS_noexcept,
    LANG_noexcept,          AC_LANG(noexcept)                             },
  { L_override,             Y_override,           KC_F, TS_override,
    LANG_override,          AC_LANG(override)                             },

  // C11 & C++11
  { L_char16_t,             Y_char16_t,           KC_D, TB_char16_t,
    LANG_char16_32_t,       AC_LANG(char16_32_t)                          },
  { L_char32_t,             Y_char32_t,           KC_D, TB_char32_t,
    LANG_char16_32_t,       AC_LANG(char16_32_t)                          },

  // C23
  { L__BitInt,              Y__BitInt,            KC_D, TB__BitInt,
    LANG__BitInt,           AC_LANG(_BitInt)                              },
  { L_typeof,               Y_typeof,             KC_D, TX_NONE,
    LANG_typeof,            AC_LANG(typeof)                               },
  { L_typeof_unqual,        Y_typeof_unqual,      KC_D, TX_NONE,
    LANG_typeof,            AC_LANG(typeof)                               },

  // C23 & C++11
  { L_nullptr,              Y_nullptr,            KC_D, TX_NONE,
    LANG_nullptr,           AC_LANG(NONE)                                 },
  { L_static_assert,        Y_static_assert,      KC_D, TX_NONE,
    LANG_static_assert,     AC_LANG(NONE)                                 },
  { L_thread_local,         Y_thread_local,       KC_D, TS_thread_local,
    LANG_thread_local,      AC_LANG(THREAD_LOCAL_STORAGE)                 },

  // C23 & C++20
  { L_char8_t,              Y_char8_t,            KC_D, TB_char8_t,
    LANG_char8_t,           AC_LANG(char8_t)                              },

  // C++20
  { L_concept,              Y_concept,            KC_D, TX_NONE,
    LANG_concept,           AC_LANG(NONE)                                 },
  { L_consteval,            Y_consteval,          KC_D, TS_consteval,
    LANG_consteval,         AC_LANG(consteval)                            },
  { L_constinit,            Y_constinit,          KC_D, TS_constinit,
    LANG_constinit,         AC_LANG(constinit)                            },
  { L_co_await,             Y_co_await,           KC_D, TX_NONE,
    LANG_COROUTINES,        AC_LANG(COROUTINES)                           },
  { L_co_return,            Y_co_return,          KC_D, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_co_yield,             Y_co_yield,           KC_D, TX_NONE,
    LANG_COROUTINES,        AC_LANG(NONE)                                 },
  { L_export,               Y_export,             KC_D, TS_export,
    LANG_export,            AC_LANG(export)                               },
  { L_requires,             Y_requires,           KC_D, TX_NONE,
    LANG_concept,           AC_LANG(NONE)                                 },

  // Alternative tokens
  { L_and,                  Y_AMPER_AMPER,        KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_and_eq,               Y_AMPER_EQUAL,        KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_bitand,               '&',                  KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_bitor,                '|',                  KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_compl,                '~',                  KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_not,                  '!',                  KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_not_eq,               Y_EXCLAM_EQUAL,       KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_or,                   Y_PIPE_PIPE,          KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_or_eq,                Y_PIPE_EQUAL,         KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_xor,                  '^',                  KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },
  { L_xor_eq,               Y_CARET_EQUAL,        KC_D, TX_NONE,
    LANG_ALT_TOKENS,        AC_LANG(ALT_TOKENS)                           },

  // C23 attributes
  { L_reproducible,         Y_reproducible,       KC_A, TA_reproducible,
    LANG_reproducible,      AC_LANG(reproducible)                         },
  { L_unsequenced,          Y_unsequenced,        KC_A, TA_unsequenced,
    LANG_unsequenced,       AC_LANG(unsequenced)                          },

  // C23 & C++11 attributes
  { L_noreturn,             Y_noreturn,           KC_D, TA_noreturn,
    LANG_noreturn,          AC_LANG(noreturn)                             },

  // C++11 attributes
  { L_carries_dependency,   Y_carries_dependency, KC_A, TA_carries_dependency,
    LANG_carries_dependency,AC_LANG(carries_dependency)                   },

  // C23 & C++14 attributes
  { L_deprecated,           Y_deprecated,         KC_A, TA_deprecated,
    LANG_deprecated,        AC_LANG(deprecated)                           },
  { L___deprecated__,       Y_deprecated,         KC_A, TA_deprecated,
    LANG___deprecated__,    AC_LANG(__deprecated__)                       },

  // C23 & C++17 attributes
  { L_maybe_unused,         Y_maybe_unused,       KC_A, TA_maybe_unused,
    LANG_maybe_unused,      AC_LANG(maybe_unused)                         },
  { L___maybe_unused__,     Y_maybe_unused,       KC_A, TA_maybe_unused,
    LANG___maybe_unused__,  AC_LANG(__maybe_unused__)                     },
  { L_nodiscard,            Y_nodiscard,          KC_A, TA_nodiscard,
    LANG_nodiscard,         AC_LANG(nodiscard)                            },
  { L___nodiscard__,        Y_nodiscard,          KC_A, TA_nodiscard,
    LANG___nodiscard__,     AC_LANG(__nodiscard__)                        },

  // C++20 attributes
#if 0                       // Not implemented because:
  { L_ASSERT,               // + These use arbitrary expressions that require
  { L_ENSURES,              //   being able to parse them -- which is a lot of
  { L_EXPECTS,              //   work for little benefit.

  { L_LIKELY,               // + These are only for statements, not
  { L_UNLIKELY,             //   declarations.
#endif
  { L_no_unique_address,    Y_no_unique_address,  KC_A, TA_no_unique_address,
    LANG_no_unique_address, AC_LANG(no_unique_address)                    },

  // Embedded C extensions
  { L_EMC__Accum,           Y_EMC__Accum,         KC_D, TB_EMC__Accum,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },
  { L_EMC__Fract,           Y_EMC__Fract,         KC_D, TB_EMC__Fract,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },
  { L_EMC__Sat,             Y_EMC__Sat,           KC_D, TB_EMC__Sat,
    LANG_C_99_EMC,          AC_LANG(C_99)                                 },

  // Unified Parallel C extensions
  { L_UPC_relaxed,          Y_UPC_relaxed,        KC_D, TS_UPC_relaxed,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },
  { L_UPC_shared,           Y_UPC_shared,         KC_D, TS_UPC_shared,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },
  { L_UPC_strict,           Y_UPC_strict,         KC_D, TS_UPC_strict,
    LANG_C_99_UPC,          AC_LANG(C_99)                                 },

  // GNU extensions
  { L_GNU___attribute__,    Y_GNU___attribute__,  KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___auto_type,      Y_auto_TYPE,          KC_D, TB_auto,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___complex,        Y__Complex,           KC_D, TB__Complex,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___complex__,      Y__Complex,           KC_D, TB__Complex,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___const,          Y_const,              KC_D, TS_const,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___inline,         Y_inline,             KC_D, TS_inline,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___inline__,       Y_inline,             KC_D, TS_inline,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___restrict,       Y_GNU___restrict,     KC_D, TS_restrict,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___restrict__,     Y_GNU___restrict,     KC_D, TS_restrict,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___signed,         Y_signed,             KC_D, TB_signed,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___signed__,       Y_signed,             KC_D, TB_signed,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___thread,         Y_thread_local,       KC_D, TS_thread_local,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___typeof__,       Y_typeof,             KC_D, TX_NONE,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___volatile,       Y_volatile,           KC_D, TS_volatile,
    LANG_ANY,               AC_LANG(ANY)                                  },
  { L_GNU___volatile__,     Y_volatile,           KC_D, TS_volatile,
    LANG_ANY,               AC_LANG(ANY)                                  },

  // Apple extensions
  { L_Apple___block,        Y_Apple___block,      KC_D, TS_APPLE___block,
    LANG_APPLE___block,     AC_LANG(APPLE___block)                        },

  // Microsoft extensions
  { L_MSC__asm,             Y_asm,                KC_D, TX_NONE,
    LANG_MSC_CALL_CONVS,    AC_LANG(NONE)                                 },
  { L_MSC___asm,            Y_asm,                KC_D, TX_NONE,
    LANG_MSC_CALL_CONVS,    AC_LANG(NONE)                                 },
  { L_MSC__cdecl,           Y_MSC___cdecl,        KC_D, TA_MSC___cdecl,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___cdecl,          Y_MSC___cdecl,        KC_D, TA_MSC___cdecl,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___clrcall,        Y_MSC___clrcall,      KC_D, TA_MSC___clrcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__declspec,        Y_MSC___declspec,     KC_D, TX_NONE,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___declspec,       Y_MSC___declspec,     KC_D, TX_NONE,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__fastcall,        Y_MSC___fastcall,     KC_D, TA_MSC___fastcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___fastcall,       Y_MSC___fastcall,     KC_D, TA_MSC___fastcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__forceinline,     Y_inline,             KC_D, TS_inline,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___forceinline,    Y_inline,             KC_D, TS_inline,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__inline,          Y_inline,             KC_D, TS_inline,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__restrict,        Y_restrict,           KC_D, TS_restrict,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__stdcall,         Y_MSC___stdcall,      KC_D, TA_MSC___stdcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___stdcall,        Y_MSC___stdcall,      KC_D, TA_MSC___stdcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___thiscall,       Y_MSC___thiscall,     KC_D, TA_MSC___thiscall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC__vectorcall,      Y_MSC___vectorcall,   KC_D, TA_MSC___vectorcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
  { L_MSC___vectorcall,     Y_MSC___vectorcall,   KC_D, TA_MSC___vectorcall,
    LANG_MSC_CALL_CONVS,    AC_LANG(MSC_CALL_CONVS)                       },
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Compares two \ref c_keyword objects.
 *
 * @param i_ck The first \ref c_keyword to compare.
 * @param j_ck The second \ref c_keyword to compare.
 * @return @return Returns a number less than 0, 0, or greater than 0 if \a
 * i_ck is less than, equal to, or greater than \a j_ck, respectively.
 */
NODISCARD
static int c_keyword_cmp( c_keyword_t const *i_ck, c_keyword_t const *j_ck ) {
  return strcmp( i_ck->literal, j_ck->literal );
}

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_keyword_find( char const *literal, c_lang_id_t lang_ids,
                                   c_keyword_ctx_t kw_ctx ) {
  assert( literal != NULL );
  assert( lang_ids != LANG_NONE );

  // the list is small, so linear search is good enough
  FOREACH_ARRAY_ELEMENT( c_keyword_t, ck, C_KEYWORDS ) {
    int const cmp = strcmp( literal, ck->literal );
    if ( cmp > 0 )
      continue;
    if ( cmp < 0 )                      // the array is sorted
      break;

    if ( (ck->lang_ids & lang_ids) == LANG_NONE )
      continue;

    if ( is_gibberish_to_english() &&
         ck->kw_ctx != C_KW_CTX_DEFAULT && kw_ctx != ck->kw_ctx ) {
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

    return ck;
  } // for

  return NULL;
}

c_keyword_t const* c_keyword_next( c_keyword_t const *ck ) {
  return ARRAY_NEXT( C_KEYWORDS, ck );
}

void c_keywords_init( void ) {
  ASSERT_RUN_ONCE();
  qsort(                                // so we can stop the search early
    C_KEYWORDS, ARRAY_SIZE( C_KEYWORDS ), sizeof( C_KEYWORDS[0] ),
    POINTER_CAST( qsort_cmp_fn_t, &c_keyword_cmp )
  );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
