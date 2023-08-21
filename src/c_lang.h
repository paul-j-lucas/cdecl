/*
**      cdecl -- C gibberish translator
**      src/c_lang.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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

#ifndef cdecl_c_lang_H
#define cdecl_c_lang_H

/**
 * @file
 * Declares macros, types, constants, and functions for C/C++ language
 * versions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "options.h"                    /* for opt_lang */
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_LANG_H_INLINE
# define C_LANG_H_INLINE _GL_INLINE
#endif /* C_LANG_H_INLINE */

/// @endcond

/**
 * @defgroup c-lang-group C/C++ Language Versions
 * Macros, types, constants, and functions for C/C++ language versions.
 *
 * @note @anchor c-lang-order Despite the year of standardization, all versions
 * of C++ are considered "newer" than all versions of C.  However, this isn't a
 * problem since **cdecl** is only ever parsing either (any version of) C or
 * (any version of) C++ at any given time.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Convenience macro for iterating over all languages.
 *
 * @param VAR The \ref c_lang loop variable.
 *
 * @sa c_lang_next()
 */
#define FOREACH_LANG(VAR) \
  for ( c_lang_t const *VAR = NULL; (VAR = c_lang_next( VAR )) != NULL; )

// languages supported
#define LANG_NONE     0u                /**< No languages. */
#define LANG_ANY      (~LANGX_MASK)     /**< Any supported language. */

#define LANG_C_OLD    LANG_C_KNR        /**< Oldest supported C language. */
#define LANG_C_KNR    (1u << 0)         /**< K&R (pre-ANSI) C. */
#define LANG_C_89     (1u << 1)         /**< C 89 (first ANSI C). */
#define LANG_C_95     (1u << 2)         /**< C 95. */
#define LANG_C_99     (1u << 3)         /**< C 99. */
#define LANG_C_11     (1u << 4)         /**< C 11. */
#define LANG_C_17     (1u << 5)         /**< C 17. */
#define LANG_C_23     (1u << 6)         /**< C 23. */
#define LANG_C_NEW    LANG_C_23         /**< Newest supported C language. */
#define LANG_C_ANY    LANG_MAX(C_NEW)   /**< Any C language. */

#define LANG_CPP_OLD  LANG_CPP_98       /**< Oldest supported C++ language. */
#define LANG_CPP_98   (1u << 9)         /**< C++ 98. */
#define LANG_CPP_03   (1u << 10)        /**< C++ 03. */
#define LANG_CPP_11   (1u << 11)        /**< C++ 11. */
#define LANG_CPP_14   (1u << 12)        /**< C++ 14. */
#define LANG_CPP_17   (1u << 13)        /**< C++ 17. */
#define LANG_CPP_20   (1u << 14)        /**< C++ 20. */
#define LANG_CPP_23   (1u << 15)        /**< C++ 23. */
#define LANG_CPP_NEW  LANG_CPP_23       /**< Newest supported C++ language. */
#define LANG_CPP_ANY  LANG_MASK_CPP     /**< Any C++ language. */

/** Language eXtensions for Embedded C. */
#define LANGX_EMC     (1u << 7)

/** Language eXtensions for Unified Parallel C. */
#define LANGX_UPC     (1u << 8)

/**
 * Embedded C, or more formally, _Programming languages - C - Extensions to
 * support embedded processors_, ISO/IEC TR&nbsp;18037:2008, which is based on
 * C99, ISO/IEC&nbsp;9899:1999.
 *
 * @note This is not a distinct language in **cdecl**, i.e., the user can't set
 * the language to "Embedded C" specifically.  It's used to mark keywords as
 * being available only in the Embedded C extensions to C99 instead of "plain"
 * C99 so that if a user does:
 *
 *      cdecl> declare _Sat as int
 *      9: warning: "_Sat" is a keyword in C99 (with Embedded C extensions)
 *
 * in a language other than C99, they'll get a warning.
 *
 * @sa [Information Technology — Programming languages - C - Extensions to support embedded processors](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1169.pdf)
 */
#define LANG_C_99_EMC (LANG_C_99 | LANGX_EMC)

