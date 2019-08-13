/*
**      cdecl -- C gibberish translator
**      src/c_type.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#define CDECL_TYPE_INLINE _GL_EXTERN_INLINE
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

// local functions
static char const*  c_type_name_impl( c_type_id_t, bool );
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
 * Mapping between C type bits, literals, and valid language(s).
 */
struct c_type {
  c_type_id_t type_id;                  ///< The type.
  char const *literal;                  ///< C string literal of the type.
  char const *english;                  ///< English version (if not NULL).
  c_lang_id_t lang_ids;                 ///< Language(s) OK in.
};
typedef struct c_type c_type_t;

/**
 * Type mapping for attributes.
 *
 * @hideinitializer
 */
static c_type_t const C_ATTRIBUTE_INFO[] = {
  { T_CARRIES_DEPENDENCY,
                    L_CARRIES_DEPENDENCY,
                    L_CARRIES_DEPENDENCY2,    LANG_MIN(CPP_11)                },
  { T_DEPRECATED,   L_DEPRECATED,       NULL, LANG_MIN(CPP_11)                },
  { T_MAYBE_UNUSED, L_MAYBE_UNUSED,
                    L_MAYBE_UNUSED2,          LANG_MIN(CPP_17)                },
  { T_NODISCARD,    L_NODISCARD,
                    L_NON_DISCARDABLE,        LANG_MIN(CPP_11)                },
  { T_NORETURN,     L__NORETURN,
                    L_NON_RETURNING,          LANG_C_CPP_11_MIN               },
};

/**
 * Type mapping for qualifiers.
 *
 * @hideinitializer
 */
static c_type_t const C_QUALIFIER_INFO[] = {
  { T_ATOMIC,       L__ATOMIC,      L_ATOMIC, LANG_MIN(C_11)                  },
  { T_CONST,        L_CONST,      L_CONSTANT, LANG_MIN(C_89)                  },
  { T_REFERENCE,    L_REFERENCE,        NULL, LANG_MIN(CPP_11)                },
  { T_RVALUE_REFERENCE,
                    L_RVALUE_REFERENCE, NULL, LANG_MIN(CPP_11)                },
  { T_RESTRICT,     L_RESTRICT, L_RESTRICTED, LANG_MIN(C_89) & ~LANG_CPP_ALL  },
  { T_VOLATILE,     L_VOLATILE,         NULL, LANG_MIN(C_89)                  },
};

/**
 * Type mapping for storage classes (or storage-class-like).
 *
 * @hideinitializer
 */
static c_type_t const C_STORAGE_INFO[] = {
  // storage classes
  { T_AUTO_C,       L_AUTO,      L_AUTOMATIC, LANG_MAX(CPP_03)                },
  { T_BLOCK,        L___BLOCK,          NULL, LANG_ALL                        },
  { T_EXTERN,       L_EXTERN,     L_EXTERNAL, LANG_ALL                        },
  { T_REGISTER,     L_REGISTER,         NULL, LANG_ALL                        },
  { T_STATIC,       L_STATIC,           NULL, LANG_ALL                        },
  { T_THREAD_LOCAL, L_THREAD_LOCAL,     NULL, LANG_C_CPP_11_MIN               },
  { T_TYPEDEF,      L_TYPEDEF,          NULL, LANG_ALL                        },

  // storage-class-like
  { T_CONSTEVAL,    L_CONSTEVAL,        NULL, LANG_MIN(CPP_20)                },
  { T_CONSTEXPR,    L_CONSTEXPR,        NULL, LANG_MIN(CPP_11)                },
  { T_DEFAULT,      L_DEFAULT,          NULL, LANG_CPP_ALL                    },
  { T_DELETE,       L_DELETE,      L_DELETED, LANG_CPP_ALL                    },
  { T_EXPLICIT,     L_EXPLICIT,         NULL, LANG_CPP_ALL                    },
  { T_FINAL,        L_FINAL,            NULL, LANG_MIN(CPP_11)                },
  { T_FRIEND,       L_FRIEND,           NULL, LANG_CPP_ALL                    },
  { T_INLINE,       L_INLINE,           NULL, LANG_MIN(C_99)                  },
  { T_MUTABLE,      L_MUTABLE,          NULL, LANG_CPP_ALL                    },
  { T_NOEXCEPT,     L_NOEXCEPT,
                    L_NO_EXCEPTION,           LANG_MIN(CPP_11)                },
  { T_OVERRIDE,     L_OVERRIDE, L_OVERRIDDEN, LANG_MIN(CPP_11)                },
  { T_THROW,        L_THROW,  L_NON_THROWING, LANG_CPP_ALL                    },
  { T_VIRTUAL,      L_VIRTUAL,          NULL, LANG_CPP_ALL                    },
  { T_PURE_VIRTUAL, L_PURE,             NULL, LANG_CPP_ALL                    },
};

