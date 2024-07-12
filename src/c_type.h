/*
**      cdecl -- C gibberish translator
**      src/c_type.h
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

#ifndef cdecl_c_type_H
#define cdecl_c_type_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ types.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_lang.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>                   /* for PRIX64, etc. */

_GL_INLINE_HEADER_BEGIN
#ifndef C_TYPE_H_INLINE
# define C_TYPE_H_INLINE _GL_INLINE
#endif /* C_TYPE_H_INLINE */

/// @endcond

/**
 * @defgroup c-types-group C/C++ Types
 * Constants, types, and functions for C/C++ types.
 * @{
 */

/**
 * The maximum width supported in the declaration of a `_BitInt(N)`.
 *
 * @note We don't use the standard `BITINT_MAXWIDTH` in `<limits.h>` because we
 * want the value to be the same regardless of the underlying platorm.
 */
#define C_BITINT_MAXWIDTH         128u

////////// types //////////////////////////////////////////////////////////////

/**
 * The C/C++ type of an identifier (variable, function, etc.). A type is split
 * into parts because the number of distinct bits needed in total exceeds 64.
 */
struct c_type {
  /**
   * The base types (`int`, `double`, etc.) including user-defined types
   * (`enum`, `struct`, etc.), modifiers (`short`, `unsigned`, etc.), and also
   * `namespace` and the generic `scope` since it makes sense to store those
   * with other scope-types (like `struct`).
   *
   * Constants for base types begin with `TB_`.
   */
  c_tid_t btids;

  /**
   * The storage classes (`extern`, `static`, etc., including `typedef`),
   * storage-class-like things (`default`, `friend`, `inline`, etc.), and also
   * qualifiers (`_Atomic`, `const`, etc.) and ref-qualifiers (`&`, `&&`).
   *
   * Constants for storage-class-like things begin with `TS_`.
   */
  c_tid_t stids;

  /**
   * Attributes.
   *
   * Constants for attributes begin with `TA_`.
   */
  c_tid_t atids;
};

/**
 * For \ref c_tid_t values, the low-order 4 bits specify the "type part ID"
 * (TPID) and thus how the remaining bits of the value should be interpreted.
 *
 * @remarks This is needed because the combination of all base types, all
 * storage classes, all storage-like classes, and attributes would take more
 * than 64 bits.
 * @remarks Type part IDs start at 1 so we know a \ref c_tid_t value has been
 * initialized properly as opposed to it being 0 by default.
 *
 * @sa #TX_TPID_MASK
 */
enum c_tpid {
  /**
   * No types.
   */
  C_TPID_NONE   = 0,

  /**
   * Base types, e.g., `int`.
   *
   * @sa \ref c-base-types-group
   * @sa \ref c-emc-types-group
   */
  C_TPID_BASE   = 1 << 0,

  /**
   * Storage types, e.g., `static`.
   *
   * @sa \ref c-storage-types-group
   * @sa \ref c-storage-like-types-group
   * @sa \ref c-qualifiers-group
   * @sa \ref c-ref-qualifiers-group
   * @sa \ref c-upc-qualifiers-group
   */
  C_TPID_STORE  = 1 << 1,

  /**
   * Attributes.
   *
   * @sa \ref c-attributes-group
   * @sa \ref c-msc-call-group
   */
  C_TPID_ATTR   = 1 << 2
};
typedef enum c_tpid c_tpid_t;

/**
 * Convenience macro for specifying a type ID literal.
 *
 * @param TPID The \ref c_tpid without the `C_TPID_` prefix.
 * @param BITS The high-order 60 bits of the type ID without the last `0`.
 * @return Returns said type ID literal.
 */
#define C_TID_LIT(TPID,BITS)      (BITS ## 0ull | C_TPID_ ## TPID)

/**
 * @defgroup c-type-literal-macros-group C/C++ Type Literal Macros
 * Convenience macros for specifying a type literal.
 *
 * For example, instead of writing:
 * ```
 * c_type_t type = (c_type_t){ TB_char, TS_NONE, TA_NONE };
 * ```
 * write:
 * ```
 * c_type_t type = C_TYPE_LIT_B( TB_char );
 * ```
 * @{
 */

/**
 * Convenience macro for specifying a complete \ref c_type literal.
 *
 * @param BTID The base \ref c_tid_t.
 * @param STID The storage \ref c_tid_t.
 * @param ATID The attribute(s) \ref c_tid_t.
 * @return Returns said literal.
 *
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_S()
 */
#define C_TYPE_LIT(BTID,STID,ATID) \
  (c_type_t const){ (BTID), (STID), (ATID) }

/**
 * Convenience macro for specifying a \ref c_type literal from \a ATID.
 *
 * @param ATID The attribute(s) \ref c_tid_t.
 * @return Returns said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_S()
 */
#define C_TYPE_LIT_A(ATID) \
  C_TYPE_LIT( TB_NONE, TS_NONE, (ATID) )

/**
 * Convenience macro for specifying a \ref c_type literal from \a BTID.
 *
 * @param BTID The base \ref c_tid_t.
 * @return Returns said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_S()
 */
#define C_TYPE_LIT_B(BTID) \
  C_TYPE_LIT( (BTID), TS_NONE, TA_NONE )

/**
 * Convenience macro for specifying a \ref c_type literal from \a STID.
 *
 * @param STID The storage \ref c_tid_t.
 * @return Returns said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_B()
 */
#define C_TYPE_LIT_S(STID) \
  C_TYPE_LIT( TB_NONE, (STID), TA_NONE )

/** @} */

#define TX_NONE                   0ull  /**< No type at all. */

/**
 * @defgroup c-base-types-group C/C++ Base Types
 * @ingroup c-types-group
 * Base types & modifiers.
 *
 * @note If you add a new `TB_xxx` macro, it _must_ also exist in `BTIDS[]`
 * inside c_type_name_impl().
 * @{
 */

/** No base type. */
#define TB_NONE                   C_TID_LIT( BASE, 0x000000000000000 )

/** Any base type. */
#define TB_ANY                    C_TID_LIT( BASE, 0xFFFFFFFFFFFFFFF )

/** `void`. */
#define TB_void                   C_TID_LIT( BASE, 0x000000000000001 )

