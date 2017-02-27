/*
**      cdecl -- C gibberish translator
**      src/keywords.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "keywords.h"
#include "lang.h"
#include "literals.h"
#include "types.h"
#include "cdgram.h"                     /* must go last */

// standard
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

#define ANY       0                     /* any language */
#define LANG_CPP  LANG_CPP_ANY          /* any C++ */
#define MIN(LANG) (LANG_ ## LANG - 1)   /* minimum required language */
#define NOT(LANG) (LANG_ ## LANG)       /* not in this language */

/**
 * Array of all C/C++ keywords (relevant for declarations).
 */
static c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,          Y_AUTO,         T_AUTO,         ANY                      },
  { L_CHAR,          Y_CHAR,         T_CHAR,         ANY                      },
  { L_DOUBLE,        Y_DOUBLE,       T_DOUBLE,       ANY                      },
  { L_EXTERN,        Y_EXTERN,       T_EXTERN,       ANY                      },
  { L_FLOAT,         Y_FLOAT,        T_FLOAT,        ANY                      },
  { L_INT,           Y_INT,          T_INT,          ANY                      },
  { L_LONG,          Y_LONG,         T_LONG,         ANY                      },
  { L_REGISTER,      Y_REGISTER,     T_REGISTER,     ANY                      },
  { L_SHORT,         Y_SHORT,        T_SHORT,        ANY                      },
  { L_STATIC,        Y_STATIC,       T_STATIC,       ANY                      },
  { L_STRUCT,        Y_STRUCT,       T_STRUCT,       ANY                      },
  { L_TYPEDEF,       Y_TYPEDEF,      T_TYPEDEF,      ANY                      },
  { L_UNION,         Y_UNION,        T_UNION,        ANY                      },
  { L_UNSIGNED,      Y_UNSIGNED,     T_UNSIGNED,     ANY                      },

  // C89
  { L_CONST,         Y_CONST,        T_CONST,        MIN(C_89)                },
  { L_ENUM,          Y_ENUM,         T_ENUM,         MIN(C_89)                },
  { L_SIGNED,        Y_SIGNED,       T_SIGNED,       MIN(C_89)                },
  { L_VOID,          Y_VOID,         T_VOID,         MIN(C_89)                },
  { L_VOLATILE,      Y_VOLATILE,     T_VOLATILE,     MIN(C_89)                },

  // C99
  { L_BOOL,          Y_BOOL,         T_BOOL,         MIN(C_99)                },
  { L_COMPLEX,       Y_COMPLEX,      T_COMPLEX,      MIN(C_99) | NOT(CPP)     },
  { L_RESTRICT,      Y_RESTRICT,     T_RESTRICT,     MIN(C_99) | NOT(CPP)     },
  { L_WCHAR_T,       Y_WCHAR_T,      T_WCHAR_T,      MIN(C_99)                },

  // C11
  { L_NORETURN,      Y_NORETURN,     T_NONE,         MIN(C_11) | NOT(CPP)     },
  { L__NORETURN,     Y_NORETURN,     T_NONE,         MIN(C_11) | NOT(CPP)     },
  { L__THREAD_LOCAL, Y_THREAD_LOCAL, T_THREAD_LOCAL, MIN(C_11) | NOT(CPP)     },

  // C++
  { L_CLASS,         Y_CLASS,        T_CLASS,        MIN(CPP_MIN)             },

  // C11 & C++11
  { L_CHAR16_T,      Y_CHAR16_T,     T_CHAR16_T,     MIN(C_11) | MIN(CPP_MIN) },
  { L_CHAR32_T,      Y_CHAR32_T,     T_CHAR32_T,     MIN(C_11) | MIN(CPP_MIN) },
  { L_THREAD_LOCAL,  Y_THREAD_LOCAL, T_THREAD_LOCAL, MIN(C_11) | MIN(CPP_11)  },

  // Apple extension
  { L___BLOCK,       Y___BLOCK,      T_BLOCK,        MIN(C_89)                },

  { NULL,            0,              T_NONE,         0                        }
};

////////// extern functions ///////////////////////////////////////////////////

c_keyword_t const* c_keyword_find_literal( char const *literal ) {
  for ( c_keyword_t const *k = C_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( literal, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

c_keyword_t const* c_keyword_find_token( int y_token ) {
  for ( c_keyword_t const *k = C_KEYWORDS; k->literal; ++k ) {
    if ( y_token == k->y_token )
      return k;
  } // for
  return T_NONE;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