/**
 * Type mapping for simpler types.
 *
 * @hideinitializer
 */
static c_type_t const C_TYPE_INFO[] = {
  { T_VOID,         L_VOID,             NULL, LANG_MIN(C_89)                  },
  { T_AUTO_CPP_11,  L_AUTO,      L_AUTOMATIC, LANG_MIN(CPP_11)                },
  { T_BOOL,         L_BOOL,             NULL, LANG_MIN(C_89)                  },
  { T_CHAR,         L_CHAR,             NULL, LANG_ALL                        },
  { T_CHAR8_T,      L_CHAR8_T,          NULL, LANG_MIN(CPP_20)                },
  { T_CHAR16_T,     L_CHAR16_T,         NULL, LANG_C_CPP_11_MIN               },
  { T_CHAR32_T,     L_CHAR32_T,         NULL, LANG_C_CPP_11_MIN               },
  { T_WCHAR_T,      L_WCHAR_T,          NULL, LANG_MIN(C_95)                  },
  { T_SHORT,        L_SHORT,            NULL, LANG_ALL                        },
  { T_INT,          L_INT,              NULL, LANG_ALL                        },
  { T_LONG,         L_LONG,             NULL, LANG_ALL                        },
  { T_LONG_LONG,    L_LONG_LONG,        NULL, LANG_MIN(C_89)                  },
  { T_SIGNED,       L_SIGNED,           NULL, LANG_MIN(C_89)                  },
  { T_UNSIGNED,     L_UNSIGNED,         NULL, LANG_ALL                        },
  { T_FLOAT,        L_FLOAT,            NULL, LANG_ALL                        },
  { T_DOUBLE,       L_DOUBLE,           NULL, LANG_ALL                        },
  { T_COMPLEX,      L__COMPLEX,    L_COMPLEX, LANG_MIN(C_99)                  },
  { T_IMAGINARY,    L__IMAGINARY,
                    L_IMAGINARY,              LANG_MIN(C_99)                  },
  { T_ENUM,         L_ENUM,    L_ENUMERATION, LANG_MIN(C_89)                  },
  { T_STRUCT,       L_STRUCT,    L_STRUCTURE, LANG_ALL                        },
  { T_UNION,        L_UNION,            NULL, LANG_ALL                        },
  { T_CLASS,        L_CLASS,            NULL, LANG_CPP_ALL                    },
  { T_TYPEDEF_TYPE, L_TYPEDEF_TYPE,     NULL, LANG_ALL                        },
};

/// @cond DOXYGEN_IGNORE

//      shorthand   legal in ...
#define __          LANG_ALL
#define XX          LANG_NONE
#define KR          LANG_C_KNR
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
 * @hideinitializer
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
  { XX,XX,C9,XX,C9,XX,XX,  P2,P1,P1,P1,PP,P1,PP,C9,__,__,__,__,__,__ },// inline
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
 * @hideinitializer
 */
