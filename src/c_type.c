/*
**      cdecl -- C gibberish translator
**      src/c_type.c
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

/**
 * @file
 * Defines functions for C/C++ types.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_TYPE_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_type.h"
#include "c_lang.h"
#include "cdecl.h"
#include "gibberish.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define C_TYPE_CHECK(...) BLOCK(            \
  c_lang_id_t const lang_ids = __VA_ARGS__; \
  if ( lang_ids != LANG_ANY )               \
    return lang_ids; )

#define C_TYPE_ID_CHECK_COMBO(TYPE,TYPES,OK_TYPE_LANGS) \
  C_TYPE_CHECK( c_type_id_check_combo( (TYPE), (TYPES), ARRAY_SIZE( TYPES ), OK_TYPE_LANGS ) )

#define C_TYPE_ID_CHECK_LEGAL(TYPE,TYPES) \
  C_TYPE_CHECK( c_type_id_check_legal( (TYPE), (TYPES), ARRAY_SIZE( TYPES ) ) )

#define C_TYPE_ID_NAME_CAT(SBUF,TYPE,TYPES,IS_ERROR,SEP,PSEP)       \
  c_type_id_name_cat( (SBUF), (TYPE), (TYPES), ARRAY_SIZE( TYPES ), \
                      (IS_ERROR), (SEP), (PSEP) )

/// @endcond

c_type_t const T_NONE     = { TB_NONE,  TS_NONE,    TA_NONE };
c_type_t const T_ANY      = { TB_ANY,   TS_ANY,     TA_ANY  };
c_type_t const T_TYPEDEF  = { TB_NONE,  TS_TYPEDEF, TA_NONE };

///////////////////////////////////////////////////////////////////////////////

/**
 * Mapping between C type bits, valid language(s), and literals.
 */
struct c_type_info {
  c_type_id_t         type_id;          ///< The type.
  c_lang_id_t         lang_ids;         ///< Language(s) OK in.
  char const         *english_lit;      ///< English version (if not NULL).

  /**
   * Array of language(s)/literal pair(s).  The array is terminated by an
   * element that has #LANG_ANY for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_lit;
};
typedef struct c_type_info c_type_info_t;

// local functions
static void         strbuf_cat_sep( strbuf_t*, char const*, char, bool* );

///////////////////////////////////////////////////////////////////////////////

/**
 * As part of the special case for `long long`, its literal is only `long`
 * because its type, #TB_LONG_LONG, is always combined with
 * <code>#TB_LONG</code>, i.e., two bits are set.  Therefore, when printed, it
 * prints one `long` for <code>#TB_LONG</code> and another `long` for
 * <code>#TB_LONG_LONG</code> (this literal).  That explains why this literal
 * is only one `long`.
 */
static char const L_LONG_LONG[] = "long";

/**
 * For convenience, this is just a concatenation of `L_RVALUE` and
 * `L_REFERENCE`.
 */
static char const L_RVALUE_REFERENCE[] = "rvalue reference";

/**
 * <code>#TB_TYPEDEF</code> exists only so there can be a row/column for it in
 * the <code>\ref OK_TYPE_LANGS</code> table to make things like `signed
 * size_t` illegal.
 *
 * <code>#TB_TYPEDEF</code> doesn't have any printable representation (only the
 * name of the type is printed); therefore, its literal is the empty string.
 */
static char const L_TYPEDEF_TYPE[] = "";

/**
 * Type mapping for attributes.
 */
static c_type_info_t const C_ATTRIBUTE_INFO[] = {
  { TA_CARRIES_DEPENDENCY, LANG_CPP_MIN(11), H_CARRIES_DEPENDENCY,
    C_LANG_LIT( { LANG_ANY, L_CARRIES_DEPENDENCY } ) },

  { TA_DEPRECATED, LANG_C_CPP_MIN(2X,11), NULL,
    C_LANG_LIT( { LANG_ANY, L_DEPRECATED } ) },

  { TA_MAYBE_UNUSED, LANG_C_CPP_MIN(2X,17), H_MAYBE_UNUSED,
    C_LANG_LIT( { LANG_ANY, L_MAYBE_UNUSED } ) },

  { TA_NODISCARD, LANG_C_CPP_MIN(2X,17), H_NON_DISCARDABLE,
    C_LANG_LIT( { LANG_ANY, L_NODISCARD } ) },

  { TA_NORETURN, LANG_C_CPP_MIN(11,11), H_NON_RETURNING,
    C_LANG_LIT( { LANG_CPP_ANY, L_NORETURN  },
                { LANG_ANY,     L__NORETURN } ) },

  { TA_NO_UNIQUE_ADDRESS, LANG_CPP_MIN(20), H_NON_UNIQUE_ADDRESS,
    C_LANG_LIT( { LANG_ANY, L_NO_UNIQUE_ADDRESS } ) },
};

/**
 * Type mapping for qualifiers.
 *
 * @note
 * This array _must_ have the same size and order as OK_QUALIFIER_LANGS.
 */
static c_type_info_t const C_QUALIFIER_INFO[] = {
  { TS_ATOMIC, LANG_MIN(C_11), L_ATOMIC,
    C_LANG_LIT( { LANG_ANY, L__ATOMIC } ) },

  { TS_CONST, LANG_ANY, L_CONSTANT,
    C_LANG_LIT( { LANG_C_KNR, L_GNU___CONST },
                { LANG_ANY,   L_CONST       } ) },

  { TS_REFERENCE, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_REFERENCE } ) },

  { TS_RVALUE_REFERENCE, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_RVALUE_REFERENCE } ) },

  { TS_RESTRICT, LANG_ANY, L_RESTRICTED,
    C_LANG_LIT( { LANG_C_MAX(95) | LANG_CPP_ANY, L_GNU___RESTRICT },
                { LANG_ANY,                      L_RESTRICT       } ) },

  { TS_VOLATILE, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_C_KNR, L_GNU___VOLATILE },
                { LANG_ANY,   L_VOLATILE       } ) },

  // Unified Parallel C extensions
  { TS_UPC_RELAXED, LANG_C_99, NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_RELAXED } ) },

  { TS_UPC_SHARED, LANG_C_99, NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_SHARED } ) },

  { TS_UPC_STRICT, LANG_C_99, NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_STRICT } ) },
};

