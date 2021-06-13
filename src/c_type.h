/*
**      cdecl -- C gibberish translator
**      src/c_type.h
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

#ifndef cdecl_c_type_H
#define cdecl_c_type_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ types.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl.h"
#include "c_lang.h"
#include "options.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>                   /* for PRIX64, etc. */

_GL_INLINE_HEADER_BEGIN
#ifndef C_TYPE_INLINE
# define C_TYPE_INLINE _GL_INLINE
#endif /* C_TYPE_INLINE */

/// @endcond

/**
 * @defgroup c-types-group C/C++ Types
 * Constants, types, and functions for C/C++ types.
 * @{
 */

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
  c_tid_t btid;

  /**
   * The storage classes (`extern`, `static`, etc., including `typedef`),
   * storage-class-like things (`default`, `friend`, `inline`, etc.), and also
   * qualifiers (`_Atomic`, `const`, etc.) and ref-qualifiers (`&`, `&&`).
   *
   * Constants for storage-class-like things begin with `TS_`.
   */
  c_tid_t stid;

  /**
   * Attributes.
   *
   * Constants for attributes begin with `TA_`.
   */
  c_tid_t atid;
};

/**
 * Convenience macro for specifying a complete <code>\ref c_type</code>
 * literal.
 *
 * @param BTID The base <code>\ref c_tid_t</code>.
 * @param STID The storage <code>\ref c_tid_t</code>.
 * @param ATID The attribute(s) <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_A_ANY()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_B_ANY()
 * @sa #C_TYPE_LIT_S()
 * @sa #C_TYPE_LIT_S_ANY()
 */
#define C_TYPE_LIT(BTID,STID,ATID) \
  (c_type_t const){ (BTID), (STID), (ATID) }

/**
 * Convenience macro for specifying a <code>\ref c_type</code> literal from
 * \a ATID.
 *
 * @param ATID The attribute(s) <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A_ANY()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_B_ANY()
 * @sa #C_TYPE_LIT_S()
 * @sa #C_TYPE_LIT_S_ANY()
 */
#define C_TYPE_LIT_A(ATID) \
  C_TYPE_LIT( TB_NONE, TS_NONE, (ATID) )

/**
 * Convenience macro for specifying a <code>\ref c_type</code> literal from
 * #TB_ANY, #TS_ANY, and \a ATID.
 *
 * @param ATID The attribute(s) <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_B_ANY()
 * @sa #C_TYPE_LIT_S()
 * @sa #C_TYPE_LIT_S_ANY()
 */
#define C_TYPE_LIT_A_ANY(ATID) \
  C_TYPE_LIT( TB_ANY, TS_ANY, (ATID) )

/**
 * Convenience macro for specifying a <code>\ref c_type</code> literal from
 * \a BTID.
 *
 * @param BTID The base <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_S()
 */
#define C_TYPE_LIT_B(BTID) \
  C_TYPE_LIT( (BTID), TS_NONE, TA_NONE )

/**
 * Convenience macro for specifying a <code>\ref c_type</code> literal from
 * \a BTID, #TS_ANY, and #TA_ANY.
 *
 * @param BTID The base <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_A_ANY()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_S()
 * @sa #C_TYPE_LIT_S_ANY()
 */
#define C_TYPE_LIT_B_ANY(BTID) \
  C_TYPE_LIT( (BTID), TS_ANY, TA_ANY )

/**
 * Convenience macro for specifying a <code>\ref c_type</code> literal from
 * \a STID.
 *
 * @param STID The storage <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_A_ANY()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_B_ANY()
 * @sa #C_TYPE_LIT_S_ANY()
 */
#define C_TYPE_LIT_S(STID) \
  C_TYPE_LIT( TB_NONE, (STID), TA_NONE )

/**
 * Convenience macro for specifying a <code>\ref c_type</code> literal from
 * #TB_ANY, \a STID, and #TA_ANY.
 *
 * @param STID The storage <code>\ref c_tid_t</code>.
 * @return Returns a reference to said literal.
 *
 * @sa #C_TYPE_LIT()
 * @sa #C_TYPE_LIT_A()
 * @sa #C_TYPE_LIT_A_ANY()
 * @sa #C_TYPE_LIT_B()
 * @sa #C_TYPE_LIT_B_ANY()
 * @sa #C_TYPE_LIT_S()
 */
