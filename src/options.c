/*
**      cdecl -- C gibberish translator
**      src/options.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas, et al.
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

/**
 * @file
 * Defines global variables and functions for **cdecl** options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "options.h"
#include "c_type.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern option variables
bool                opt_alt_tokens;
#ifdef ENABLE_CDECL_DEBUG
bool                opt_cdecl_debug;
#endif /* ENABLE_CDECL_DEBUG */
char const         *opt_conf_path;
bool                opt_east_const;
bool                opt_echo_commands;
bool                opt_english_types = true;
bool                opt_explain;
c_tid_t             opt_explicit_ecsu = TB_STRUCT | TB_UNION;
c_graph_t           opt_graph;
c_lang_id_t         opt_lang;
bool                opt_prompt = true;
bool                opt_read_conf = true;
bool                opt_semicolon = true;
bool                opt_typedefs = true;
bool                opt_using = true;

/**
 * The integer type(s) that `int` shall be print explicitly for in C/C++
 * declarations even when not needed because the type(s) contain at least one
 * integer modifier, e.g., `unsigned`.
 *
 * The elements are:
 *
 *  Idx | Contains type(s) for
 *  ----|---------------------
 *  `0` | signed integers
 *  `1` | unsigned integers
 *
 * @sa any_explicit_int()
 * @sa is_explicit_int()
 * @sa parse_explicit_int()
 */
static c_tid_t      opt_explicit_int[] = { TB_NONE, TB_NONE };

////////// extern functions ///////////////////////////////////////////////////

bool any_explicit_int( void ) {
  return opt_explicit_int[0] != TB_NONE || opt_explicit_int[1] != TB_NONE;
}

bool is_explicit_int( c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );

  if ( btids == TB_UNSIGNED ) {
    //
    // Special case: "unsigned" by itself means "unsigned int."
    //
    btids |= TB_INT;
  }
  else if ( c_tid_is_any( btids, TB_LONG_LONG ) ) {
    //
    // Special case: for long long, its type is always combined with TB_LONG,
    // i.e., two bits are set.  Therefore, to check for explicit int for long
    // long, we first have to turn off the TB_LONG bit.
    //
    btids &= c_tid_compl( TB_LONG );
  }
  bool const is_unsigned = c_tid_is_any( btids, TB_UNSIGNED );
  btids &= c_tid_compl( TB_UNSIGNED );
  return c_tid_is_any( btids, opt_explicit_int[ is_unsigned ] );
}

bool parse_explicit_ecsu( char const *ecsu_format ) {
  assert( ecsu_format != NULL );

  c_tid_t btids = TB_NONE;

  for ( char const *s = ecsu_format; *s != '\0'; ++s ) {
    switch ( tolower( *s ) ) {
      case 'e':
        btids |= TB_ENUM;
        break;
      case 'c':
        btids |= TB_CLASS;
        break;
      case 's':
        btids |= TB_STRUCT;
        break;
      case 'u':
        btids |= TB_UNION;
        break;
      default:
        return false;
    } // switch
  } // for

  opt_explicit_ecsu = btids;
  return true;
}

bool parse_explicit_int( char const *ei_format ) {
  assert( ei_format != NULL );

  c_tid_t tid = opt_explicit_int[0] = opt_explicit_int[1] = TB_NONE;

  for ( char const *s = ei_format; *s != '\0'; ++s ) {
    switch ( *s ) {
      case 'i':
      case 'I':
        if ( (tid & TB_UNSIGNED) == TB_NONE ) {
          // If only 'i' is specified, it means all signed integer types shall
          // be explicit.
          tid |= TB_SHORT | TB_INT | TB_LONG | TB_LONG_LONG;
        } else {
          tid |= TB_INT;
        }
        break;
      case 'l':
      case 'L':
        if ( s[1] == 'l' || s[1] == 'L' ) {
          tid |= TB_LONG_LONG;
          ++s;
        } else {
          tid |= TB_LONG;
        }
        break;
      case 's':
      case 'S':
        tid |= TB_SHORT;
        break;
      case 'u':
      case 'U':
        tid |= TB_UNSIGNED;
        if ( s[1] == '\0' || s[1] == ',' ) {
          // If only 'u' is specified, it means all unsigned integer types
          // shall be explicit.
          tid |= TB_SHORT | TB_INT | TB_LONG | TB_LONG_LONG;
          break;
        }
        continue;
      case ',':
        break;
      default:
        return false;
    } // switch

    bool const is_unsigned = c_tid_is_any( tid, TB_UNSIGNED );
    opt_explicit_int[ is_unsigned ] |= tid & c_tid_compl( TB_UNSIGNED );
    tid = TB_NONE;
  } // for

  return true;
}

void print_explicit_ecsu( FILE *out ) {
  if ( (opt_explicit_ecsu & TB_ENUM) != TB_NONE )
    FPUTC( 'e', out );
  if ( (opt_explicit_ecsu & TB_CLASS) != TB_NONE )
    FPUTC( 'c', out );
  if ( (opt_explicit_ecsu & TB_STRUCT) != TB_NONE )
    FPUTC( 's', out );
  if ( (opt_explicit_ecsu & TB_UNION) != TB_NONE )
    FPUTC( 'u', out );
}

void print_explicit_int( FILE *out ) {
  bool const is_explicit_s   = is_explicit_int( TB_SHORT );
  bool const is_explicit_i   = is_explicit_int( TB_INT );
  bool const is_explicit_l   = is_explicit_int( TB_LONG );
  bool const is_explicit_ll  = is_explicit_int( TB_LONG_LONG );

  bool const is_explicit_us  = is_explicit_int( TB_UNSIGNED | TB_SHORT );
  bool const is_explicit_ui  = is_explicit_int( TB_UNSIGNED | TB_INT );
  bool const is_explicit_ul  = is_explicit_int( TB_UNSIGNED | TB_LONG );
  bool const is_explicit_ull = is_explicit_int( TB_UNSIGNED | TB_LONG_LONG );

  if ( is_explicit_s & is_explicit_i && is_explicit_l && is_explicit_ll ) {
    FPUTC( 'i', out );
  }
  else {
    if ( is_explicit_s   ) FPUTC(  's',  out );
    if ( is_explicit_i   ) FPUTC(  'i',  out );
    if ( is_explicit_l   ) FPUTC(  'l',  out );
    if ( is_explicit_ll  ) FPUTS(  "ll", out );
  }

  if ( is_explicit_us & is_explicit_ui && is_explicit_ul && is_explicit_ull ) {
    FPUTC( 'u', out );
  }
  else {
    if ( is_explicit_us  ) FPUTS( "us",  out );
    if ( is_explicit_ui  ) FPUTS( "ui",  out );
    if ( is_explicit_ul  ) FPUTS( "ul",  out );
    if ( is_explicit_ull ) FPUTS( "ull", out );
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