/**
 * Type mapping for storage classes (or storage-class-like).
 *
 * @note
 * This array _must_ have the same size and order as OK_STORAGE_LANGS.
 */
static c_type_info_t const C_STORAGE_INFO[] = {
  // storage classes
  { TS_AUTO, LANG_MAX(CPP_03), L_AUTOMATIC,
    C_LANG_LIT( { LANG_ANY, L_AUTO } ) },

  { TS_APPLE_BLOCK, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_APPLE___BLOCK } ) },

  { TS_EXTERN, LANG_ANY, L_EXTERNAL,
    C_LANG_LIT( { LANG_ANY, L_EXTERN } ) },

  { TS_EXTERN_C, LANG_CPP_ANY, "external \"C\" linkage",
    C_LANG_LIT( { LANG_ANY, "extern \"C\"" } ) },

  { TS_REGISTER, LANG_MAX(CPP_14), NULL,
    C_LANG_LIT( { LANG_ANY, L_REGISTER } ) },

  { TS_STATIC, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_STATIC } ) },

  { TS_THREAD_LOCAL, LANG_ANY, H_THREAD_LOCAL,
    C_LANG_LIT( { LANG_C_MAX(99) | LANG_CPP_MAX(03),  L_GNU___THREAD  },
                { LANG_ANY,                           L_THREAD_LOCAL  } ) },

  { TS_TYPEDEF, LANG_ANY, L_TYPE,
    C_LANG_LIT( { LANG_ANY, L_TYPEDEF } ) },

  // storage-class-like
  { TS_CONSTEVAL, LANG_CPP_MIN(20), NULL,
    C_LANG_LIT( { LANG_ANY, L_CONSTEVAL } ) },

  { TS_CONSTEXPR, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_CONSTEXPR } ) },

  { TS_CONSTINIT, LANG_CPP_MIN(20), NULL,
    C_LANG_LIT( { LANG_ANY, L_CONSTINIT } ) },

  { TS_DEFAULT, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_DEFAULT } ) },

  { TS_DELETE, LANG_CPP_MIN(11), L_DELETED,
    C_LANG_LIT( { LANG_ANY, L_DELETE } ) },

  { TS_EXPLICIT, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_EXPLICIT } ) },

  { TS_EXPORT, LANG_CPP_MIN(20), L_EXPORTED,
    C_LANG_LIT( { LANG_ANY, L_EXPORT } ) },

  { TS_FINAL, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_FINAL } ) },

  { TS_FRIEND, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_FRIEND } ) },

  { TS_INLINE, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_C_KNR, L_GNU___INLINE },
                { LANG_ANY,   L_INLINE       } ) },

  { TS_MUTABLE, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_MUTABLE } ) },

  { TS_NOEXCEPT, LANG_CPP_MIN(11), H_NO_EXCEPTION,
    C_LANG_LIT( { LANG_ANY, L_NOEXCEPT } ) },

  { TS_OVERRIDE, LANG_CPP_MIN(11), L_OVERRIDDEN,
    C_LANG_LIT( { LANG_ANY, L_OVERRIDE } ) },

  { TS_THROW, LANG_CPP_ANY, H_NON_THROWING,
    C_LANG_LIT( { LANG_ANY, L_THROW } ) },

  { TS_VIRTUAL, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_VIRTUAL } ) },

  { TS_PURE_VIRTUAL, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_PURE } ) },
};

/**
 * Type mapping for simpler types.
 *
 * @note
 * This array _must_ have the same size and order as OK_TYPE_LANGS.
 */