/**
 * _Unified Parallel C_, which is based on C99, ISO/IEC&nbsp;9899:1999.
 *
 * @note This is not a distinct language in **cdecl**, i.e., the user can't set
 * the language to "Unified Parallel C" specifically.  It's used to mark
 * keywords as being available only in the Unified Parallel C extensions to C99
 * instead of "plain" C99 so that if a user does:
 *
 *      cdecl> declare shared as int
 *      9: warning: "shared" is a keyword in C99 (with Unified Parallel C extensions)
 *
 * in a language other than C99, they'll get a warning.
 *
 * @sa [Unified Parallel C](http://upc-lang.org/)
 */
#define LANG_C_99_UPC (LANG_C_99 | LANGX_UPC)

// bit masks
#define LANG_MASK_C   0x01FFu           /**< C languages bitmask. */
#define LANG_MASK_CPP 0xFE00u           /**< C++ languages bitmask. */
#define LANGX_MASK    0x0180u           /**< Language extensions bitmask. */

/**
 * Expands into \a LANG_MACRO for autocompletion only if GNU **readline**(3) is
 * compiled in; nothing if not.
 *
 * @param LANG_MACRO A `LANG_*` macro without the `LANG_` prefix.
 */
#ifdef WITH_READLINE
# define AC_LANG(LANG_MACRO)      LANG_##LANG_MACRO
#else
# define AC_LANG(LANG_MACRO)      /* nothing */
#endif /* WITH_READLINE */

/**
 * All languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_MAX()
 * @sa #LANG_MIN()
 * @sa #LANG_RANGE()
 */
#define LANG_MAX(L)               (BITS_LE( LANG_##L ) & ~LANGX_MASK)

/**
 * All languages \a L and later.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_MIN()
 * @sa #LANG_MAX()
 * @sa #LANG_RANGE()
 */
#define LANG_MIN(L)               (BITS_GE( LANG_##L ) & ~LANGX_MASK)

/**
 * C-only languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_CPP_MAX()
 * @sa #LANG_C_MIN()
 * @sa #LANG_MAX()
 */
#define LANG_C_MAX(L)             LANG_MAX( C_##L )

/**
 * C-only languages \a L and later.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_CPP_MIN()
 * @sa #LANG_C_MAX()
 * @sa #LANG_MIN()
 */
#define LANG_C_MIN(L)             (LANG_MIN( C_##L ) & LANG_MASK_C)

/**
 * C++-only languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_CPP_MIN()
 * @sa #LANG_C_CPP_MAX()
 * @sa #LANG_MAX()
 */
#define LANG_CPP_MAX(L)           (LANG_MAX( CPP_##L ) & LANG_MASK_CPP)

/**
 * C++-only languages \a L and later.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_CPP_MIN()
 * @sa #LANG_CPP_MAX()
 * @sa #LANG_MIN()
 */
#define LANG_CPP_MIN(L)           LANG_MIN( CPP_##L )

/**
 * C-only languages up to and including \a LC; and C++-only languages up to and
 * including \a LCPP.
 *
 * @param LC The C language _without_ the `LANG_` prefix.
 * @param LCPP The C++ language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_MAX()
 * @sa #LANG_C_CPP_MIN()
 * @sa #LANG_CPP_MAX()
 */
#define LANG_C_CPP_MAX(LC,LCPP)   (LANG_C_MAX(LC) | LANG_CPP_MAX(LCPP))

/**
 * C-only languages \a LC and later; and C++-only languages \a LCPP and later.
 *
 * @param LC The C language _without_ the `LANG_` prefix.
 * @param LCPP The C++ language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_C_CPP_MAX()
 * @sa #LANG_C_MIN()
 * @sa #LANG_CPP_MIN()
 */
#define LANG_C_CPP_MIN(LC,LCPP)   (LANG_C_MIN(LC) | LANG_CPP_MIN(LCPP))

/**
 * All languages between \a LMIN and \a LMAX, inclusive.
 *
 * @param LMIN The minimum language _without_ the `LANG_` prefix.
 * @param LMAX The maximum language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_MAX()
 * @sa #LANG_MIN()
 */
