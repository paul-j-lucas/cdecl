/*
**      cdecl -- C gibberish translator
**      src/types.c
*/

// local
#include "config.h"                     /* must go first */
#include "lang.h"
#include "literals.h"
#include "options.h"
#include "types.h"
#include "util.h"

// system
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MIN(LANG) (LANG - 1)

#define __  LANG_NONE                   /* legal in all languages */
#define XX  LANG_ALL                    /* illegal in all languages  */
#define KO ~LANG_C_KNR                  /* legal in K&R C only */
#define C8  MIN(LANG_C_89)              /* minimum C89 */
#define C9  MIN(LANG_C_99)              /* minimum C99 */
#define PP  MIN(LANG_CPP)               /* minimum C++ */

///////////////////////////////////////////////////////////////////////////////

static char const L_LONG_LONG[] = "long long";

/**
 * Mapping between C type literals and type.
 */
struct c_type_info {
  char const *literal;                  // C string literal of the type
  c_type_t    type;
};
typedef struct c_type_info c_type_info_t;

static c_type_info_t const C_TYPE_INFO[] = {
  { L_VOID,         T_VOID,         },
  { L_BOOL,         T_BOOL,         },
  { L_CHAR,         T_CHAR,         },
  { L_CHAR16_T,     T_CHAR16_T,     },
  { L_CHAR32_T,     T_CHAR32_T,     },
  { L_WCHAR_T,      T_WCHAR_T,      },
  { L_SHORT,        T_SHORT,        },
  { L_INT,          T_INT,          },
  { L_LONG,         T_LONG,         },
  { L_LONG_LONG,    T_LONG_LONG,    },
  { L_SIGNED,       T_SIGNED,       },
  { L_UNSIGNED,     T_UNSIGNED,     },
  { L_FLOAT,        T_FLOAT,        },
  { L_DOUBLE,       T_DOUBLE,       },
  { L_COMPLEX,      T_COMPLEX,      },
  { L_ENUM,         T_ENUM,         },
  { L_STRUCT,       T_STRUCT,       },
  { L_UNION,        T_UNION,        },
  { L_CLASS,        T_CLASS,        },
};

/**
 * Restrictions on type combinations in languages.
 */
static lang_t const RESTRICTIONS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
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
  /* class     */ { XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,PP },
};

////////// inline functions ///////////////////////////////////////////////////

static inline bool only_one_bit_set( unsigned n ) {
  return n && !(n & (n - 1));
}

////////// extern functions ///////////////////////////////////////////////////

bool c_type_check( c_type_t type ) {
  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
    if ( type & C_TYPE_INFO[i].type ) {
      for ( size_t j = 0; j < i; ++j ) {
        if ( (type & C_TYPE_INFO[j].type) && (opt_lang & RESTRICTIONS[i][j]) ) {
          PRINT_ERR(
            "warning: \"%s\" with \"%s\" is illegal in %s\n",
            C_TYPE_INFO[i].literal, C_TYPE_INFO[j].literal,
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
