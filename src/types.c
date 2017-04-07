/*
**      cdecl -- C gibberish translator
**      src/types.c
**
**      Paul J. Lucas
*/

// local
#include "config.h"                     /* must go first */
#include "diagnostics.h"
#include "lang.h"
#include "literals.h"
#include "options.h"
#include "types.h"
#include "util.h"

// system
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define STRCAT(DST,SRC)           ((DST) += strcpy_len( (DST), (SRC) ))

///////////////////////////////////////////////////////////////////////////////

/**
 * As part of the special case for <code>long long</code>, its literal is only
 * \c long because it's type, T_LONG_LONG, is always combined with T_LONG,
 * i.e., two bits are set.  Therefore, when printed, it prints one \c long for
 * T_LONG and another \c long for T_LONG_LONG (this literal).  That explains
 * why this literal is only one \c long.
 */
static char const L_LONG_LONG[] = "long";

/**
 * Mapping between C type bits and literals.
 */
struct c_type_info {
  c_type_t    type;
  char const *literal;                  // C string literal of the type
};
typedef struct c_type_info c_type_info_t;

static c_type_info_t const C_TYPE_INFO[] = {
  // types
  { T_VOID,         L_VOID          },
  { T_BOOL,         L_BOOL          },
  { T_CHAR,         L_CHAR          },
  { T_CHAR16_T,     L_CHAR16_T      },
  { T_CHAR32_T,     L_CHAR32_T      },
  { T_WCHAR_T,      L_WCHAR_T       },
  { T_SHORT,        L_SHORT         },
  { T_INT,          L_INT           },
  { T_LONG,         L_LONG          },
  { T_LONG_LONG,    L_LONG_LONG     },  // special case
  { T_SIGNED,       L_SIGNED        },
  { T_UNSIGNED,     L_UNSIGNED      },
  { T_FLOAT,        L_FLOAT         },
  { T_DOUBLE,       L_DOUBLE        },
  { T_COMPLEX,      L_COMPLEX       },
  { T_ENUM,         L_ENUM          },
  { T_STRUCT,       L_STRUCT        },
  { T_UNION,        L_UNION         },
  { T_CLASS,        L_CLASS         },
  // storage classes
  { T_AUTO,         L_AUTO          },
  { T_BLOCK,        L___BLOCK       },  // Apple extension
  { T_EXTERN,       L_EXTERN        },
  { T_REGISTER,     L_REGISTER      },
  { T_STATIC,       L_STATIC        },
  { T_THREAD_LOCAL, L_THREAD_LOCAL  },
  { T_TYPEDEF,      L_TYPEDEF       },
  { T_VIRTUAL,      L_VIRTUAL       },
  { T_PURE_VIRTUAL, L_PURE          },
  // qualifiers
  { T_CONST,        L_CONST         },
  { T_RESTRICT,     L_RESTRICT      },
  { T_VOLATILE,     L_VOLATILE      },
};

#define NUM_TYPES     19
#define BEFORE(LANG)  (LANG - 1)        /* illegal before LANG */

// shorthand, illegal in/before/unless ...
#define __    LANG_NONE                 /* legal in all languages */
#define XX    LANG_ALL                  /* illegal in all languages  */
#define KO   ~LANG_C_KNR                /* legal in K&R C only */
#define C8    BEFORE(LANG_C_89)         /* minimum C89 */
#define C5    BEFORE(LANG_C_95)         /* minimum C95 */
#define C9    BEFORE(LANG_C_99)         /* minimum C99 */
#define C1    BEFORE(LANG_C_11)         /* minimum C11 */
#define PP    BEFORE(LANG_CPP_MIN)      /* minimum C++ */
#define P1    BEFORE(LANG_CPP_11)       /* minimum C++11 */
#define E1  ~(LANG_C_11 | LANG_CPP_11)  /* legal in either C11 or C++11 only */

/**
 * Illegal combinations of types in languages.
 * Only the lower triangle is used.
 */