/**
 * `auto` deduced type.
 *
 * @sa #TS_auto
*/
#define TB_auto                   C_TID_LIT( BASE, 0x000000000000002 )

/** `_BitInt`. */
#define TB__BitInt                C_TID_LIT( BASE, 0x000000000000004 )

/** `_Bool` or `bool`. */
#define TB_bool                   C_TID_LIT( BASE, 0x000000000000008 )

/** `char`. */
#define TB_char                   C_TID_LIT( BASE, 0x000000000000010 )

/**
 * `char8_t`.
 *
 * @remarks While this is a distinct type in C++20, it's just a `typedef` in
 * C23.  It's simpler to treat it as a distinct type in C also.
 */
#define TB_char8_t                C_TID_LIT( BASE, 0x000000000000020 )

/**
 * `char16_t`
 *
 * @remarks While this is a distinct type in C++11, it's just a `typedef` in
 * C11.  It's simpler to treat it as a distinct type in C also.
 */
#define TB_char16_t               C_TID_LIT( BASE, 0x000000000000040 )

/**
 * `char32_t`.
 *
 * @remarks While this is a distinct type in C++11, it's just a `typedef` in
 * C11.  It's simpler to treat it as a distinct type in C also.
 */
#define TB_char32_t               C_TID_LIT( BASE, 0x000000000000080 )

/**
 * `wchar_t`.
 *
 * @remarks While this is a distinct type in C++, it's just a `typedef` in C.
 * It's simpler to treat it as a distinct type in C also.
 */
#define TB_wchar_t                C_TID_LIT( BASE, 0x000000000000100 )

/** `short`. */
#define TB_short                  C_TID_LIT( BASE, 0x000000000000200 )

/** `int`. */
#define TB_int                    C_TID_LIT( BASE, 0x000000000000400 )

/** `long`. */
#define TB_long                   C_TID_LIT( BASE, 0x000000000000800 )

/** `long long`. */
#define TB_long_long              C_TID_LIT( BASE, 0x000000000001000 )

/** `signed`. */
#define TB_signed                 C_TID_LIT( BASE, 0x000000000002000 )

/** `unsigned`. */
#define TB_unsigned               C_TID_LIT( BASE, 0x000000000004000 )

/** `float`. */
#define TB_float                  C_TID_LIT( BASE, 0x000000000008000 )

/** `double`. */
#define TB_double                 C_TID_LIT( BASE, 0x000000000010000 )

/** `_Complex`. */
#define TB__Complex               C_TID_LIT( BASE, 0x000000000020000 )

/** `_Imaginary`. */
#define TB__Imaginary             C_TID_LIT( BASE, 0x000000000040000 )

/** `enum`. */
#define TB_enum                   C_TID_LIT( BASE, 0x000000000080000 )

/** `struct`. */
#define TB_struct                 C_TID_LIT( BASE, 0x000000000100000 )

/** `union`. */
#define TB_union                  C_TID_LIT( BASE, 0x000000000200000 )

/** `class`. */
#define TB_class                  C_TID_LIT( BASE, 0x000000000400000 )

/**
 * `namespace`.
 *
 * @remarks Even though a namespace isn't a type, it's simpler to store the
 * fact that a name is a namespace as the scope of another name just like
 * #TB_class, #TB_enum, #TB_struct, and #TB_union.
 */
#define TB_namespace              C_TID_LIT( BASE, 0x000000000800000 )

/**
 * A generic scope when the specific type of scope is unknown.
 *
 * @remarks
 * @parblock
 * For example:
 *
 *      c++decl> explain int T::x
 *      declare x of scope T as integer
 *
 * If `T` wasn't previously declared, **cdecl** knows `T` must be one of
 * `class`, `namespace`, `struct`, or `union`, but it has no way to know which
 * one, so it uses the generic `scope` instead.
 * @endparblock
 */
#define TB_SCOPE                  C_TID_LIT( BASE, 0x000000001000000 )

/**
 * A `typedef`'d type, e.g., `size_t`.
 *
 * @remarks
 * @parblock
 * The difference between this and #TS_typedef is that:
 *
 *  + #TB_typedef is the "type" for a particular `typedef`'d type, e.g.,
 *    `size_t`, after parsing a declaration and the new type has been defined.
 *    Hence, #TB_typedef is somewhat unnecessary when the kind of an AST is
 *    #K_TYPEDEF, but it has to have some type.
 *
 *  + #TS_typedef is the "storage class" for a declaration as a whole while
 *    it's being declared during parsing.
 * @endparblock
 *
 * @sa #TS_typedef
 */
#define TB_typedef                C_TID_LIT( BASE, 0x000000002000000 )

/** @} */

/**
 * @defgroup c-emc-types-group Embedded C Base Types
 * Embedded C types & modifiers.
 *
 * @note If you add a new `TB_EMC_xxx` macro, it must also exist in `BTIDS[]`
 * inside c_type_name_impl().
 *
 * @sa #LANG_C_99_EMC
 * @sa [Information Technology — Programming languages - C - Extensions to support embedded processors](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1169.pdf)
 * @{
 */

/** Embedded C `_Accum`. */
#define TB_EMC__Accum             C_TID_LIT( BASE, 0x000000004000000 )

/** Embedded C `_Fract`. */
#define TB_EMC__Fract             C_TID_LIT( BASE, 0x000000008000000 )

/** Embedded C `_Sat`. */
#define TB_EMC__Sat               C_TID_LIT( BASE, 0x000000010000000 )

/** @} */

/**
 * @defgroup c-storage-types-group C/C++ Storage Class Types
 * C/C++ storage classes.
 *
 * @note If you add a new `TS_xxx` macro:
 * 1. It must also exist in `STIDS[]` inside c_type_name_impl().
 * 2. #TS_ANY_STORAGE may need to be updated.
 *
 * @{
 */

/** No storage type. */
#define TS_NONE                   C_TID_LIT( STORE, 0x000000000000000 )

/** Any storage type. */
#define TS_ANY                    C_TID_LIT( STORE, 0xFFFFFFFFFFFFFFF )

