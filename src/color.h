/*
**      cdecl -- C gibberish translator
**      src/color.h
*/

#ifndef cdecl_color_H
#define cdecl_color_H

// local
#include "config.h"                     /* must go first */

// standard
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

#define COLOR_WHEN_DEFAULT        COLOR_NOT_FILE
#define COLORS_DEFAULT \
  "caret=01;32:" \
  "error=01;31:" \
  "note=01;36:" \
  "warning=01;35:" \
  "range1=32:range2=34:" \
  "locus=01:" \
  "quote=01:" \
  "fixit-insert=32:fixit-delete=31"

/**
 * Starts printing in the given, predefined color.
 *
 * @param STREAM The FILE to use.
 * @param COLOR The predefined color.
 * @hideinitializer
 */
#define SGR_START_COLOR(STREAM,COLOR) BLOCK(            \
  if ( colorize && (sgr_ ## COLOR) )                    \
    FPRINTF( (STREAM), sgr_start, (sgr_ ## COLOR) ); )

/**
 * Ends printing in color.
 *
 * @param STREAM The FILE to use.
 * @hideinitializer
 */
#define SGR_END_COLOR(STREAM) \
  BLOCK( if ( colorize ) FPUTS( sgr_end, (STREAM) ); )

/**
 * When to colorize output.
 */
enum color_when {
  COLOR_NEVER,                          // never colorize
  COLOR_ISATTY,                         // colorize only if isatty(3)
  COLOR_NOT_FILE,                       // colorize only if !ISREG stdout
  COLOR_ALWAYS                          // always colorize
};
typedef enum color_when color_when_t;

// extern variables
extern bool         colorize;           // colorize diagnostics?
extern char const  *sgr_start;          // start color output
extern char const  *sgr_end;            // end color output

extern char const  *sgr_caret;          // caret color
extern char const  *sgr_error;          // error color
extern char const  *sgr_warning;        // warning color

///////////////////////////////////////////////////////////////////////////////

/**
 * Parses and sets the sequence of gcc color capabilities.
 *
 * @param capabilities The gcc capabilities to parse.
 * @return Returns \c true only if at least one capability was parsed
 * successfully.
 */
bool parse_gcc_colors( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param c The color_when value.
 * @return Returns \c true only if we should do color.
 */
bool should_colorize( color_when_t c );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_color_H */
/* vim:set et sw=2 ts=2: */
