/*
**      cdecl -- C gibberish translator
**      src/c_type.h
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
 * For \ref c_tid_t values, the low-order 4 bits specify the type part ID and
 * thus how the value should be interpreted.
 */
enum c_tpid {
  //
  // Type part IDs start at 1 so we know a c_tid_t value has been initialized
  // properly as opposed to it being 0 by default.
  //
  C_TPID_NONE   = 0u,                   ///< No types.
  C_TPID_BASE   = (1u << 0),            ///< Base types, e.g., `int`.
  C_TPID_STORE  = (1u << 1),            ///< Storage types, e.g., `static`.
  C_TPID_ATTR   = (1u << 2)             ///< Attributes.
};
typedef enum c_tpid c_tpid_t;

/**
 * Convenience macro for specifying a complete \ref c_type literal.
 *
 * @param BTID The base \ref c_tid_t.
 * @param STID The storage \ref c_tid_t.
 * @param ATID The attribute(s) \ref c_tid_t.
 * @return Returns said literal.
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
 * Convenience macro for specifying a \ref c_type literal from \a ATID.
 *
 * @param ATID The attribute(s) \ref c_tid_t.
 * @return Returns said literal.
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
 * Convenience macro for specifying a \ref c_type literal from #TB_ANY,
 * #TS_ANY, and \a ATID.
 *
 * @param ATID The attribute(s) \ref c_tid_t.
 * @return Returns said literal.
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
 * Convenience macro for specifying a \ref c_type literal from \a BTID,
 * #TS_ANY, and #TA_ANY.
 *
 * @param BTID The base \ref c_tid_t.
 * @return Returns said literal.
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
 * Convenience macro for specifying a \ref c_type literal from \a STID.
 *
 * @param STID The storage \ref c_tid_t.
 * @return Returns said literal.
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
 * Convenience macro for specifying a \ref c_type literal from #TB_ANY, \a
 * STID, and #TA_ANY.
 *
 * @param STID The storage \ref c_tid_t.
 * @return Returns said literal.
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

//
// Base types & modifiers
//
// If you add a new TB_xxx here, it must also exist in BTIDS[] inside
// c_type_name_impl().
//
#define TB_NONE               0x0000000000000001ull /**< No base type.        */
#define TB_ANY                0xFFFFFFFFFFFFFFF1ull /**< Any base type.       */
#define TB_VOID               0x0000000000000011ull /**< `void`               */
#define TB_AUTO               0x0000000000000021ull /**< C23, C++11 `auto`.   */
#define TB_BITINT             0x0000000000000041ull /**< `_BitInt`            */
#define TB_BOOL               0x0000000000000081ull /**< `_Bool` or `bool`    */
#define TB_CHAR               0x0000000000000101ull /**< `char`               */
#define TB_CHAR8_T            0x0000000000000201ull /**< `char8_t`            */
#define TB_CHAR16_T           0x0000000000000401ull /**< `char16_t`           */
#define TB_CHAR32_T           0x0000000000000801ull /**< `char32_t`           */
#define TB_WCHAR_T            0x0000000000001001ull /**< `wchar_t`            */
#define TB_SHORT              0x0000000000002001ull /**< `short`              */
#define TB_INT                0x0000000000004001ull /**< `int`                */
#define TB_LONG               0x0000000000008001ull /**< `long`               */
#define TB_LONG_LONG          0x0000000000010001ull /**< `long long`          */
#define TB_SIGNED             0x0000000000020001ull /**< `signed`             */
#define TB_UNSIGNED           0x0000000000040001ull /**< `unsigned`           */
#define TB_FLOAT              0x0000000000080001ull /**< `float`              */
#define TB_DOUBLE             0x0000000000100001ull /**< `double`             */
#define TB_COMPLEX            0x0000000000200001ull /**< `_Complex`           */
#define TB_IMAGINARY          0x0000000000400001ull /**< `_Imaginary`         */
#define TB_ENUM               0x0000000000800001ull /**< `enum`               */
#define TB_STRUCT             0x0000000001000001ull /**< `struct`             */
#define TB_UNION              0x0000000002000001ull /**< `union`              */
#define TB_CLASS              0x0000000004000001ull /**< `class`              */
#define TB_NAMESPACE          0x0000000008000001ull /**< `namespace`          */
#define TB_SCOPE              0x0000000010000001ull /**< Generic scope.       */
#define TB_TYPEDEF            0x0000000020000001ull /**< E.g., `size_t`       */