#define C_TYPE_LIT_S_ANY(STID) \
  C_TYPE_LIT( TB_ANY, (STID), TA_ANY )

/**
 * For <code>\ref c_tid_t</code> values, the low-order 4 bits specify the
 * <code>\ref c_tpid</code> and thus how the value should be interpreted.
 */
enum c_tpid {
  //
  // Type part IDs start at 1 so we know a c_tid_t value has been initialized
  // properly as opposed to it being 0 by default.
  //
  C_TPID_NONE   = 0,                    ///< No types.
  C_TPID_BASE   = (1u << 0),            ///< Base types, e.g., `int`.
  C_TPID_STORE  = (1u << 1),            ///< Storage types, e.g., `static`.
  C_TPID_ATTR   = (1u << 2)             ///< Attributes.
};

//
// The difference between TB_TYPEDEF and TS_TYPEDEF is that:
//
//  * TS_TYPEDEF is the "storage class" for a declaration as a whole while it's
//    being declared during parsing.
//
//  * TB_TYPEDEF is the "type" for a particular typedef'd type, e.g., size_t,
//    after parsing a declaration and the new type has been defined.  Hence,
//    TB_TYPEDEF is the somewhat unnecessary when the kind of an AST is
//    K_TYPEDEF, but it has to have some type.
//

#define TX_NONE               0x0000000000000000ull /**< No type at all.      */

// base types & modifiers
#define TB_NONE               0x0000000000000001ull /**< No base type.        */
#define TB_ANY                0xFFFFFFFFFFFFFFF1ull /**< Any base type.       */
#define TB_VOID               0x0000000000000011ull /**< `void`               */
#define TB_AUTO               0x0000000000000021ull /**< C++11's `auto`.      */
#define TB_BOOL               0x0000000000000041ull /**< `_Bool` or `bool`    */
#define TB_CHAR               0x0000000000000081ull /**< `char`               */
#define TB_CHAR8_T            0x0000000000000101ull /**< `char8_t`            */
#define TB_CHAR16_T           0x0000000000000201ull /**< `char16_t`           */
#define TB_CHAR32_T           0x0000000000000401ull /**< `char32_t`           */
#define TB_WCHAR_T            0x0000000000000801ull /**< `wchar_t`            */
#define TB_SHORT              0x0000000000001001ull /**< `short`              */
#define TB_INT                0x0000000000002001ull /**< `int`                */
#define TB_LONG               0x0000000000004001ull /**< `long`               */
#define TB_LONG_LONG          0x0000000000008001ull /**< `long long`          */
#define TB_SIGNED             0x0000000000010001ull /**< `signed`             */
#define TB_UNSIGNED           0x0000000000020001ull /**< `unsigned`           */
#define TB_FLOAT              0x0000000000040001ull /**< `float`              */
#define TB_DOUBLE             0x0000000000080001ull /**< `double`             */
#define TB_COMPLEX            0x0000000000100001ull /**< `_Complex`           */
#define TB_IMAGINARY          0x0000000000200001ull /**< `_Imaginary`         */
#define TB_ENUM               0x0000000000400001ull /**< `enum`               */
#define TB_STRUCT             0x0000000000800001ull /**< `struct`             */
#define TB_UNION              0x0000000001000001ull /**< `union`              */
#define TB_CLASS              0x0000000002000001ull /**< `class`              */
#define TB_NAMESPACE          0x0000000004000001ull /**< `namespace`          */
#define TB_SCOPE              0x0000000008000001ull /**< Generic scope.       */
#define TB_TYPEDEF            0x0000000010000001ull /**< E.g., `size_t`       */

// Embedded C types & modifiers
#define TB_EMC_ACCUM          0x0000000020000001ull /**< `_Accum`             */
#define TB_EMC_FRACT          0x0000000040000001ull /**< `_Fract`             */
#define TB_EMC_SAT            0x0000000080000001ull /**< `_Sat`               */

// storage classes
#define TS_NONE               0x0000000000000002ull /**< No storage type.     */
#define TS_ANY                0xFFFFFFFFFFFFFFF2ull /**< Any storage type.    */
#define TS_AUTO               0x0000000000000012ull /**< C's `auto`.          */
#define TS_APPLE_BLOCK        0x0000000000000022ull /**< Block.               */
#define TS_EXTERN             0x0000000000000042ull /**< `extern`             */
#define TS_EXTERN_C           0x0000000000000082ull /**< `extern "C"`         */
#define TS_MUTABLE            0x0000000000000102ull /**< `mutable`            */
#define TS_REGISTER           0x0000000000000202ull /**< `register`           */
#define TS_STATIC             0x0000000000000402ull /**< `static`             */
#define TS_THREAD_LOCAL       0x0000000000000802ull /**< `thread_local`       */
#define TS_TYPEDEF            0x0000000000001002ull /**< `typedef` or `using` */

