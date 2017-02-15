/*
**    cdecl -- C gibberish translator
**    src/lang.h
*/

#ifndef cdecl_lang_H
#define cdecl_lang_H

////////// literals ///////////////////////////////////////////////////////////

// english
extern char const L_ARRAY[];
extern char const L_AS[];
extern char const L_CAST[];
extern char const L_DECLARE[];
extern char const L_EXIT[];
extern char const L_EXPLAIN[];
extern char const L_FUNC[];             // synonym for "function"
extern char const L_FUNCTION[];
extern char const L_HELP[];
extern char const L_INTO[];
extern char const L_MEMBER[];
extern char const L_OF[];
extern char const L_POINTER[];
extern char const L_PTR[];              // synonym for "pointer"
extern char const L_Q[];                // synonym for "quit"
extern char const L_QUIT[];
extern char const L_REF[];              // synonym for "reference"
extern char const L_REFERENCE[];
extern char const L_RET[];              // synonym for "returning"
extern char const L_RETURNING[];
extern char const L_SET[];
extern char const L_TO[];
extern char const L_VECTOR[];           // synonym for "array"

// K&R
extern char const L_AUTO[];
extern char const L_CHAR[];
extern char const L_CHARACTER[];        // synonym for "char"
extern char const L_DOUBLE[];
extern char const L_EXTERN[];
extern char const L_FLOAT[];
extern char const L_INT[];
extern char const L_INTEGER[];          // synonym for "int"
extern char const L_LONG[];
extern char const L_REGISTER[];
extern char const L_SHORT[];
extern char const L_STATIC[];
extern char const L_STRUCT[];
extern char const L_STRUCTURE[];        // synonym for "struct"
extern char const L_UNION[];
extern char const L_UNSIGNED[];

// C89
extern char const L_CONST[];
extern char const L_CONSTANT[];         // synonym for "const"
extern char const L_ENUM[];
extern char const L_ENUMERATION[];      // synonym for "enum"
extern char const L_NOALIAS[];
extern char const L_SIGNED[];
extern char const L_VOID[];
extern char const L_VOLATILE[];

// C99
extern char const L__BOOL[];
extern char const L_BOOL[];
extern char const L__COMPLEX[];
extern char const L_COMPLEX[];
extern char const L_RESTRICT[];
extern char const L_WCHAR_T[];

// C11
extern char const L__NORETURN[];
extern char const L_NORETURN[];         // synonym for "_Noreturn"
extern char const L__THREAD_LOCAL[];
extern char const L_THREAD_LOCAL[];     // synonym for "_Thread_local"

// C++
extern char const L_CLASS[];

// C11 & C++11
extern char const L_CHAR16_T[];
extern char const L_CHAR32_T[];

// miscellaneous
extern char const L___BLOCK[];          // Apple extension
extern char const L_BLOCK[];            // synonym for "__block"

///////////////////////////////////////////////////////////////////////////////

// languages supported
#define LANG_NONE   0x0000
#define LANG_C_KNR  0x0001
#define LANG_C_89   0x0002
#define LANG_C_95   0x0004
#define LANG_C_99   0x0008
#define LANG_C_11   0x0010
#define LANG_CPP    0x0020
#define LANG_CPP_11 0x0040

/**
 * Bitmask for combination of languages.
 */
typedef unsigned lang_t;

/**
 * C/C++ language keyword info.
 */
struct c_keyword {
  char const *literal;                  // C string literal of the keyword
  int         ytoken;                   // yacc token number
  lang_t      lang_not_ok;              // which language(s) it's not OK in
};
typedef struct c_keyword c_keyword_t;

/**
 * Array of all C/C++ keywords (relevant for declarations).
 */
extern c_keyword_t const C_KEYWORDS[];

////////// types //////////////////////////////////////////////////////////////

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the printable name of the given language.
 *
 * @param lang The language to get the name of.
 * @return Returns said name.
 */
char const* lang_name( lang_t lang );

///////////////////////////////////////////////////////////////////////////////
#endif /* cdecl_lang_H */
/* vim:set et sw=2 ts=2: */
