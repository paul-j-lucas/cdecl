/*
**      cdecl -- C gibberish translator
**      src/types.c
*/

// local
#include "config.h"                     /* must go first */
#include "lang.h"
#include "literals.h"
#include "types.h"
#include "util.h"

// system
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define T_ALL ~0

///////////////////////////////////////////////////////////////////////////////

/**
 * Mapping between C type literals and type.
 */
struct c_type_info {
  char const *literal;                  // C string literal of the type
  c_type_t    type;
};
typedef struct c_type_info c_type_info_t;

static char const L_LONG_LONG[] = "long long";

static c_type_info_t const C_TYPE_INFO[] = {

  // types
  { L_VOID,         T_VOID,         },
  { L_BOOL,         T_BOOL,         },
  { L_CHAR,         T_CHAR,         },
  { L_CHAR16_T,     T_CHAR16_T,     },
  { L_CHAR32_T,     T_CHAR32_T,     },
  { L_WCHAR_T,      T_WCHAR_T,      },
  { L_ENUM,         T_ENUM,         },
  { L_SHORT,        T_SHORT,        },
  { L_INT,          T_INT,          },
  { L_LONG,         T_LONG,         },
  { L_LONG_LONG,    T_LONG_LONG,    },
  { L_SIGNED,       T_SIGNED,       },
  { L_UNSIGNED,     T_UNSIGNED,     },
  { L_FLOAT,        T_FLOAT,        },
  { L_DOUBLE,       T_DOUBLE,       },
  { L_COMPLEX,      T_COMPLEX,      },
  { L_STRUCT,       T_STRUCT,       },
  { L_CLASS,        T_CLASS,        },

  // storage classes
  { L_AUTO,         T_AUTO,         },
  { L_BLOCK,        T_BLOCK,        },
  { L_EXTERN,       T_EXTERN,       },
  { L_REGISTER,     T_REGISTER,     },
  { L_STATIC,       T_STATIC,       },
  { L_THREAD_LOCAL, T_THREAD_LOCAL, },

  // qualifiers
  { L_CONST,        T_CONST,        },
  { L_RESTRICT,     T_RESTRICT,     },
  { L_VOLATILE,     T_VOLATILE,     },
};

////////// inline functions ///////////////////////////////////////////////////

static inline bool only_one_bit_set( unsigned n ) {
  return n && !(n & (n - 1));
}

////////// extern functions ///////////////////////////////////////////////////

void c_type_check( c_type_t type ) {
  typedef unsigned restriction_t;

#define LANG_ALL ~LANG_NONE

#define __ LANG_NONE
#define XX LANG_ALL
#define CK LANG_C_KNR
#define C8 LANG_C_89
#define C9 LANG_C_99
#define C1 LANG_C_11
#define P  LANG_CPP
#define P1 LANG_CPP_11

  static restriction_t const RESTRICTIONS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
    /*                v  b  c  16 32 wc s  i  l  ll s  u  f  d  c */
    /* void      */ { __,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
    /* bool      */ { XX,__,__,__,__,__,__,__,__,__,__,__,__,__,__ },
    /* char      */ { XX,XX,__,__,__,__,__,__,__,__,__,__,__,__,__ },
    /* char16_t  */ { XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__,__ },
    /* char32_t  */ { XX,XX,XX,XX,__,__,__,__,__,__,__,__,__,__,__ },
    /* wchar_t   */ { XX,XX,XX,__,__,CK,__,__,__,__,__,__,__,__,__ },
    /* short     */ { XX,XX,XX,__,__,XX,__,__,__,__,__,__,__,__,__ },
    /* int       */ { XX,XX,XX,__,__,XX,__,__,__,__,__,__,__,__,__ },
    /* long      */ { XX,XX,XX,__,__,XX,XX,__,__,__,__,__,__,__,__ },
    /* long long */ { XX,XX,XX,__,__,XX,CK,__,__,__,__,__,__,__,__ },
    /* signed    */ { XX,XX,CK,__,__,XX,CK,CK,CK,__,__,__,__,__,__ },
    /* unsigned  */ { XX,XX,__,__,__,XX,__,__,__,__,XX,__,__,__,__ },
    /* float     */ { XX,XX,XX,__,__,XX,XX,XX,C8,XX,XX,XX,__,__,__ },
    /* double    */ { XX,XX,XX,__,__,XX,XX,XX,CK,XX,XX,XX,__,__,__ },
    /* complex   */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,__,__,__ }
  };

#undef __
#undef XX
#undef CK
#undef C8
#undef C9
#undef C1
#undef P
#undef P1

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
    if ( type & C_TYPE_INFO[i].type ) {
      for ( size_t j = 0; j < i; ++j ) {
        if ( type & C_TYPE_INFO[j].type ) {
          char const *const t1 = C_TYPE_INFO[i].literal;
          char const *const t2 = C_TYPE_INFO[j].literal;
          switch ( RESTRICTIONS[i][j] ) {
            case LANG_NONE:
              break;
#if 0
            case LANG_C_89:
              if ( opt_lang != LANG_C_KNR )
                illegal( LANG_C_89, t1, t2 );
              break;
            case LANG_C_KNR:
              if ( opt_lang == LANG_C_KNR )
                illegal( opt_lang, t1, t2 );
              break;
            case LANG_ALL:
              illegal( opt_lang, t1, t2 );
              break;
#endif
          } // switch
        }
      } // for
    }
  } // for
}

char const* c_type_name( c_type_t type ) {
  if ( only_one_bit_set( type ) ) {
		for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i )
			if ( type == C_TYPE_INFO[i].type )
				return C_TYPE_INFO[i].literal;
		INTERNAL_ERR( "%X: unexpected value for type", type );
	}

  static char c_type_buf[ 80 ];
  c_type_buf[0] = '\0';
  bool add_space = false;

  static c_type_t const C_STORAGE_CLASS[] = {
    T_AUTO,
    T_BLOCK,
    T_EXTERN,
    T_REGISTER,
    T_STATIC,
    T_THREAD_LOCAL,
  };
  for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_CLASS ); ++i ) {
    if ( type & C_STORAGE_CLASS[i] ) {
      strcat( c_type_buf, c_type_name( C_STORAGE_CLASS[i] ) );
      add_space = true;
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
      if ( add_space )
        strcat( c_type_buf, " " );
      else
        add_space = true;
      strcat( c_type_buf, c_type_name( C_QUALIFIER[i] ) );
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
    T_ENUM,
    T_LONG_LONG,
    T_INT,
    T_FLOAT,
    T_DOUBLE,
    T_COMPLEX,
    T_STRUCT,
    T_CLASS,
  };

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE ); ++i ) {
    if ( type & C_QUALIFIER[i] ) {
      if ( add_space )
        strcat( c_type_buf, " " );
      else
        add_space = true;
      strcat( c_type_buf, c_type_name( C_TYPE[i] ) );
    }
  } // for

  return c_type_buf;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