// storage-class-like
#define TS_CONSTEVAL          0x0000000000002002ull /**< `consteval`          */
#define TS_CONSTEXPR          0x0000000000004002ull /**< `constexpr`          */
#define TS_CONSTINIT          0x0000000000008002ull /**< `constinit`          */
#define TS_DEFAULT            0x0000000000010002ull /**< `= default`          */
#define TS_DELETE             0x0000000000020002ull /**< `= delete`           */
#define TS_EXPLICIT           0x0000000000040002ull /**< `explicit`           */
#define TS_EXPORT             0x0000000000080002ull /**< `export`             */
#define TS_FINAL              0x0000000000100002ull /**< `final`              */
#define TS_FRIEND             0x0000000000200002ull /**< `friend`             */
#define TS_INLINE             0x0000000000400002ull /**< `inline`             */
#define TS_NOEXCEPT           0x0000000000800002ull /**< `noexcept`           */
#define TS_OVERRIDE           0x0000000001000002ull /**< `override`           */
#define TS_PURE_VIRTUAL       0x0000000002000002ull /**< `= 0`                */
#define TS_THROW              0x0000000004000002ull /**< `throw()`            */
#define TS_VIRTUAL            0x0000000008000002ull /**< `virtual`            */

// qualifiers
#define TS_ATOMIC             0x0000000010000002ull /**< `_Atomic`            */
#define TS_CONST              0x0000000020000002ull /**< `const`              */
#define TS_RESTRICT           0x0000000040000002ull /**< `restrict`           */
#define TS_VOLATILE           0x0000000080000002ull /**< `volatile`           */

// Unified Parallel C qualifiers
#define TS_UPC_RELAXED        0x0000000100000002ull /**< `relaxed`            */
#define TS_UPC_SHARED         0x0000000200000002ull /**< `shared`             */
#define TS_UPC_STRICT         0x0000000400000002ull /**< `strict`             */

// ref-qualifiers
#define TS_REFERENCE          0x0000000800000002ull /**< `void f() &`         */
#define TS_RVALUE_REFERENCE   0x0000001000000002ull /**< `void f() &&`        */

// attributes
#define TA_NONE               0x0000000000000004ull /**< No attribute.        */
#define TA_ANY                0xFFFFFFFFFFFFFFF4ull /**< Any attribute.       */
#define TA_CARRIES_DEPENDENCY 0x0000000000000014ull /**< `carries_dependency` */
#define TA_DEPRECATED         0x0000000000000024ull /**< `deprecated`         */
#define TA_MAYBE_UNUSED       0x0000000000000044ull /**< `maybe_unused`       */
#define TA_NODISCARD          0x0000000000000084ull /**< `nodiscard`          */
#define TA_NORETURN           0x0000000000000104ull /**< `noreturn`           */
#define TA_NO_UNIQUE_ADDRESS  0x0000000000000204ull /**< `no_unique_address`  */

// bit masks
#define TX_MASK_TPID          0x000000000000000Full /**< Type part ID bitmask.*/
#define TS_MASK_STORAGE       0x000000000FFFFFF2ull /**< Storage bitmask.     */
#define TS_MASK_QUALIFIER     0x00000007F0000002ull /**< Qualifier bitmask.   */
#define TS_MASK_REF_QUALIFIER 0x0000001800000002ull /**< Ref-qual bitmask.    */

extern c_type_t const T_NONE;           ///< No type.
extern c_type_t const T_ANY;            ///< All types.
extern c_type_t const T_ANY_CONST_CLASS;///< Any `const` `class`-like type.
extern c_type_t const T_TS_TYPEDEF;     ///< Type containing only #TS_TYPEDEF.

// shorthands

/// Shorthand for any character type.
#define TB_ANY_CHAR           ( TB_CHAR | TB_WCHAR_T \
                              | TB_CHAR8_T | TB_CHAR16_T | TB_CHAR32_T )

/// Shorthand for `class`, `struct`, or `union`.
#define TB_ANY_CLASS          ( TB_CLASS | TB_STRUCT | TB_UNION )

