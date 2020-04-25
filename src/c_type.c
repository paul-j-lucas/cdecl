/*
**      cdecl -- C gibberish translator
**      src/c_type.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
#include "c_lang.h"
/// @cond DOXYGEN_IGNORE
#define C_TYPE_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_type.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define C_TYPE_CHECK(...) BLOCK(            \
  c_lang_id_t const lang_ids = __VA_ARGS__; \
  if ( lang_ids != LANG_ALL )               \
    return lang_ids; )

#define C_TYPE_CHECK_COMBO(TYPE,TYPES,OK_TYPE_LANGS) \
  c_type_check_combo( (TYPE), (TYPES), ARRAY_SIZE( TYPES ), OK_TYPE_LANGS )

#define C_TYPE_CHECK_LEGAL(TYPE,TYPES) \
  c_type_check_legal( (TYPE), (TYPES), ARRAY_SIZE( TYPES ) )

#define C_TYPE_NAME_CAT(PNAME,TYPE,TYPES,IS_ERROR,SEP,PSEP) \
  c_type_name_cat( (PNAME), (TYPE), (TYPES), ARRAY_SIZE( TYPES ), \
                   (IS_ERROR), (SEP), (PSEP) )

#define CHRCAT(DST,SRC)           ((DST) = chrcpy_end( (DST), (SRC) ))

#define LANG_C_CPP_11_MIN         (LANG_C_MIN(11) | LANG_MIN(CPP_11))

/// @endcond

typedef struct c_type c_type_t;

// local functions
C_WARN_UNUSED_RESULT
static char const*  c_type_literal( c_type_t const*, bool );

C_WARN_UNUSED_RESULT
static char const*  c_type_name_impl( c_type_id_t, bool );

C_WARN_UNUSED_RESULT
static char*        strcpy_sep( char*, char const*, char, bool* );

///////////////////////////////////////////////////////////////////////////////

/**
 * As part of the special case for `long long`, its literal is only `long`
 * because its type, <code>\ref T_LONG_LONG</code>, is always combined with
 * <code>\ref T_LONG</code>, i.e., two bits are set.  Therefore, when printed,
 * it prints one `long` for <code>\ref T_LONG</code> and another `long` for
 * <code>\ref T_LONG_LONG</code> (this literal).  That explains why this
 * literal is only one `long`.
 */
static char const L_LONG_LONG[] = "long";

/**
 * For convenience, this is just a concatenation of `L_RVALUE` and
 * `L_REFERENCE`.
 */
static char const L_RVALUE_REFERENCE[] = "rvalue reference";

/**
 * <code>\ref T_TYPEDEF_TYPE</code> exists only so there can be a row/column
 * for it in the <code>\ref OK_TYPE_LANGS</code> table to make things like
 * `signed size_t` illegal.
 *
 * <code>\ref T_TYPEDEF_TYPE</code> doesn't have any printable representation
 * (only the name of the type is printed); therefore, its literal is the empty
 * string.
 */
static char const L_TYPEDEF_TYPE[] = "";

/**
 * C/C++ language(s)/literal pairs: for the given language(s) only, use the
 * given literal.  This allows different languages to use different literals,
 * e.g., "_Noreturn" for C and "noreturn" for C++.
 */
struct c_lang_lit {
  c_lang_id_t   lang_ids;
  char const   *literal;
};
typedef struct c_lang_lit c_lang_lit_t;

/**
 * Mapping between C type bits, literals, and valid language(s).
 */
struct c_type {
  c_type_id_t         type_id;          ///< The type.
  c_lang_id_t         lang_ids;         ///< Language(s) OK in.
  char const         *english;          ///< English version (if not NULL).

  /**
   * Array of language(s)/literal pair(s).  The array is terminated by an
   * element that has #LANG_ALL for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_lit;
};

/**
 * Type mapping for attributes.
 */
static c_type_t const C_ATTRIBUTE_INFO[] = {
  { T_CARRIES_DEPENDENCY, LANG_MIN(CPP_11), L_CARRIES_DEPENDENCY_ENG,
    (c_lang_lit_t[]){ { LANG_ALL, L_CARRIES_DEPENDENCY } } },

  { T_DEPRECATED, LANG_MIN(CPP_11), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_DEPRECATED } } },

  { T_MAYBE_UNUSED, LANG_MIN(CPP_17), L_MAYBE_UNUSED_ENG,
    (c_lang_lit_t[]){ { LANG_ALL, L_MAYBE_UNUSED } } },

  { T_NODISCARD, LANG_MIN(CPP_11), L_NON_DISCARDABLE_ENG,
    (c_lang_lit_t[]){ { LANG_ALL, L_NODISCARD } } },

  { T_NORETURN, LANG_C_CPP_11_MIN, L_NON_RETURNING_ENG,
    (c_lang_lit_t[]){ { LANG_CPP_ALL, L_NORETURN  },
                      { LANG_ALL,     L__NORETURN } } },
};