/**
 * `auto` storage class.
 *
 * @sa #TS_APPLE___block
 * @sa #TB_auto
 * @sa #TS_extern
 * @sa #TS_register
 * @sa #TS_static
 * @sa #TS_thread_local
 */
#define TS_auto                   C_TID_LIT( STORE, 0x000000000000001 )

/**
 * `__block` storage class.
 *
 * @sa [Apple's Extensions to C](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1370.pdf)
 * @sa [Blocks Programming Topics](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks)
 *
 * @sa #TS_auto
 * @sa #TS_extern
 * @sa #TS_register
 * @sa #TS_static
 * @sa #TS_thread_local
 */
#define TS_APPLE___block          C_TID_LIT( STORE, 0x000000000000002 )

/**
 * `extern`.
 *
 * @sa #TS_APPLE___block
 * @sa #TS_auto
 * @sa #TS_extern
 * @sa #TS_extern_C
 * @sa #TS_register
 * @sa #TS_thread_local
 */
#define TS_extern                 C_TID_LIT( STORE, 0x000000000000004 )

/**
 * `extern "C"`.
 *
 * @sa #TS_extern
 */
#define TS_extern_C               C_TID_LIT( STORE, 0x000000000000008 )

/** `mutable`. */
#define TS_mutable                C_TID_LIT( STORE, 0x000000000000010 )

/**
 * `register` storage class.
 *
 * @sa #TS_APPLE___block
 * @sa #TS_auto
 * @sa #TS_extern
 * @sa #TS_static
 * @sa #TS_thread_local
 */
#define TS_register               C_TID_LIT( STORE, 0x000000000000020 )

/**
 * `static` storage class.
 *
 * @sa #TS_APPLE___block
 * @sa #TS_auto
 * @sa #TS_extern
 * @sa #TS_register
 * @sa #TS_thread_local
 */
#define TS_static                 C_TID_LIT( STORE, 0x000000000000040 )

/**
 * `_Thread_local` or `thread_local` storage class.
 *
 * @sa #TS_APPLE___block
 * @sa #TS_auto
 * @sa #TS_extern
 * @sa #TS_register
 * @sa #TS_static
 */
#define TS_thread_local           C_TID_LIT( STORE, 0x000000000000080 )

/**
 * `typedef` "storage class" in a declaration like:
 *
 *      typedef void F(int)
 *
 * @remarks
 * @parblock
 * The difference between this and #TB_typedef is that:
 *
 *  + #TB_typedef is the "type" for a particular `typedef`'d type, e.g.,
 *    `size_t`, after parsing a declaration and the new type has been defined.
 *    Hence, #TB_typedef is somewhat unnecessary when the kind of an AST is
 *    #K_TYPEDEF, but it has to have some type.
 *
 *  + #TS_typedef is the "storage class" for a declaration as a whole while
 *    it's being declared during parsing.
 * @endparblock
 *
 * @sa #TB_typedef
 *
 * @note C++ `using` declarations are stored as their equivalent `typedef`
 * declarations.
 */
#define TS_typedef                C_TID_LIT( STORE, 0x000000000000100 )

/** @} */

/**
 * @defgroup c-storage-like-types-group C/C++ Storage-Like Types
 * C/C++ storage-like types.
 *
 * @note If you add a new `TS_xxx` macro:
 * 1. It must also exist in `STIDS[]` inside c_type_name_impl().
 * 2. #TS_ANY_STORAGE may need to be updated.
 *
 * @{
 */

/** `consteval`. */
#define TS_consteval              C_TID_LIT( STORE, 0x000000000000200 )

/** `constexpr`. */
#define TS_constexpr              C_TID_LIT( STORE, 0x000000000000400 )

/** `constinit`. */
#define TS_constinit              C_TID_LIT( STORE, 0x000000000000800 )

/** `= default`. */
#define TS_default                C_TID_LIT( STORE, 0x000000000001000 )

/** `= delete`. */
#define TS_delete                 C_TID_LIT( STORE, 0x000000000002000 )

/** `explicit`. */
#define TS_explicit               C_TID_LIT( STORE, 0x000000000004000 )

/** `export`. */
#define TS_export                 C_TID_LIT( STORE, 0x000000000008000 )

/** `final`. */
#define TS_final                  C_TID_LIT( STORE, 0x000000000010000 )

/** `friend`. */
#define TS_friend                 C_TID_LIT( STORE, 0x000000000020000 )

/** `inline`. */
#define TS_inline                 C_TID_LIT( STORE, 0x000000000040000 )

/** `noexcept`. */
#define TS_noexcept               C_TID_LIT( STORE, 0x000000000080000 )

/** `override`. */
#define TS_override               C_TID_LIT( STORE, 0x000000000100000 )

/**
 * `= 0`.
 *
 * @sa #TS_virtual
 */
#define TS_PURE_virtual           C_TID_LIT( STORE, 0x000000000200000 )

/** Explicit `this` parameter. */
#define TS_this                   C_TID_LIT( STORE, 0x000000000400000 )

/** `throw()`. */
#define TS_throw                  C_TID_LIT( STORE, 0x000000000800000 )

/**
 * `virtual`.
 *
 * @sa #TS_PURE_virtual
 */
#define TS_virtual                C_TID_LIT( STORE, 0x000000001000000 )

/** @} */

/**
 * @defgroup c-qualifiers-group C/C++ Qualifiers
 * C/C++ qualifiers.
 *
 * @note If you add a new `TS_xxx` macro, it must also exist in `QUAL_STIDS[]`
 * inside c_type_name_impl().
 * @{
 */

/** `_Atomic`. */
#define TS__Atomic                C_TID_LIT( STORE, 0x000000010000000 )

/** `const`. */
#define TS_const                  C_TID_LIT( STORE, 0x000000020000000 )

/** `restrict` (C) or `__restrict` (GNU C++). */
#define TS_restrict               C_TID_LIT( STORE, 0x000000040000000 )

/** `volatile`. */
#define TS_volatile               C_TID_LIT( STORE, 0x000000080000000 )