/// Shorthand for any Embedded C type.
#define TB_ANY_EMC            ( TB_EMC_ACCUM | TB_EMC_FRACT )

/// Shorthand for any floating-point type.
#define TB_ANY_FLOAT          ( TB_FLOAT | TB_DOUBLE )

/// Shorthand for any integral type.
#define TB_ANY_INTEGRAL       ( TB_BOOL | TB_ANY_CHAR | TB_INT \
                              | TB_ANY_MODIFIER )

/// Shorthand for any type char can be modified by #TB_ANY_MODIFIER.
#define TB_ANY_MODIFIEE       ( TB_CHAR | TB_ANY_FLOAT | TB_ANY_EMC )

/// Shorthand for an any modifier.
#define TB_ANY_MODIFIER       ( TB_SHORT | TB_LONG | TB_LONG_LONG | TB_SIGNED \
                              | TB_UNSIGNED )

/// Shorthand for `class`, `struct`, `union`, or `namespace`.
#define TB_ANY_SCOPE          ( TB_ANY_CLASS | TB_NAMESPACE )

/// Shorthand for any reference qualifier.
#define TS_ANY_REFERENCE      ( TS_REFERENCE | TS_RVALUE_REFERENCE )

/// Shorthand for `const` or `volatile`.
#define TS_CONST_VOLATILE     ( TS_CONST | TS_VOLATILE )

/**
 * The only types that can apply to in-class constructor declarations.
 *
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_CONSTRUCTOR_ONLY
 * @sa #TS_FUNC_LIKE
 */
#define TS_CONSTRUCTOR_DECL   ( TS_CONSTEXPR | TS_DEFAULT | TS_DELETE \
                              | TS_EXPLICIT | TS_FRIEND | TS_INLINE \
                              | TS_NOEXCEPT | TS_THROW )

/**
 * A subset of #TS_CONSTRUCTOR_DECL that can apply to file-scope constructor
 * definitions.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 */
#define TS_CONSTRUCTOR_DEF    ( TS_CONSTEXPR | TS_INLINE | TS_NOEXCEPT \
                              | TS_THROW )

/**
 * The types that can apply only to constructors.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 */
#define TS_CONSTRUCTOR_ONLY   TS_EXPLICIT

/**
 * The only types that can apply to in-class destructor declarations.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_DESTRUCTOR_DEF
 */
#define TS_DESTRUCTOR_DECL    ( TS_DEFAULT | TS_DELETE | TS_FINAL | TS_FRIEND \
                              | TS_INLINE | TS_NOEXCEPT | TS_OVERRIDE \
                              | TS_PURE_VIRTUAL | TS_THROW | TS_VIRTUAL )

/**
 * A subset of #TS_DESTRUCTOR_DECL that can apply to file-scope destructor
 * definitions.
 *
 * @sa #TS_DESTRUCTOR_DECL
 */
#define TS_DESTRUCTOR_DEF     ( TS_INLINE | TS_NOEXCEPT | TS_THROW )

/**
 * The only types that can apply to function-like things (functions, blocks,
 * constructors, destructors, operators, and user-defined conversion
 * operators and literals).
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_MAIN_FUNC
 * @sa #TS_NEW_DELETE_OPER
 * @sa #TS_USER_DEF_CONV
 */
#define TS_FUNC_LIKE          ( TS_CONST_VOLATILE | TS_CONSTEVAL \
                              | TS_CONSTEXPR | TS_DEFAULT | TS_DELETE \
                              | TS_EXPLICIT | TS_EXPORT | TS_EXTERN \
                              | TS_EXTERN_C | TS_FINAL | TS_FRIEND | TS_INLINE \
                              | TS_NOEXCEPT | TS_OVERRIDE | TS_PURE_VIRTUAL \
                              | TS_ANY_REFERENCE | TS_RESTRICT | TS_STATIC \
                              | TS_THROW | TS_TYPEDEF | TS_VIRTUAL )

/**
 * The only types that can apply to the program's `main()` function.
 *
 * @sa #TS_FUNC_LIKE
 */
#define TS_MAIN_FUNC          ( TS_EXTERN | TS_FRIEND | TS_NOEXCEPT | TS_THROW )

/**
 * The types that can apply only to member functions, operators, or user-
 * defined conversions operators.
 *
 * @sa #TS_FUNC_LIKE
 * @sa #TS_NONMEMBER_FUNC_ONLY
 */