//
// Embedded C types & modifiers
//
// If you add a new TB_EMC_xxx here, it must also exist in BTIDS[] inside
// c_type_name_impl().
//
#define TB_EMC_ACCUM          0x0000000040000001ull /**< `_Accum`             */
#define TB_EMC_FRACT          0x0000000080000001ull /**< `_Fract`             */
#define TB_EMC_SAT            0x0000000100000001ull /**< `_Sat`               */

//
// Storage classes
//
// If you add a new TS_xxx here:
//
// + It must also exist in STIDS[] inside c_type_name_impl().
// + TS_ANY_STORAGE may need to be updated.
//
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

//
// Storage-class-like
//
// If you add a new TS_xxx here:
//
// + It must also exist in STIDS[] inside c_type_name_impl().
// + TS_ANY_STORAGE may need to be updated.
//
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
#define TS_THIS               0x0000000004000002ull /**< `this`               */
#define TS_THROW              0x0000000008000002ull /**< `throw()`            */
#define TS_VIRTUAL            0x0000000010000002ull /**< `virtual`            */

//
// Qualifiers
//
// If you add a new TS_xxx here, it must also exist in QUAL_STIDS[] inside
// c_type_name_impl().
//
#define TS_ATOMIC             0x0000000100000002ull /**< `_Atomic`            */
#define TS_CONST              0x0000000200000002ull /**< `const`              */
#define TS_RESTRICT           0x0000000400000002ull /**< `restrict`           */
#define TS_VOLATILE           0x0000000800000002ull /**< `volatile`           */
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
#define TS_NON_EMPTY_ARRAY    0x0000001000000002ull

//
// Unified Parallel C qualifiers
//
// If you add a new TS_UPC_xxx here, it must also exist in QUAL_STIDS[] inside
// c_type_name_impl().
//
#define TS_UPC_RELAXED        0x0000002000000002ull /**< `relaxed`            */
#define TS_UPC_SHARED         0x0000004000000002ull /**< `shared`             */
#define TS_UPC_STRICT         0x0000008000000002ull /**< `strict`             */

//
// Ref-qualifiers
//
// If you add a new TS_xxx here, it must also exist in QUAL_STIDS[] inside
// c_type_name_impl().
//
#define TS_REFERENCE          0x0000010000000002ull /**< `void f() &`         */
#define TS_RVALUE_REFERENCE   0x0000020000000002ull /**< `void f() &&`        */

//
// Attributes
//
// If you add a new TA_xxx here, it must also exist in ATIDS[] inside
// c_type_name_impl().
//
#define TA_NONE               0x0000000000000004ull /**< No attribute.        */
#define TA_ANY                0xFFFFFFFFFFFFFFF4ull /**< Any attribute.       */
#define TA_CARRIES_DEPENDENCY 0x0000000000000014ull /**< `carries_dependency` */
#define TA_DEPRECATED         0x0000000000000024ull /**< `deprecated`         */
#define TA_MAYBE_UNUSED       0x0000000000000044ull /**< `maybe_unused`       */
#define TA_NODISCARD          0x0000000000000084ull /**< `nodiscard`          */
#define TA_NORETURN           0x0000000000000104ull /**< `noreturn`           */
#define TA_NO_UNIQUE_ADDRESS  0x0000000000000204ull /**< `no_unique_address`  */
#define TA_REPRODUCIBLE       0x0000000000000404ull /**< `reproducible`       */
#define TA_UNSEQUENCED        0x0000000000000804ull /**< `unsequenced`        */