static c_type_info_t const C_TYPE_INFO[] = {
  { TB_VOID, LANG_MIN(C_89), NULL,
    C_LANG_LIT( { LANG_ANY, L_VOID } ) },

  { TB_AUTO, LANG_MIN(C_89), L_AUTOMATIC,
    C_LANG_LIT( { LANG_MAX(CPP_03), L_GNU___AUTO_TYPE },
                { LANG_ANY,         L_AUTO            } ) },

  { TB_BOOL, LANG_MIN(C_99), NULL,
    C_LANG_LIT( { LANG_C_ANY, L__BOOL },
                { LANG_ANY,   L_BOOL  } ) },

  { TB_CHAR, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_CHAR } ) },

  { TB_CHAR8_T, LANG_C_CPP_MIN(2X,20), NULL,
    C_LANG_LIT( { LANG_ANY, L_CHAR8_T } ) },

  { TB_CHAR16_T, LANG_C_CPP_MIN(11,11), NULL,
    C_LANG_LIT( { LANG_ANY, L_CHAR16_T } ) },

  { TB_CHAR32_T, LANG_C_CPP_MIN(11,11), NULL,
    C_LANG_LIT( { LANG_ANY, L_CHAR32_T } ) },

  { TB_WCHAR_T, LANG_MIN(C_95), NULL,
    C_LANG_LIT( { LANG_ANY, L_WCHAR_T } ) },

  { TB_SHORT, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_SHORT } ) },

  { TB_INT, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_INT } ) },

  { TB_LONG, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_LONG } ) },

  { TB_LONG_LONG, LANG_MIN(C_99), NULL,
    C_LANG_LIT( { LANG_ANY, L_LONG_LONG } ) },

  { TB_SIGNED, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_C_KNR, L_GNU___SIGNED },
                { LANG_ANY,   L_SIGNED       } ) },

  { TB_UNSIGNED, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_UNSIGNED } ) },

  { TB_FLOAT, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_FLOAT } ) },

  { TB_DOUBLE, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_DOUBLE } ) },

  { TB_COMPLEX, LANG_C_ANY, L_COMPLEX,
    C_LANG_LIT( { LANG_C_MAX(95), L_GNU___COMPLEX },
                { LANG_ANY,       L__COMPLEX      } ) },

  { TB_IMAGINARY, LANG_C_MIN(99), L_IMAGINARY,
    C_LANG_LIT( { LANG_ANY, L__IMAGINARY } ) },

  { TB_ENUM, LANG_MIN(C_89), L_ENUMERATION,
    C_LANG_LIT( { LANG_ANY, L_ENUM } ) },

  { TB_STRUCT, LANG_ANY, L_STRUCTURE,
    C_LANG_LIT( { LANG_ANY, L_STRUCT } ) },

  { TB_UNION, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_UNION } ) },

  { TB_CLASS, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_CLASS } ) },

  { TB_TYPEDEF, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_TYPEDEF_TYPE } ) },

  // Embedded C extensions
  { TB_EMC_ACCUM, LANG_C_99, L_EMC_ACCUM,
    C_LANG_LIT( { LANG_ANY, L_EMC__ACCUM } ) },

  { TB_EMC_FRACT, LANG_C_99, L_EMC_FRACT,
    C_LANG_LIT( { LANG_ANY, L_EMC__FRACT } ) },

  { TB_EMC_SAT, LANG_C_99, L_EMC_SATURATED,
    C_LANG_LIT( { LANG_ANY, L_EMC__SAT } ) },
};

/// @cond DOXYGEN_IGNORE

//      shorthand   legal in ...
#define __          LANG_ANY
#define XX          LANG_NONE
#define KR          LANG_C_KNR
#define C8          LANG_MIN(C_89)
#define C5          LANG_MIN(C_95)
#define c9          LANG_C_99
#define C9          LANG_MIN(C_99)
#define C1          LANG_MIN(C_11)
#define C2          LANG_C_CPP_MIN(2X,20)
#define PP          LANG_CPP_ANY
#define P3          LANG_CPP_MIN(03)
#define P1          LANG_CPP_MIN(11)
#define P2          LANG_CPP_MIN(20)
#define E1          LANG_C_CPP_MIN(11,11)

/// @endcond

// There is no OK_ATTRIBUTE_LANGS because all combinations of attributes are
// legal.

/**
 * Legal combinations of qualifiers in languages.
 *
 * @note
 * This array _must_ have the same size and order as C_QUALIFIER_INFO.
 */
static c_lang_id_t const OK_QUALIFIER_LANGS[][ ARRAY_SIZE( C_QUALIFIER_INFO ) ] = {
// Only the lower triangle is used.
//  a  c  r  rr re v    rx sh st
  { E1,__,__,__,__,__,  __,__,__ }, // atomic
  { E1,__,__,__,__,__,  __,__,__ }, // const
  { XX,PP,PP,__,__,__,  __,__,__ }, // reference
  { XX,P1,XX,P1,__,__,  __,__,__ }, // rvalue reference
  { XX,__,PP,P1,__,__,  __,__,__ }, // restrict
  { E1,__,PP,P1,__,__,  __,__,__ }, // volatile

  // Unified Parallel C extensions
  { XX,c9,XX,XX,c9,c9,  c9,__,__ }, // relaxed
  { XX,c9,XX,XX,c9,c9,  c9,c9,__ }, // shared
  { XX,c9,XX,XX,c9,c9,  XX,c9,c9 }, // strict
};

/**
 * Legal combinations of storage classes in languages.
 *
 * @note
 * This array _must_ have the same size and order as C_STORAGE_INFO.
 */
