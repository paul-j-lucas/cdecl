/*
**      cdecl -- C gibberish translator
**      src/common.h
*/

#ifndef cdecl_common_H
#define cdecl_common_H

///////////////////////////////////////////////////////////////////////////////

#define JSON_INDENT               2     /* speces per JSON indent level */

// extern variables
extern char const  *me;                 // program name
extern char const  *prompt;
extern char         prompt_buf[];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints an error message to standard error.
 *
 * @param what What is causing the error.
 * @param hint A hint as to how to correct the error.  May ne null.
 */
void c_error( char const *what, char const *hint );

/**
 * Prints a warning message to standard error.
 *
 * @param what What is causing the warning.
 * @param hint A hint as to how to correct the warning.  May ne null.
 */
void c_warning( char const *what, char const *hint );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_common_H */
/* vim:set et sw=2 ts=2: */
