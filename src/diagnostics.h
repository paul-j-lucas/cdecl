/*
**      cdecl -- C gibberish translator
**      src/diagnostics.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_diagnostics_H
#define cdecl_diagnostics_H

/**
 * @file
 * Declares functions for printing error and warning messages.
 */

// local
#include "config.h"                     /* must go first */
#include "common.h"                     /* for YYLTYPE */

// standard
#include <stddef.h>                     /* for size_t */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints a '^' (in color, if possible and requested) under the offending
 * token.
 *
 * @param col The zero-based column to print the caret at.
 */
void print_caret( size_t col );

/**
 * Prints an error message to standard error.
 *
 * @param loc The location of the error.
 * @param format The \c printf() style format string.
 */
void print_error( YYLTYPE const *loc, char const *format, ... );

/**
 * Prints a hint message to standard error.
 *
 * @param format The \c printf() style format string.
 */
void print_hint( char const *format, ... );

/**
 * Prints a warning message to standard error.
 *
 * @param loc The location of the warning.
 * @param format The \c printf() style format string.
 */
void print_warning( YYLTYPE const *loc, char const *format, ... );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_diagnostics_H */
/* vim:set et sw=2 ts=2: */
