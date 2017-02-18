/*
**      cdecl -- C gibberish translator
**      src/lang.h
*/

#ifndef cdecl_lang_H
#define cdecl_lang_H

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