static c_lang_id_t const OK_TYPE_LANGS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
// Only the lower triangle is used.
//v  a  b  c  8  16 32 wc s  i  l  ll s  u  f  d  co im e  st un cl t
{ C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// void
{ XX,P1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// auto
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
static inline bool is_long_int( c_type_id_t type_id ) {
  return (type_id & (T_LONG | T_FLOAT | T_DOUBLE)) == T_LONG;
}

/**
 * Gets the literal of a given <code>\ref c_type</code>, either gibberish
 * or, if appropriate and available, English.
 *
 * @param t A pointer to the <code>\ref c_type</code> to get the literal
 * of.
 * @param is_error `true` if getting the literal for part of an error message.
 * @return Returns said literal.
 */
static inline char const* c_type_literal( c_type_t const *t, bool is_error ) {
  bool const is_english = c_mode == MODE_ENGLISH_TO_GIBBERISH;
  return is_english == is_error && t->english != NULL ? t->english : t->literal;
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
static c_lang_id_t
c_type_check_legal( c_type_id_t type_id, c_type_t const types[],
                    size_t types_size ) {
  for ( size_t row = 0; row < types_size; ++row ) {
    c_type_t const *const ti = &types[ row ];
    if ( (type_id & ti->type_id) != T_NONE &&
         (opt_lang & ti->lang_ids) == LANG_NONE ) {
      return ti->lang_ids;
    }
  } // for
  return LANG_ALL;
}

/**
 * Gets the name of an individual type.
 *
 * @param type_id The <code>\ref c_type_id_t</code> to get the name for;
 * \a type_id must have exactly one bit set.
 * @param is_error `true` if getting the name for part of an error message.
 * @return Returns said name.
 */
static char const* c_type_name_1( c_type_id_t type_id, bool is_error ) {
  assert( exactly_one_bit_set( type_id ) );

  for ( size_t i = 0; i < ARRAY_SIZE( C_ATTRIBUTE_INFO ); ++i ) {
    if ( type_id == C_ATTRIBUTE_INFO[i].type_id ) {
      char const *literal = c_type_literal( &C_ATTRIBUTE_INFO[i], is_error );
      if ( literal == L__NORETURN && C_LANG_IS_CPP() ) {
        //
        // _Noreturn is a special case.  In C11, it's "_Noreturn"; in C++11,
        // it's "noreturn".  Since this is the only special case like this,
        // it's not worth extending c_type to handle language-specific
        // literals, so we just check for "_Noreturn" here and change it to
        // "noreturn" when translating to C++.
        //
        literal = L_NORETURN;
      }
      return literal;
    }
  } // for

  for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER_INFO ); ++i )
    if ( type_id == C_QUALIFIER_INFO[i].type_id )
      return c_type_literal( &C_QUALIFIER_INFO[i], is_error );

  for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_INFO ); ++i )
    if ( type_id == C_STORAGE_INFO[i].type_id )
      return c_type_literal( &C_STORAGE_INFO[i], is_error );

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i )
    if ( type_id == C_TYPE_INFO[i].type_id )
      return c_type_literal( &C_TYPE_INFO[i], is_error );

  INTERNAL_ERR( "unexpected value (0x%" PRIX_C_TYPE_T ") for type\n", type_id );
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
 * @param sep_cpy A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
static void c_type_name_cat( char **pname, c_type_id_t type_id,
                             c_type_id_t const types[], size_t types_size,
                             bool is_error, char sep, bool *sep_cpy ) {
  assert( pname != NULL );
  for ( size_t i = 0; i < types_size; ++i ) {
    if ( (type_id & types[i]) != T_NONE )
      *pname = strcpy_sep(
        *pname, c_type_name_1( types[i], is_error ), sep, sep_cpy
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
      c_mode == MODE_ENGLISH_TO_GIBBERISH &&
      !is_error;

    bool comma = false;
    char const sep = brackets ? ',' : ' ';
    bool *const sep_cpy = brackets ? &comma : &space;

    if ( brackets )
      STRCAT( name, graph_token_c( "[[" ) );
    C_TYPE_NAME_CAT( &name, type_id, C_ATTRIBUTE, is_error, sep, sep_cpy );
    if ( brackets )
      STRCAT( name, graph_token_c( "]]" ) );
    space = true;
  }

  // Special cases.
  if ( (type_id & T_CHAR) == T_NONE ) {
    // Explicit "signed" isn't needed for any type except char.
    type_id &= ~T_SIGNED;
  }
  if ( (type_id & (T_UNSIGNED | T_SHORT | T_LONG | T_LONG_LONG)) != T_NONE ) {
    // Explicit "int" isn't needed when at least one int modifier is present.
    type_id &= ~T_INT;
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
    T_AUTO_C,
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
    T_AUTO_CPP_11,
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
 * @param sep_cpy A pointer to a variable to keep track of whether \a sep has
 * been copied.
 */
static char* strcpy_sep( char *dst, char const *src, char sep, bool *sep_cpy ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( sep_cpy != NULL );

  if ( true_or_set( sep_cpy ) )
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