static c_lang_t const BAD_TYPE_LANGS[ NUM_TYPES ][ NUM_TYPES ] = {
  /*                v  b  c  16 32 wc s  i  l  ll s  u  f  d  c  E  S  U  C */
  /* void      */ { C8,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* bool      */ { XX,C9,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* char      */ { XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* char16_t  */ { XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* char32_t  */ { XX,XX,XX,XX,E1,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* wchar_t   */ { XX,XX,XX,XX,XX,C5,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* short     */ { XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* int       */ { XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* long      */ { XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* long long */ { XX,XX,XX,XX,XX,XX,XX,C9,__,C9,__,__,__,__,__,__,__,__,__ },
  /* signed    */ { XX,XX,C8,XX,XX,XX,C8,C8,C8,C8,C8,__,__,__,__,__,__,__,__ },
  /* unsigned  */ { XX,XX,__,XX,XX,XX,__,__,__,C8,XX,__,__,__,__,__,__,__,__ },
  /* float     */ { XX,XX,XX,XX,XX,XX,XX,XX,KO,XX,XX,XX,__,__,__,__,__,__,__ },
  /* double    */ { XX,XX,XX,XX,XX,XX,XX,XX,C8,XX,XX,XX,XX,__,__,__,__,__,__ },
  /* complex   */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,C9,__,__,__,__ },
  /* enum      */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,__,__,__ },
  /* struct    */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__ },
  /* union     */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__ },
  /* class     */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,PP },
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
  return (type & T_LONG) && !(type & (T_FLOAT | T_DOUBLE));
}

////////// extern functions ///////////////////////////////////////////////////

bool c_type_add( c_type_t *dest_type, c_type_t new_type, YYLTYPE const *loc ) {
  assert( dest_type != NULL );

  if ( is_long_int( new_type ) && is_long_int( *dest_type ) )
    new_type = T_LONG_LONG;

  if ( (new_type & *dest_type) ) {
    print_error( loc,
      "\"%s\" can not be combined with previous declaration\n",
      c_type_name( new_type )
    );
    return false;
  }

  *dest_type |= new_type;
  return true;
}

c_lang_t c_type_check( c_type_t type ) {
  for ( size_t row = 0; row < NUM_TYPES; ++row ) {
    if ( (type & C_TYPE_INFO[ row ].type) ) {
      for ( size_t col = 0; col <= row; ++col ) {
        c_lang_t const bad_langs = BAD_TYPE_LANGS[ row ][ col ];
        if ( (type & C_TYPE_INFO[ col ].type) && (opt_lang & bad_langs) )
          return bad_langs;
      } // for
    }
  } // for
  return LANG_NONE;
}

char const* c_type_name( c_type_t type ) {
  if ( only_one_bit_set( type ) ) {
    for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i )
      if ( type == C_TYPE_INFO[i].type )
        return C_TYPE_INFO[i].literal;
    INTERNAL_ERR( "unexpected value (0x%X) for type\n", type );
  }

  static char name_buf[ 80 ];
  name_buf[0] = '\0';
  char *name = name_buf;
  bool space = false;

  static c_type_t const C_STORAGE_CLASS[] = {
    T_AUTO,
    T_BLOCK,
    T_EXTERN,
    T_REGISTER,
    T_STATIC,
    T_THREAD_LOCAL,
    T_TYPEDEF,
    T_PURE_VIRTUAL,
    T_VIRTUAL,
  };
  for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_CLASS ); ++i ) {
    if ( (type & C_STORAGE_CLASS[i]) ) {
      if ( true_or_set( &space ) )
        STRCAT( name, " " );
      STRCAT( name, c_type_name( C_STORAGE_CLASS[i] ) );
    }
  } // for

  static c_type_t const C_QUALIFIER[] = {
    T_CONST,
    T_RESTRICT,
    T_VOLATILE,
  };
  for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER ); ++i ) {
    if ( (type & C_QUALIFIER[i]) ) {
      if ( true_or_set( &space ) )
        STRCAT( name, " " );
      STRCAT( name, c_type_name( C_QUALIFIER[i] ) );
    }
  } // for

  static c_type_t const C_TYPE[] = {

    // These are first so we get names like "unsigned int".
    T_SIGNED,
    T_UNSIGNED,

    // These are second so we get names like "unsigned long int".
    T_LONG,
    T_SHORT,

    T_VOID,
    T_BOOL,
    T_CHAR,
    T_CHAR16_T,
    T_CHAR32_T,
    T_LONG_LONG,
    T_INT,
    T_COMPLEX,
    T_FLOAT,
    T_DOUBLE,
    T_ENUM,
    T_STRUCT,
    T_UNION,
    T_CLASS,
  };

  if ( !(type & T_CHAR) ) {
    //
    // Special case: explicit "signed" isn't needed for any type except char.
    //
    type &= ~T_SIGNED;
  }

  if ( (type & (T_UNSIGNED | T_SHORT | T_LONG | T_LONG_LONG)) ) {
    //
    // Special case: explicit "int" isn't needed when at least one int modifier
    // is present.
    //
    type &= ~T_INT;
  }

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE ); ++i ) {
    if ( (type & C_TYPE[i]) ) {
      if ( true_or_set( &space ) )
        STRCAT( name, " " );
      STRCAT( name, c_type_name( C_TYPE[i] ) );
    }
  } // for

  return name_buf;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
