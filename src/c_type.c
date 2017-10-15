/*
**      cdecl -- C gibberish translator
**      src/c_type.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "config.h"                     /* must go first */
#include "c_lang.h"
#include "c_type.h"
#include "diagnostics.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// system
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define C_TYPE_NAME_CAT(PNAME,TYPE,TYPES,IS_ERROR,PSPACE) \
  c_type_name_cat( (PNAME), (TYPE), (TYPES), ARRAY_SIZE( TYPES ), \
                   (IS_ERROR), (PSPACE) )

#define STRCAT(DST,SRC)           ((DST) = strcpy_end( (DST), (SRC) ))

// local functions
static char const* c_type_name_impl( c_type_t, bool );

///////////////////////////////////////////////////////////////////////////////

/**
 * As part of the special case for <code>long long</code>, its literal is only
 * \c long because its type, T_LONG_LONG, is always combined with T_LONG, i.e.,
 * two bits are set.  Therefore, when printed, it prints one \c long for T_LONG
 * and another \c long for T_LONG_LONG (this literal).  That explains why this
 * literal is only one \c long.
 */
static char const L_LONG_LONG[] = "long";

/**
 * For convenience, this is just a concatenation of L_RVALUE and L_REFERENCE.
 */
static char const L_RVALUE_REFERENCE[] = "rvalue reference";

/**
 * T_TYPEDEF_TYPE exists only so there can be a row/column for it in the
 * OK_TYPE_LANGS table to make things like <code>signed size_t</code> illegal.
 *
 * T_TYPEDEF_TYPE doesn't have any printable representation (only the name of
 * the type is printed); therefore, its literal is the empty string.
 */
static char const L_TYPEDEF_TYPE[] = "";

/**
 * Mapping between C type bits, literals, and valid language(s).
 */
struct c_type_info {
  c_type_t    type;
  char const *literal;                  // C string literal of the type
  char const *english;                  // English version, if not NULL
  c_lang_t    ok_langs;
};
typedef struct c_type_info c_type_info_t;

static c_type_info_t const C_QUALIFIER_INFO[] = {
  { T_ATOMIC,       L__ATOMIC,      L_ATOMIC, LANG_MIN(C_11)                  },
  { T_CONST,        L_CONST,      L_CONSTANT, LANG_MIN(C_89)                  },
  { T_REFERENCE,    L_REFERENCE,        NULL, LANG_MIN(CPP_11)                },
  { T_RVALUE_REFERENCE,
                    L_RVALUE_REFERENCE, NULL, LANG_MIN(CPP_11)                },
  { T_RESTRICT,     L_RESTRICT, L_RESTRICTED, LANG_MIN(C_89) & ~LANG_CPP_ALL  },
  { T_VOLATILE,     L_VOLATILE,         NULL, LANG_MIN(C_89)                  },
};

static c_type_info_t const C_STORAGE_INFO[] = {
  // storage classes
  { T_AUTO_C,       L_AUTO,      L_AUTOMATIC, LANG_MAX(CPP_03)                },
  { T_BLOCK,        L___BLOCK,          NULL, LANG_ALL                        },
  { T_EXTERN,       L_EXTERN,     L_EXTERNAL, LANG_ALL                        },
  { T_REGISTER,     L_REGISTER,         NULL, LANG_ALL                        },
  { T_STATIC,       L_STATIC,           NULL, LANG_ALL                        },
  { T_THREAD_LOCAL, L_THREAD_LOCAL,     NULL, LANG_C_11 | LANG_MIN(CPP_11)    },
  { T_TYPEDEF,      L_TYPEDEF,          NULL, LANG_ALL                        },

  // storage-class-like
  { T_CONSTEXPR,    L_CONSTEXPR,        NULL, LANG_MIN(CPP_11)                },
  { T_FINAL,        L_FINAL,            NULL, LANG_MIN(CPP_11)                },
  { T_FRIEND,       L_FRIEND,           NULL, LANG_CPP_ALL                    },
  { T_INLINE,       L_INLINE,           NULL, LANG_MIN(C_99)                  },
  { T_MUTABLE,      L_MUTABLE,          NULL, LANG_MIN(CPP_MIN)               },
  { T_NOEXCEPT,     L_NOEXCEPT,
                    L_NO_EXCEPTION,           LANG_MIN(CPP_11)                },
  { T_NORETURN,     L__NORETURN,
                    L_NON_RETURNING,          LANG_C_11                       },
  { T_OVERRIDE,     L_OVERRIDE, L_OVERRIDDEN, LANG_MIN(CPP_11)                },
  { T_THROW,        L_THROW,  L_NON_THROWING, LANG_MIN(CPP_MIN)               },
  { T_VIRTUAL,      L_VIRTUAL,          NULL, LANG_CPP_ALL                    },
  { T_PURE_VIRTUAL, L_PURE,             NULL, LANG_CPP_ALL                    },
};

