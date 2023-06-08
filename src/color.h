/*
**      cdecl -- C gibberish translator
**      src/color.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef cdecl_color_H
#define cdecl_color_H

/**
 * @file
 * Declares constants, macros, types, global variables, and functions for
 * printing to an ANSI terminal in color using [Select Graphics Rendition
 * (SGR)](https://en.wikipedia.org/wiki/ANSI_escape_code#SGR) codes.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>

_GL_INLINE_HEADER_BEGIN
#ifndef COLOR_H_INLINE
# define COLOR_H_INLINE _GL_INLINE
#endif /* COLOR_H_INLINE */

/// @endcond

/**
 * @defgroup printing-color-group Printing Color
 * Constants, macros, types, global variables, and functions for printing to an
 * ANSI terminal in color using [Select Graphics Rendition
 * (SGR)](https://en.wikipedia.org/wiki/ANSI_escape_code#SGR) codes.
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup sgr-group Select Graphics Rendition (SGR) Macros
 * Macros for [Select Graphics Rendition
 * (SGR)](https://en.wikipedia.org/wiki/ANSI_escape_code#SGR) colors and other
 * terminal cababilities.
 * @ingroup printing-color-group
 * @{
 */

#define SGR_START           "\33[%sm"   /**< Start color sequence.        */
#define SGR_END             "\33[m"     /**< End color sequence.          */
#define SGR_EL              "\33[K"     /**< Erase in Line (EL) sequence. */

/** @} */

/**
 * @addtogroup printing-color-group
 * @{
 */

/** When to colorize default. */
#define COLOR_WHEN_DEFAULT  COLOR_NOT_FILE

/**
 * When to colorize output.
 */
enum color_when {
  COLOR_NEVER,                          ///< Never colorize.
  COLOR_ISATTY,                         ///< Colorize only if **isatty**(3).
  COLOR_NOT_FILE,                       ///< Colorize only if `!ISREG` stdout.
  COLOR_ALWAYS                          ///< Always colorize.
};
typedef enum color_when color_when_t;

// extern constants
extern char const   COLORS_DEFAULT[];   ///< Default colors.

// extern variables
extern bool         cdecl_colorize;     ///< Colorize errors & warnings?
extern char const  *sgr_caret;          ///< Color of the caret `^`.
extern char const  *sgr_error;          ///< Color of `error`.
extern char const  *sgr_help_keyword;   ///< Color of cdecl keyword.
extern char const  *sgr_help_nonterm;   ///< Color of grammar nonterminal.
extern char const  *sgr_help_punct;     ///< Color of punctuation.
extern char const  *sgr_help_title;     ///< Color of help title.
extern char const  *sgr_locus;          ///< Color of error location.
extern char const  *sgr_prompt;         ///< Color of the cdecl prompt.
extern char const  *sgr_warning;        ///< Color of `warning`.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Ends printing in \a sgr_color.
 *
 * @param file The `FILE` to print to.
 * @param sgr_color The predefined color.  If NULL, does nothing.  This _must_
 * be the same value that was passed to color_start().
 *
 * @sa color_start()
 */
COLOR_H_INLINE
void color_end( FILE *file, char const *sgr_color ) {
  if ( cdecl_colorize && sgr_color != NULL )
    FPUTS( SGR_END SGR_EL, file );
}

/**
 * Starts printing in the predefined \a sgr_color.
 *
 * @param file The `FILE` to print to.
 * @param sgr_color The predefined color.  If NULL, does nothing.
 *
 * @sa color_end()
 * @sa color_strbuf_start()
 */
COLOR_H_INLINE
void color_start( FILE *file, char const *sgr_color ) {
  if ( cdecl_colorize && sgr_color != NULL )
    FPRINTF( file, SGR_START SGR_EL, sgr_color );
}

/**
 * Appends the bytes to \a sbuf that, when printed to a terminal, will end
 * printing in \a sgr_color.
 *
 * @param sbuf The string buffer to write to.
 * @param sgr_color The predefined color.  If NULL, does nothing.  This _must_
 * be the same value that was passed to color_strbuf_start().
 *
 * @sa color_strbuf_start()
 */
COLOR_H_INLINE
void color_strbuf_end( strbuf_t *sbuf, char const *sgr_color ) {
  if ( cdecl_colorize && sgr_color != NULL )
    strbuf_puts( sbuf, SGR_END SGR_EL );
}

/**
 * Appends the bytes to \a sbuf that, when printed to a terminal, will start
 * printing in \a sgr_color.
 *
 * @param sbuf The string buffer to write to.
 * @param sgr_color The predefined color.  If NULL, does nothing.
 *
 * @sa color_start()
 * @sa color_strbuf_end()
 */
COLOR_H_INLINE
void color_strbuf_start( strbuf_t *sbuf, char const *sgr_color ) {
  if ( cdecl_colorize && sgr_color != NULL )
    strbuf_printf( sbuf, SGR_START SGR_EL, sgr_color );
}

/**
 * Parses and sets the sequence of gcc color capabilities.
 *
 * @param capabilities The gcc capabilities to parse.  It's of the form:
 *  <table border="0">
 *    <tr><td>&nbsp;</td><td>&nbsp;</td></tr>
 *    <tr>
 *      <td><i>capapilities</i></td>
 *      <td>::= <i>capability</i> [<tt>:</tt><i>capability</i>]*</td>
 *    </tr>
 *    <tr>
 *      <td><i>capability</i></td>
 *      <td>::= <i>cap-name</i><tt>=</tt><i>sgr-list</i></td>
 *    </tr>
 *    <tr>
 *      <td><i>cap-name</i></td>
 *      <td>::= [<tt>a-zA-Z-</tt>]+</td>
 *    </tr>
 *    <tr>
 *      <td><i>sgr-list</i></td>
 *      <td>::= <i>sgr</i>[<tt>;</tt><i>sgr</i>]*</td>
 *    </tr>
 *    <tr>
 *      <td><i>sgr</i></td>
 *      <td>::= [<tt>1-9</tt>][<tt>0-9</tt>]*</td>
 *    </tr>
 *    <tr><td>&nbsp;</td><td>&nbsp;</td></tr>
 *  </table>
 * where <i>sgr</i> is a [Select Graphics
 * Rendition](https://en.wikipedia.org/wiki/ANSI_escape_code#SGR) code.  An
 * example \a capabilities is: `caret=42;1:error=41;1:warning=43;1`.
 * @return Returns `true` only if at least one capability was parsed
 * successfully.
 *
 * @warning If this function returns `true`, it must never be called again.
 */
NODISCARD
bool colors_parse( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param when The \ref color_when value.
 * @return Returns `true` only if we should do color.
 */
NODISCARD
bool should_colorize( color_when_t when );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_color_H */
/* vim:set et sw=2 ts=2: */