//
// Microsoft calling conventions
//
// If you add a new TA_MSC_xxx here, it must also exist in MSC_CALL_ATIDS[]
// inside c_type_name_impl().
//
#define TA_MSC_CDECL          0x0000000000001004ull /**< `__cdecl`            */
#define TA_MSC_CLRCALL        0x0000000000002004ull /**< `__clrcall`          */
#define TA_MSC_FASTCALL       0x0000000000004004ull /**< `__fastcall`         */
#define TA_MSC_STDCALL        0x0000000000008004ull /**< `__stdcall`          */
#define TA_MSC_THISCALL       0x0000000000010004ull /**< `__thiscall`         */
#define TA_MSC_VECTORCALL     0x0000000000020004ull /**< `__vectorcall`       */

// bit masks
#define TX_MASK_TPID          0x000000000000000Full /**< Type part ID bitmask.*/

extern c_type_t const T_NONE;           ///< No type.
extern c_type_t const T_ANY;            ///< All types.
extern c_type_t const T_ANY_CONST_CLASS;///< Any `const` `class`-like type.
extern c_type_t const T_TS_TYPEDEF;     ///< Type containing only #TS_TYPEDEF.

// shorthands

/**
 * The only attributes that can apply to functions.
 *
 * @sa #TA_OBJECT
 */
#define TA_FUNC               ( TA_ANY_MSC_CALL | TA_CARRIES_DEPENDENCY \
                              | TA_DEPRECATED | TA_MAYBE_UNUSED \
                              | TA_NODISCARD | TA_NORETURN | TA_REPRODUCIBLE \
                              | TA_UNSEQUENCED )

/**
 * The only attributes that can apply to objects.
 *
 * @sa #TA_FUNC
 */
#define TA_OBJECT             ( TA_CARRIES_DEPENDENCY | TA_DEPRECATED \
                              | TA_MAYBE_UNUSED | TA_NO_UNIQUE_ADDRESS )

/// Shorthand for any character type.
#define TB_ANY_CHAR           ( TB_CHAR | TB_WCHAR_T \
                              | TB_CHAR8_T | TB_CHAR16_T | TB_CHAR32_T )

/// Shorthand for `class`, `struct`, or `union`.
#define TB_ANY_CLASS          ( TB_CLASS | TB_STRUCT | TB_UNION )

/// Shorthand for `enum`, `class`, `struct`, or `union`.
#define TB_ANY_ECSU           ( TB_ENUM | TB_ANY_CLASS )

/// Shorthand for any Embedded C type.
#define TB_ANY_EMC            ( TB_EMC_ACCUM | TB_EMC_FRACT )

/// Shorthand for any floating-point type.
#define TB_ANY_FLOAT          ( TB_FLOAT | TB_DOUBLE )

/// Shorthand for any integral type.
#define TB_ANY_INTEGRAL       ( TB_BOOL | TB_ANY_CHAR | TB_BITINT | TB_INT \
                              | TB_ANY_MODIFIER )

/// Shorthand for an any modifier.
#define TB_ANY_MODIFIER       ( TB_SHORT | TB_LONG | TB_LONG_LONG | TB_SIGNED \
                              | TB_UNSIGNED )

/// Shorthand for any Microsoft C/C++ calling convention.
#define TA_ANY_MSC_CALL       ( TA_MSC_CDECL | TA_MSC_CLRCALL \
                              | TA_MSC_FASTCALL | TA_MSC_STDCALL \
                              | TA_MSC_THISCALL | TA_MSC_VECTORCALL )

/// Shorthand for any array qualifier.
#define TS_ANY_ARRAY_QUALIFIER \
                              ( TS_CVR | TS_NON_EMPTY_ARRAY )