/**
 * C99 adds yet another use for `static`: to make function parameters using
 * array syntax (really, pointers) require their arguments to be both non-null
 * and have a minimum size:
 *
 *      cdecl> explain int x[static 4]
 *
 * If `static` were used in the conversion to pseudo-English:
 *
 *      declare x as static array 4 of integer
 *
 * then it's ambiguous with:
 *
 *      cdecl> explain static int x[4]
 *      declare x as static array 4 of integer
 *
 * Because the new `static` is more qualifier-like (e.g., `const`) than
 * storage-class-like, we define a separate #TS_NON_EMPTY_ARRAY qualifier to
 * mean a C99 "non-null, minimum-sized array" and use `non-empty` in pseudo-
 * English to distinguish it from an ordinary "static array":
 *
 *      cdecl> explain int x[static 4]
 *      declare x as non-empty array 4 of integer
 */
#define TS_NON_EMPTY_ARRAY        C_TID_LIT( STORE, 0x000000100000000 )

/** @} */

/**
 * @defgroup c-upc-qualifiers-group Unified Parallel C Qualifiers
 * Unified Parallel C qualifiers.
 *
 * @note If you add a new `TS_xxx` macro, it must also exist in `QUAL_STIDS[]`
 * inside c_type_name_impl().
 *
 * @sa #LANG_C_99_UPC
 * @sa [Unified Parallel C](http://upc-lang.org/)
 * @{
 */

/** Unified Parallel C `relaxed` qualifier. */
#define TS_UPC_relaxed            C_TID_LIT( STORE, 0x000000200000000 )

/** Unified Parallel C `shared` qualifier. */
#define TS_UPC_shared             C_TID_LIT( STORE, 0x000000400000000 )

/** Unified Parallel C `strict` qualifier. */
#define TS_UPC_strict             C_TID_LIT( STORE, 0x000000800000000 )

/** @} */

/**
 * @defgroup c-ref-qualifiers-group C++ Ref Qualifiers
 * C++ ref-qualifiers.
 *
 * @note If you add a new `TS_xxx` macro, it must also exist in `QUAL_STIDS[]`
 * inside c_type_name_impl().
 * @{
 */

/** E.g., `void f() &`. */
#define TS_REFERENCE              C_TID_LIT( STORE, 0x000001000000000 )

/** E.g., `void f() &&`. */
#define TS_RVALUE_REFERENCE       C_TID_LIT( STORE, 0x000002000000000 )

/** @} */

/**
 * @defgroup c-attributes-group C/C++ Attributes
 * C/C++ attributes.
 *
 * @note If you add a new `TA_xxx` macro, it must also exist in `ATIDS[]`
 * inside c_type_name_impl().
 * @{
 */

/** No attribute. */
#define TA_NONE                   C_TID_LIT( ATTR, 0x000000000000000 )

/** Any attribute. */
#define TA_ANY                    C_TID_LIT( ATTR, 0xFFFFFFFFFFFFFFF )

/** `carries_dependency`. */
#define TA_carries_dependency     C_TID_LIT( ATTR, 0x000000000000001 )

/** `deprecated`. */
#define TA_deprecated             C_TID_LIT( ATTR, 0x000000000000002 )

/** `maybe_unused`. */
#define TA_maybe_unused           C_TID_LIT( ATTR, 0x000000000000004 )

/** `nodiscard`. */
#define TA_nodiscard              C_TID_LIT( ATTR, 0x000000000000008 )

/** `noreturn`. */
#define TA_noreturn               C_TID_LIT( ATTR, 0x000000000000010 )

/** `no_unique_address`. */
#define TA_no_unique_address      C_TID_LIT( ATTR, 0x000000000000020 )

/** `reproducible`. */
#define TA_reproducible           C_TID_LIT( ATTR, 0x000000000000040 )

/** `unsequenced`. */
#define TA_unsequenced            C_TID_LIT( ATTR, 0x000000000000080 )

/** @} */

/**
 * @defgroup c-msc-call-group Microsoft C/C++ Calling Conventions
 * Microsoft Windows C/C++ calling conventions
 *
 * @note If you add a new `TA_MSC_xxx` macro, it must also exist in
 * `MSC_CALL_ATIDS[]` inside c_type_name_impl().
 *
 * @sa [Microsoft Windows calling conventions](https://docs.microsoft.com/en-us/cpp/cpp/argument-passing-and-naming-conventions)
 * @{
 */

/** Microsoft Windows calling convention `__cdecl`. */
#define TA_MSC___cdecl            C_TID_LIT( ATTR, 0x000000000000100 )

/** Microsoft Windows calling convention `__clrcall`. */
#define TA_MSC___clrcall          C_TID_LIT( ATTR, 0x000000000000200 )

/** Microsoft Windows calling convention `__fastcall`. */
#define TA_MSC___fastcall         C_TID_LIT( ATTR, 0x000000000000400 )

/** Microsoft Windows calling convention `__stdcall`. */
#define TA_MSC___stdcall          C_TID_LIT( ATTR, 0x000000000000800 )

/** Microsoft Windows calling convention `__thiscall`. */
#define TA_MSC___thiscall         C_TID_LIT( ATTR, 0x000000000001000 )

/** Microsoft Windows calling convention `__vectorcall`. */
#define TA_MSC___vectorcall       C_TID_LIT( ATTR, 0x000000000002000 )

/** @} */

/**
 * Type part ID bitmask.
 *
 * @sa c_tpid
 */
#define TX_TPID_MASK              0xFull

////////// shorthands /////////////////////////////////////////////////////////

/**
 * @ingroup c-msc-call-group
 * Shorthand for any Microsoft C/C++ calling convention.
 *
 * @sa \ref c-msc-call-group
 * @sa [Microsoft Windows calling conventions](https://docs.microsoft.com/en-us/cpp/cpp/argument-passing-and-naming-conventions)
 */
#define TA_ANY_MSC_CALL       ( TA_MSC___cdecl | TA_MSC___clrcall \
                              | TA_MSC___fastcall | TA_MSC___stdcall \
                              | TA_MSC___thiscall | TA_MSC___vectorcall )

/**
 * @ingroup c-attributes-group
 * The only attributes that can apply to functions.
 *
 * @sa #TA_OBJECT
 */