#define TS_MEMBER_FUNC_ONLY   ( TS_CONST_VOLATILE \
                              | (opt_lang < LANG_CPP_20 ? TS_DEFAULT : TS_NONE)\
                              | TS_DELETE | TS_FINAL | TS_OVERRIDE \
                              | TS_ANY_REFERENCE | TS_RESTRICT | TS_VIRTUAL )

/**
 * The only types that can apply to operators `new`, `new[]`, `delete`, or
 * `delete[]`.
 *
 * @sa #TS_FUNC_LIKE
 */
#define TS_NEW_DELETE_OPER    ( TS_EXTERN | TS_FRIEND | TS_NOEXCEPT \
                              | TS_STATIC | TS_THROW )

/**
 * The types that can apply only to non-member functions or operators.
 *
 * @sa #TS_MEMBER_FUNC_ONLY
 */
#define TS_NONMEMBER_FUNC_ONLY TS_FRIEND

/**
 * The types that can apply only to function-like things except constructors.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_CONSTRUCTOR_ONLY
 * @sa #TS_FUNC_LIKE
 */
#define TS_NOT_CONSTRUCTOR    ( TS_CONST_VOLATILE | TS_EXTERN | TS_EXTERN_C \
                              | TS_FINAL | TS_OVERRIDE | TS_ANY_REFERENCE \
                              | TS_RESTRICT | TS_STATIC | TS_VIRTUAL )

/**
 * The only types that can apply to user-defined conversion operators.
 *
 * @sa #TS_FUNC_LIKE
 */
#define TS_USER_DEF_CONV      ( TS_CONST | TS_CONSTEXPR | TS_EXPLICIT \
                              | TS_FINAL | TS_FRIEND | TS_INLINE \
                              | TS_NOEXCEPT | TS_OVERRIDE | TS_PURE_VIRTUAL \
                              | TS_THROW | TS_VIRTUAL )

/**
 * Hexadecimal print conversion specifier for <code>\ref c_tid_t</code>.
 */
