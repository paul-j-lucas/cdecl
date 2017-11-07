/*
**      cdecl -- C gibberish translator
**      src/color.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "config.h"                     /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

#define SGR_BG_BLACK        "40"        /**< Background black.   */
#define SGR_BG_RED          "41"        /**< Background red.     */
#define SGR_BG_GREEN        "42"        /**< Background green.   */
#define SGR_BG_YELLOW       "43"        /**< Background yellow.  */
#define SGR_BG_BLUE         "44"        /**< Background blue.    */
#define SGR_BG_MAGENTA      "45"        /**< Background magenta. */
#define SGR_BG_CYAN         "46"        /**< Background cyan.    */
#define SGR_BG_WHITE        "47"        /**< Background white.   */

#define SGR_FG_BLACK        "30"        /**< Foreground black.   */
#define SGR_FG_RED          "31"        /**< Foreground red.     */
#define SGR_FG_GREEN        "32"        /**< Foreground green.   */
#define SGR_FG_YELLOW       "33"        /**< Foreground yellow.  */
#define SGR_FG_BLUE         "34"        /**< Foreground blue.    */
#define SGR_FG_MAGENTA      "35"        /**< Foreground magenta. */
#define SGR_FG_CYAN         "36"        /**< Foreground cyan.    */
#define SGR_FG_WHITE        "37"        /**< Foreground white.   */

#define SGR_BOLD            "1"         /**< Bold. */
#define SGR_CAP_SEP         ":"         /**< Capability separator. */
#define SGR_SEP             ";"         /**< Attribute/RGB separator. */

#define SGR_START           "\33[%sm"   /**< Start color sequence. */
#define SGR_END             "\33[m"     /**< End color sequence. */
#define SGR_EL              "\33[K"     /**< Erase in Line (EL) sequence */

///< @cond IGNORE

#define COLOR_WHEN_DEFAULT  COLOR_NOT_FILE
#define COLORS_DEFAULT                                        \
  "caret="        SGR_FG_GREEN  SGR_SEP SGR_BOLD  SGR_CAP_SEP \
  "error="        SGR_FG_RED    SGR_SEP SGR_BOLD  SGR_CAP_SEP \
  "HELP-keyword="                       SGR_BOLD  SGR_CAP_SEP \
  "HELP-nonterm=" SGR_FG_CYAN                     SGR_CAP_SEP \
  "HELP-punct="   SGR_FG_BLACK  SGR_SEP SGR_BOLD  SGR_CAP_SEP \
  "HELP-title="   SGR_FG_BLUE   SGR_SEP SGR_BOLD  SGR_CAP_SEP \
  "locus="                              SGR_BOLD  SGR_CAP_SEP \
  "PROMPT="       SGR_FG_GREEN                    SGR_CAP_SEP \
  "warning="      SGR_FG_YELLOW SGR_SEP SGR_BOLD  SGR_CAP_SEP

///< @endcond

/**
 * Starts printing in the predefined \a COLOR.
 *
 * @param STREAM The `FILE` to use.
 * @param COLOR The predefined color.
 * @hideinitializer
 */
#define SGR_START_COLOR(STREAM,COLOR) BLOCK(  \
  if ( colorize && (sgr_ ## COLOR) != NULL )  \
    FPRINTF( (STREAM), SGR_START SGR_EL, (sgr_ ## COLOR) ); )

/**
 * Writes the bytes to \a STRING that, when printed to a terminal, will start
 * printing in \a COLOR.
 *
 * @param STRING The string to write to.
 * @param COLOR The predefined color.
 * @hideinitializer
 */
#define SGR_SSTART_COLOR(STRING,COLOR) BLOCK( \
  if ( colorize && (sgr_ ## COLOR) != NULL )  \
    sprintf( (STRING), SGR_START SGR_EL, (sgr_ ## COLOR) ); )

/**
 * Ends printing in color.
 *
 * @param STREAM The `FILE` to use.
 * @hideinitializer
 */
#define SGR_END_COLOR(STREAM) \
  BLOCK( if ( colorize ) FPUTS( SGR_END SGR_EL, (STREAM) ); )

/**
 * Writes the bytes to \a STRING that, when printed to a terminal, will end
 * printing in color.
 *
 * @param STRING The string to write to.
 * @hideinitializer
 */
#define SGR_SEND_COLOR(STRING) \
  BLOCK( if ( colorize ) strcpy( (STRING), SGR_END SGR_EL ); )

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

// extern variables
extern bool         colorize;           ///< Colorize diagnostics?
extern char const  *sgr_caret;          ///< Color of the caret `^`.
extern char const  *sgr_error;          ///< Color of `error`.
extern char const  *sgr_help_keyword;   ///< Color of cdecl keyword.
extern char const  *sgr_help_nonterm;   ///< Color of grammar nonterminal.
extern char const  *sgr_help_punct;     ///< Color of punctuation.
extern char const  *sgr_help_title;     ///< Color of help title.
extern char const  *sgr_locus;          ///< Color of error location.
extern char const  *sgr_prompt;         ///< Color of cdecl prompt.
extern char const  *sgr_warning;        ///< Color of `warning`.

///////////////////////////////////////////////////////////////////////////////

/**
 * Parses and sets the sequence of gcc color capabilities.
 *
 * @param capabilities The gcc capabilities to parse.
 * @return Returns `true` only if at least one capability was parsed
 * successfully.
 */
bool colors_parse( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param when The `color_when` value.
 * @return Returns `true` only if we should do color.
 */
bool should_colorize( color_when_t when );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_color_H */
/* vim:set et sw=2 ts=2: */