static c_type_info_t const C_TYPE_INFO[] = {
  { T_VOID,         L_VOID,             NULL, LANG_MIN(C_89)                  },
  { T_AUTO_CPP_11,  L_AUTO,      L_AUTOMATIC, LANG_MIN(CPP_11)                },
  { T_BOOL,         L_BOOL,             NULL, LANG_MIN(C_89)                  },
  { T_CHAR,         L_CHAR,             NULL, LANG_ALL                        },
  { T_CHAR16_T,     L_CHAR16_T,         NULL, LANG_C_11 | LANG_MIN(CPP_11)    },
  { T_CHAR32_T,     L_CHAR32_T,         NULL, LANG_C_11 | LANG_MIN(CPP_11)    },
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
#define E1          LANG_C_11 | LANG_MIN(CPP_11)

/**
 * Legal combinations of storage classes in languages.
 * Only the lower triangle is used.
 */
static c_lang_t const OK_STORAGE_LANGS[][ ARRAY_SIZE( C_STORAGE_INFO ) ] = {
/*                   a  b  e  r  s  tl td   ce fi fr in mu ne nr o  t  v  pv */
/* auto         */ { __,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__ },
/* block        */ { __,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__ },
/* extern       */ { XX,__,__,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__ },
/* register     */ { XX,__,XX,__,__,__,__,  __,__,__,__,__,__,__,__,__,__,__ },
/* static       */ { XX,XX,XX,XX,__,__,__,  __,__,__,__,__,__,__,__,__,__,__ },
/* thread_local */ { XX,E1,E1,XX,E1,E1,__,  __,__,__,__,__,__,__,__,__,__,__ },
/* typedef      */ { XX,__,XX,XX,XX,XX,__,  __,__,__,__,__,__,__,__,__,__,__ },

/* constexpr    */ { P1,P1,P1,XX,P1,XX,XX,  P1,__,__,__,__,__,__,__,__,__,__ },
/* final        */ { XX,XX,XX,XX,XX,XX,XX,  XX,P1,__,__,__,__,__,__,__,__,__ },
/* friend       */ { XX,XX,XX,XX,XX,XX,XX,  P1,XX,PP,__,__,__,__,__,__,__,__ },
/* inline       */ { XX,XX,C9,XX,C9,XX,XX,  P1,P1,PP,C9,__,__,__,__,__,__,__ },
/* mutable      */ { XX,XX,XX,XX,XX,XX,XX,  XX,XX,XX,XX,P3,__,__,__,__,__,__ },
/* noexcept     */ { XX,XX,P1,XX,P1,XX,P1,  XX,P1,P1,P1,XX,P1,__,__,__,__,__ },
/* noreturn     */ { XX,XX,C1,XX,C1,XX,XX,  XX,XX,XX,C1,XX,C1,C1,__,__,__,__ },
/* override     */ { XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,C1,XX,C1,XX,P1,__,__,__ },
/* throw        */ { XX,XX,PP,XX,PP,XX,PP,  XX,PP,XX,PP,XX,XX,XX,PP,PP,__,__ },
/* virtual      */ { XX,XX,XX,XX,XX,XX,XX,  XX,P1,XX,PP,XX,C1,XX,P1,__,PP,__ },
/* pure virtual */ { XX,XX,XX,XX,XX,XX,XX,  XX,XX,XX,PP,XX,C1,XX,P1,__,PP,PP },
};

/**
 * Legal combinations of types in languages.
 * Only the lower triangle is used.
 */
static c_lang_t const OK_TYPE_LANGS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
  //v  a1 b  c  16 32 wc s  i  l  ll s  u  f  d  co im e  st un cl t
  { C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// v
  { XX,P1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// a11
  { XX,XX,C9,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// b
  { XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c
  { XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c16
  { XX,XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// c32
  { XX,XX,XX,XX,XX,XX,C5,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// wc
  { XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// s
  { XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// i
  { XX,XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },// l
  { XX,XX,XX,XX,XX,XX,XX,XX,C9,__,C9,__,__,__,__,__,__,__,__,__,__,__ },// ll
  { XX,XX,XX,C8,XX,XX,XX,C8,C8,C8,C8,C8,__,__,__,__,__,__,__,__,__,__ },// s
  { XX,XX,XX,__,XX,XX,XX,__,__,__,C8,XX,__,__,__,__,__,__,__,__,__,__ },// u
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,KR,XX,XX,XX,__,__,__,__,__,__,__,__,__ },// f
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,XX,XX,XX,XX,__,__,__,__,__,__,__,__ },// d
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,C9,__,__,__,__,__,__ },// co
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,XX,C9,__,__,__,__,__ },// im
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,__,__,__,__ },// e
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P1,__,__,__,__ },// st
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__ },// un
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P1,XX,XX,PP,__ },// cl
  { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__ },// t
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether the given type is some form of <code>long int</code> only,
 * and \e not either <code>long float</code> (K&R) or <code>long double</code>
 * (C89).
 *
 * @param type The type to check.
 * @return Returns \c true only if \a type is some form of <code>long
 * int</code>.
 */
static inline bool is_long_int( c_type_t type ) {
  return (type & T_LONG) != T_NONE && (type & (T_FLOAT | T_DOUBLE)) == T_NONE;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets the literal of a given c_type_info, either gibberish or, if appropriate
 * and available, English.
 *
 * @param t A pointer to the c_type_info to get the literal of.
 * @return Returns said literal.
 */
static char const* c_type_literal( c_type_info_t const *t, bool is_error ) {
  bool const is_english = c_mode == MODE_ENGLISH;
  return is_english == is_error && t->english != NULL ? t->english : t->literal;
}

/**
 * Given an individual type, get its name.
 *
 * @param type The type to get the name for; \a type must have exactly one bit
 * set.
 * @param is_error \c true if getting the name for part of an error message.
 * @return Returns said name.
 */
static char const* c_type_name_1( c_type_t type, bool is_error ) {
  assert( exactly_one_bit_set( type ) );

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i )
    if ( type == C_TYPE_INFO[i].type )
      return c_type_literal( &C_TYPE_INFO[i], is_error );
  for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_INFO ); ++i )
    if ( type == C_STORAGE_INFO[i].type )
      return c_type_literal( &C_STORAGE_INFO[i], is_error );
  for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER_INFO ); ++i )
    if ( type == C_QUALIFIER_INFO[i].type )
      return c_type_literal( &C_QUALIFIER_INFO[i], is_error );

  INTERNAL_ERR( "unexpected value (0x%" PRIX_C_TYPE_T ") for type\n", type );
}

