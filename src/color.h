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

#define SGR_BG_BLACK        "40"        /**< Background black.            */
#define SGR_BG_RED          "41"        /**< Background red.              */
#define SGR_BG_GREEN        "42"        /**< Background green.            */
#define SGR_BG_YELLOW       "43"        /**< Background yellow.           */
#define SGR_BG_BLUE         "44"        /**< Background blue.             */
#define SGR_BG_MAGENTA      "45"        /**< Background magenta.          */
#define SGR_BG_CYAN         "46"        /**< Background cyan.             */
#define SGR_BG_WHITE        "47"        /**< Background white.            */

#define SGR_FG_BLACK        "30"        /**< Foreground black.            */
#define SGR_FG_RED          "31"        /**< Foreground red.              */
#define SGR_FG_GREEN        "32"        /**< Foreground green.            */
#define SGR_FG_YELLOW       "33"        /**< Foreground yellow.           */
#define SGR_FG_BLUE         "34"        /**< Foreground blue.             */
#define SGR_FG_MAGENTA      "35"        /**< Foreground magenta.          */
#define SGR_FG_CYAN         "36"        /**< Foreground cyan.             */
#define SGR_FG_WHITE        "37"        /**< Foreground white.            */

#define SGR_BOLD            "1"         /**< Bold.                        */
#define SGR_CAP_SEP         ":"         /**< Capability separator.        */
#define SGR_SEP             ";"         /**< Attribute/RGB separator.     */

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

// extern variables
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
  if ( sgr_color != NULL )
    FPUTS( SGR_END SGR_EL, file );
}

/**
 * Initializes when to print in color and the colors.
 *
 * @note This function must be called exactly once.
 */
void color_init( void );

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
  if ( sgr_color != NULL )
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
  if ( sgr_color != NULL )
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
  if ( sgr_color != NULL )
    strbuf_printf( sbuf, SGR_START SGR_EL, sgr_color );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_color_H */
/* vim:set et sw=2 ts=2: */