#define TA_FUNC               ( TA_ANY_MSC_CALL | TA_carries_dependency \
                              | TA_deprecated | TA_maybe_unused \
                              | TA_nodiscard | TA_noreturn | TA_reproducible \
                              | TA_unsequenced )

/**
 * @ingroup c-attributes-group
 * The only attributes that can apply to objects.
 *
 * @sa #TA_FUNC
 */
#define TA_OBJECT             ( TA_carries_dependency | TA_deprecated \
                              | TA_maybe_unused | TA_no_unique_address )

/// @ingroup c-base-types-group
/// Shorthand for any character type.
#define TB_ANY_CHAR           ( TB_char | TB_wchar_t \
                              | TB_char8_t | TB_char16_t | TB_char32_t )

/// @ingroup c-base-types-group
/// Shorthand for `class`, `struct`, or `union`.
#define TB_ANY_CLASS          ( TB_class | TB_struct | TB_union )

/// @ingroup c-base-types-group
/// Shorthand for `enum`, `class`, `struct`, or `union`.
#define TB_ANY_ECSU           ( TB_enum | TB_ANY_CLASS )

/// @ingroup c-emc-types-group
/// Shorthand for any Embedded C type.
#define TB_ANY_EMC            ( TB_EMC__Accum | TB_EMC__Fract )

/// @ingroup c-base-types-group
/// Shorthand for any floating-point type.
#define TB_ANY_FLOAT          ( TB_float | TB_double )

/// @ingroup c-base-types-group
/// Shorthand for any integral type.
#define TB_ANY_INTEGRAL       ( TB_bool | TB_ANY_CHAR | TB__BitInt | TB_int \
                              | TB_ANY_INT_MODIFIER )

/// @ingroup c-base-types-group
/// Shorthand for an any modifier.
#define TB_ANY_INT_MODIFIER   ( TB_short | TB_long | TB_long_long | TB_signed \
                              | TB_unsigned )

/// @ingroup c-base-types-group
/// Shorthand for `class`, `struct`, `union`, or `namespace`.
#define TB_ANY_SCOPE          ( TB_ANY_CLASS | TB_namespace )

/// @ingroup c-storage-types-group
/// Shorthand for any linkage.
#define TS_ANY_LINKAGE        ( TS_extern | TS_extern_C | TS_static )

/// @ingroup c-qualifiers-group
/// Shorthand for any array qualifier.
#define TS_ANY_ARRAY_QUALIFIER \
                              ( TS_CVR | TS_NON_EMPTY_ARRAY )

/// @ingroup c-qualifiers-group
/// Shorthand for any qualifier.
#define TS_ANY_QUALIFIER      ( TS_ANY_ARRAY_QUALIFIER | TS_ANY_UPC \
                              | TS__Atomic )

/// @ingroup c-ref-qualifiers-group
/// Shorthand for any reference qualifier.
#define TS_ANY_REFERENCE      ( TS_REFERENCE | TS_RVALUE_REFERENCE )

/// @ingroup c-storage-types-group
/// Shorthand for any storage.
#define TS_ANY_STORAGE        0x00000000FFFFFFF2ull

/// @ingroup c-upc-qualifiers-group
/// Shorthand for any UPC qualifier.
#define TS_ANY_UPC            ( TS_UPC_relaxed | TS_UPC_shared | TS_UPC_strict )

/// @ingroup c-qualifiers-group
/// The only qualfiers that can apply to concepts.
#define TS_CONCEPT            TS_CV

/// @ingroup c-qualifiers-group
/// Shorthand for `const` or `volatile`.
#define TS_CV                 ( TS_const | TS_volatile )

/// @ingroup c-qualifiers-group
/// Shorthand for `const`, `volatile`, or `restrict`.
#define TS_CVR                ( TS_CV | TS_restrict )

/**
 * @ingroup c-storage-like-types-group
 * The only types that can apply to constructor declarations.
 *
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_CONSTRUCTOR_ONLY
 * @sa #TS_FUNC_LIKE_CPP
 */
#define TS_CONSTRUCTOR_DECL   ( TS_CONSTRUCTOR_DEF | TS_CONSTRUCTOR_ONLY \
                              | TS_default | TS_delete | TS_friend )

/**
 * @ingroup c-storage-like-types-group
 * A subset of #TS_CONSTRUCTOR_DECL that can apply to file-scope constructor
 * definitions.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_ONLY
 */
#define TS_CONSTRUCTOR_DEF    ( TS_constexpr | TS_inline | TS_noexcept \
                              | TS_throw )

/**
 * @ingroup c-storage-like-types-group
 * The types that can apply only to constructors.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 */
#define TS_CONSTRUCTOR_ONLY   TS_explicit

/**
 * @ingroup c-storage-like-types-group
 * The only types that can apply to destructor declarations.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_DESTRUCTOR_DEF
 */
#define TS_DESTRUCTOR_DECL    ( TS_DESTRUCTOR_DEF | TS_delete | TS_final \
                              | TS_friend | TS_override | TS_PURE_virtual \
                              | TS_virtual )

/**
 * @ingroup c-storage-like-types-group
 * A subset of #TS_DESTRUCTOR_DECL that can apply to file-scope destructor
 * definitions.
 *
 * @sa #TS_DESTRUCTOR_DECL
 */
#define TS_DESTRUCTOR_DEF     ( TS_inline | TS_noexcept | TS_throw )

/**
 * @ingroup c-storage-types-group
 * The only storage types that can apply to C functions.
 *
 * @sa #TS_FUNC_LIKE_CPP
 * @sa #TS_MAIN_FUNC_C
 */
#define TS_FUNC_C             ( TS_extern | TS_inline | TS_static | TS_typedef )

/**
 * @ingroup c-storage-like-types-group
 * The only storage types that can apply to C++ function-like things
 * (functions, blocks, constructors, destructors, operators, and user-defined
 * conversion operators and literals).
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_MAIN_FUNC_CPP
 * @sa #TS_NEW_DELETE_OP
 * @sa #TS_USER_DEF_CONV
 */
#define TS_FUNC_LIKE_CPP      ( TS_CVR | TS_consteval | TS_constexpr \
                              | TS_default | TS_delete | TS_explicit \
                              | TS_export | TS_extern_C | TS_final \
                              | TS_friend | TS_FUNC_C | TS_noexcept \
                              | TS_override | TS_PURE_virtual \
                              | TS_ANY_REFERENCE | TS_throw | TS_virtual )

