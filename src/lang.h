/*
**      cdecl -- C gibberish translator
**      src/lang.h
*/

#ifndef cdecl_lang_H
#define cdecl_lang_H

///////////////////////////////////////////////////////////////////////////////

// languages supported
#define LANG_NONE     0
#define LANG_ALL      ~LANG_NONE

#define LANG_C_KNR    (1 << 0)
#define LANG_C_89     (1 << 1)
#define LANG_C_95     (1 << 2)
#define LANG_C_99     (1 << 3)
#define LANG_C_11     (1 << 4)
#define LANG_C_MAX    LANG_C_11

#define LANG_CPP_MIN  LANG_CPP_03
#define LANG_CPP_03   (1 << 5)
#define LANG_CPP_11   (1 << 6)
#define LANG_CPP_MAX  LANG_CPP_11
#define LANG_CPP_ANY  (LANG_CPP_03 | LANG_CPP_11)

/**
 * Bitmask for combination of languages.
 */
typedef unsigned lang_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the language corresponding to the given string (case insensitive).
 *
 * @param s The null-terminated string to get the language for.
 * @return Returns said language or \c LANG_NONE if \a s doesn't correspond to
 * any supported language.
 */
lang_t lang_find( char const *s );

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
