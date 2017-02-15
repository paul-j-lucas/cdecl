/*
**    cdecl -- C gibberish translator
**    src/lang.c
*/

// local
#include "lang.h"
#include "cdgram.h"
#include "util.h"

// system
#include <stdlib.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// english literals
char const L_ARRAY[]          = "array";
char const L_AS[]             = "as";
char const L_CAST[]           = "cast";
char const L_DECLARE[]        = "declare";
char const L_EXIT[]           = "exit";
char const L_EXPLAIN[]        = "explain";
char const L_FUNC[]           = "func";
char const L_FUNCTION[]       = "function";
char const L_HELP[]           = "help";
char const L_INTO[]           = "into";
char const L_MEMBER[]         = "member";
char const L_OF[]             = "of";
char const L_POINTER[]        = "pointer";
char const L_PTR[]            = "ptr";
char const L_Q[]              = "q";
char const L_QUIT[]           = "quit";
char const L_REF[]            = "ref";
char const L_REFERENCE[]      = "reference";
char const L_RET[]            = "ret";
char const L_RETURNING[]      = "returning";
char const L_SET[]            = "set";
char const L_TO[]             = "to";
char const L_VECTOR[]         = "vector";

// K&R
char const L_AUTO[]           = "auto";
char const L_CHAR[]           = "char";
char const L_CHARACTER[]      = "character";
char const L_DOUBLE[]         = "double";
char const L_EXTERN[]         = "extern";
char const L_FLOAT[]          = "float";
char const L_INT[]            = "int";
char const L_INTEGER[]        = "integer";
char const L_LONG[]           = "long";
char const L_REGISTER[]       = "register";
char const L_SHORT[]          = "short";
char const L_STATIC[]         = "static";
char const L_STRUCT[]         = "struct";
char const L_STRUCTURE[]      = "structure";
char const L_UNION[]          = "union";
char const L_UNSIGNED[]       = "unsigned";

// C89
char const L_CONST[]          = "const";
char const L_CONSTANT[]       = "constant";
char const L_ENUM[]           = "enum";
char const L_ENUMERATION[]    = "enumeration";
char const L_NOALIAS[]        = "noalias";
char const L_SIGNED[]         = "signed";
char const L_VOID[]           = "void";
char const L_VOLATILE[]       = "volatile";

// C99
char const L__BOOL[]          = "_Bool";
char const L_BOOL[]           = "bool";
char const L__COMPLEX[]       = "_Complex";
char const L_COMPLEX[]        = "complex";
char const L_RESTRICT[]       = "restrict";
char const L_WCHAR_T[]        = "wchar_t";

// C11
char const L__NORETURN[]      = "_Noreturn";
char const L_NORETURN[]       = "Noreturn";
char const L__THREAD_LOCAL[]  = "_Thread_local";
char const L_THREAD_LOCAL[]   = "Thread_local";

// C++
char const L_CLASS[]          = "class";

// C11 & C++11
char const L_CHAR16_T[]       = "char16_t";
char const L_CHAR32_T[]       = "char32_t";

// Miscellaneous
char const L___BLOCK[]        = "__block";
char const L_BLOCK[]          = "block";

////////// keywords ///////////////////////////////////////////////////////////

#define NOT_BEFORE(LANG)          (LANG - 1)

c_keyword_t const C_KEYWORDS[] = {
  // K&R C
  { L_AUTO,           T_AUTO,         0 },
  { L_CHAR,           T_CHAR,         0 },
  { L_DOUBLE,         T_DOUBLE,       0 },
  { L_EXTERN,         T_EXTERN,       0 },
  { L_FLOAT,          T_FLOAT,        0 },
  { L_INT,            T_INT,          0 },
  { L_LONG,           T_LONG,         0 },
  { L_REGISTER,       T_REGISTER,     0 },
  { L_SHORT,          T_SHORT,        0 },
  { L_STATIC,         T_STATIC,       0 },
  { L_STRUCT,         T_STRUCT,       0 },
  { L_UNION,          T_UNION,        0 },
  { L_UNSIGNED,       T_UNSIGNED,     0 },

  // C89
  { L_CONST,          T_CONST,        NOT_BEFORE( LANG_C_89 ) },
  { L_ENUM,           T_ENUM,         NOT_BEFORE( LANG_C_89 ) },
  { L_NOALIAS,        T_NOALIAS,      NOT_BEFORE( LANG_C_89 ) },
  { L_SIGNED,         T_SIGNED,       NOT_BEFORE( LANG_C_89 ) },
  { L_VOID,           T_VOID,         NOT_BEFORE( LANG_C_89 ) },
  { L_VOLATILE,       T_VOLATILE,     NOT_BEFORE( LANG_C_89 ) },

  // C99
  { L_BOOL,           T_BOOL,         NOT_BEFORE( LANG_C_99 ) },
  { L_COMPLEX,        T_COMPLEX,      NOT_BEFORE( LANG_C_99 ) },
  { L_RESTRICT,       T_RESTRICT,     NOT_BEFORE( LANG_C_99 ) },
  { L_WCHAR_T,        T_WCHAR_T,      NOT_BEFORE( LANG_C_99 ) },

  // C11
  { L_NORETURN,       T_NORETURN,     NOT_BEFORE( LANG_C_11 ) },
  { L__NORETURN,      T_NORETURN,     NOT_BEFORE( LANG_C_11 ) },
  { L_THREAD_LOCAL,   T_THREAD_LOCAL, NOT_BEFORE( LANG_C_11 ) },
  { L__THREAD_LOCAL,  T_THREAD_LOCAL, NOT_BEFORE( LANG_C_11 ) },

  // C++
  { L_CLASS,          T_CLASS,        NOT_BEFORE( LANG_CPP )  },

  // C11 & C++11
  { L_CHAR16_T,       T_CHAR16_T,     NOT_BEFORE( LANG_CPP_11 ) },
  { L_CHAR32_T,       T_CHAR32_T,     NOT_BEFORE( LANG_CPP_11 ) },

  // Apple extension
  { L___BLOCK,        T_BLOCK,        0 },
  { L_BLOCK,          T_BLOCK,        0 },

  { NULL,             0,              0 }
};

////////// extern functions ///////////////////////////////////////////////////

char const* lang_name( lang_t lang ) {
  switch ( lang ) {
    case LANG_NONE  : return "";
    case LANG_C_KNR : return "K&R C";
    case LANG_C_89  : return "C89";
    case LANG_C_95  : return "C95";
    case LANG_C_99  : return "C99";
    case LANG_C_11  : return "C11";
    case LANG_CPP   : return "C++";
    case LANG_CPP_11: return "C++11";
    default:
      INTERNAL_ERR( "\"%d\": unexpected value for lang\n", (int)lang );
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
