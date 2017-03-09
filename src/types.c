/*
**      cdecl -- C gibberish translator
**      src/types.c
*/

// local
#include "config.h"                     /* must go first */
#include "common.h"
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
 * As part of the special case for <code>long long</code>, its literal it only
 * \c long because it's type, T_LONG_LONG, is always combined with T_LONG,
 * i.e., two bits are set.  Therefore, when printed, it prints one \c long for
 * T_LONG and another \c long for T_LONG_LONG (this literal).  That explains
 * why this literal is only one \c long.
 */
static char const L_LONG_LONG[] = "long";

/**
 * Mapping between C type literals and type bits.
 */
struct c_type_info {
  char const *literal;                  // C string literal of the type
  c_type_t    type;
};
typedef struct c_type_info c_type_info_t;

static c_type_info_t const C_TYPE_INFO[] = {
  // types
  { L_VOID,         T_VOID          },
  { L_BOOL,         T_BOOL          },
  { L_CHAR,         T_CHAR          },
  { L_CHAR16_T,     T_CHAR16_T      },
  { L_CHAR32_T,     T_CHAR32_T      },
  { L_WCHAR_T,      T_WCHAR_T       },
  { L_SHORT,        T_SHORT         },
  { L_INT,          T_INT           },
  { L_LONG,         T_LONG          },
  { L_LONG_LONG,    T_LONG_LONG     },  // special case
  { L_SIGNED,       T_SIGNED        },
  { L_UNSIGNED,     T_UNSIGNED      },
  { L_FLOAT,        T_FLOAT         },
  { L_DOUBLE,       T_DOUBLE        },
  { L_COMPLEX,      T_COMPLEX       },
  { L_ENUM,         T_ENUM          },
  { L_STRUCT,       T_STRUCT        },
  { L_UNION,        T_UNION         },
  { L_CLASS,        T_CLASS         },
  // storage classes
  { L_AUTO,         T_AUTO          },
  { L___BLOCK,      T_BLOCK         },  // Apple extension
  { L_EXTERN,       T_EXTERN        },
  { L_REGISTER,     T_REGISTER      },
  { L_STATIC,       T_STATIC        },
  { L_THREAD_LOCAL, T_THREAD_LOCAL  },
  { L_TYPEDEF,      T_TYPEDEF,      },
  // qualifiers
  { L_CONST,        T_CONST         },
  { L_RESTRICT,     T_RESTRICT      },
  { L_VOLATILE,     T_VOLATILE      },
};

#define NUM_TYPES 19
#define MIN(LANG) (LANG - 1)            /* illegal before LANG */

// shorthand      // illegal in...
#define __        LANG_NONE             /* legal in all languages */
#define XX        LANG_ALL              /* illegal in all languages  */
#define KO       ~LANG_C_KNR            /* legal in K&R C only */
#define C8        MIN(LANG_C_89)        /* minimum C89 */
#define C9        MIN(LANG_C_99)        /* minimum C99 */
#define P3        MIN(LANG_CPP_03)      /* minimum C++03 */

/**
 * Illegal combinations of types in languages.
 * Only the lower triangle is used.
 */
static lang_t const BAD_TYPE_LANG[ NUM_TYPES ][ NUM_TYPES ] = {
  /*                v  b  c  16 32 wc s  i  l  ll s  u  f  d  c  E  S  U  C */
  /* void      */ { __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* bool      */ { XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* char      */ { XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* char16_t  */ { XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* char32_t  */ { XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* wchar_t   */ { XX,XX,XX,XX,XX,C9,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* short     */ { XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* int       */ { XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* long      */ { XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* long long */ { XX,XX,XX,XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__ },
  /* signed    */ { XX,XX,C8,XX,XX,XX,C8,C8,C8,C8,C8,__,__,__,__,__,__,__,__ },
  /* unsigned  */ { XX,XX,__,XX,XX,XX,__,__,__,C8,XX,__,__,__,__,__,__,__,__ },
  /* float     */ { XX,XX,XX,XX,XX,XX,XX,XX,KO,XX,XX,XX,__,__,__,__,__,__,__ },
  /* double    */ { XX,XX,XX,XX,XX,XX,XX,XX,C8,XX,XX,XX,XX,__,__,__,__,__,__ },
  /* complex   */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C9,C9,__,__,__,__,__ },
  /* enum      */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,C8,__,__,__ },
  /* struct    */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__ },
  /* union     */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__ },
  /* class     */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,P3 },
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

/**
 * Checks whether only 1 bit is set in the given integer.
 *
 * @param n The number to check.
 * @reeturn Returns \c true only if exactly 1 bit is set.
 */
static inline bool only_one_bit_set( unsigned n ) {
  return n && !(n & (n - 1));
}

////////// extern functions ///////////////////////////////////////////////////

bool c_type_add( c_type_t *dest_type, c_type_t new_type ) {
  assert( dest_type );

  if ( is_long_int( new_type ) && is_long_int( *dest_type ) )
    new_type = T_LONG_LONG;

  if ( new_type & *dest_type ) {
    PRINT_ERR(
      "error: \"%s\" can not be combined with previous declaration\n",
      c_type_name( new_type )
    );
    return false;
  }

  *dest_type |= new_type;
  return true;
}

bool c_type_check( c_type_t type ) {
  for ( size_t row = 0; row < NUM_TYPES; ++row ) {
    if ( type & C_TYPE_INFO[ row ].type ) {
      for ( size_t col = 0; col < row; ++col ) {
        if ( (type & C_TYPE_INFO[ col ].type) &&
             (opt_lang & BAD_TYPE_LANG[ row ][ col ]) ) {
          PRINT_ERR(
            "warning: \"%s\" with \"%s\" is illegal in %s\n",
            C_TYPE_INFO[ row ].literal, C_TYPE_INFO[ col ].literal,
            lang_name( opt_lang )
          );
          return false;
        }
      } // for
    }
  } // for
  return true;
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
  };
  for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_CLASS ); ++i ) {
    if ( type & C_STORAGE_CLASS[i] ) {
      STRCAT( name, c_type_name( C_STORAGE_CLASS[i] ) );
      space = true;
      break;
    }
  } // for

  static c_type_t const C_QUALIFIER[] = {
    T_CONST,
    T_RESTRICT,
    T_VOLATILE,
  };
  for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER ); ++i ) {
    if ( type & C_QUALIFIER[i] ) {
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
    T_FLOAT,
    T_DOUBLE,
    T_COMPLEX,
    T_ENUM,
    T_STRUCT,
    T_UNION,
    T_CLASS,
  };

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE ); ++i ) {
    if ( type & C_TYPE[i] ) {
      if ( true_or_set( &space ) )
        STRCAT( name, " " );
      STRCAT( name, c_type_name( C_TYPE[i] ) );
    }
  } // for

  return name_buf;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