/// Shorthand for any qualifier.
#define TS_ANY_QUALIFIER      ( TS_ANY_ARRAY_QUALIFIER | TS_ANY_UPC \
                              | TS_ATOMIC )

/// Shorthand for any reference qualifier.
#define TS_ANY_REFERENCE      ( TS_REFERENCE | TS_RVALUE_REFERENCE )

/// Shorthand for `class`, `struct`, `union`, or `namespace`.
#define TB_ANY_SCOPE          ( TB_ANY_CLASS | TB_NAMESPACE )

/// Shorthand for any storage.
#define TS_ANY_STORAGE        0x00000000FFFFFFF2ull

/// Shorthand for any UPC qualifier.
#define TS_ANY_UPC            ( TS_UPC_RELAXED | TS_UPC_SHARED | TS_UPC_STRICT )

/// Shorthand for `const` or `volatile`.
#define TS_CV                 ( TS_CONST | TS_VOLATILE )

/// Shorthand for `const`, `volatile`, or `restrict`.
#define TS_CVR                ( TS_CV | TS_RESTRICT )

/**
 * The only types that can apply to constructor declarations.
 *
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_CONSTRUCTOR_ONLY
 * @sa #TS_FUNC_LIKE_CPP
 */
#define TS_CONSTRUCTOR_DECL   ( TS_CONSTRUCTOR_DEF | TS_CONSTRUCTOR_ONLY \
                              | TS_DEFAULT | TS_DELETE | TS_FRIEND )

/**
 * A subset of #TS_CONSTRUCTOR_DECL that can apply to file-scope constructor
 * definitions.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_ONLY
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
 * The only types that can apply to destructor declarations.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_DESTRUCTOR_DEF
 */
#define TS_DESTRUCTOR_DECL    ( TS_DESTRUCTOR_DEF | TS_DELETE | TS_FINAL \
                              | TS_FRIEND | TS_OVERRIDE | TS_PURE_VIRTUAL \
                              | TS_VIRTUAL )

/**
 * A subset of #TS_DESTRUCTOR_DECL that can apply to file-scope destructor
 * definitions.
 *
 * @sa #TS_DESTRUCTOR_DECL
 */
#define TS_DESTRUCTOR_DEF     ( TS_INLINE | TS_NOEXCEPT | TS_THROW )

/**
 * The only storage types that can apply to C functions.
 *
 * @sa #TS_FUNC_LIKE_CPP
 * @sa #TS_MAIN_FUNC_C
 */
#define TS_FUNC_C             ( TS_EXTERN | TS_INLINE | TS_STATIC | TS_TYPEDEF )

/**
 * The only storage types that can apply to C++ function-like things
 * (functions, blocks, constructors, destructors, operators, and user-defined
 * conversion operators and literals).
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_FUNC_LIKE_NOT_CTOR
 * @sa #TS_MAIN_FUNC_CPP
 * @sa #TS_NEW_DELETE_OPER
 * @sa #TS_USER_DEF_CONV
 */
#define TS_FUNC_LIKE_CPP      ( TS_CV | TS_CONSTEVAL | TS_CONSTEXPR \
                              | TS_DEFAULT | TS_DELETE | TS_EXPLICIT \
                              | TS_EXPORT | TS_EXTERN_C | TS_FINAL \
                              | TS_FRIEND | TS_FUNC_C | TS_NOEXCEPT \
                              | TS_OVERRIDE | TS_PURE_VIRTUAL \
                              | TS_ANY_REFERENCE | TS_RESTRICT | TS_THROW \
                              | TS_VIRTUAL )

/**
 * The types that can apply only to function-like things except constructors.
 *
 * @sa #TS_CONSTRUCTOR_DECL
 * @sa #TS_CONSTRUCTOR_DEF
 * @sa #TS_CONSTRUCTOR_ONLY
 * @sa #TS_FUNC_LIKE_CPP
 */