#define PRIX_C_TID_T          PRIX64

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a type to an existing type, e.g., `short` to `int`, ensuring that a
 * particular type is never added more than once, e.g., `short` to `short int`.
 *
 * A special case has to be made for `long` to allow for `long long` yet not
 * allow for `long long long`.
 *
 * @param dst_tids The <code>\ref c_tid_t</code> to add to.
 * @param new_tid The <code>\ref c_tid_t</code> to add.
 * @param new_loc The source location of \a new_id.
 * @return Returns `true` only if the type added successfully.
 *
 * @sa c_type_add(()
 */
PJL_WARN_UNUSED_RESULT
bool c_tid_add( c_tid_t *dst_tids, c_tid_t new_tid, c_loc_t const *new_loc );

/**
 * "Normalize" \a tid:
 *
 *  1. If it's #TB_SIGNED and not #TB_CHAR, remove #TB_SIGNED.
 *  2. If it becomes #TB_NONE, add #TB_INT.
 *
 * @param tid The type to normalize.
 * @return Returns the normalized type.
 */
PJL_WARN_UNUSED_RESULT
c_tid_t c_tid_normalize( c_tid_t tid );

/**
 * Gets the "order" value of a <code>\ref c_tid_t</code> so it can be compared
 * by its order.  The order is:
 *
 * + { _none_ | `scope` }  &lt; [`inline`] `namespace` &lt;
 *   { `struct` | `union` | `class` } &lt;
 *   `enum` [`class`]
 *
 * I.e., the order of T1 &le; T2 only if T1 can appear to the left (&lt;) of T2
 * in a declaration.  For example, given:
 * ```
 *  namespace N { class C { // ...
 * ```
 * order(`N`) &le; order(`C`) because `N` can appear to the left of `C` in a
 * declaration.  However, given:
 * ```
 *  class D { namespace M { // ...
 * ```
 * order(`D`) &gt; order(`M`) and so `D` can not appear to the left of `M`.
 *
 * @param tid The scope-type ID to get the order of.
 * @return Returns said order.
 */
unsigned c_tid_scope_order( c_tid_t tid );

/**
 * Gets the `c_tpid_t` from \a tid.
 *
 * @param tid The type ID.
 * @return Returns said `c_tpid_t`.
 *
 * @sa c_tid_no_tpid()
 */
PJL_WARN_UNUSED_RESULT
c_tpid_t c_tid_tpid( c_tid_t tid );

/**
 * Adds a type to an existing type, e.g., `short` to `int`, ensuring that a
 * particular type is never added more than once, e.g., `short` to `short int`.
 *
 * A special case has to be made for `long` to allow for `long long` yet not
 * allow for `long long long`.
 *
 * @param dst_type The <code>\ref c_type</code> to add to.
 * @param new_type The <code>\ref c_type</code> to add.
 * @param new_loc The source location of \a new_type.
 * @return Returns `true` only if \a new_type added successfully.
 *
 * @sa c_tid_add(()
 * @sa c_type_add_tid()
 * @sa c_type_or_eq()
 */
PJL_WARN_UNUSED_RESULT
bool c_type_add( c_type_t *dst_type, c_type_t const *new_type,
                 c_loc_t const *new_loc );

/**
 * Adds \a new_tid to \a dst_type.
 *
 * @param dst_type The <code>\ref c_type</code> to add to.
 * @param new_tid The <code>\ref c_tid_t</code> to add.
 * @param new_loc The source location of \a new_tid.
 * @return Returns `true` only if \a new_tid added successfully.
 *
 * @sa c_type_add()
 */
PJL_WARN_UNUSED_RESULT
bool c_type_add_tid( c_type_t *dst_type, c_tid_t new_tid,
                     c_loc_t const *new_loc );

/**
 * Performs the bitwise-and of all the parts of \a i_type and \a j_type.
 *
 * @param i_type The first <code>\ref c_type</code>.
 * @param j_type The second <code>\ref c_type</code>.
 * @return Returns the resultant <code>\ref c_type</code>.
 *
 * @sa c_type_and_eq_compl()
 * @sa c_type_or()
 */
PJL_WARN_UNUSED_RESULT
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
 * @param type The <code>\ref c_type</code> to check.
 * @return Returns the bitwise-or of the language(s) \a type is legal in.
 */
PJL_WARN_UNUSED_RESULT
c_lang_id_t c_type_check( c_type_t const *type );

/**
 * Checks whether \a i_type and \a j_type are equal.
 *
 * @param i_type The first <code>\ref c_type</code>.
 * @param j_type The second <code>\ref c_type</code>.
 * @return Returns `true` only if \a i_type equals \a j_type.
 *
 * @sa c_type_is_none()
 */
PJL_WARN_UNUSED_RESULT
bool c_type_equal( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Creates a <code>\ref c_type</code> based on the type part ID of \a tid.
 *
 * @param tid The <code>\ref c_tid_t</code> to create the <code>\ref
 * c_type</code> from.
 * @return Returns said <code>\ref c_type</code>.
 */
PJL_WARN_UNUSED_RESULT
c_type_t c_type_from_tid( c_tid_t tid );

/**
 * Gets the <code>\ref c_tid_t</code> of \a type that corresponds to the type
 * part ID of \a tid.
 *
 * @param type The <code>\ref c_type</code> to get the relevant <code>\ref
 * c_tid_t</code> of.
 * @param tid The <code>\ref c_tid_t</code> that specifies the part of \a type
 * to get the pointer to.
 * @return Returns the corresponding <code>\ref c_tid_t</code> of \a type for
 * the part of \a tid.
 *
 * @sa c_type_get_tid_ptr()
 */
PJL_WARN_UNUSED_RESULT
c_tid_t c_type_get_tid( c_type_t const *type, c_tid_t tid );

/**
 * Gets a pointer to the <code>\ref c_tid_t</code> of \a type that corresponds
 * to the type part ID of \a tid.
 *
 * @param type The <code>\ref c_type</code> to get a pointer to the relevant
 * <code>\ref c_tid_t</code> of.
 * @param tid The <code>\ref c_tid_t</code> that specifies the part of \a type
 * to get the pointer to.
 * @return Returns a pointer to the corresponding <code>\ref c_tid_t</code> of
 * \a type for the part of \a tid.
 *
 * @sa c_type_get_tid()
 */
PJL_WARN_UNUSED_RESULT
c_tid_t* c_type_get_tid_ptr( c_type_t *type, c_tid_t tid );

/**
 * For all type part IDs of \a j_type that are not none, gets whether the
 * corresponding type part ID of \a i_type is any of them.
 *
 * @param i_type The first <code>\ref c_type</code>.
 * @param j_type The second <code>\ref c_type</code>.
 * @return Returns `true` only if \a i_type contains any \a j_type.
 *
 * @sa c_tid_is_any()
 */
PJL_WARN_UNUSED_RESULT
bool c_type_is_any( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Gets the name of \a type.
 *
 * @note
 * This function isn't normally called directly; use one of c_type_name_c(),
 * c_type_name_english(), or c_type_name_error().
 *
 * @param type The type to get the name for.
 * @param in_english If `true`, return the pseudo-English name if possible.
 * @return Returns said name.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_type_name_c()
 * @sa c_type_name_english()
 * @sa c_type_name_error()
 */
PJL_WARN_UNUSED_RESULT
char const* c_type_name( c_type_t const *type, bool in_english );

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
PJL_WARN_UNUSED_RESULT
c_type_t c_type_or( c_type_t const *i_type, c_type_t const *j_type );

/**
 * Performs the bitwise-or of \a dst_type with \a add_type and stores the
 * result in \a dst_type.
 * @note Unlike c_type_add(), no checks are made.
 *
 * @param dst_type The type to modify.
 * @param add_type The source type.
 *
 * @sa c_type_add()
 * @sa c_type_and_eq_compl()
 * @sa c_type_or()
 */
void c_type_or_eq( c_type_t *dst_type, c_type_t const *add_type );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks that the type part ID of \a tid is \a tpid.
 *
 * @param tid The <code>\ref c_tid_t</code> to check.
 * @param tpid The <code>\ref c_tpid_t</code> to check against.
 * @return Returns \a tid.
 */
C_TYPE_INLINE PJL_NOWARN_UNUSED_RESULT
c_tid_t c_tid_check( c_tid_t tid, c_tpid_t tpid ) {
  assert( (tid & TX_MASK_TPID) == tpid );
  return tid;
}

/**
 * Checks whether \a tid has been complemented via `~`.
 *
 * @param tid The <code>\ref c_tid_t</code> to check.
 * @return Returns `true` only if \a tid has been complemented.
 *
 * @sa c_tid_compl()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
bool c_tid_is_compl( c_tid_t tid ) {
  //
  // The low-order 4 bits specify the c_tpid.  Currently, type part IDs are 1
  // (0b0001), 2 (0b0010), and 4 (0b0100).  If tid is 0b1xxx, it means that it
  // was complemented.
  //
  return (tid & 0x8) != 0;
}

/**
 * Bitwise-complements \a tid.  The `~` operator can't be used alone because
 * the part ID of \a tid would be complemented also.  This function complements
 * \a tid while preserving the original part ID.
 *
 * @param tid The <code>\ref c_tid_t</code> to complement.
 * @return Returns \a tid complemented.
 *
 * @sa c_tid_is_compl()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
c_tid_t c_tid_compl( c_tid_t tid ) {
  assert( !c_tid_is_compl( tid ) );
  return ~tid ^ TX_MASK_TPID;
}

/**
 * Gets the C/C++ name of \a tid.
 *
 * @param tid The <code>\ref c_tid_t</code> to get the name of.
 * @return Returns said name.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_eng()
 * @sa c_tid_name_error()
 * @sa c_type_name_c()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
char const* c_tid_name_c( c_tid_t tid ) {
  c_type_t const type = c_type_from_tid( tid );
  return c_type_name( &type, /*in_english=*/false );
}

/**
 * Gets the pseudo-English name of \a tid, if available; the C/C++ name if not.
 *
 * @param tid The <code>\ref c_tid_t</code> to get the name of.
 * @return Returns said name.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_c()
 * @sa c_tid_name_error()
 * @sa c_type_name_english()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
char const* c_tid_name_eng( c_tid_t tid ) {
  c_type_t const type = c_type_from_tid( tid );
  return c_type_name( &type, /*in_english=*/true );
}

