/*
**      cdecl -- C gibberish translator
**      src/color.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
 * printing to an ANSI terminal in color using Select Graphics Rendition (SGR)
 * codes.
 */

// local
#include "pjl_config.h"                 /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

/**
 * @defgroup printing-color-group Printing Color
 * Declares constants, macros, types, global variables, and functions for
 * printing to an ANSI terminal in color using Select Graphics Rendition (SGR)
 * codes.
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup sgr-group Select Graphics Rendition (SGR) Macros
 * Macros for Select Graphics Rendition (SGR) colors and other terminal
 * cababilities.
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
 * Starts printing in the predefined \a COLOR.
 *
 * @param STREAM The `FILE` to use.
 * @param COLOR The predefined color without the `sgr_` prefix.
 *
 * @sa #SGR_END_COLOR
 * @sa #SGR_STRBUF_START_COLOR
 */
#define SGR_START_COLOR(STREAM,COLOR) BLOCK(  \
  if ( colorize && (sgr_ ## COLOR) != NULL )  \
    FPRINTF( (STREAM), SGR_START SGR_EL, (sgr_ ## COLOR) ); )

/**
 * Ends printing in color.
 *
 * @param STREAM The `FILE` to use.
 *
 * @sa #SGR_START_COLOR
 */
#define SGR_END_COLOR(STREAM) \
  BLOCK( if ( colorize ) FPUTS( SGR_END SGR_EL, (STREAM) ); )

/**
 * Writes the bytes to \a SBUF that, when printed to a terminal, will start
 * printing in \a COLOR.
 *
 * @param SBUF The string to write to.
 * @param COLOR The predefined color without the `sgr_` prefix.
 *
 * @sa #SGR_STRBUF_END_COLOR
 * @sa #SGR_START_COLOR
 */
#define SGR_STRBUF_START_COLOR(SBUF,COLOR) BLOCK( \
  if ( colorize && (sgr_ ## COLOR) != NULL )  \
    strbuf_printf( (SBUF), SGR_START SGR_EL, (sgr_ ## COLOR) ); )

/**
 * Writes the bytes to \a SBUF that, when printed to a terminal, will end
 * printing in color.
 *
 * @param SBUF The string to write to.
 *
 * @sa #SGR_STRBUF_START_COLOR
 */
#define SGR_STRBUF_END_COLOR(SBUF) \
  BLOCK( if ( colorize ) strbuf_puts( (SBUF), SGR_END SGR_EL ); )

/**
 * When to colorize output.
 */
enum color_when {
  COLOR_NEVER,                          ///< Never colorize.
  COLOR_ISATTY,                         ///< Colorize only if `isatty`(3).
  COLOR_NOT_FILE,                       ///< Colorize only if `!ISREG` stdout.
  COLOR_ALWAYS                          ///< Always colorize.
};
typedef enum color_when color_when_t;

// extern constants
extern char const   COLORS_DEFAULT[];   ///< Default colors.

// extern variables
extern bool         colorize;           ///< Colorize errors & warnings?
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
 * Parses and sets the sequence of gcc color capabilities.
 *
 * @param capabilities The gcc capabilities to parse.
 * @return Returns `true` only if at least one capability was parsed
 * successfully.
 */
PJL_WARN_UNUSED_RESULT
bool colors_parse( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param when The \ref color_when value.
 * @return Returns `true` only if we should do color.
 */
PJL_WARN_UNUSED_RESULT
bool should_colorize( color_when_t when );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_color_H */
/* vim:set et sw=2 ts=2: */