#define TS_FUNC_LIKE_NOT_CTOR ( TS_CV | TS_EXTERN | TS_EXTERN_C | TS_FINAL \
                              | TS_OVERRIDE | TS_ANY_REFERENCE | TS_RESTRICT \
                              | TS_STATIC | TS_VIRTUAL )

/**
 * The only storage types that can _not_ apply to C++ function like things
 * (functions and operators) that have an explicit object parameter (`this`).
 */
#define TS_FUNC_LIKE_NOT_EXPLICIT_OBJ_PARAM \
                              ( TS_ANY_REFERENCE | TS_CV | TS_STATIC \
                              | TS_VIRTUAL )

/**
 * The only storage types that can apply to C++ function-like parameters.
 */
#define TS_FUNC_LIKE_PARAM    ( TS_REGISTER | TS_THIS )

/**
 * The only storage types that can apply to a C++ lambda.
 */
#define TS_LAMBDA             ( TS_CONSTEXPR | TS_CONSTEVAL | TS_MUTABLE \
                              | TS_NOEXCEPT | TS_STATIC | TS_THROW )

/**
 * The only storage types that can apply to a C program's `main()` function.
 *
 * @sa #TS_FUNC_C
 * @sa #TS_MAIN_FUNC_CPP
 */
#define TS_MAIN_FUNC_C        TS_EXTERN

/**
 * The only types that can apply to a C++ program's `main()` function.
 *
 * @sa #TS_FUNC_LIKE_CPP
 * @sa #TS_MAIN_FUNC_C
 */
#define TS_MAIN_FUNC_CPP      ( TS_FRIEND | TS_MAIN_FUNC_C | TS_NOEXCEPT \
                              | TS_THROW )

/**
 * The types that can apply only to member functions, operators, or user-
 * defined conversions operators.
 *
 * @sa #TS_FUNC_LIKE_CPP
 * @sa #TS_NONMEMBER_FUNC_ONLY
 */
#define TS_MEMBER_FUNC_ONLY   ( TS_CV | TS_DELETE | TS_FINAL | TS_OVERRIDE \
                              | TS_ANY_REFERENCE | TS_RESTRICT | TS_VIRTUAL )

/**
 * The only types that can apply to operators `new`, `new[]`, `delete`, or
 * `delete[]`.
 *
 * @sa #TS_FUNC_LIKE_CPP
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
 * The only types that can apply to user-defined conversion operators.
 *
 * @sa #TS_FUNC_LIKE_CPP
 */
#define TS_USER_DEF_CONV      ( TS_CONST | TS_CONSTEXPR | TS_EXPLICIT \
                              | TS_FINAL | TS_FRIEND | TS_INLINE \
                              | TS_NOEXCEPT | TS_OVERRIDE | TS_PURE_VIRTUAL \
                              | TS_THROW | TS_VIRTUAL )

/**
 * Hexadecimal print conversion specifier for \ref c_tid_t.
 */
