/*
**      cdecl -- C gibberish translator
**      src/c_lang.h
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

#ifndef cdecl_c_lang_H
#define cdecl_c_lang_H

/**
 * @file
 * Declares macros, types, constants, and functions for C/C++ language
 * versions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "options.h"                    /* for opt_lang_id */
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */

_GL_INLINE_HEADER_BEGIN
#ifndef C_LANG_H_INLINE
# define C_LANG_H_INLINE _GL_INLINE
#endif /* C_LANG_H_INLINE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup c-lang-vers-group C/C++ Language Versions
 * Macros, types, constants, and functions for C/C++ language versions.
 *
 * @remarks Languages can be bitwise-or'd together to represent a set of
 * languages. The macros #LANG_MIN(), #LANG_MAX(), #LANG_C_MIN(),
 * #LANG_C_MAX(), #LANG_CPP_MIN(), #LANG_CPP_MAX(), #LANG_C_CPP_MIN(),
 * #LANG_C_CPP_MAX(), #LANG_RANGE(), and #OPT_LANG_IS() can be used with these.
 *
 * @note @anchor c-lang-order Despite the year of standardization, all versions
 * of C++ are considered "newer" than all versions of C.  However, this isn't a
 * problem since **cdecl** is only ever parsing either (any version of) C or
 * (any version of) C++ at any given time.
 * @{
 */

/**
 * Convenience macro for iterating over all languages.
 *
 * @param VAR The \ref c_lang loop variable.
 *
 * @sa c_lang_next()
 */
#define FOREACH_LANG(VAR) \
  for ( c_lang_t const *VAR = NULL; (VAR = c_lang_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

/**
 * Language eXtensions for Embedded C.
 *
 * @sa #LANG_C_99_EMC
 * @sa #LANGX_MASK
 * @sa [Information Technology — Programming languages - C - Extensions to support embedded processors](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1169.pdf)
 */
#define LANGX_EMC     (1u << 0)

/**
 * Language eXtensions for Unified Parallel C.
 *
 * @sa #LANG_C_99_UPC
 * @sa #LANGX_MASK
 * @sa [Unified Parallel C](http://upc-lang.org/)
 */
#define LANGX_UPC     (1u << 1)

/**
 * Language eXtensions bitmask.
 *
 * @remarks The two currently supported language extensions, #LANGX_EMC and
 * #LANGX_UPC, together use two bits within a \ref c_lang_id_t.  For many
 * operations involving a \ref c_lang_id_t, the language extensions _must_ be
 * masked off first, for example:
 *
 *      lang_ids &= ~LANGX_MASK;
 *
 * @sa #LANG_C_99_EMC
 * @sa #LANG_C_99_UPC
 * @sa #LANGX_EMC
 * @sa #LANGX_UPC
 */
#define LANGX_MASK    (LANGX_EMC | LANGX_UPC)

///////////////////////////////////////////////////////////////////////////////

// languages supported
#define LANG_NONE     0u                /**< No languages. */
#define LANG_ANY      (~LANGX_MASK)     /**< Any supported language. */

#define LANG_C_OLD    LANG_C_KNR        /**< Oldest supported C language. */
#define LANG_C_KNR    (1u << 2)         /**< K&R (pre-ANSI) C. */
#define LANG_C_89     (1u << 3)         /**< C 89 (first ANSI C). */
#define LANG_C_95     (1u << 4)         /**< C 95. */
#define LANG_C_99     (1u << 5)         /**< C 99. */
#define LANG_C_11     (1u << 6)         /**< C 11. */
#define LANG_C_17     (1u << 7)         /**< C 17. */
#define LANG_C_23     (1u << 8)         /**< C 23. */
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
#define LANG_CPP_ANY  0xFE00u           /**< Any C++ language. */

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
 *                     ^
 *      9: warning: "_Sat" is a keyword in C99 (with Embedded C extensions)
 *
 * in a language other than C99, they'll get a warning.
 *
 * @sa #LANG_C_99
 * @sa #LANGX_EMC
 * @sa \ref c-emc-types-group
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
 *                     ^
 *      9: warning: "shared" is a keyword in C99 (with Unified Parallel C extensions)
 *
 * in a language other than C99, they'll get a warning.
 *
 * @sa #LANG_C_99
 * @sa #LANGX_UPC
 * @sa \ref c-upc-qualifiers-group
 * @sa [Unified Parallel C](http://upc-lang.org/)
 */
#define LANG_C_99_UPC (LANG_C_99 | LANGX_UPC)

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
#define LANG_C_MIN(L)             (LANG_MIN( C_##L ) & LANG_C_ANY)

/**
 * C++-only languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 *
 * @sa #LANG_CPP_MIN()
 * @sa #LANG_C_CPP_MAX()
 * @sa #LANG_MAX()
 */
#define LANG_CPP_MAX(L)           (LANG_MAX( CPP_##L ) & LANG_CPP_ANY)

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
 * Shorthand for the common case of getting whether \ref opt_lang_id is among
 * the languages specified by \a LANG_MACRO.
 *
 * @param LANG_MACRO A `LANG_*` macro without the `LANG_` prefix.
 * @return Returns `true` only if \ref opt_lang_id is among the languages
 * specified by \a LANG_MACRO.
 *
 * @sa opt_lang_id
 * @sa opt_lang_is_any()
 */
#define OPT_LANG_IS(LANG_MACRO)   opt_lang_is_any( LANG_##LANG_MACRO )

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup c-lang-feat-group C/C++ Language Features
 * Macros for C/C++ language features.
 *
 * @remarks The macros have the same `LANG_` prefix as \ref c-lang-vers-group
 * so they can be used instead of hard-coding specific languages directly.  For
 * example, instead of writing:
 *
 *      OPT_LANG_IS( MIN(C_95) )
 *
 * write:
 *
 *      OPT_LANG_IS( DIGRAPHS )
 *
 * Macros are in all upper-case except if they contain a C/C++ keyword that is
 * written as it actually is, e.g., #LANG_const.
 * @{
 */

/**
 * Languages the `_Alignas` keyword is supported in.
 *
 * @sa #LANG_ALIGNED_CSUS
 * @sa #LANG_ALIGNMENT
 * @sa #LANG_alignas
 */
#define LANG__Alignas                   LANG_C_MIN(11)

/**
 * Languages the `alignas` keyword is supported in.
 *
 * @sa #LANG_ALIGNED_CSUS
 * @sa #LANG_ALIGNMENT
 * @sa #LANG__Alignas
 */
#define LANG_alignas                    LANG_C_CPP_MIN(23,11)

/**
 * Languages `alignas` may be used with `enum`, `class`, `struct`, and `union`
 * declarations.
 *
 * @sa #LANG_ALIGNMENT
 * @sa #LANG__Alignas
 * @sa #LANG_alignas
 */
#define LANG_ALIGNED_CSUS               LANG_CPP_MIN(11)

/**
 * Lanuages aligned storage is supported in.
 *
 * @sa #LANG_ALIGNED_CSUS
 * @sa #LANG__Alignas
 * @sa #LANG_alignas
 */
#define LANG_ALIGNMENT                  (LANG__Alignas | LANG_alignas)

/**
 * Languages the `_Alignof` keyword is supported in.
 *
 * @sa #LANG_ALIGNMENT
 * @sa #LANG_alignof
 */
#define LANG__Alignof                   LANG_C_MIN(11)

/**
 * Languages the `alignof` keyword is supported in.
 *
 * @sa #LANG_ALIGNMENT
 * @sa #LANG__Alignof
 */
#define LANG_alignof                    LANG_C_CPP_MIN(23,11)

/**
 * Languages "alternative tokens" are supported in.
 */
#define LANG_ALT_TOKENS                 LANG_MIN(C_95)

/**
 * Languages Apple's `__block` keyword is supported in.
 *
 * @sa [Apple's Extensions to C](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1370.pdf)
 * @sa [Blocks Programming Topics](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks)
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
 * Languages the `auto` keyword for parameters are supported in.
 *
 * @sa #LANG_auto_POINTER_TYPES
 * @sa #LANG_auto_RETURN_TYPES
 * @sa #LANG_auto_STORAGE
 * @sa #LANG_auto_TYPE
 * @sa #LANG_auto_TYPE_MULTI_DECL
 */
#define LANG_auto_PARAMS                LANG_CPP_MIN(20)

/**
 * Languages the `auto` keyword as a pointer type is supported in.
 *
 * @sa #LANG_auto_PARAMS
 * @sa #LANG_auto_RETURN_TYPES
 * @sa #LANG_auto_STORAGE
 * @sa #LANG_auto_TYPE
 * @sa #LANG_auto_TYPE_MULTI_DECL
 */
#define LANG_auto_POINTER_TYPES         LANG_CPP_MIN(11)

/**
 * Languages the `auto` keyword for function return types are supported in.
 *
 * @sa #LANG_auto_PARAMS
 * @sa #LANG_auto_POINTER_TYPES
 * @sa #LANG_auto_STORAGE
 * @sa #LANG_auto_TYPE
 * @sa #LANG_auto_TYPE_MULTI_DECL
 */
#define LANG_auto_RETURN_TYPES          LANG_CPP_MIN(14)

/**
 * Languages the `auto` keyword as a storage class is supported in.
 *
 * @sa #LANG_auto_PARAMS
 * @sa #LANG_auto_POINTER_TYPES
 * @sa #LANG_auto_RETURN_TYPES
 * @sa #LANG_auto_TYPE
 * @sa #LANG_auto_TYPE_MULTI_DECL
 */
#define LANG_auto_STORAGE               LANG_C_CPP_MAX(17,03)

/**
 * Languages the `auto` keyword as a type is supported in.
 *
 * @sa #LANG_auto_PARAMS
 * @sa #LANG_auto_POINTER_TYPES
 * @sa #LANG_auto_RETURN_TYPES
 * @sa #LANG_auto_STORAGE
 * @sa #LANG_auto_TYPE_MULTI_DECL
 */
#define LANG_auto_TYPE                  LANG_C_CPP_MIN(23,11)

/**
 * Languages the `auto` keyword as a type declaring multiple variables is
 * supported in.
 *
 * @sa #LANG_auto_PARAMS
 * @sa #LANG_auto_POINTER_TYPES
 * @sa #LANG_auto_RETURN_TYPES
 * @sa #LANG_auto_STORAGE
 * @sa #LANG_auto_TYPE
 */
#define LANG_auto_TYPE_MULTI_DECL       LANG_CPP_MIN(11)

/**
 * Languages the `_BitInt` keyword is supported in.
 */
#define LANG__BitInt                    LANG_C_MIN(23)

/**
 * Languages the `_Bool` keyword is supported in.
 *
 * @sa #LANG_bool
 * @sa #LANG_BOOLEAN
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
 * @sa #LANG_BOOLEAN
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
#define LANG_BOOLEAN                    (LANG__Bool | LANG_bool)

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
 * Languages the `class` keyword is supported in.
 */
#define LANG_class                      LANG_CPP_ANY

/**
 * Languages the `_Complex` keyword is supported in.
 *
 * @sa #LANG__Imaginary
 */
#define LANG__Complex                   LANG_C_MIN(99)

/**
 * Languages the `concept` keyword is supported in.
 */
#define LANG_concept                    LANG_CPP_MIN(20)

/**
 * Languages constructors are supported in.
 */
#define LANG_CONSTRUCTORS               LANG_CPP_ANY

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
 * @sa #LANG_constexpr_RETURN_TYPES
 * @sa #LANG_constexpr_virtual
 */
#define LANG_constexpr                  LANG_C_CPP_MIN(23,11)

/**
 * Languages the `constexpr` keyword for return types are supported in.
 *
 * @sa #LANG_constexpr
 */
#define LANG_constexpr_RETURN_TYPES     LANG_CPP_MIN(14)

/**
 * Languages the `constexpr virtual` functions are supported in.
 *
 * @sa #LANG_constexpr
 */
#define LANG_constexpr_virtual          LANG_CPP_MIN(20)

/**
 * Languages the `constinit` keyword is supported in.
 */
#define LANG_constinit                  LANG_CPP_MIN(20)

/**
 * Languages `class`, `struct`, or `union` for return types are supported.
 */
#define LANG_CSU_RETURN_TYPES           LANG_MIN(C_89)

/**
 * Languages the `__DATE__` predefined macro is supported in.
 *
 * @sa #LANG___TIME__
 */
#define LANG___DATE__                   LANG_MIN(C_89)

/**
 * Languages the `decltype` keyword is supported in.
 */
#define LANG_decltype                   LANG_CPP_MIN(11)

/**
 * Languages the `default` and `delete` keywords for functions are supported
 * in.
 */
#define LANG_default_delete_FUNCS       LANG_CPP_MIN(11)

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
 * Languages in which `enum`, `class`, `struct`, and `union` declarations are
 * implicitly types.
 *  ```c
 * struct S;            // In C, tag only (not a type); in C++, it's a type.
 * typedef struct S S;  // Now it's a type in C.
 *  ```
 */
#define LANG_ECSU_IS_IMPLICIT_TYPE      LANG_CPP_ANY

/**
 * Languages the `enum` keyword is supported in.
 *
 * @sa #LANG_enum_BITFIELDS
 * @sa #LANG_enum_class
 * @sa #LANG_FIXED_TYPE_enum
 */
#define LANG_enum                       LANG_MIN(C_89)

/**
 * Languages `enum` bitfields are supported in.
 *
 * @sa #LANG_enum
 * @sa #LANG_enum_class
 * @sa #LANG_FIXED_TYPE_enum
 */
#define LANG_enum_BITFIELDS             LANG_CPP_ANY

/**
 * Languages `enum class` is supported in.
 *
 * @sa #LANG_enum
 * @sa #LANG_enum_BITFIELDS
 * @sa #LANG_FIXED_TYPE_enum
 */
#define LANG_enum_class                 LANG_CPP_MIN(11)

/**
 * Languages the `explicit` keyword is supported in.
 *
 * @sa #LANG_explicit_USER_DEF_CONVS
 */
#define LANG_explicit                   LANG_CPP_ANY

/**
 * Languages explicit `this` parameter supported in.
 */
#define LANG_EXPLICIT_OBJ_PARAM_DECLS   LANG_CPP_MIN(23)

/**
 * Languages `explicit` user-defined conversion operators are supported in.
 *
 * @sa #LANG_explicit
 * @sa #LANG_USER_DEF_CONVS
 */
#define LANG_explicit_USER_DEF_CONVS    LANG_CPP_MIN(11)

/**
 * Languages the `export` keyword is supported in.
 */
#define LANG_export                     LANG_CPP_MIN(20)

/**
 * Languages `extern void x` is supported in.
 *
 * @sa #LANG_void
 */
#define LANG_extern_void                LANG_C_MIN(89)

/**
 * Languages the `__FILE__` predefined macro is supported in.
 *
 * @sa #LANG___LINE__
 */
#define LANG___FILE__                   LANG_MIN(C_89)

/**
 * Languages the `final` keyword is supported in.
 *
 * @sa #LANG_override
 */
#define LANG_final                      LANG_CPP_MIN(11)

/**
 * Languages fixed type `enum`s are supported in.
 *
 * @sa #LANG_enum
 * @sa #LANG_enum_BITFIELDS
 * @sa #LANG_enum_class
 */
#define LANG_FIXED_TYPE_enum            LANG_C_CPP_MIN(23,11)

/**
 * Languages the `friend` keyword is supported in.
 */
#define LANG_friend                     LANG_CPP_ANY

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
 * @sa #LANG_inline_VARIABLES
 */
#define LANG_inline                     LANG_MIN(C_99)

/**
 * Languages `inline namespace` is supported in.
 *
 * @sa #LANG_inline
 * @sa #LANG_inline_VARIABLES
 */
#define LANG_inline_namespace           LANG_CPP_MIN(11)

/**
 * Languages `inline` variables are supported in.
 *
 * @sa #LANG_inline
 * @sa #LANG_inline_namespace
 */
#define LANG_inline_VARIABLES           LANG_CPP_MIN(17)

/**
 * Languages K&R style function definitions are supported in.
 *
 * @sa #LANG_PROTOTYPES
 */
#define LANG_KNR_FUNC_DEFS              LANG_C_MAX(17)

/**
 * Lanuages lambdas are supported in.
 */
#define LANG_LAMBDAS                    LANG_CPP_MIN(11)

/**
 * Languages the `<=>` operator is supported in.
 *
 * @sa #LANG_operator
 */
#define LANG_LESS_EQUAL_GREATER         LANG_CPP_MIN(20)

/**
 * Languages the `__LINE__` predefined macro is supported in.
 *
 * @sa #LANG___FILE__
 */
#define LANG___LINE__                   LANG_MIN(C_89)

/**
 * Languages linkage declarations are supported in.
 */
#define LANG_LINKAGE_DECLS              LANG_CPP_ANY

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
#define LANG_long_long                  LANG_C_CPP_MIN(99,11)

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
 * Languages member functions are supported in.
 */
#define LANG_MEMBER_FUNCTIONS           LANG_CPP_ANY

/**
 * Languages Microsoft calling conventions are supported in.
 */
#define LANG_MSC_CALL_CONVS             LANG_MIN(C_89)

/**
 * Languages the `mutable` keyword is supported in.
 */
#define LANG_mutable                    LANG_CPP_ANY

/**
 * Languages `operator[]` can have only exactly 1 parameter.
 *
 * @sa #LANG_N_ARY_OP_BRACKETS
 * @sa #LANG_operator
 */
#define LANG_1_ARY_OP_BRACKETS          LANG_CPP_MAX(20)

/**
 * Languages `operator[]` can have any number of parameters.
 *
 * @sa #LANG_1_ARY_OP_BRACKETS
 * @sa #LANG_operator
 */
#define LANG_N_ARY_OP_BRACKETS          LANG_CPP_MIN(23)

/**
 * Languages the `namespace` keyword is supported in.
 */
#define LANG_namespace                  LANG_CPP_ANY

/**
 * Languages nested `namespace` declarations are supported in.
 */
#define LANG_NESTED_namespace           LANG_CPP_MIN(17)

/**
 * Languages nested types are supported in.
 */
#define LANG_NESTED_TYPES               LANG_CPP_ANY

/**
 * Languages the `const_cast`, `dynamic_cast`, `reinterpret_cast`, and
 * `static_cast` keywords are supported in.
 */
#define LANG_NEW_STYLE_CASTS            LANG_CPP_ANY

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
 * @sa #LANG_NONRETURNING_FUNCS
 * @sa #LANG___noreturn__
 * @sa #LANG_noreturn
 * @sa #LANG__Noreturn_NOT_DEPRECATED
 */
#define LANG__Noreturn                  LANG_C_MIN(11)

/**
 * Languages `_Noreturn` is _not_ deprecated in.
 *
 * @sa #LANG__Noreturn
 */
#define LANG__Noreturn_NOT_DEPRECATED   LANG_C_MAX(17)

/**
 * Languages the `noreturn` keyword is supported in.
 *
 * @sa #LANG__Noreturn
 * @sa #LANG___noreturn__
 * @sa #LANG_NONRETURNING_FUNCS
 */
#define LANG_noreturn                   LANG_C_CPP_MIN(23,11)

/**
 * Languages the `__noreturn__` attribute is supported in.
 *
 * @sa #LANG__Noreturn
 * @sa #LANG_noreturn
 * @sa #LANG_NONRETURNING_FUNCS
 */
#define LANG___noreturn__               LANG_C_MIN(23)

/**
 * Languages that support non-returning functions.
 *
 * @sa #LANG__Noreturn
 * @sa #LANG___noreturn__
 * @sa #LANG_noreturn
 */
#define LANG_NONRETURNING_FUNCS         (LANG__Noreturn | LANG_noreturn)

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
 * Languages the `operator` keyword is supported in.
 *
 * @sa #LANG_1_ARY_OP_BRACKETS
 * @sa #LANG_N_ARY_OP_BRACKETS
 */
#define LANG_operator                   LANG_CPP_ANY

/**
 * Languages the `override` keyword is supported in.
 *
 * @sa #LANG_final
 */
#define LANG_override                   LANG_CPP_MIN(11)

/**
 * Languages the preprocessor `##` operator is supported in.
 *
 * @sa #LANG_P_STRINGIFY
 */
#define LANG_P_CONCAT                   LANG_MIN(C_89)

/**
 * Languages the preprocessor `#` operator is supported in.
 *
 * @sa #LANG_P_CONCAT
 */
#define LANG_P_STRINGIFY                LANG_MIN(C_89)

/**
 * Languages `__VA_OPT__` is supported in.
 *
 * @sa #LANG_VARIADIC_MACROS
 */
#define LANG_P___VA_OPT__               LANG_C_CPP_MIN(23,20)

/**
 * Languages parameter packs (`...`) are supported in.
 */
#define LANG_PARAMETER_PACKS            LANG_CPP_MIN(20)

/**
 * Languages pointers to member are supported in.
 */
#define LANG_POINTERS_TO_MEMBER         LANG_CPP_ANY

/**
 * Languages function prototypes are supported in.
 *
 * @sa #LANG_KNR_FUNC_DEFS
 */
#define LANG_PROTOTYPES                 LANG_MIN(C_89)

/**
 * Languages qualified array parameters are supported in.
 */
#define LANG_QUALIFIED_ARRAYS           LANG_C_MIN(99)

/**
 * Languages references are supported in.
 *
 * @sa #LANG_RVALUE_REFERENCES
 */
#define LANG_REFERENCES                 LANG_CPP_ANY

/**
 * Languages reference qualified functions are supported in.
 */
#define LANG_REF_QUALIFIED_FUNCS        LANG_CPP_MIN(11)

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
 *
 * @sa #LANG_REFERENCES
 */
#define LANG_RVALUE_REFERENCES          LANG_CPP_MIN(11)

/**
 * Languages scoped names are supported in.
 */
#define LANG_SCOPED_NAMES               LANG_CPP_ANY

/**
 * Languages the `signed` keyword is supported in.
 */
#define LANG_signed                     LANG_MIN(C_89)

/**
 * Languages `//` comments are supported in.
 */
#define LANG_SLASH_SLASH_COMMENT        LANG_MIN(C_89)

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
 *
 * @sa #LANG_operator
 */
#define LANG_static_OP_PARENS           LANG_CPP_MIN(23)

/**
 * Languages the `__STDC__` predefined macro is supported in.
 *
 * @sa #LANG___STDC_VERSION__
 */
#define LANG___STDC__                   LANG_C_MIN(89)

/**
 * Languages the `__STDC__` predefined macro is supported in.
 *
 * @sa #LANG___STDC__
 */
#define LANG___STDC_VERSION__           LANG___STDC__

/**
 * Languages "structured bindings" are supported in.
 */
#define LANG_STRUCTURED_BINDINGS        LANG_CPP_MIN(17)

/**
 * Languages "tentative definitions" are supported in.
 */
#define LANG_TENTATIVE_DEFS             LANG_C_ANY

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
 * Languages thread-local storage is supported in.
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
 * Languages the `__TIME__` predefined macro is supported in.
 *
 * @sa #LANG___DATE__
 */
#define LANG___TIME__                   LANG_MIN(C_89)

/**
 * Languages trailing return types are supported in.
 */
#define LANG_TRAILING_RETURN_TYPES      LANG_CPP_MIN(11)

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
 * @sa #LANG_BOOLEAN
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
 * Languages user-defined conversion operators are supported in.
 *
 * @sa #LANG_explicit_USER_DEF_CONVS
 */
#define LANG_USER_DEF_CONVS             LANG_CPP_ANY

/**
 * Languages user-defined literals are supported in.
 */
#define LANG_USER_DEF_LITS              LANG_CPP_MIN(11)

/**
 * Languages `using` declarations are supported in.
 */
#define LANG_using_DECLS                LANG_CPP_MIN(11)

/**
 * Languages variadic macros are supported in.
 *
 * @sa #LANG_P___VA_OPT__
 */
#define LANG_VARIADIC_MACROS            LANG_MIN(C_99)

/**
 * Languages that allow `...` as the only function parameter.
 */
#define LANG_VARIADIC_ONLY_PARAMS       LANG_C_CPP_MIN(23,OLD)

/**
 * Languages the `virtual` keyword is supported in.
 */
#define LANG_virtual                    LANG_CPP_ANY

/**
 * Languages variable length arrays are supported in.
 */
#define LANG_VLAS                       LANG_C_MIN(99)

/**
 * Languages the `void` keyword is supported in.
 *
 * @sa #LANG_extern_void
 */
#define LANG_void                       LANG_MIN(C_89)

/**
 * Languages the `volatile` keyword is supported in.
 *
 * @sa #LANG_const
 * @sa #LANG_volatile_PARAMS_NOT_DEPRECATED
 * @sa #LANG_volatile_RETURN_TYPES_NOT_DEPRECATED
 * @sa #LANG_volatile_STRUCTURED_BINDINGS_NOT_DEPRECATED
 */
#define LANG_volatile                   LANG_const

/**
 * Languages `volatie` parameters are _not_ deprecated in.
 *
 * @sa #LANG_volatile
 * @sa #LANG_volatile_RETURN_TYPES_NOT_DEPRECATED
 * @sa #LANG_volatile_STRUCTURED_BINDINGS_NOT_DEPRECATED
 */
#define LANG_volatile_PARAMS_NOT_DEPRECATED \
                                        LANG_MAX(CPP_17)

/**
 * Languages `volatie` return types are _not_ deprecated in.
 *
 * @sa #LANG_volatile
 * @sa #LANG_volatile_PARAMS_NOT_DEPRECATED
 * @sa #LANG_volatile_STRUCTURED_BINDINGS_NOT_DEPRECATED
 */
#define LANG_volatile_RETURN_TYPES_NOT_DEPRECATED \
                                        LANG_MAX(CPP_17)

/**
 * Languages `volatie` structured bindings are _not_ deprecated in.
 *
 * @sa #LANG_volatile
 * @sa #LANG_volatile_PARAMS_NOT_DEPRECATED
 * @sa #LANG_volatile_RETURN_TYPES_NOT_DEPRECATED
 */
#define LANG_volatile_STRUCTURED_BINDINGS_NOT_DEPRECATED \
                                        LANG_CPP_MAX(17)

/**
 * Languages the `wchar_t` keyword is supported in.
 */
#define LANG_wchar_t                    LANG_MIN(C_95)

/** @} */

/**
 * @addtogroup c-lang-vers-group
 * @{
 */

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
 * Gets the value of the `__cplusplus` macro for \a lang_id.
 *
 * @param lang_id The language.  _Exactly one_ language _must_ be set.
 * @return Returns said value or NULL if `__cplusplus` does not have a value
 * for \a lang_id.
 *
 * @sa c_lang_coarse_name()
 * @sa c_lang_name()
 * @sa c_lang___STDC__()
 * @sa c_lang___STDC_VERSION__()
 */
NODISCARD
char const* c_lang___cplusplus( c_lang_id_t lang_id );

/**
 * Gets the value of the `__STDC_VERSION__` macro for \a lang_id.
 *
 * @param lang_id The language.  _Exactly one_ language _must_ be set.
 * @return Returns said value or NULL if `__STDC_VERSION__` does not have a
 * value for \a lang_id.
 *
 * @sa c_lang_coarse_name()
 * @sa c_lang___cplusplus()
 * @sa c_lang_name()
 * @sa c_lang___STDC__()
 */
NODISCARD
char const* c_lang___STDC_VERSION__( c_lang_id_t lang_id );

/**
 * Gets all the language(s) \a lang_id and \ref c-lang-order "newer".
 *
 * @param lang_id The language.  _Exactly one_ language _must_ be set.
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
 * @sa c_lang___cplusplus()
 * @sa c_lang_is_one()
 * @sa c_lang_name()
 * @sa c_lang___STDC__()
 * @sa c_lang___STDC_VERSION__()
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
 * Gets whether \a lang_ids contains language(s) for only either C or C++, but
 * not both.
 *
 * @param lang_ids The bitwise-of of language(s) to check.
 * @return
 *  + If \a lang_ids only contains any versions of C and no versions of C++,
 *    returns #LANG_C_ANY.
 *  + If \a lang_ids only contains any versions of C++ and no versions of C,
 *    returns #LANG_CPP_ANY.
 *  + Otherwise returns #LANG_NONE.
 *
 * @sa c_lang_coarse_name()
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
 * @sa c_lang___cplusplus()
 * @sa c_lang___STDC__()
 * @sa c_lang___STDC_VERSION__()
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
  return ms_bit1_32( lang_ids & ~LANGX_MASK );
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
 * Gets a string specifying a language when a particular feature isn't, hasn't
 * been, or won't be legal unless, since, or until, if ever.
 *
 * @param lang_ids The bitwise-or of legal language(s).
 * @return
 *  + If \a lang_ids is #LANG_NONE, returns the empty string.
 *
 *  + If \a lang_ids contains exactly one language:
 *
 *      + If the current language is that language, returns the empty string;
 *
 *      + Otherwise returns `" unless "` followed by the name of that language.
 *
 *  + Otherwise:
 *
 *      + If the current language is any version of C and \a lang_ids does not
 *        contain any version of C, returns `" in C"`.
 *
 *      + If the current language is any version of C++ and \a lang_ids does
 *        not contain any version of C++, returns `" in C++"`.
 *
 *      + If the current language is older than oldest language in \a lang_ids,
 *        returns `" until "` followed by the name of the oldest C language
 *        version (if the current language is any version of C) or the name of
 *        the oldest C++ language version (if the current language is any
 *        version of C++).
 *
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
 * Checks whether \a name is reserved in any language.
 *
 * @remarks A name is reserved if it matches any of these patterns:
 *
 *      _*          // C: external only; C++: global namespace only.
 *      _[A-Z_]*
 *      *__*        // C++ only.
 *
 * However, we don't check for the first one since **cdecl** doesn't have
 * either the linkage or the scope of a name.
 *
 * @param name The name to check.
 * @return Returns the bitwise-or of language(s) that \a name is reserved in.
 */
NODISCARD
c_lang_id_t is_reserved_name( char const *name );

/**
 * Convenience function for checking whether \ref opt_lang_id is among \a
 * lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s) to check.
 * @return Returns `true` only if it is.
 *
 * @sa opt_lang_id
 * @sa #OPT_LANG_IS()
 */
NODISCARD C_LANG_H_INLINE
bool opt_lang_is_any( c_lang_id_t lang_ids ) {
  return (opt_lang_id & lang_ids) != LANG_NONE;
}

/**
 * Gets the value of the `__STDC__` macro for \a lang_id.
 *
 * @param lang_id The language.  _Exactly one_ language _must_ be set.
 * @return Returns `"1"` only if \a lang_id is any version of C _except_
 * K&R&nbsp;C; NULL if \a lang_id is K&R&nbsp;C.
 *
 * @sa c_lang_coarse_name()
 * @sa c_lang___cplusplus()
 * @sa c_lang_name()
 * @sa c_lang___STDC_VERSION__()
 */
NODISCARD C_LANG_H_INLINE
char const* c_lang___STDC__( c_lang_id_t lang_id ) {
  assert( is_1_bit( lang_id ) );
  return (lang_id & LANG___STDC__) != LANG_NONE  ? "1" : NULL;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_lang_H */
/* vim:set et sw=2 ts=2: */