/**
 * Type mapping for qualifiers.
 */
static c_type_t const C_QUALIFIER_INFO[] = {
  { T_ATOMIC, LANG_MIN(C_11), L_ATOMIC,
    (c_lang_lit_t[]){ { LANG_ALL, L__ATOMIC } } },

  { T_CONST, LANG_MIN(C_89), L_CONSTANT,
    (c_lang_lit_t[]){ { LANG_ALL, L_CONST } } },

  { T_REFERENCE, LANG_MIN(CPP_OLD), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_REFERENCE } } },

  { T_RVALUE_REFERENCE, LANG_MIN(CPP_11), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_RVALUE_REFERENCE } } },

  { T_RESTRICT, LANG_MIN(C_89), L_RESTRICTED,
    (c_lang_lit_t[]){ { LANG_C_KNR | LANG_CPP_ALL, L___RESTRICT__ },
                      { LANG_ALL,                  L_RESTRICT     } } },

  { T_VOLATILE, LANG_MIN(C_89), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_VOLATILE } } },
};

/**
 * Type mapping for storage classes (or storage-class-like).
 *
 * @note
 * This array _must_ have the same size and order as OK_STORAGE_LANGS.
 */
static c_type_t const C_STORAGE_INFO[] = {
  // storage classes
  { T_AUTO_STORAGE, LANG_MAX(CPP_03), L_AUTOMATIC,
    (c_lang_lit_t[]){ { LANG_ALL, L_AUTO } } },

  { T_BLOCK, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L___BLOCK } } },

  { T_EXTERN, LANG_ALL, L_EXTERNAL,
    (c_lang_lit_t[]){ { LANG_ALL, L_EXTERN } } },

  { T_REGISTER, LANG_MAX(CPP_14), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_REGISTER } } },

  { T_STATIC, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_STATIC } } },

  { T_THREAD_LOCAL, LANG_C_CPP_11_MIN, L_THREAD_LOCAL_ENG,
    (c_lang_lit_t[]){ { LANG_MAX(CPP_03), L___THREAD     },
                      { LANG_ALL,         L_THREAD_LOCAL } } },

  { T_TYPEDEF, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_TYPEDEF } } },

  // storage-class-like
  { T_CONSTEVAL, LANG_MIN(CPP_20), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CONSTEVAL } } },

  { T_CONSTEXPR, LANG_MIN(CPP_11), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CONSTEXPR } } },

  { T_DEFAULT, LANG_MIN(CPP_11), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_DEFAULT } } },

  { T_DELETE, LANG_MIN(CPP_11), L_DELETED,
    (c_lang_lit_t[]){ { LANG_ALL, L_DELETE } } },

  { T_EXPLICIT, LANG_CPP_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_EXPLICIT } } },

  { T_FINAL, LANG_MIN(CPP_11), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_FINAL } } },

  { T_FRIEND, LANG_CPP_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_FRIEND } } },

  { T_INLINE, LANG_MIN(C_89), NULL,
    (c_lang_lit_t[]){ { LANG_C_KNR, L___INLINE__  },
                      { LANG_ALL,   L_INLINE      } } },

  { T_MUTABLE, LANG_CPP_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_MUTABLE } } },

  { T_NOEXCEPT, LANG_MIN(CPP_11), L_NO_EXCEPTION_ENG,
    (c_lang_lit_t[]){ { LANG_ALL, L_NOEXCEPT } } },

  { T_OVERRIDE, LANG_MIN(CPP_11), L_OVERRIDDEN_ENG,
    (c_lang_lit_t[]){ { LANG_ALL, L_OVERRIDE } } },

  { T_THROW, LANG_CPP_ALL, L_NON_THROWING_ENG,
    (c_lang_lit_t[]){ { LANG_ALL, L_THROW } } },

  { T_VIRTUAL, LANG_CPP_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_VIRTUAL } } },

  { T_PURE_VIRTUAL, LANG_CPP_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_PURE } } },
};