/**
 * @ingroup c-storage-like-types-group
 * The only storage types that can _not_ apply to C++ function like things
 * (functions and operators) that have an explicit object parameter (`this`).
 */
#define TS_FUNC_LIKE_NOT_EXPLICIT_OBJ_PARAM \
                              ( TS_ANY_REFERENCE | TS_CV | TS_static \
                              | TS_virtual )

/**
 * @ingroup c-storage-types-group
 * The only storage types that can apply to C++ function-like parameters.
 */
#define TS_FUNC_LIKE_PARAM    ( TS_register | TS_this )

/**
 * @ingroup c-storage-like-types-group
 * The only storage types that can apply to a C++ lambda.
 */
#define TS_LAMBDA             ( TS_constexpr | TS_consteval | TS_mutable \
                              | TS_noexcept | TS_static | TS_throw )

/**
 * @ingroup c-storage-types-group
 * The only storage types that can apply to a C program's `main()` function.
 *
 * @sa #TS_FUNC_C
 * @sa #TS_MAIN_FUNC_CPP
 */
#define TS_MAIN_FUNC_C        TS_extern

/**
 * @ingroup c-storage-like-types-group
 * The only types that can apply to a C++ program's `main()` function.
 *
 * @sa #TS_FUNC_LIKE_CPP
 * @sa #TS_MAIN_FUNC_C
 */
#define TS_MAIN_FUNC_CPP      ( TS_friend | TS_MAIN_FUNC_C | TS_noexcept \
                              | TS_throw )

/**
 * @ingroup c-storage-like-types-group
 * The types that can apply only to member functions, operators, or user-
 * defined conversions operators.
 *
 * @sa #TS_FUNC_LIKE_CPP
 * @sa #TS_NONMEMBER_FUNC_ONLY
 */
#define TS_MEMBER_FUNC_ONLY   ( TS_CV | TS_delete | TS_final | TS_override \
                              | TS_ANY_REFERENCE | TS_restrict | TS_virtual )

/**
 * @ingroup c-storage-like-types-group
 * The only types that can apply to operators `new`, `new[]`, `delete`, or
 * `delete[]`.
 *
 * @sa #TS_FUNC_LIKE_CPP
 */
#define TS_NEW_DELETE_OP      ( TS_extern | TS_friend | TS_noexcept \
                              | TS_static | TS_throw )

/**
 * @ingroup c-storage-like-types-group
 * The types that can apply only to non-member functions or operators.
 *
 * @sa #TS_MEMBER_FUNC_ONLY
 */
#define TS_NONMEMBER_FUNC_ONLY TS_friend

/**
 * @ingroup c-storage-like-types-group
 * The only types that can apply to structured bindings.
 */
#define TS_STRUCTURED_BINDING ( TS_ANY_REFERENCE | TS_CV | TS_static \
                              | TS_thread_local)

/**
 * @ingroup c-storage-like-types-group
 * The only types that can apply to user-defined conversion operators.
 *
 * @sa #TS_FUNC_LIKE_CPP
 */
#define TS_USER_DEF_CONV      ( TS_const | TS_constexpr | TS_explicit \
                              | TS_final | TS_friend | TS_inline \
                              | TS_noexcept | TS_override | TS_PURE_virtual \
                              | TS_throw | TS_virtual )

/**
 * Hexadecimal print conversion specifier for \ref c_tid_t.
 */
#define PRIX_c_tid_t          PRIX64

////////// extern constants ///////////////////////////////////////////////////