#define PRIX_C_TID_T          PRIX64

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
 * @param new_loc The source location of \a new_id.
 * @return Returns `true` only if the type added successfully; otherwise,
 * prints an error message at \a new_loc and returns `false`.
 *
 * @sa c_type_add(()
 */
NODISCARD
bool c_tid_add( c_tid_t *dst_tids, c_tid_t new_tids, c_loc_t const *new_loc );

/**
 * Gets the C/C++ name of \a tids.
 *
 * @param tids The \ref c_tid_t to get the name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_english()
 * @sa c_tid_name_error()
 * @sa c_type_name_c()
 */
NODISCARD
char const* c_tid_name_c( c_tid_t tids );

/**
 * Gets the pseudo-English name of \a tids, if available; the C/C++ name if
 * not.
 *
 * @param tids The \ref c_tid_t to get the name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_c()
 * @sa c_tid_name_error()
 * @sa c_type_name_english()
 */
NODISCARD
char const* c_tid_name_english( c_tid_t tids );

/**
 * Gets the name of \a tids for part of an error message.  If translating from
 * pseudo-English to gibberish and the type has an pseudo-English alias, return
 * the alias, e.g., `non-returning` rather than `noreturn`.
 *
 * @param tids The \ref c_tid_t to get the name of.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_c()
 * @sa c_tid_name_english()
 * @sa c_type_name_error()
 */
NODISCARD
char const* c_tid_name_error( c_tid_t tids );

/**
 * "Normalize" \a tids:
 *
 *  1. If it's #TB_SIGNED and not #TB_CHAR, remove #TB_SIGNED.  If it becomes
 *     #TB_NONE, make it #TB_INT.
 *  2. If it's only implicitly #TB_INT (e.g., `unsigned`), make it explicitly
 *     #TB_INT (e.g., `unsigned int`).
 *
 * @param tids The \ref c_tid_t to normalize.
 * @return Returns the normalized \ref c_tid_t.
 */
NODISCARD
c_tid_t c_tid_normalize( c_tid_t tids );

/**
 * Gets the "order" value of a \ref c_tid_t so it can be compared by its order.
 *
 * The order is:
 *
 * + { _none_ | `scope` } &lt;
 *   [`inline`] `namespace` &lt;
 *   { `struct` | `union` | `class` } &lt;
 *   `enum` [`class`]
 *
 * I.e., order(T1) &le; order(T2) only if T1 can appear to the left of T2 in a
 * declaration.  For example, given:
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
 * @param btids The scope-type ID to get the order of.
 * @return Returns said order.
 *
 * @note The return value by itself is meaningless.  All that matters is the
 * result of comparing two orders.
 */
NODISCARD
unsigned c_tid_scope_order( c_tid_t btids );

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
 * Gets the C/C++ name of \a type.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_c()
 * @sa c_type_name_ecsu()
 * @sa c_type_name_english()
 * @sa c_type_name_error()
 */
NODISCARD
char const* c_type_name_c( c_type_t const *type );

/**
 * Gets the the C/C++ name for an `enum`, `struct`, `class`, or `union`.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_type_name_c()
 * @sa c_type_name_english()
 * @sa c_type_name_error()
 */
NODISCARD
char const* c_type_name_ecsu( c_type_t const *type );

/**
 * Gets the pseudo-English name of \a type, if available; the C/C++ name if
 * not.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_type_name_c()
 * @sa c_type_name_ecsu()
 * @sa c_type_name_error()
 */
NODISCARD
char const* c_type_name_english( c_type_t const *type );

/**
 * Gets the name of \a type for part of an error message.  If translating
 * from pseudo-English to gibberish and the type has a pseudo-English alias,
 * return the alias, e.g., `non-returning` rather than `noreturn`.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_name_error()
 * @sa c_type_name_c()
 * @sa c_type_name_ecsu()
 * @sa c_type_name_english()
 */
NODISCARD
char const* c_type_name_error( c_type_t const *type );

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
C_TYPE_H_INLINE PJL_DISCARD
c_tid_t c_tid_check( c_tid_t tids, c_tpid_t tpid ) {
  assert( (tids & TX_MASK_TPID) == tpid );
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
 * Bitwise-complements \a tids.  The `~` operator can't be used alone because
 * the part ID of \a tids would be complemented also.  This function
 * complements \a tids while preserving the original part ID.
 *
 * @param tids The \ref c_tid_t to complement.
 * @return Returns \a tids complemented.
 *
 * @sa c_tid_is_compl()
 */
NODISCARD C_TYPE_H_INLINE
c_tid_t c_tid_compl( c_tid_t tids ) {
  assert( !c_tid_is_compl( tids ) );
  return ~tids ^ TX_MASK_TPID;
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
  return tids & ~TX_MASK_TPID;
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
  return (tids & c_tid_compl( TB_INT )) == (TB_UNSIGNED | TB_LONG);
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