/**
 * Type mapping for simpler types.
 *
 * @note
 * This array _must_ have the same size and order as OK_TYPE_LANGS.
 */
static c_type_t const C_TYPE_INFO[] = {
  { T_VOID, LANG_MIN(C_89), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_VOID } } },

  { T_AUTO_TYPE, LANG_MIN(C_89), L_AUTOMATIC,
    (c_lang_lit_t[]){ { LANG_MAX(CPP_03), L___AUTO_TYPE },
                      { LANG_ALL,         L_AUTO        } } },

  { T_BOOL, LANG_MIN(C_89), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_BOOL } } },

  { T_CHAR, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CHAR } } },

  { T_CHAR8_T, LANG_MIN(CPP_20), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CHAR8_T } } },

  { T_CHAR16_T, LANG_C_CPP_11_MIN, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CHAR16_T } } },

  { T_CHAR32_T, LANG_C_CPP_11_MIN, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CHAR32_T } } },

  { T_WCHAR_T, LANG_MIN(C_95), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_WCHAR_T } } },

  { T_SHORT, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_SHORT } } },

  { T_INT, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_INT } } },

  { T_LONG, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_LONG } } },

  { T_LONG_LONG, LANG_MIN(C_99), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_LONG_LONG } } },

  { T_SIGNED, LANG_MIN(C_89), NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_SIGNED } } },

  { T_UNSIGNED, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_UNSIGNED } } },

  { T_FLOAT, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_FLOAT } } },

  { T_DOUBLE, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_DOUBLE } } },

  { T_COMPLEX, LANG_MIN(C_99), L_COMPLEX,
    (c_lang_lit_t[]){ { LANG_ALL, L__COMPLEX } } },

  { T_IMAGINARY, LANG_MIN(C_99), L_IMAGINARY,
    (c_lang_lit_t[]){ { LANG_ALL, L__IMAGINARY } } },

  { T_ENUM, LANG_MIN(C_89), L_ENUMERATION,
    (c_lang_lit_t[]){ { LANG_ALL, L_ENUM } } },

  { T_STRUCT, LANG_ALL, L_STRUCTURE,
    (c_lang_lit_t[]){ { LANG_ALL, L_STRUCT } } },

  { T_UNION, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_UNION } } },

  { T_CLASS, LANG_CPP_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_CLASS } } },

  { T_TYPEDEF_TYPE, LANG_ALL, NULL,
    (c_lang_lit_t[]){ { LANG_ALL, L_TYPEDEF_TYPE } } },
};

/// @cond DOXYGEN_IGNORE

//      shorthand   legal in ...
#define __          LANG_ALL
#define XX          LANG_NONE
#define KR          LANG_C_KNR
#define C_          LANG_MAX(C_NEW)
#define C8          LANG_MIN(C_89)
#define C5          LANG_MIN(C_95)
#define C9          LANG_MIN(C_99)
#define C1          LANG_MIN(C_11)
#define PP          LANG_CPP_ALL
#define P3          LANG_MIN(CPP_03)
#define P1          LANG_MIN(CPP_11)
#define P2          LANG_MIN(CPP_20)
#define E1          LANG_C_CPP_11_MIN

/// @endcond

// There is no OK_ATTRIBUTE_LANGS because all combinations of attributes are
// legal.

/**
 * Legal combinations of storage classes in languages.
 *
 * @note
 * This array _must_ have the same size and order as C_STORAGE_INFO.
 */