/**
 * Concatenates the partial type name onto the full type name being made.
 *
 * @param pname A pointer to the pointer to the name to concatenate to.
 * @param type The type to concatenate the name of.
 * @param types The array of types to use.
 * @param types_size The size of \a types.
 * @param is_error \c true if concatenating the name for part of an error
 * message.
 * @param pspace A pointer to a variable to keep track of whether a space has
 * been concatenated.
 */
static void c_type_name_cat( char **pname, c_type_t type,
                             c_type_t const types[], size_t types_size,
                             bool is_error, bool *pspace ) {
  for ( size_t i = 0; i < types_size; ++i ) {
    if ( (type & types[i]) != T_NONE ) {
      if ( true_or_set( pspace ) )
        STRCAT( *pname, " " );
      STRCAT( *pname, c_type_name_impl( types[i], is_error ) );
    }
  } // for
}

/**
 * Given a type, get its name.
 *
 * @param type The type to get the name for.
 * @param is_error \c true if getting the name for part of an error message.
 * @return Returns said name.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same \c printf() statement.
 */
static char const* c_type_name_impl( c_type_t type, bool is_error ) {
  if ( exactly_one_bit_set( type ) )
    return c_type_name_1( type, is_error );

  static char name_buf[ 80 ];
  char *name = name_buf;
  name[0] = '\0';
  bool space = false;

  static c_type_t const C_STORAGE_CLASS[] = {
    T_AUTO_C,
    T_BLOCK,
    T_EXTERN,
    T_FRIEND,
    T_REGISTER,
    T_MUTABLE,
    T_STATIC,
    T_THREAD_LOCAL,
    T_TYPEDEF,
    T_PURE_VIRTUAL,
    T_VIRTUAL,

    // This is second so we get names like "static inline".
    T_INLINE,

    // These are third so we get names like "static inline noreturn".
    T_CONSTEXPR,
    T_NORETURN,

    T_OVERRIDE,
    T_FINAL,
    T_NOEXCEPT,
    T_THROW
  };
  C_TYPE_NAME_CAT( &name, type, C_STORAGE_CLASS, is_error, &space );

  static c_type_t const C_QUALIFIER[] = {
    T_CONST,
    T_RESTRICT,
    T_VOLATILE,

    T_REFERENCE,
    T_RVALUE_REFERENCE,

    // This is last so we get names like "const _Atomic".
    T_ATOMIC,
  };
  C_TYPE_NAME_CAT( &name, type, C_QUALIFIER, is_error, &space );

  static c_type_t const C_TYPE[] = {

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

  if ( (type & T_CHAR) == T_NONE ) {
    //
    // Special case: explicit "signed" isn't needed for any type except char.
    //
    type &= ~T_SIGNED;
  }

  if ( (type & (T_UNSIGNED | T_SHORT | T_LONG | T_LONG_LONG)) != T_NONE ) {
    //
    // Special case: explicit "int" isn't needed when at least one int modifier
    // is present.
    //
    type &= ~T_INT;
  }

  C_TYPE_NAME_CAT( &name, type, C_TYPE, is_error, &space );

  return name_buf;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_type_add( c_type_t *dest_type, c_type_t new_type, c_loc_t const *loc ) {
  assert( dest_type != NULL );

  if ( is_long_int( *dest_type ) && is_long_int( new_type ) ) {
    //
    // If the existing type is "long" and the new type is "long", turn the new
    // type into "long long".
    //
    new_type = T_LONG_LONG;
  }

  if ( (*dest_type & new_type) != T_NONE ) {
    char const *const new_name = check_strdup( c_type_name_error( new_type ) );
    print_error( loc,
      "\"%s\" can not be combined with \"%s\"",
      new_name, c_type_name_error( *dest_type )
    );
    FREE( new_name );
    return false;
  }

  *dest_type |= new_type;
  return true;
}

c_lang_t c_type_check( c_type_t type ) {
  //
  // Check that the storage-class is legal in the current language.
  //
  for ( size_t row = 0; row < ARRAY_SIZE( C_STORAGE_INFO ); ++row ) {
    c_type_info_t const *const si = &C_STORAGE_INFO[ row ];
    if ( (type & si->type) != T_NONE && (opt_lang & si->ok_langs) == LANG_NONE )
      return si->ok_langs;
  } // for

  //
  // Check that the type is legal in the current language.
  //
  for ( size_t row = 0; row < ARRAY_SIZE( C_TYPE_INFO ); ++row ) {
    c_type_info_t const *const ti = &C_TYPE_INFO[ row ];
    if ( (type & ti->type) != T_NONE && (opt_lang & ti->ok_langs) == LANG_NONE )
      return ti->ok_langs;
  } // for

  //
  // Check that the qualifier(s) are legal in the current language.
  //
  for ( size_t row = 0; row < ARRAY_SIZE( C_QUALIFIER_INFO ); ++row ) {
    c_type_info_t const *const qi = &C_QUALIFIER_INFO[ row ];
    if ( (type & qi->type) != T_NONE && (opt_lang & qi->ok_langs) == LANG_NONE )
      return qi->ok_langs;
  } // for

  //
  // Check that the storage class combination is legal in the current language.
  //
  for ( size_t row = 0; row < ARRAY_SIZE( C_STORAGE_INFO ); ++row ) {
    if ( (type & C_STORAGE_INFO[ row ].type) != T_NONE ) {
      for ( size_t col = 0; col <= row; ++col ) {
        c_lang_t const ok_langs = OK_STORAGE_LANGS[ row ][ col ];
        if ( (type & C_STORAGE_INFO[ col ].type) != T_NONE &&
             (opt_lang & ok_langs) == LANG_NONE ) {
          return ok_langs;
        }
      } // for
    }
  } // for

  //
  // Check that the type combination is legal in the current language.
  //
  for ( size_t row = 0; row < ARRAY_SIZE( C_TYPE_INFO ); ++row ) {
    if ( (type & C_TYPE_INFO[ row ].type) != T_NONE ) {
      for ( size_t col = 0; col <= row; ++col ) {
        c_lang_t const ok_langs = OK_TYPE_LANGS[ row ][ col ];
        if ( (type & C_TYPE_INFO[ col ].type) != T_NONE &&
             (opt_lang & ok_langs) == LANG_NONE ) {
          return ok_langs;
        }
      } // for
    }
  } // for

  return LANG_ALL;
}

char const* c_type_name( c_type_t type ) {
  return c_type_name_impl( type, /*is_error=*/false );
}

char const* c_type_name_error( c_type_t type ) {
  return c_type_name_impl( type, /*is_error=*/true );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