static c_lang_id_t const OK_STORAGE_LANGS[][ ARRAY_SIZE( C_STORAGE_INFO ) ] = {
// Only the lower triangle is used.
//  a  b  e  ec,r  s  th td   cv cx ci df de ex ep fi fr in mu ne o  t  v  pv
  { __,__,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// auto
  { __,__,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// block
  { XX,__,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// extern
  { XX,__,__,PP,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// extern C
  { XX,__,XX,XX,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// register
  { XX,XX,XX,XX,XX,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// static
  { XX,__,__,P1,XX,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// thread
  { XX,__,XX,PP,XX,XX,XX,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// typedef

  { P1,P1,P1,P2,XX,P1,XX,XX,  P2,P1,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c'eval
  { P1,P1,P1,P1,XX,P1,XX,XX,  XX,P1,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c'expr
  { XX,XX,P2,P2,XX,P2,P2,XX,  XX,XX,P2,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c'init
  { XX,XX,XX,XX,XX,XX,XX,XX,  P1,P1,XX,P1,__,__,__,__,__,__,__,__,__,__,__,__ },// default
  { XX,XX,XX,XX,XX,XX,XX,XX,  P1,P1,XX,XX,P1,__,__,__,__,__,__,__,__,__,__,__ },// delete
  { XX,XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,P1,P1,PP,__,__,__,__,__,__,__,__,__,__ },// explicit
  { XX,XX,P2,XX,XX,XX,XX,XX,  XX,P2,P2,XX,XX,XX,P2,__,__,__,__,__,__,__,__,__ },// export
  { XX,XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,XX,XX,XX,XX,P1,__,__,__,__,__,__,__,__ },// final
  { XX,XX,XX,XX,XX,XX,XX,XX,  P2,P1,XX,XX,XX,XX,XX,XX,PP,__,__,__,__,__,__,__ },// friend
  { XX,XX,__,PP,XX,__,XX,XX,  P2,P1,P2,P1,P1,PP,P2,P1,PP,C9,__,__,__,__,__,__ },// inline
  { XX,XX,XX,XX,XX,XX,XX,XX,  XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P3,__,__,__,__,__ },// mutable
  { XX,XX,P1,PP,XX,P1,XX,P1,  P2,P1,XX,P1,P1,PP,P2,P1,P1,P1,XX,P1,__,__,__,__ },// noexcept
  { XX,XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,XX,XX,XX,XX,P1,XX,C1,XX,C1,P1,__,__,__ },// override
  { XX,XX,PP,PP,XX,PP,XX,PP,  P2,P1,XX,P1,P1,PP,XX,PP,XX,PP,XX,XX,PP,PP,__,__ },// throw
  { XX,XX,XX,XX,XX,XX,XX,XX,  XX,P2,XX,XX,XX,XX,XX,P1,XX,PP,XX,C1,P1,PP,PP,__ },// virtual
  { XX,XX,XX,XX,XX,XX,XX,XX,  XX,P2,XX,XX,XX,XX,XX,XX,XX,PP,XX,C1,P1,PP,PP,PP },// pure
};

/**
 * Legal combinations of types in languages.
 *
 * @note
 * This array _must_ have the same size and order as C_TYPE_INFO.
 */
static c_lang_id_t const OK_TYPE_LANGS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
// Only the lower triangle is used.
//  v  a1 b  c  8  16 32 wc s  i  l  ll s  u  f  d  co im e  st un cl t  ac fr sa
  { C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// void
  { XX,C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// auto
  { XX,XX,C9,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// bool
  { XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// char
  { XX,XX,XX,XX,C2,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// char8_t
  { XX,XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// char16_t
  { XX,XX,XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// char32_t
  { XX,XX,XX,XX,XX,XX,XX,C5,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// wchar_t
  { XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// short
  { XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// int
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// long
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,__,C9,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// long long
  { XX,XX,XX,C8,XX,XX,XX,XX,C8,C8,C8,C8,C8,__,__,__,__,__,__,__,__,__,__,__,__,__ },// signed
  { XX,XX,XX,__,XX,XX,XX,XX,__,__,__,C8,XX,__,__,__,__,__,__,__,__,__,__,__,__,__ },// unsigned
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,KR,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__ },// float
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__ },// double
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,C9,__,__,__,__,__,__,__,__,__ },// complex
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,XX,C9,__,__,__,__,__,__,__,__ },// imaginary
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,__,__,__,__,__,__,__ },// enum
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P1,__,__,__,__,__,__,__ },// struct
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__ },// union
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P1,XX,XX,PP,__,__,__,__ },// class
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__ },// typedef
  { XX,XX,XX,XX,XX,XX,XX,XX,C9,XX,C9,XX,C9,C9,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,__,__ },// _Accum
  { XX,XX,XX,XX,XX,XX,XX,XX,C9,XX,C9,XX,C9,C9,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,__ },// _Fract
  { XX,XX,XX,XX,XX,XX,XX,XX,C9,XX,C9,XX,C9,C9,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,C9 },// _Sat
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a tid is some form of <code>long int</code> only, and _not_
 * either `long float` (K&R) or `long double` (C89).
 *
 * @param tid The <code>\ref c_type_id_t</code> to check.
 * @return Returns `true` only if \a tid is some form of `long int`.
 */
PJL_WARN_UNUSED_RESULT
static inline bool is_long_int( c_type_id_t tid ) {
  return  c_type_id_tpid( tid ) == C_TPID_BASE &&
          (tid & (TB_LONG | TB_FLOAT | TB_DOUBLE)) == TB_LONG;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks that the type combination is legal in the current language.
 *
 * @param tid The <code>\ref c_type_id_t</code> to check.
 * @param type_infos The array of <code>\ref c_type_info</code> to check
 * against.
 * @param type_infos_size The size of \a type_infos.
 * @param type_langs The type/languages array to check against.
 * @return Returns the bitwise-or of the language(s) \a tid is legal in.
 */
PJL_WARN_UNUSED_RESULT
static c_lang_id_t
c_type_id_check_combo( c_type_id_t tid, c_type_info_t const type_infos[const],
                       size_t type_infos_size,
                       c_lang_id_t const type_langs[][type_infos_size] ) {
  for ( size_t row = 0; row < type_infos_size; ++row ) {
    if ( !c_type_id_is_none( tid & type_infos[ row ].type_id ) ) {
      for ( size_t col = 0; col <= row; ++col ) {
        c_lang_id_t const lang_ids = type_langs[ row ][ col ];
        if ( !c_type_id_is_none( tid & type_infos[ col ].type_id ) &&
             (opt_lang & lang_ids) == LANG_NONE ) {
          return lang_ids;
        }
      } // for
    }
  } // for
  return LANG_ANY;
}

/**
 * Checks that \a tid is legal in the current language.
 *
 * @param tid The <code>\ref c_type_id_t</code> to check.
 * @param type_infos The array of <code>\ref c_type_info</code> to check
 * against.
 * @param type_infos_size The size of \a type_infos.
 * @return Returns the bitwise-or of the language(s) \a type_id is legal in.
 */
PJL_WARN_UNUSED_RESULT
static c_lang_id_t
c_type_id_check_legal( c_type_id_t tid, c_type_info_t const type_infos[const],
                       size_t type_infos_size ) {
  for ( size_t row = 0; row < type_infos_size; ++row ) {
    c_type_info_t const *const ti = &type_infos[ row ];
    if ( !c_type_id_is_none( tid & ti->type_id ) &&
         (opt_lang & ti->lang_ids) == LANG_NONE ) {
      return ti->lang_ids;
    }
  } // for
  return LANG_ANY;
}

/**
 * Gets the literal of a given <code>\ref c_type_info</code>, either gibberish
 * or, if appropriate and available, English.
 *
 * @param ti The <code>\ref c_type_info</code> to get the literal of.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @return Returns said literal.
 */
PJL_WARN_UNUSED_RESULT
static char const* c_type_literal( c_type_info_t const *ti, bool in_english ) {
  return in_english && ti->english_lit != NULL ?
    ti->english_lit : c_lang_literal( ti->lang_lit );
}

/**
 * Gets the name of an individual type.
 *
 * @param tid The <code>\ref c_type_id_t</code> to get the name for; \a tid
 * _must_ have _exactly one_ bit set.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @return Returns said name.
 */
PJL_WARN_UNUSED_RESULT
static char const* c_type_id_name_1( c_type_id_t tid, bool in_english ) {
  assert( exactly_one_bit_set( c_type_id_no_tpid( tid ) ) );

  switch ( c_type_id_tpid( tid ) ) {
    case C_TPID_BASE:
      for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_TYPE_INFO[i];
        if ( tid == ti->type_id )
          return c_type_literal( ti, in_english );
      } // for
      break;

    case C_TPID_STORE:
      for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_QUALIFIER_INFO[i];
        if ( tid == ti->type_id )
          return c_type_literal( ti, in_english );
      } // for

      for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_STORAGE_INFO[i];
        if ( tid == ti->type_id )
          return c_type_literal( ti, in_english );
      } // for
      break;

    case C_TPID_ATTR:
      for ( size_t i = 0; i < ARRAY_SIZE( C_ATTRIBUTE_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_ATTRIBUTE_INFO[i];
        if ( tid == ti->type_id )
          return c_type_literal( ti, in_english );
      } // for
      break;
  } // switch

  UNEXPECTED_INT_VALUE( tid );
}

/**
 * Concatenates the partial type name onto the full type name being made.
 *
 * @param sbuf A pointer to the buffer to concatenate the name to.
 * @param tid The <code>\ref c_type_id_t</code> to concatenate the name of.
 * @param tids The array of types to use.
 * @param tids_size The size of \a tids.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @param sep The separator character.
 * @param sep_cat A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
static void c_type_id_name_cat( strbuf_t *sbuf, c_type_id_t tid,
                                c_type_id_t const tids[], size_t tids_size,
                                bool in_english, char sep, bool *sep_cat ) {
  assert( sbuf != NULL );
  for ( size_t i = 0; i < tids_size; ++i ) {
    if ( !c_type_id_is_none( tid & tids[i] ) ) {
      strbuf_cat_sep(
        sbuf, c_type_id_name_1( tids[i], in_english ), sep, sep_cat
      );
    }
  } // for
}

/**
 * Creates a <code>\ref c_type</code> based on the group ID of \a tid.
 *
 * @param tid The <code>\ref c_type_id_t</code> to create the <code>\ref
 * c_type</code> from.
 * @return Returns said <code>\ref c_type</code>.
 */
PJL_WARN_UNUSED_RESULT
static c_type_t c_type_from_tid( c_type_id_t tid ) {
  switch ( c_type_id_tpid( tid ) ) {
    case C_TPID_BASE:
      return C_TYPE_LIT_B( tid );
    case C_TPID_STORE:
      return C_TYPE_LIT_S( tid );
    case C_TPID_ATTR:
      return C_TYPE_LIT_A( tid );
  } // switch
  UNEXPECTED_INT_VALUE( tid );
}

/**
 * Helper function for c_type_name_c(), c_type_name_eng(), and
 * c_type_name_error() that gets the name of \a type.
 *
 * @param type The type to get the name for.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @return Returns said name.
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 */
PJL_WARN_UNUSED_RESULT
static char const* c_type_name_impl( c_type_t const *type, bool in_english ) {
  static strbuf_t sbufs[ 2 ];
  static unsigned buf_index;

  strbuf_t *const sbuf = &sbufs[ buf_index++ % ARRAY_SIZE( sbufs ) ];
  strbuf_free( sbuf );
  bool space = false;

  c_type_id_t base_tid  = c_type_id_normalize( type->base_tid );
  c_type_id_t store_tid = type->store_tid;
  c_type_id_t attr_tid  = type->attr_tid;

  if ( OPT_LANG_IS(C_ANY) && (attr_tid & TA_NORETURN) != TA_NONE ) {
    //
    // Special case: we store _Noreturn as an attribute, but in C, it's a
    // distinct keyword and printed as such instead being printed between
    // brackets [[like this]].
    //
    static c_type_id_t const C_NORETURN[] = { TA_NORETURN };
    C_TYPE_ID_NAME_CAT(
      sbuf, TA_NORETURN, C_NORETURN, in_english, ' ', &space
    );
    //
    // Now that we've handled _Noreturn for C, remove its bit and fall through
    // to the regular attribute-printing code.
    //
    attr_tid &= c_type_id_compl( TA_NORETURN );
  }

  if ( attr_tid != TA_NONE ) {
    static c_type_id_t const C_ATTRIBUTE[] = {
      TA_CARRIES_DEPENDENCY,
      TA_DEPRECATED,
      TA_MAYBE_UNUSED,
      TA_NODISCARD,
      TA_NORETURN,                      // still here for C++'s [[noreturn]]
      TA_NO_UNIQUE_ADDRESS,
    };

    bool const print_brackets =
      opt_lang >= LANG_C_2X &&
      lexer_is_english() &&
      !in_english;

    bool comma = false;
    char const sep = print_brackets ? ',' : ' ';
    bool *const sep_cat = print_brackets ? &comma : &space;

    if ( print_brackets ) {
      if ( space )
        strbuf_cat( sbuf, " ", 1 );
      strbuf_cat( sbuf, graph_token_c( "[[" ), -1 );
    }
    C_TYPE_ID_NAME_CAT( sbuf, attr_tid, C_ATTRIBUTE, in_english, sep, sep_cat );
    if ( print_brackets )
      strbuf_cat( sbuf, graph_token_c( "]]" ), -1 );
    space = true;
  }

  if ( true ) {
    // Special cases.
    if ( lexer_is_english() ) {
      if ( is_explicit_int( base_tid ) ) {
        base_tid |= TB_INT;
      } else if ( (base_tid & TB_ANY_MODIFIER) != TB_NONE ) {
        // In C/C++, explicit "int" isn't needed when at least one int modifier
        // is present.
        base_tid &= c_type_id_compl( TB_INT );
      }
      if ( (store_tid & (TS_FINAL | TS_OVERRIDE)) != TS_NONE ) {
        // In C/C++, explicit "virtual" shouldn't be present when either
        // "final" or "overrride" is.
        store_tid &= c_type_id_compl( TS_VIRTUAL );
      }
    } else /* !lexer_is_english() */ {
      if ( (base_tid & TB_ANY_MODIFIER) != TB_NONE &&
          (base_tid & (TB_CHAR | TB_ANY_FLOAT | TB_ANY_EMC)) == TB_NONE ) {
        // In English, be explicit about "int".
        base_tid |= TB_INT;
      }
      if ( (store_tid & (TS_FINAL | TS_OVERRIDE)) != TS_NONE ) {
        // In English, either "final" or "overrride" implies "virtual".
        store_tid |= TS_VIRTUAL;
      }
    }
  }

  // Types here MUST have a corresponding row AND column in OK_STORAGE_LANGS.
  static c_type_id_t const C_STORAGE_CLASS[] = {

    // These are first so we get names like "deleted constructor".
    TS_DEFAULT,
    TS_DELETE,
    TS_EXTERN_C,

    // These are second so we get names like "static int".
    TS_AUTO,
    TS_APPLE_BLOCK,
    TS_EXPORT,
    TS_EXTERN,
    TS_FRIEND,
    TS_REGISTER,
    TS_MUTABLE,
    TS_STATIC,
    TS_THREAD_LOCAL,
    TS_TYPEDEF,

    // These are third so we get names like "static inline".
    TS_EXPLICIT,
    TS_INLINE,

    // These are fourth so we get names like "static inline final".
    TS_OVERRIDE,
    TS_FINAL,

    // These are fifth so we get names like "overridden virtual".
    TS_PURE_VIRTUAL,
    TS_VIRTUAL,
    TS_NOEXCEPT,
    TS_THROW,

    // These are sixth so we get names like "static inline constexpr".
    TS_CONSTEVAL,
    TS_CONSTEXPR,
    TS_CONSTINIT,
  };
  C_TYPE_ID_NAME_CAT(
    sbuf, store_tid, C_STORAGE_CLASS, in_english, ' ', &space
  );

  c_type_id_t east_tid = TS_NONE;
  if ( opt_east_const && lexer_is_english() ) {
    east_tid = store_tid & (TS_CONST | TS_VOLATILE);
    store_tid &= c_type_id_compl( TS_CONST | TS_VOLATILE );
  }

  static c_type_id_t const C_QUALIFIER[] = {
    // These are before "shared" so we get names like "strict shared".
    TS_UPC_RELAXED,
    TS_UPC_STRICT,

    TS_UPC_SHARED,

    TS_CONST,
    TS_RESTRICT,
    TS_VOLATILE,

    // These are next so we get names like "const reference".
    TS_REFERENCE,
    TS_RVALUE_REFERENCE,

    // This is last so we get names like "const _Atomic".
    TS_ATOMIC,
  };
  C_TYPE_ID_NAME_CAT( sbuf, store_tid, C_QUALIFIER, in_english, ' ', &space );

  static c_type_id_t const C_TYPE[] = {
    // These are first so we get names like "unsigned int".
    TB_SIGNED,
    TB_UNSIGNED,

    // These are next so we get names like "unsigned long int".
    TB_LONG,
    TB_SHORT,

    // This is next so we get names like "unsigned long _Sat _Fract".
    TB_EMC_SAT,

    TB_VOID,
    TB_AUTO,
    TB_BOOL,
    TB_CHAR,
    TB_CHAR8_T,
    TB_CHAR16_T,
    TB_CHAR32_T,
    TB_WCHAR_T,
    TB_LONG_LONG,
    TB_INT,
    TB_COMPLEX,
    TB_IMAGINARY,
    TB_FLOAT,
    TB_DOUBLE,
    TB_ENUM,
    TB_STRUCT,
    TB_UNION,
    TB_CLASS,

    TB_EMC_ACCUM,
    TB_EMC_FRACT,
  };
  C_TYPE_ID_NAME_CAT( sbuf, base_tid, C_TYPE, in_english, ' ', &space );

  if ( east_tid != TS_NONE )
    C_TYPE_ID_NAME_CAT( sbuf, east_tid, C_QUALIFIER, in_english, ' ', &space );

  // Really special cases.
  if ( (base_tid & TB_NAMESPACE) != TB_NONE )
    strbuf_cat_sep( sbuf, L_NAMESPACE, ' ', &space );
  else if ( (base_tid & TB_SCOPE) != TB_NONE )
    strbuf_cat_sep( sbuf, L_SCOPE, ' ', &space );

  return sbuf->str != NULL ? sbuf->str : "";
}

/**
 * Possibly copies \a sep followed by \a src to \a dst.
 *
 * @param sbuf A pointer to the buffer to concatenate a copy of \a s to.
 * @param src The null-terminated string to copy.
 * @param sep The separator character.
 * @param sep_cat A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
static void strbuf_cat_sep( strbuf_t *sbuf, char const *s, char sep,
                            bool *sep_cat ) {
  assert( sbuf != NULL );
  assert( s != NULL );
  assert( sep_cat != NULL );

  if ( true_or_set( sep_cat ) )
    strbuf_cat( sbuf, &sep, 1 );
  strbuf_cat( sbuf, s, -1 );
}

////////// extern functions ///////////////////////////////////////////////////

c_lang_id_t c_type_check( c_type_t const *type ) {
  // Check that the attribute(s) are legal in the current language.
  C_TYPE_ID_CHECK_LEGAL( type->attr_tid, C_ATTRIBUTE_INFO );

  // Check that the storage class is legal in the current language.
  C_TYPE_ID_CHECK_LEGAL( type->store_tid, C_STORAGE_INFO );

  // Check that the type is legal in the current language.
  C_TYPE_ID_CHECK_LEGAL( type->base_tid, C_TYPE_INFO );

  // Check that the qualifier(s) are legal in the current language.
  C_TYPE_ID_CHECK_LEGAL( type->store_tid, C_QUALIFIER_INFO );

  // Check that the storage class combination is legal in the current language.
  C_TYPE_ID_CHECK_COMBO( type->store_tid, C_STORAGE_INFO, OK_STORAGE_LANGS );

  // Check that the type combination is legal in the current language.
  C_TYPE_ID_CHECK_COMBO( type->base_tid, C_TYPE_INFO, OK_TYPE_LANGS );

  // Check that the qualifier combination is legal in the current language.
  C_TYPE_ID_CHECK_COMBO( type->store_tid, C_QUALIFIER_INFO, OK_QUALIFIER_LANGS );

  return LANG_ANY;
}

bool c_type_add( c_type_t *dst_type, c_type_t const *new_type,
                 c_loc_t const *new_loc ) {
  assert( dst_type != NULL );
  assert( new_type != NULL );
  assert( new_loc != NULL );

  return  c_type_id_add( &dst_type->base_tid, new_type->base_tid, new_loc ) &&
          c_type_id_add( &dst_type->store_tid, new_type->store_tid, new_loc ) &&
          c_type_id_add( &dst_type->attr_tid, new_type->attr_tid, new_loc );
}

bool c_type_add_tid( c_type_t *dst_type, c_type_id_t new_tid,
                     c_loc_t const *new_loc ) {
  c_type_id_t *const dst_tid = c_type_get_tid_ptr( dst_type, new_tid );
  return c_type_id_add( dst_tid, new_tid, new_loc );
}

c_type_t c_type_and( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  return C_TYPE_LIT(
    i_type->base_tid  & j_type->base_tid,
    i_type->store_tid & j_type->store_tid,
    i_type->attr_tid  & j_type->attr_tid
  );
}

bool c_type_equal( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  return  i_type->base_tid  == j_type->base_tid   &&
          i_type->store_tid == j_type->store_tid  &&
          i_type->attr_tid  == j_type->attr_tid;
}

bool c_type_is_any( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  return  c_type_id_is_any( i_type->base_tid,   j_type->base_tid  ) ||
          c_type_id_is_any( i_type->store_tid,  j_type->store_tid ) ||
          c_type_id_is_any( i_type->attr_tid,   j_type->attr_tid  );
}

char const* c_type_name_c( c_type_t const *type ) {
  return c_type_name_impl( type, /*in_english=*/false );
}

char const* c_type_name_eng( c_type_t const *type ) {
  return c_type_name_impl( type, /*in_english=*/true );
}

char const* c_type_name_error( c_type_t const *type ) {
  return c_type_name_impl( type, /*in_english=*/lexer_is_english() );
}

c_type_t c_type_or( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  return C_TYPE_LIT(
    i_type->base_tid  | j_type->base_tid,
    i_type->store_tid | j_type->store_tid,
    i_type->attr_tid  | j_type->attr_tid
  );
}

void c_type_or_eq( c_type_t *dst_type, c_type_t const *add_type ) {
  assert( dst_type != NULL );
  assert( add_type != NULL );

  dst_type->base_tid  |= add_type->base_tid;
  dst_type->store_tid |= add_type->store_tid;
  dst_type->attr_tid  |= add_type->attr_tid;
}

void c_type_and_eq_compl( c_type_t *dst_type, c_type_t const *rm_type ) {
  assert( dst_type != NULL );
  assert( rm_type != NULL );

  dst_type->base_tid  &= c_type_id_compl( rm_type->base_tid );
  dst_type->store_tid &= c_type_id_compl( rm_type->store_tid );
  dst_type->attr_tid  &= c_type_id_compl( rm_type->attr_tid );
}

c_type_id_t* c_type_get_tid_ptr( c_type_t *type, c_type_id_t tid ) {
  assert( type != NULL );

  switch ( c_type_id_tpid( tid ) ) {
    case C_TPID_BASE:
      return &type->base_tid;
    case C_TPID_STORE:
      return &type->store_tid;
    case C_TPID_ATTR:
      return &type->attr_tid;
  } // switch

  UNEXPECTED_INT_VALUE( tid );
}

bool c_type_id_add( c_type_id_t *dst_tid, c_type_id_t new_tid,
                    c_loc_t const *new_loc ) {
  assert( dst_tid != NULL );
  assert( new_loc != NULL );
  assert( c_type_id_tpid( *dst_tid ) == c_type_id_tpid( new_tid ) );

  if ( is_long_int( *dst_tid ) && is_long_int( new_tid ) ) {
    //
    // Special case: if the existing type is "long" and the new type is "long",
    // turn the new type into "long long".
    //
    new_tid = TB_LONG_LONG;
  }

  if ( !c_type_id_is_none( *dst_tid & new_tid ) ) {
    print_error( new_loc,
      "\"%s\" can not be combined with \"%s\"\n",
       c_type_id_name_error( new_tid ), c_type_id_name_error( *dst_tid )
    );
    return false;
  }

  *dst_tid |= new_tid;
  return true;
}

PJL_WARN_UNUSED_RESULT
c_type_part_id_t c_type_id_tpid( c_type_id_t tid ) {
  //
  // If tid has been complemented, e.g., ~TS_REGISTER to denote "all but
  // register," then we have to complement tid back first.
  //
  if ( c_type_id_is_compl( tid ) )
    tid = ~tid;
  tid &= TX_MASK_PART_ID;
  assert( tid <= C_TPID_ATTR );
  return STATIC_CAST( c_type_part_id_t, tid );
}

char const* c_type_id_name_c( c_type_id_t tid ) {
  c_type_t const type = c_type_from_tid( tid );
  return c_type_name_impl( &type, /*in_english=*/false );
}

char const* c_type_id_name_eng( c_type_id_t tid ) {
  c_type_t const type = c_type_from_tid( tid );
  return c_type_name_impl( &type, /*in_english=*/true );
}

char const* c_type_id_name_error( c_type_id_t tid ) {
  c_type_t const type = c_type_from_tid( tid );
  return c_type_name_impl( &type, /*in_english=*/lexer_is_english() );
}

c_type_id_t c_type_id_normalize( c_type_id_t tid ) {
  switch ( c_type_id_tpid( tid ) ) {
    case C_TPID_BASE:
      if ( (tid & TB_SIGNED) != TB_NONE && (tid & TB_CHAR) == TB_NONE ) {
        tid &= c_type_id_compl( TB_SIGNED );
        if ( tid == TB_NONE )
          tid = TB_INT;
      }
      break;
    default:
      /* suppress warning */;
  } // switch
  return tid;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