static c_lang_id_t const OK_STORAGE_LANGS[][ ARRAY_SIZE( C_STORAGE_INFO ) ] = {
// Only the lower triangle is used.
//  a  b  e  r  s  tl td   cv cx df de ex fi fr in mu ne o  t  v  pv
  { __,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// auto
  { __,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// block
  { XX,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// extern
  { XX,__,XX,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// regist
  { XX,XX,XX,XX,__,__,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// static
  { XX,E1,E1,XX,E1,E1,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// thread
  { XX,__,XX,XX,XX,XX,__,  __,__,__,__,__,__,__,__,__,__,__,__,__,__ },// typed

  { P1,P1,P1,XX,P1,XX,XX,  P2,P1,__,__,__,__,__,__,__,__,__,__,__,__ },// c'eval
  { P1,P1,P1,XX,P1,XX,XX,  XX,P1,__,__,__,__,__,__,__,__,__,__,__,__ },// c'expr
  { XX,XX,XX,XX,XX,XX,XX,  P1,P1,P1,__,__,__,__,__,__,__,__,__,__,__ },// defaul
  { XX,XX,XX,XX,XX,XX,XX,  P1,P1,XX,P1,__,__,__,__,__,__,__,__,__,__ },// delete
  { XX,XX,XX,XX,XX,XX,XX,  XX,P1,P1,P1,PP,__,__,__,__,__,__,__,__,__ },// explic
  { XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,XX,XX,P1,__,__,__,__,__,__,__,__ },// final
  { XX,XX,XX,XX,XX,XX,XX,  P2,P1,XX,XX,XX,XX,PP,__,__,__,__,__,__,__ },// friend
  { XX,XX,C8,XX,C8,XX,XX,  P2,P1,P1,P1,PP,P1,PP,C8,__,__,__,__,__,__ },// inline
  { XX,XX,XX,XX,XX,XX,XX,  XX,XX,XX,XX,XX,XX,XX,XX,P3,__,__,__,__,__ },// mut
  { XX,XX,P1,XX,P1,XX,P1,  P2,P1,P1,P1,PP,P1,P1,P1,XX,P1,__,__,__,__ },// noexc
  { XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,XX,XX,P1,XX,C1,XX,C1,P1,__,__,__ },// overr
  { XX,XX,PP,XX,PP,XX,PP,  P2,P1,P1,P1,PP,PP,XX,PP,XX,XX,PP,PP,__,__ },// throw
  { XX,XX,XX,XX,XX,XX,XX,  XX,P2,XX,XX,XX,P1,XX,PP,XX,C1,P1,PP,PP,__ },// virt
  { XX,XX,XX,XX,XX,XX,XX,  XX,P2,XX,XX,XX,XX,XX,PP,XX,C1,P1,PP,PP,PP },// pure
};

/**
 * Legal combinations of types in languages.
 *
 * @note
 * This array _must_ have the same size and order as C_TYPE_INFO.
 */
static c_lang_id_t const OK_TYPE_LANGS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
// Only the lower triangle is used.
//  v  a1 b  c  8  16 32 wc s  i  l  ll s  u  f  d  co im e  st un cl t
  { C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// void
  { XX,C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// auto
  { XX,XX,C9,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// bool
  { XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// char
  { XX,XX,XX,XX,P2,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c8
  { XX,XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c16
  { XX,XX,XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c32
  { XX,XX,XX,XX,XX,XX,XX,C5,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// wcha
  { XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// shor
  { XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// int
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// long
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,__,C9,__,__,__,__,__,__,__,__,__,__,__ },// llon
  { XX,XX,XX,C8,XX,XX,XX,XX,C8,C8,C8,C8,C8,__,__,__,__,__,__,__,__,__,__ },// sign
  { XX,XX,XX,__,XX,XX,XX,XX,__,__,__,C8,XX,__,__,__,__,__,__,__,__,__,__ },// unsi
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,KR,XX,XX,XX,__,__,__,__,__,__,__,__,__ },// floa
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,XX,XX,XX,XX,__,__,__,__,__,__,__,__ },// doub
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,C9,__,__,__,__,__,__ },// comp
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,XX,C9,__,__,__,__,__ },// imag
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,__,__,__,__ },// enum
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P1,__,__,__,__ },// stru
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__ },// unio
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P1,XX,XX,PP,__ },// clas
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__ },// type
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a type_id is some form of <code>long int</code> only, and \e
 * not either `long float` (K&R) or `long double` (C89).
 *
 * @param type_id The <code>\ref c_type_id_t</code> to check.
 * @return Returns `true` only if \a type_id is some form of `long int`.
 */
C_WARN_UNUSED_RESULT
static inline bool is_long_int( c_type_id_t type_id ) {
  return (type_id & (T_LONG | T_FLOAT | T_DOUBLE)) == T_LONG;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks that the type combination is legal in the current language.
 *
 * @param type_id The <code>\ref c_type_id_t</code> to check.
 * @param types The array of types to check against.
 * @param types_size The size of \a types.
 * @param type_langs The type/languages array to check against.
 * @return Returns the bitwise-or of the language(s) \a type_id is legal in.
 */
C_WARN_UNUSED_RESULT
static c_lang_id_t
c_type_check_combo( c_type_id_t type_id, c_type_t const types[],
                    size_t types_size,
                    c_lang_id_t const type_langs[][types_size] ) {
  for ( size_t row = 0; row < types_size; ++row ) {
    if ( (type_id & types[ row ].type_id) != T_NONE ) {
      for ( size_t col = 0; col <= row; ++col ) {
        c_lang_id_t const lang_ids = type_langs[ row ][ col ];
        if ( (type_id & types[ col ].type_id) != T_NONE &&
             (opt_lang & lang_ids) == LANG_NONE ) {
          return lang_ids;
        }
      } // for
    }
  } // for
  return LANG_ALL;
}

/**
 * Checks that \a type_id is legal in the current language.
 *
 * @param type_id The <code>\ref c_type_id_t</code> to check.
 * @param types The array of types to check against.
 * @param types_size The size of \a types.
 * @return Returns the bitwise-or of the language(s) \a type_id is legal in.
 */
C_WARN_UNUSED_RESULT
static c_lang_id_t
c_type_check_legal( c_type_id_t type_id, c_type_t const types[],
                    size_t types_size ) {
  for ( size_t row = 0; row < types_size; ++row ) {
    c_type_t const *const t = &types[ row ];
    if ( (type_id & t->type_id) != T_NONE &&
         (opt_lang & t->lang_ids) == LANG_NONE ) {
      return t->lang_ids;
    }
  } // for
  return LANG_ALL;
}

/**
 * Gets the literal of a given <code>\ref c_type</code>, either gibberish or,
 * if appropriate and available, English.
 *
 * @param t A pointer to the <code>\ref c_type</code> to get the literal of.
 * @param is_error `true` if getting the literal for part of an error message.
 * @return Returns said literal.
 */
C_WARN_UNUSED_RESULT
static char const* c_type_literal( c_type_t const *t, bool is_error ) {
  bool const is_english = c_mode == C_ENGLISH_TO_GIBBERISH;
  if ( is_english == is_error && t->english != NULL )
    return t->english;
  for ( c_lang_lit_t const *ll = t->lang_lit; true; ++ll ) {
    if ( (ll->lang_ids & opt_lang) != LANG_NONE )
      return ll->literal;
  } // for
}

/**
 * Gets the name of an individual type.
 *
 * @param type_id The <code>\ref c_type_id_t</code> to get the name for;
 * \a type_id must have exactly one bit set.
 * @param is_error `true` if getting the name for part of an error message.
 * @return Returns said name.
 */
C_WARN_UNUSED_RESULT
static char const* c_type_name_1( c_type_id_t type_id, bool is_error ) {
  assert( exactly_one_bit_set( type_id ) );

  for ( size_t i = 0; i < ARRAY_SIZE( C_ATTRIBUTE_INFO ); ++i ) {
    c_type_t const *const t = &C_ATTRIBUTE_INFO[i];
    if ( type_id == t->type_id )
      return c_type_literal( t, is_error );
  } // for

  for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER_INFO ); ++i ) {
    c_type_t const *const t = &C_QUALIFIER_INFO[i];
    if ( type_id == t->type_id )
      return c_type_literal( t, is_error );
  } // for

  for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_INFO ); ++i ) {
    c_type_t const *const t = &C_STORAGE_INFO[i];
    if ( type_id == t->type_id )
      return c_type_literal( t, is_error );
  } // for

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
    c_type_t const *const t = &C_TYPE_INFO[i];
    if ( type_id == t->type_id )
      return c_type_literal( t, is_error );
  } // for

  UNEXPECTED_INT_VALUE( type_id );
}

/**
 * Concatenates the partial type name onto the full type name being made.
 *
 * @param pname A pointer to the pointer to the name to concatenate to.
 * @param type_id The <code>\ref c_type_id_t</code> to concatenate the name of.
 * @param types The array of types to use.
 * @param types_size The size of \a types.
 * @param is_error `true` if concatenating the name for part of an error
 * message.
 * @param sep The separator character.
 * @param sep_cat A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
static void c_type_name_cat( char **pname, c_type_id_t type_id,
                             c_type_id_t const types[], size_t types_size,
                             bool is_error, char sep, bool *sep_cat ) {
  assert( pname != NULL );
  for ( size_t i = 0; i < types_size; ++i ) {
    if ( (type_id & types[i]) != T_NONE )
      *pname = strcpy_sep(
        *pname, c_type_name_1( types[i], is_error ), sep, sep_cat
      );
  } // for
}

/**
 * Gets the name of \a type_id.
 *
 * @param type_id The <code>\ref c_type_id_t</code> to get the name for.
 * @param is_error `true` if getting the name for part of an error message.
 * @return Returns said name.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 */
C_WARN_UNUSED_RESULT
static char const* c_type_name_impl( c_type_id_t type_id, bool is_error ) {
  static char name_buf[ 256 ];
  char *name = name_buf;
  name[0] = '\0';
  bool space = false;

  if ( (type_id & T_MASK_ATTRIBUTE) != T_NONE ) {
    static c_type_id_t const C_ATTRIBUTE[] = {
      T_CARRIES_DEPENDENCY,
      T_DEPRECATED,
      T_MAYBE_UNUSED,
      T_NODISCARD,
      T_NORETURN,
    };

    bool const brackets =
      C_LANG_IS_CPP() &&
      c_mode == C_ENGLISH_TO_GIBBERISH &&
      !is_error;

    bool comma = false;
    char const sep = brackets ? ',' : ' ';
    bool *const sep_cat = brackets ? &comma : &space;

    if ( brackets )
      STRCAT( name, graph_token_c( "[[" ) );
    C_TYPE_NAME_CAT( &name, type_id, C_ATTRIBUTE, is_error, sep, sep_cat );
    if ( brackets )
      STRCAT( name, graph_token_c( "]]" ) );
    space = true;
  }

  // Special cases.
  if ( (type_id & T_CHAR) == T_NONE ) {
    // Explicit "signed" isn't needed for any type except char.
    type_id &= ~T_SIGNED;
  }
  if ( c_mode == C_GIBBERISH_TO_ENGLISH ) {
    if ( (type_id & T_INT_MODIFIER) != T_NONE &&
         (type_id & T_ANY_CHAR) == T_NONE &&
         (type_id & T_ANY_FLOAT) == T_NONE ) {
      // In English, be explicit about "int".
      type_id |= T_INT;
    }
  } else /* c_mode == C_ENGLISH_TO_GIBBERISH */ {
    if ( is_explicit_int( type_id ) ) {
      type_id |= T_INT;
    } else if ( (type_id & T_INT_MODIFIER) != T_NONE ) {
      // In C/C++, explicit "int" isn't needed when at least one int modifier
      // is present.
      type_id &= ~T_INT;
    }
  }
  if ( (type_id & (T_FINAL | T_OVERRIDE)) != T_NONE ) {
    // Either "final" or "overrride" implies "virtual".
    type_id |= T_VIRTUAL;
  }

  // Types here MUST have a corresponding row AND column in OK_STORAGE_LANGS.
  static c_type_id_t const C_STORAGE_CLASS[] = {

    // These are first so we get names like "deleted constructor".
    T_DEFAULT,
    T_DELETE,

    // These are second so we get names like "static int".
    T_AUTO_STORAGE,
    T_BLOCK,
    T_EXTERN,
    T_FRIEND,
    T_REGISTER,
    T_MUTABLE,
    T_STATIC,
    T_THREAD_LOCAL,
    T_TYPEDEF,

    // These are third so we get names like "static inline".
    T_EXPLICIT,
    T_INLINE,

    // These are fourth so we get names like "static inline final".
    T_OVERRIDE,
    T_FINAL,

    // These are fifth so we get names like "overridden virtual".
    T_PURE_VIRTUAL,
    T_VIRTUAL,
    T_NOEXCEPT,
    T_THROW,

    // These are sixth so we get names like "static inline constexpr".
    T_CONSTEVAL,
    T_CONSTEXPR,
  };
  C_TYPE_NAME_CAT( &name, type_id, C_STORAGE_CLASS, is_error, ' ', &space );

  static c_type_id_t const C_QUALIFIER[] = {
    T_CONST,
    T_RESTRICT,
    T_VOLATILE,

    T_REFERENCE,
    T_RVALUE_REFERENCE,

    // This is last so we get names like "const _Atomic".
    T_ATOMIC,
  };
  C_TYPE_NAME_CAT( &name, type_id, C_QUALIFIER, is_error, ' ', &space );

  static c_type_id_t const C_TYPE[] = {

    // These are first so we get names like "unsigned int".
    T_SIGNED,
    T_UNSIGNED,

    // These are second so we get names like "unsigned long int".
    T_LONG,
    T_SHORT,

    T_VOID,
    T_AUTO_TYPE,
    T_BOOL,
    T_CHAR,
    T_CHAR8_T,
    T_CHAR16_T,
    T_CHAR32_T,
    T_WCHAR_T,
    T_LONG_LONG,
    T_INT,
    T_COMPLEX,
    T_IMAGINARY,
    T_FLOAT,
    T_DOUBLE,
    T_ENUM,
    T_STRUCT,
    T_UNION,
    T_CLASS,
  };
  C_TYPE_NAME_CAT( &name, type_id, C_TYPE, is_error, ' ', &space );

  // Really special cases.
  if ( (type_id & T_NAMESPACE) != T_NONE )
    name = strcpy_sep( name, L_NAMESPACE, ' ', &space );
  else if ( (type_id & T_SCOPE) != T_NONE )
    name = strcpy_sep( name, L_SCOPE, ' ', &space );

  return name_buf;
}

/**
 * Possibly copies \a sep followed by \a src to \a dst.
 *
 * @param dst A pointer to receive the copy of \a src.
 * @param src The null-terminated string to copy.
 * @param sep The separator character.
 * @param sep_cat A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
C_WARN_UNUSED_RESULT
static char* strcpy_sep( char *dst, char const *src, char sep, bool *sep_cat ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( sep_cat != NULL );

  if ( true_or_set( sep_cat ) )
    CHRCAT( dst, sep );
  STRCAT( dst, src );
  return dst;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_type_add( c_type_id_t *dest_type, c_type_id_t new_type,
                 c_loc_t const *new_loc ) {
  assert( dest_type != NULL );
  assert( new_loc != NULL );

  if ( is_long_int( *dest_type ) && is_long_int( new_type ) ) {
    //
    // If the existing type is "long" and the new type is "long", turn the new
    // type into "long long".
    //
    new_type = T_LONG_LONG;
  }

  if ( (*dest_type & new_type) != T_NONE ) {
    char const *const new_name =
      FREE_STRDUP_LATER( c_type_name_error( new_type ) );
    print_error( new_loc,
      "\"%s\" can not be combined with \"%s\"",
      new_name, c_type_name_error( *dest_type )
    );
    return false;
  }

  *dest_type |= new_type;
  return true;
}

c_lang_id_t c_type_check( c_type_id_t type_id ) {
  // Check that the attribute(s) are legal in the current language.
  C_TYPE_CHECK( C_TYPE_CHECK_LEGAL( type_id, C_ATTRIBUTE_INFO ) );

  // Check that the storage class is legal in the current language.
  C_TYPE_CHECK( C_TYPE_CHECK_LEGAL( type_id, C_STORAGE_INFO ) );

  // Check that the type is legal in the current language.
  C_TYPE_CHECK( C_TYPE_CHECK_LEGAL( type_id, C_TYPE_INFO ) );

  // Check that the qualifier(s) are legal in the current language.
  C_TYPE_CHECK( C_TYPE_CHECK_LEGAL( type_id, C_QUALIFIER_INFO ) );

  // Check that the storage class combination is legal in the current language.
  C_TYPE_CHECK(
    C_TYPE_CHECK_COMBO( type_id, C_STORAGE_INFO, OK_STORAGE_LANGS )
  );

  // Check that the type combination is legal in the current language.
  C_TYPE_CHECK( C_TYPE_CHECK_COMBO( type_id, C_TYPE_INFO, OK_TYPE_LANGS ) );

  return LANG_ALL;
}

char const* c_type_name( c_type_id_t type_id ) {
  return c_type_name_impl( type_id, /*is_error=*/false );
}

char const* c_type_name_error( c_type_id_t type_id ) {
  return c_type_name_impl( type_id, /*is_error=*/true );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