#define LANG_RANGE(LMIN,LMAX) \
  ((BITS_GE( LANG_##LMIN ) & BITS_LE( LANG_##LMAX )) & ~LANGX_MASK)

/**
 * Convenience macro for specifying a constant array of \ref c_lang_lit.
 *
 * @param ... The array of \ref c_lang_lit elements.
 */
#define C_LANG_LIT(...)           (c_lang_lit_t const[]){ __VA_ARGS__ }

/**
 * Shorthand for calling c_lang_which() with a `LANG_*` macro.
 *
 * @param LANG_MACRO A `LANG_*` macro without the `LANG_` prefix.
 * @return Returns the value of c_lang_which().
 */
#define C_LANG_WHICH(LANG_MACRO)  c_lang_which( LANG_##LANG_MACRO )

/**
 * Shorthand for the common case of getting whether the current language is
 * among the languages specified by \a LANG_MACRO.
 *
 * @param LANG_MACRO A `LANG_*` macro without the `LANG_` prefix.
 * @return Returns `true` only if the current language is among the languages
 * specified by \a LANG_MACRO.
 *
 * @sa opt_lang_is_any()
 */
#define OPT_LANG_IS(LANG_MACRO)   opt_lang_is_any( LANG_##LANG_MACRO )

///////////////////////////////////////////////////////////////////////////////

/**
 * Languages the `_Alignas` keyword is supported in.
 *
 * @sa #LANG_alignas
 */
#define LANG__Alignas                   LANG_C_MIN(11)

/**
 * Languages the `alignas` keyword is supported in.
 *
 * @sa #LANG__Alignas
 */
#define LANG_alignas                    LANG_C_CPP_MIN(23,11)

/**
 * Lanuages aligned storage is supported in.
 *
 * @sa #LANG__Alignas
 * @sa #LANG_alignas
 */
#define LANG_ALIGNMENT                  (LANG__Alignas | LANG_alignas)

/**
 * Languages the `_Alignof` keyword is supported in.
 *
 * @sa #LANG_alignof
 */
#define LANG__Alignof                   LANG_C_MIN(11)

/**
 * Languages the `alignof` keyword is supported in.
 *
 * @sa #LANG__Alignof
 */
#define LANG_alignof                    LANG_C_CPP_MIN(23,11)

/**
 * Languages "alternative tokens" are supported in.
 */
#define LANG_ALT_TOKENS                 LANG_MIN(C_95)

/**
 * Languages Apple's `__block` keyword is supported in.
 */
#define LANG_APPLE___block              LANG_MIN(C_89)

/**
 * Languages the `asm` keyword is supported in.
 */
#define LANG_asm                        LANG_MIN(C_89)

/**
 * Languages the `_Atomic` keyword is supported in.
 */
#define LANG__Atomic                    LANG_C_CPP_MIN(11,23)

/**
 * Languages `[[`...`]]` attribute syntax is supported in.
 */
#define LANG_ATTRIBUTES                 LANG_C_CPP_MIN(23,11)

/**
 * Languages the `auto` keyword as a storage class is supported in.
 *
 * @sa #LANG_auto_TYPE
 */
#define LANG_auto_STORAGE               LANG_C_CPP_MAX(17,03)

/**
 * Languages the `auto` keyword as a type is supported in.
 *
 * @sa #LANG_auto_STORAGE
 */
#define LANG_auto_TYPE                  LANG_C_CPP_MIN(23,11)

/**
 * Languages the `auto` keyword for parameters are supported in.
 */
#define LANG_auto_PARAMETER             LANG_CPP_MIN(20)

/**
 * Languages the `auto` keyword for function return types are supported in.
 */
#define LANG_auto_RETURN_TYPE           LANG_CPP_MIN(14)

/**
 * Languages the `_BitInt` keyword is supported in.
 */
#define LANG__BitInt                    LANG_C_MIN(23)

/**
 * Languages the `_Bool` keyword is supported in.
 *
 * @sa #LANG_bool
 * @sa #LANG_BOOL_TYPE
 * @sa #LANG_true_false
 */
#define LANG__Bool                      LANG_C_MIN(99)

/**
 * Languages the `bool` keyword is supported in.
 *
 * @note Even though `bool` as a keyword isn't supported in C until C23, we
 * support it starting in C99 due to the `bool` macro in `stdbool.h`.
 *
 * @sa #LANG__Bool
 * @sa #LANG_BOOL_TYPE
 * @sa #LANG_true_false
 */
#define LANG_bool                       LANG_MIN(C_23)

/**
 * Languages a Boolean type is supported in.
 *
 * @sa #LANG__Bool
 * @sa #LANG_bool
 * @sa #LANG_true_false
 */
#define LANG_BOOL_TYPE                  (LANG__Bool | LANG_bool)

/**
 * Lanuages capturing `*this` in lambdas is supported in.
 */
#define LANG_CAPTURE_STAR_THIS          LANG_CPP_MIN(17)

/**
 * Languages the `carries_dependency` attribute is supported in.
 */
#define LANG_carries_dependency         LANG_CPP_MIN(11)

/**
 * Languages the `char16_t` and `char32_t` keywords are supported in.
 *
 * @sa #LANG_char8_t
 */
#define LANG_char16_32_t                LANG_C_CPP_MIN(11,11)

/**
 * Languages the `char8_t` keyword is supported in.
 *
 * @sa #LANG_char16_32_t
 */
#define LANG_char8_t                    LANG_C_CPP_MIN(23,20)

/**
 * Languages the `_Complex` keyword is supported in.
 *
 * @sa #LANG__Imaginary
 */
#define LANG__Complex                   LANG_C_MIN(99)

/**
 * Languages "concepts" are supported in.
 */
#define LANG_CONCEPTS                   LANG_CPP_MIN(20)

/**
 * Languages "coroutines" are supported in.
 */
#define LANG_COROUTINES                 LANG_CPP_MIN(20)

/**
 * Languages the `const` keyword is supported in.
 *
 * @sa #LANG_volatile
 */
#define LANG_const                      LANG_MIN(C_89)

/**
 * Languages the `consteval` keyword is supported in.
 */
#define LANG_consteval                  LANG_CPP_MIN(20)

/**
 * Languages the `constexpr` keyword is supported in.
 *
 * @sa #LANG_constexpr_RETURN_TYPE
 */
#define LANG_constexpr                  LANG_C_CPP_MIN(23,11)

/**
 * Languages the `constexpr` keyword for return types are supported in.
 *
 * @sa #LANG_constexpr
 */
#define LANG_constexpr_RETURN_TYPE      LANG_CPP_MIN(14)

/**
 * Languages the `constinit` keyword is supported in.
 */
#define LANG_constinit                  LANG_CPP_MIN(20)

/**
 * Languages `class`, `struct`, or `union` for return types are supported.
 */
#define LANG_CSU_RETURN_TYPE            LANG_MIN(C_89)

/**
 * Languages the `decltype` keyword is supported in.
 */
#define LANG_decltype                   LANG_CPP_MIN(11)

/**
 * Languages the `default` and `delete` keywords for functions are supported
 * in.
 */
#define LANG_default_delete_FUNC        LANG_CPP_MIN(11)

/**
 * Languages the `default` keyword for relational operators are supported in.
 */
#define LANG_default_RELOPS             LANG_CPP_MIN(20)

/**
 * Languages the `__deprecated__` attribute is supported in.
 *
 * @sa #LANG_deprecated
 */
#define LANG___deprecated__             LANG_C_MIN(23)

/**
 * Languages the `deprecated` attribute is supported in.
 *
 * @sa #LANG___deprecated__
 */
#define LANG_deprecated                 LANG_C_CPP_MIN(23,14)

/**
 * Languages "digraphs" are supported in.
 */
#define LANG_DIGRAPHS                   LANG_MIN(C_95)

/**
 * Languages the `enum` keyword is supported in.
 */
#define LANG_enum                       LANG_MIN(C_89)

/**
 * Languages `enum` bitfields are supported in.
 */
#define LANG_enum_BITFIELDS             LANG_CPP_ANY

/**
 * Languages `enum class` is supported in.
 */
#define LANG_enum_class                 LANG_CPP_MIN(11)

/**
 * Languages explicit `this` parameter supported in.
 */
#define LANG_EXPLICIT_OBJ_PARAM_DECL    LANG_CPP_MIN(23)

/**
 * Languages `explicit` user-defined conversion operators are supported in.
 */
#define LANG_explicit_USER_DEF_CONV     LANG_CPP_MIN(11)

/**
 * Languages the `export` keyword is supported in.
 */
#define LANG_export                     LANG_CPP_MIN(20)

/**
 * Languages the `final` keyword is supported in.
 *
 * @sa #LANG_override
 */
#define LANG_final                      LANG_CPP_MIN(11)

/**
 * Languages fixed type `enum`s are supported in.
 */
#define LANG_FIXED_TYPE_enum            LANG_C_CPP_MIN(23,11)

/**
 * Languages the `_Generic` keyword is supported in.
 */
#define LANG__Generic                   LANG_C_MIN(11)

/**
 * Languages the `_Imaginary` keyword is supported in.
 *
 * @sa #LANG__Complex
 */
#define LANG__Imaginary                 LANG__Complex

/**
 * Languages where `int` is implicit.
 */
#define LANG_IMPLICIT_int               LANG_MAX(C_95)

/**
 * Languages the `inline` keyword is supported in.
 *
 * @sa #LANG_inline_namespace
 * @sa #LANG_inline_VARIABLE
 */
#define LANG_inline                     LANG_MIN(C_99)

/**
 * Languages `inline namespace` is supported in.
 *
 * @sa #LANG_inline
 * @sa #LANG_inline_VARIABLE
 */
#define LANG_inline_namespace           LANG_CPP_MIN(11)

/**
 * Languages `inline` variables are supported in.
 *
 * @sa #LANG_inline
 * @sa #LANG_inline_namespace
 */
#define LANG_inline_VARIABLE            LANG_CPP_MIN(17)

/**
 * Languages K&R style function definitions are supported in.
 *
 * @sa #LANG_PROTOTYPES
 */
#define LANG_KNR_FUNC_DEFINITION        LANG_C_MAX(17)

/**
 * Lanuages lambdas are supported in.
 */
#define LANG_LAMBDA                     LANG_CPP_MIN(11)

/**
 * Lanuages the `long double` type is supported in.
 */
#define LANG_long_double                LANG_MIN(C_89)

/**
 * Lanuages the `long float` type is supported in.
 */
#define LANG_long_float                 LANG_C_KNR

/**
 * Lanuages the `long long` type is supported in.
 */
#define LANG_long_long                  LANG_MIN(C_99)

/**
 * Languages the `__maybe_unused__` attribute is supported in.
 *
 * @sa #LANG_maybe_unused
 */
#define LANG___maybe_unused__           LANG_C_MIN(23)

/**
 * Languages the `maybe_unused` attribute is supported in.
 *
 * @sa #LANG___maybe_unused__
 */
#define LANG_maybe_unused               LANG_C_CPP_MIN(23,17)

/**
 * Languages Microsoft extensions are supported in.
 */
#define LANG_MSC_EXTENSIONS             LANG_MIN(C_89)

/**
 * Languages nested `namespace` declarations  are supported in.
 */
#define LANG_NESTED_namespace           LANG_CPP_MIN(17)

/**
 * Languages the `__nodiscard__` attribute is supported in.
 *
 * @sa #LANG_nodiscard
 */
#define LANG___nodiscard__              LANG_C_MIN(23)

/**
 * Languages the `nodiscard` attribute is supported in.
 *
 * @sa #LANG___nodiscard__
 */
#define LANG_nodiscard                  LANG_C_CPP_MIN(23,17)

/**
 * Languages the `noexcept` keyword is supported in.
 */
#define LANG_noexcept                   LANG_CPP_MIN(11)

/**
 * Languages the `_Noreturn` keyword is supported in.
 *
 * @sa #LANG_noreturn
 * @sa #LANG___noreturn__
 * @sa #LANG_NONRETURNING_FUNC
 */
#define LANG__Noreturn                  LANG_C_MIN(11)

/**
 * Languages the `noreturn` keyword is supported in.
 *
 * @sa #LANG__Noreturn
 * @sa #LANG___noreturn__
 * @sa #LANG_NONRETURNING_FUNC
 */
#define LANG_noreturn                   LANG_C_CPP_MIN(23,11)

/**
 * Languages the `__noreturn__` attribute is supported in.
 *
 * @sa #LANG__Noreturn
 * @sa #LANG_noreturn
 * @sa #LANG_NONRETURNING_FUNC
 */
#define LANG___noreturn__               LANG_C_MIN(23)

/**
 * Languages that support non-returning functions.
 *
 * @sa #LANG__Noreturn
 * @sa #LANG___noreturn__
 * @sa #LANG_noreturn
 */
#define LANG_NONRETURNING_FUNC          (LANG__Noreturn | LANG_noreturn)

/**
 * Languages the `no_unique_address` attribute is supported in.
 *
 * @sa #LANG_ATTRIBUTES
 */
#define LANG_no_unique_address          LANG_CPP_MIN(20)

/**
 * Languages the `nullptr` keyword is supported in.
 */
#define LANG_nullptr                    LANG_C_CPP_MIN(23,11)

/**
 * Languages the `override` keyword is supported in.
 *
 * @sa #LANG_final
 */
#define LANG_override                   LANG_CPP_MIN(11)

/**
 * Languages function prototypes are supported in.
 *
 * @sa #LANG_KNR_FUNC_DEFINITION
 */
#define LANG_PROTOTYPES                 LANG_MIN(C_89)

/**
 * Languages qualified array parameters are supported in.
 */
#define LANG_QUALIFIED_ARRAY            LANG_C_MIN(99)

/**
 * Languages reference qualified functions are supported in.
 */
#define LANG_REF_QUALIFIED_FUNC         LANG_CPP_MIN(11)

/**
 * Languages `register` variables are supported in.
 */
#define LANG_register                   LANG_MAX(CPP_14)

/**
 * Languages the `reproducible` attribute is supported in.
 */
#define LANG_reproducible               LANG_C_MIN(23)

/**
 * Languages the `restrict` keyword is supported in.
 */
#define LANG_restrict                   LANG_C_MIN(99)

/**
 * Languages rvalue references are supported in.
 */
#define LANG_RVALUE_REFERENCE           LANG_CPP_MIN(11)

/**
 * Languages the `signed` keyword is supported in.
 */
#define LANG_signed                     LANG_MIN(C_89)

/**
 * Languages the `_Static_assert` keyword is supported in.
 *
 * @sa #LANG_static_assert
 */
#define LANG__Static_assert             LANG_C_MIN(11)

/**
 * Languages the `static_assert` keyword is supported in.
 *
 * @sa #LANG__Static_assert
 */
#define LANG_static_assert              LANG_C_CPP_MIN(23,11)

/**
 * Languages `operator()` can be a `static` member.
 */
#define LANG_STATIC_OP_PARENS           LANG_CPP_MIN(23)

/**
 * Languages the `_Thread_local` keyword is supported in.
 *
 * @sa #LANG_thread_local
 * @sa #LANG_THREAD_LOCAL_STORAGE
 */
#define LANG__Thread_local              LANG_C_MIN(11)

/**
 * Languages the `thread_local` keyword is supported in.
 *
 * @sa #LANG__Thread_local
 * @sa #LANG_THREAD_LOCAL_STORAGE
 */
#define LANG_thread_local               LANG_C_CPP_MIN(23,11)

/**
 * Language thread-local storage is supported in.
 *
 * @sa #LANG__Thread_local
 * @sa #LANG_thread_local
 */
#define LANG_THREAD_LOCAL_STORAGE       (LANG__Thread_local | LANG_thread_local)

/**
 * Languages `throw` with dynamic exception specifications are supported in.
 */
#define LANG_throw                      LANG_CPP_MAX(14)

/**
 * Languages trailing return types are supported in.
 */
#define LANG_TRAILING_RETURN_TYPE       LANG_CPP_MIN(11)

/**
 * Languages "trigraphs" are supported in.
 */
#define LANG_TRIGRAPHS \
  (LANG_RANGE(C_89,C_17) | LANG_CPP_MAX(14))

/**
 * Languages the `true` and `false` keywords are supported in.
 *
 * @sa #LANG__Bool
 * @sa #LANG_bool
 * @sa #LANG_BOOL_TYPE
 */
#define LANG_true_false                 LANG_MIN(C_23)

/**
 * Languages `typeof` and `typeof_unqual` are supported in.
 */
#define LANG_typeof                     LANG_C_MIN(23)

/**
 * Languages the `unsequenced` attribute is supported in.
 */
#define LANG_unsequenced                LANG_C_MIN(23)

/**
 * Lanuages the `unsigned char` type is supported in.
 */
#define LANG_unsigned_char              LANG_MIN(C_89)

/**
 * Lanuages the `unsigned long` type is supported in.
 */
#define LANG_unsigned_long              LANG_MIN(C_89)

/**
 * Lanuages the `unsigned short` type is supported in.
 */
#define LANG_unsigned_short             LANG_MIN(C_89)

/**
 * Languages user-defined literals are supported in.
 */
#define LANG_USER_DEFINED_LITERAL       LANG_CPP_MIN(11)

/**
 * Languages `using` declarations are supported in.
 */
#define LANG_using_DECLARATION          LANG_CPP_MIN(11)

/**
 * Languages that allow `...` as the only function parameter.
 */
#define LANG_VARIADIC_ONLY_PARAMETER    LANG_C_CPP_MIN(23,OLD)

/**
 * Languages variable length arrays are supported in.
 */
#define LANG_VLA                        LANG_C_MIN(99)

/**
 * Languages the `void` keyword is supported in.
 */
#define LANG_void                       LANG_MIN(C_89)

/**
 * Languages the `volatile` keyword is supported in.
 *
 * @sa #LANG_const
 */
#define LANG_volatile                   LANG_const

/**
 * Languages the `wchar_t` keyword is supported in.
 */
#define LANG_wchar_t                    LANG_MIN(C_95)

///////////////////////////////////////////////////////////////////////////////

/**
 * A mapping between a language name and its corresponding \ref c_lang_id_t.
 */
struct c_lang {
  char const   *name;                   ///< Language name.
  bool          is_alias;               ///< Alias for another language name?
  c_lang_id_t   lang_id;                ///< Language bit.
};
typedef struct c_lang c_lang_t;

/**
 * C/C++ language(s)/literal pairs: for the given language(s) only, use the
 * given literal.
 *
 * @remarks This allows different languages to use different literals, e.g.,
 * `_Noreturn` for C and `noreturn` for C++.
 *
 * @sa #C_LANG_LIT()
 */
struct c_lang_lit {
  c_lang_id_t   lang_ids;               ///< Language(s) literal is in.
  char const   *literal;                ///< The literal.
};
typedef struct c_lang_lit c_lang_lit_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets all the language(s) \a lang_id and \ref c-lang-order "newer".
 *
 * @param lang_id The language.  Exactly one language must be set.
 * @return Returns the bitwise-or of all language(s) \a lang_id and newer.
 *
 * @sa c_lang_oldest()
 * @sa c_lang_newer()
 * @sa c_lang_newest()
 */
NODISCARD C_LANG_H_INLINE
c_lang_id_t c_lang_and_newer( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( is_1_bit( lang_id ) );
  return BITS_GE( lang_id );
}

/**
 * Gets the "coarse" name of \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return
 *  + If \a lang_ids only contains any versions of C, returns `"C"`.
 *  + If \a lang_ids only contains any versions of C++, returns `"C++"`.
 *  + Otherwise returns NULL.
 *
 * @sa c_lang_name()
 */
NODISCARD
char const* c_lang_coarse_name( c_lang_id_t lang_ids );

/**
 * Gets the \ref c_lang_id_t corresponding to \a name.
 *
 * @param name The language name (case insensitive) to get the corresponding
 * \ref c_lang_id_t for.
 * @return Returns said language or #LANG_NONE if \a name doesn't correspond to
 * any supported language.
 */
NODISCARD
c_lang_id_t c_lang_find( char const *name );

/**
 * Gets whether \a lang_ids is any version of C.
 *
 * @param lang_ids The bitwise-or of language(s) to check.
 * @return Returns `true` only if any one of \a lang_ids is a version of C.
 *
 * @sa c_lang_is_cpp()
 * @sa c_lang_is_one()
 */
NODISCARD C_LANG_H_INLINE
bool c_lang_is_c( c_lang_id_t lang_ids ) {
  return (lang_ids & LANG_MASK_C) != LANG_NONE;
}

/**
 * Gets whether \a lang_ids is any version of C++.
 *
 * @param lang_ids The bitwise-of of language(s) to check.
 * @return Returns `true` only if any one of \a lang_ids is a version of C++.
 *
 * @sa c_lang_is_c()
 * @sa c_lang_is_one()
 */
NODISCARD C_LANG_H_INLINE
bool c_lang_is_cpp( c_lang_id_t lang_ids ) {
  return (lang_ids & LANG_MASK_CPP) != LANG_NONE;
}

/**
 * Gets whether \a lang_ids contains language(s) for only either C or C++, but
 * not both.
 *
 * @param lang_ids The bitwise-of of language(s) to check.
 * @return
 *  + If \a lang_ids contains any version of C and no version of C++, returns
 *    #LANG_C_ANY.
 *  + If \a lang_ids contains any version of C++ and no version of C, returns
 *    #LANG_CPP_ANY.
 *  + Otherwise returns #LANG_NONE.
 *
 * @sa c_lang_is_c()
 * @sa c_lang_is_cpp()
 */
NODISCARD
c_lang_id_t c_lang_is_one( c_lang_id_t lang_ids );

/**
 * Gets the literal appropriate for the current language.
 *
 * @param lang_lit A \ref c_lang_lit array.  The last element _must_ always
 * have a \ref c_lang_lit::lang_ids "lang_ids" value of #LANG_ANY.  If the
 * corresponding \ref c_lang_lit::literal "literal" value is NULL, it means
 * there is no appropriate literal for the current language.
 * @return Returns said literal or NULL if there is no appropriate literal for
 * the current language.
 */
NODISCARD
char const* c_lang_literal( c_lang_lit_t const *lang_lit );

/**
 * Gets the printable name of \a lang_id.
 *
 * @param lang_id The language to get the name of.  _Exactly one_ language
 * _must_ be set.
 * @return Returns said name.
 *
 * @sa c_lang_coarse_name()
 */
NODISCARD
char const* c_lang_name( c_lang_id_t lang_id );

/**
 * Gets the bitwise-or of language(s) that are \ref c-lang-order "newer than"
 * \a lang_id.
 *
 * @param lang_id The language.  _Exactly one_ language _must_ be set.
 * @return Returns the bitwise-or of languages \a lang_id or newer; or
 * #LANG_NONE if no language(s) are newer.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newest()
 * @sa c_lang_oldest()
 */
NODISCARD C_LANG_H_INLINE
c_lang_id_t c_lang_newer( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( is_1_bit( lang_id ) );
  return BITS_GT( lang_id );
}

/**
 * Gets the \ref c-lang-order "newest" language among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns said language.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newer()
 * @sa c_lang_oldest()
 */
NODISCARD C_LANG_H_INLINE
c_lang_id_t c_lang_newest( c_lang_id_t lang_ids ) {
  return ms_bit1_32( lang_ids & ~LANGX_MASK ) | (lang_ids & LANGX_MASK);
}

/**
 * Iterates to the next C/C++ language.
 *
 * @param lang A pointer to the previous language. For the first iteration,
 * NULL should be passed.
 * @return Returns the next C/C++ language or NULL for none.
 *
 * @sa #FOREACH_LANG()
 */
NODISCARD
c_lang_t const* c_lang_next( c_lang_t const *lang );

/**
 * Gets the \ref c-lang-order "oldest" language among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns said language.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newer()
 * @sa c_lang_newest()
 */
NODISCARD C_LANG_H_INLINE
c_lang_id_t c_lang_oldest( c_lang_id_t lang_ids ) {
  return ls_bit1_32( lang_ids & ~LANGX_MASK ) | (lang_ids & LANGX_MASK);
}

/**
 * Sets the current language and the corresponding prompt.
 *
 * @param lang_id The language to set.  _Exactly one_ language _must_ be set.
 */
void c_lang_set( c_lang_id_t lang_id );

/**
 * Gets a string specifying a language when a particular feature isn't, hasn't
 * been, or won't be legal unless, since, or until, if ever.
 *
 * @param lang_ids The bitwise-or of legal language(s).
 * @return
 *  + If \a lang_ids is #LANG_NONE, returns the empty string.
 *  + If \a lang_ids contains exactly one language:
 *      + If the current language is that language, returns the empty string;
 *      + Otherwise returns `" unless "` followed by the name of that language.
 *  + Otherwise:
 *      + If the current language is any version of C and \a lang_ids does not
 *        contain any version of C, returns `" in C"`.
 *      + If the current language is any version of C++ and \a lang_ids does
 *        not contain any version of C++, returns `" in C++"`.
 *      + If the current language is older than oldest language in \a lang_ids,
 *        returns `" until "` followed by the name of the oldest C language
 *        version (if the current language is any version of C) or the name of
 *        the oldest C++ language version (if the current language is any
 *        version of C++).
 *      + Otherwise returns `" since "` followed by the name of the newest C
 *        language version (if the current language is any version of C) or the
 *        name of the newest C++ language version (if the current language is
 *        any version of C++).
 *
 * @note The returned string is presumed to follow `"not supported"` (with no
 * trailing space), e.g., `"... not supported until C99"`.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa #C_LANG_WHICH()
 */
NODISCARD
char const* c_lang_which( c_lang_id_t lang_ids );

/**
 * Convenience function for checking whether \ref opt_lang is among \a
 * lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s) to check.
 * @return Returns `true` only if it is.
 *
 * @sa #OPT_LANG_IS()
 */
NODISCARD C_LANG_H_INLINE
bool opt_lang_is_any( c_lang_id_t lang_ids ) {
  return (opt_lang & lang_ids) != LANG_NONE;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_lang_H */
/* vim:set et sw=2 ts=2: */