extern c_type_t const T_NONE;           ///< No type.
extern c_type_t const T_ANY;            ///< All types.
extern c_type_t const T_ANY_const_CLASS;///< Any `const` `class`-like type.
extern c_type_t const T_const_char;     ///< `const char`.
extern c_type_t const T_TS_typedef;     ///< Type containing only #TS_typedef.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a type to an existing type, e.g., `short` to `int`, ensuring that a
 * particular type is never added more than once, e.g., `short` to `short int`.
 *
 * @note A special case has to be made for `long` to allow for `long long` yet
 * not allow for `long long long`.
 *
 * @param dst_tids The \ref c_tid_t to add to.
 * @param new_tids The \ref c_tid_t to add.
 * @param new_loc The source location of \a new_tids.
 * @return Returns `true` only if the type added successfully; otherwise,
 * prints an error message at \a new_loc and returns `false`.
 *
 * @sa c_type_add(()
 */
NODISCARD
bool c_tid_add( c_tid_t *dst_tids, c_tid_t new_tids, c_loc_t const *new_loc );

/**
 * Gets the pseudo-English name of \a tids, if available; the C/C++ name if
 * not.
 *
 * @param tids The \ref c_tid_t to get the name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_tid_error()
 * @sa c_tid_gibberish()
 * @sa c_type_english()
 */
NODISCARD
char const* c_tid_english( c_tid_t tids );

/**
 * Gets the name of \a tids for part of an error message.  If translating from
 * pseudo-English to gibberish and the type has a pseudo-English alias, return
 * the alias, e.g., `non-returning` rather than `noreturn`.
 *
 * @param tids The \ref c_tid_t to get the name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_tid_english()
 * @sa c_tid_gibberish()
 * @sa c_type_error()
 */
NODISCARD
char const* c_tid_error( c_tid_t tids );

/**
 * Gets the C/C++ name of \a tids.
 *
 * @param tids The \ref c_tid_t to get the name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_tid_english()
 * @sa c_tid_error()
 * @sa c_type_gibberish()
 */
NODISCARD
char const* c_tid_gibberish( c_tid_t tids );

/**
 * "Normalize" \a tids:
 *
 *  1. If it's #TB_signed and not #TB_char, remove #TB_signed.  If it becomes
 *     #TB_NONE, make it #TB_int.
 *
 *  2. If it's only implicitly #TB_int (e.g., `unsigned`), make it explicitly
 *     #TB_int (e.g., `unsigned int`).
 *
 * @param tids The \ref c_tid_t to normalize.
 * @return Returns the normalized \ref c_tid_t.
 */
NODISCARD
c_tid_t c_tid_normalize( c_tid_t tids );

/**
 * Gets whether the ordering of the scope types \a i_btids, \a j_btids is OK,
 * that is \a i_btids can appear to the left of \a j_btids in a declaration
 * (or, said another way, \a j_btids can nest within \a i_btids).
 *
 * @remarks
 * @parblock
 * The scope type order is:
 *
 * + [`inline`] `namespace` &lt;
 *   { `struct` | `union` | `class` } &lt;
 *   `enum` [`class`]
 *
 * I.e., T1, T2 is OK only if T1 can appear to the left of T2 in a declaration
 * (or T2 can nest within T1).  For example, given:
 *
 *      namespace N { class C { // ...
 *
 * the order `N`, `C` is OK because `N` can appear to the left of `C` in a
 * declaration (`C` can nest within `N`).  However, given:
 *
 *      class D { namespace M { // ...
 *
 * the order `D`, `M` is not OK because `D` can _not_ appear to the left of `M`
 * in a declaration (`M` can _not_ next within `D`).
 * @endparblock
 *
 * @param i_btids The first scope-type ID to compare.
 * @param j_btids The second scope-type ID to compare against.
 * @return Returns `true` only if the scope order is OK.
 */
NODISCARD
bool c_tid_scope_order_ok( c_tid_t i_btids, c_tid_t j_btids );

/**
 * Gets the \ref c_tpid_t from \a tids.
 *
 * @param tids The type ID.
 * @return Returns said \ref c_tpid_t.
 *
 * @sa c_tid_no_tpid()
 */
NODISCARD
c_tpid_t c_tid_tpid( c_tid_t tids );

/**
 * Adds a type to an existing type, e.g., `short` to `int`, ensuring that a
 * particular type is never added more than once, e.g., `short` to `short int`.
 *
 * @note A special case has to be made for `long` to allow for `long long` yet
 * not allow for `long long long`.
 *
 * @param dst_type The \ref c_type to add to.
 * @param new_type The \ref c_type to add.
 * @param new_loc The source location of \a new_type.
 * @return Returns `true` only if \a new_type added successfully; otherwise,
 * prints an error message at \a new_loc and returns `false`.
 *
 * @sa c_tid_add(()
 * @sa c_type_add_tid()
 * @sa c_type_or_eq()
 */
NODISCARD
bool c_type_add( c_type_t *dst_type, c_type_t const *new_type,
                 c_loc_t const *new_loc );

/**
 * Adds \a new_tids to \a dst_type.
 *
 * @param dst_type The \ref c_type to add to.
 * @param new_tids The \ref c_tid_t to add.
 * @param new_loc The source location of \a new_tids.
 * @return Returns `true` only if \a new_tids added successfully; otherwise,
 * prints an error message at \a new_loc and returns `false`.
 *
 * @sa c_type_add()
 */
NODISCARD
bool c_type_add_tid( c_type_t *dst_type, c_tid_t new_tids,
                     c_loc_t const *new_loc );

/**
 * Performs the bitwise-and of all the parts of \a i_type and \a j_type.
 *
 * @param i_type The first \ref c_type.
 * @param j_type The second \ref c_type.
 * @return Returns the resultant \ref c_type.
 *
 * @sa c_type_and_eq_compl()
 * @sa c_type_or()
 */
NODISCARD
c_type_t c_type_and( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Performs the bitwise-and of all the parts of \a dst_type with the complement
 * of \a rm_type and stores the result in \a dst_type.
 *
 * @param dst_type The type to modify.
 * @param rm_type The type to remove from \a dst_type.
 *
 * @sa c_type_or_eq()
 */
void c_type_and_eq_compl( c_type_t *dst_type, c_type_t const *rm_type );

/**
 * Checks that \a type is valid.
 *
 * @param type The \ref c_type to check.
 * @return Returns the bitwise-or of the language(s) \a type is legal in.
 */
NODISCARD
c_lang_id_t c_type_check( c_type_t const *type );

/**
 * Gets the pseudo-English name of \a type, if available; the C/C++ name if
 * not.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_type_error()
 * @sa c_type_gibberish()
 * @sa c_type_name_ecsu()
 */
NODISCARD
char const* c_type_english( c_type_t const *type );

/**
 * Checks whether \a i_type and \a j_type are equivalent (not bitwise
 * identical).  Specifically, the base types are normalized prior to
 * comparison.
 *
 * @param i_type The first \ref c_type.
 * @param j_type The second \ref c_type.
 * @return Returns `true` only if \a i_type is equivalent to \a j_type.
 *
 * @sa c_tid_normalize()
 * @sa c_type_is_none()
 */
NODISCARD
bool c_type_equiv( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Gets the \ref c_tid_t of \a type that corresponds to the type part ID of \a
 * tids.
 *
 * @param type The \ref c_type to get the relevant \ref c_tid_t of.
 * @param tids The \ref c_tid_t that specifies the part of \a type to get the
 * pointer to.
 * @return Returns the corresponding \ref c_tid_t of \a type for the part of \a
 * tids.
 */
NODISCARD
c_tid_t c_type_get_tid( c_type_t const *type, c_tid_t tids );

/**
 * Gets the C/C++ name of \a type.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_tid_gibberish()
 * @sa c_type_english()
 * @sa c_type_error()
 * @sa c_type_name_ecsu()
 */
NODISCARD
char const* c_type_gibberish( c_type_t const *type );

/**
 * For all type part IDs of \a j_type that are not none, gets whether the
 * corresponding type part ID of \a i_type is any of them.
 *
 * @param i_type The first \ref c_type.
 * @param j_type The second \ref c_type.
 * @return Returns `true` only if \a i_type is among \a j_type.
 *
 * @sa c_tid_is_any()
 */
NODISCARD
bool c_type_is_any( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Gets the the C/C++ name for an `enum`, `struct`, `class`, or `union`.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_type_english()
 * @sa c_type_error()
 * @sa c_type_gibberish()
 */
NODISCARD
char const* c_type_name_ecsu( c_type_t const *type );

/**
 * Gets the name of \a type for part of an error message.  If translating
 * from pseudo-English to gibberish and the type has a pseudo-English alias,
 * return the alias, e.g., `non-returning` rather than `noreturn`.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than three times in the same
 * `printf()` statement.
 *
 * @sa c_tid_error()
 * @sa c_type_english()
 * @sa c_type_gibberish()
 * @sa c_type_name_ecsu()
 */
NODISCARD
char const* c_type_error( c_type_t const *type );

/**
 * Performs the bitwise-or of \a i_type and \a j_type.
 *
 * @param i_type The first type.
 * @param j_type The second type.
 * @return Returns the bitwise-or of \a i_type and \a j_type.
 *
 * @sa c_type_and()
 * @sa c_type_and_eq_compl()
 * @sa c_type_or_eq()
 */
NODISCARD
c_type_t c_type_or( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Performs the bitwise-or of \a dst_type with \a add_type and stores the
 * result in \a dst_type.
 *
 * @param dst_type The type to modify.
 * @param add_type The source type.
 *
 * @note Unlike c_type_add(), no checks are made.
 *
 * @sa c_type_add()
 * @sa c_type_and_eq_compl()
 * @sa c_type_or()
 */
void c_type_or_eq( c_type_t *dst_type, c_type_t const *add_type );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks that the type part ID of \a tids is \a tpid.
 *
 * @param tids The \ref c_tid_t to check.
 * @param tpid The \ref c_tpid_t to check against.
 * @return Returns \a tids.
 */
PJL_DISCARD C_TYPE_H_INLINE
c_tid_t c_tid_check( c_tid_t tids, c_tpid_t tpid ) {
  assert( (tids & TX_TPID_MASK) == tpid );
#ifdef NDEBUG
  (void)tpid;
#endif /* NDEBUG */
  return tids;
}

/**
 * Checks whether \a tids has been complemented via `~`.
 *
 * @param tids The \ref c_tid_t to check.
 * @return Returns `true` only if \a tids has been complemented.
 *
 * @sa c_tid_compl()
 */
NODISCARD C_TYPE_H_INLINE
bool c_tid_is_compl( c_tid_t tids ) {
  //
  // The low-order 4 bits specify the c_tpid.  Currently, type part IDs are 1
  // (0b0001), 2 (0b0010), and 4 (0b0100).  If tids is 0b1xxx, it means that it
  // was complemented.
  //
  return (tids & 0x8) != 0;
}

/**
 * Bitwise-complements \a tids.
 *
 * @remarks The `~` operator can't be used alone because the part ID of \a tids
 * would be complemented also.  This function complements \a tids while
 * preserving the original part ID.
 *
 * @param tids The \ref c_tid_t to complement.
 * @return Returns \a tids complemented.
 *
 * @sa c_tid_is_compl()
 */
NODISCARD C_TYPE_H_INLINE
c_tid_t c_tid_compl( c_tid_t tids ) {
  assert( !c_tid_is_compl( tids ) );
  return ~tids ^ TX_TPID_MASK;
}

/**
 * Checks whether \a tids is all of \a is_tids but not also any one of \a
 * except_tids.
 *
 * @param tids The \ref c_tid_t to check.
 * @param is_tids The bitwise-or of \ref c_tid_t to check for.
 * @param except_tids The bitwise-or of \ref c_tid_t to exclude.
 * @return Returns `true` only if \a tids contains any of \a is_tids, but not
 * any of \a except_tids.
 */
NODISCARD C_TYPE_H_INLINE
bool c_tid_is_except_any( c_tid_t tids, c_tid_t is_tids, c_tid_t except_tids ) {
  return (tids & (is_tids | except_tids)) == is_tids;
}

/**
 * Gets the type ID value without the part ID.
 *
 * @param tids The \ref c_tid_t to get the value of.
 * @return Returns the type ID value without the part ID.
 *
 * @sa c_tid_tpid()
 */
NODISCARD C_TYPE_H_INLINE
c_tid_t c_tid_no_tpid( c_tid_t tids ) {
  return tids & ~TX_TPID_MASK;
}

/**
 * Gets whether any \a i_tids are among any \a j_tids.
 *
 * @param i_tids The first \ref c_tid_t.
 * @param j_tids The second \ref c_tid_t.
 * @return Returns `true` only if any \a i_tids are among any \a j_tids.
 *
 * @sa c_type_is_any()
 */
NODISCARD C_TYPE_H_INLINE
bool c_tid_is_any( c_tid_t i_tids, c_tid_t j_tids ) {
  assert( c_tid_tpid( i_tids ) == c_tid_tpid( j_tids ) );
  return c_tid_no_tpid( i_tids & j_tids ) != TX_NONE;
}

/**
 * Checks whether \a tids is "none."
 *
 * @param tids The \ref c_tid_t to check.
 * @return Returns `true` only if \a tids is `Tx_NONE`.
 *
 * @note This function is useful only when the part ID of \a tids can be any
 * part ID.
 *
 * @sa c_type_is_none()
 */
NODISCARD C_TYPE_H_INLINE
bool c_tid_is_none( c_tid_t tids ) {
  return c_tid_no_tpid( tids ) == TX_NONE;
}

/**
 * Checks if \a tids is equivalent to `size_t`.
 *
 * @param tids The \ref c_tid_t to check.
 * @return Returns `true` only if \a tids is `size_t`.
 *
 * @note In **cdecl**, `size_t` is `typedef`d to be `unsigned long` in
 * `c_typedef.c`.
 *
 * @sa c_ast_is_size_t()
 */
NODISCARD C_TYPE_H_INLINE
bool c_tid_is_size_t( c_tid_t tids ) {
  c_tid_check( tids, C_TPID_BASE );
  return (tids & c_tid_compl( TB_int )) == (TB_unsigned | TB_long);
}

/**
 * Checks whether \a type is \ref T_NONE.
 *
 * @param type The \ref c_type to check.
 * @return Returns `true` only if \a type is none.
 *
 * @sa c_type_equiv()
 */
NODISCARD C_TYPE_H_INLINE
bool c_type_is_none( c_type_t const *type ) {
  return c_type_equiv( type, &T_NONE );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_type_H */
/* vim:set et sw=2 ts=2: */