/**
 * Gets the name of \a tid for part of an error message.  If translating from
 * pseudo-English to gibberish and the type has an pseudo-English alias, return
 * the alias, e.g., `non-returning` rather than `noreturn`.
 *
 * @param tid The <code>\ref c_tid_t</code> to get the name of.
 * @return Returns said name.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_c()
 * @sa c_tid_name_eng()
 * @sa c_type_name_error()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
char const* c_tid_name_error( c_tid_t tid ) {
  c_type_t const type = c_type_from_tid( tid );
  // When giving an error message, return the type name in pseudo-English if
  // we're parsing pseudo-English or in C/C++ if we're parsing C/C++.
  return c_type_name( &type, /*in_english=*/c_mode == C_ENGLISH_TO_GIBBERISH );
}

/**
 * Gets the type ID value without the part ID.
 *
 * @param tid The <code>\ref c_tid_t</code> to get the value of.
 * @return Returns the type ID value without the part ID.
 *
 * @sa c_tid_tpid()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
c_tid_t c_tid_no_tpid( c_tid_t tid ) {
  return tid & ~TX_MASK_TPID;
}

/**
 * Gets whether \a i_tid contains any of \a j_tid.
 *
 * @param i_tid The first <code>\ref c_tid_t</code>.
 * @param j_tid The second <code>\ref c_tid_t</code>.
 * @return Returns `true` only if \a i_tid contains any \a j_tid.
 *
 * @sa c_type_is_any()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
bool c_tid_is_any( c_tid_t i_tid, c_tid_t j_tid ) {
  assert( c_tid_tpid( i_tid ) == c_tid_tpid( j_tid ) );
  return c_tid_no_tpid( i_tid & j_tid ) != TX_NONE;
}

/**
 * Checks whether \a tid is "none."
 *
 * @note This function is useful only when the part ID of \a tid can be any
 * part ID.
 * @param tid The <code>\ref c_tid_t</code> to check.
 * @return Returns `true` only if \a tid is `Tx_NONE`.
 *
 * @sa c_type_is_none()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
bool c_tid_is_none( c_tid_t tid ) {
  return c_tid_no_tpid( tid ) == TX_NONE;
}

/**
 * Checks if \a tid is equivalent to `size_t`.
 *
 * @note
 * In cdecl, `size_t` is `typedef`d to be `unsigned long` in `c_typedef.c`.
 *
 * @param tid The <code>\ref c_tid_t</code> to check.
 * @return Returns `true` only if \a tid is `size_t`.
 *
 * @sa c_ast_is_size_t()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
bool c_tid_is_size_t( c_tid_t tid ) {
  assert( (tid & TX_MASK_TPID) == C_TPID_BASE );
  return (tid & c_tid_compl( TB_INT )) == (TB_UNSIGNED | TB_LONG);
}

/**
 * Checks whether \a type is T_NONE.
 *
 * @param type The <code>\ref c_type</code> to check.
 * @return Returns `true` only if \a type is none.
 *
 * @sa c_type_equal()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
bool c_type_is_none( c_type_t const *type ) {
  return c_type_equal( type, &T_NONE );
}

/**
 * Checks whether the relevant <code>\ref c_tid_t</code> of \a type is any
 * of \a tids.
 *
 * @param type The <code>\ref c_type</code> to check.
 * @param tids The <code>\ref c_tid_t</code> to check against.
 * @return Returns `true` only if the relevant <code>\ref c_tid_t</code> of
 * \a type contains any of \a tids.
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
bool c_type_is_tid_any( c_type_t const *type, c_tid_t tids ) {
  return c_tid_is_any( c_type_get_tid( type, tids ), tids );
}

/**
 * Gets the C/C++ name of \a type.
 *
 * @param type The type to get the name for.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_type_name()
 * @sa c_type_name_english()
 * @sa c_type_name_error()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
char const* c_type_name_c( c_type_t const *type ) {
  return c_type_name( type, /*in_english=*/false );
}

/**
 * Gets the pseudo-English name of \a type, if available; the C/C++ name if
 * not.
 *
 * @param type The type to get the name for.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_type_name()
 * @sa c_type_name_c()
 * @sa c_type_name_error()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
char const* c_type_name_english( c_type_t const *type ) {
  return c_type_name( type, /*in_english=*/true );
}

/**
 * Gets the name of \a type for part of an error message.  If translating
 * from pseudo-English to gibberish and the type has a pseudo-English alias,
 * return the alias, e.g., `non-returning` rather than `noreturn`.
 *
 * @param type The type to get the name for.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_type_name()
 * @sa c_type_name_c()
 * @sa c_type_name_english()
 */
C_TYPE_INLINE PJL_WARN_UNUSED_RESULT
char const* c_type_name_error( c_type_t const *type ) {
  // See comment in c_tid_name_error().
  return c_type_name( type, /*in_english=*/c_mode == C_ENGLISH_TO_GIBBERISH );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_type_H */
/* vim:set et sw=2 ts=2: */
