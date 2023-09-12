/*
**      cdecl -- C gibberish translator
**      src/options.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas, et al.
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
#include "strbuf.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>

/// @endcond

/**
 * @addtogroup cdecl-options-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries for each option.

// extern option variables
bool                opt_alt_tokens;
#ifdef ENABLE_CDECL_DEBUG
bool                opt_cdecl_debug;
#endif /* ENABLE_CDECL_DEBUG */
color_when_t        opt_color_when = COLOR_NOT_FILE;
char const         *opt_conf_path;
bool                opt_east_const;
bool                opt_echo_commands;
bool                opt_english_types = true;
bool                opt_explain;
c_tid_t             opt_explicit_ecsu_btids = TB_struct | TB_union;
c_graph_t           opt_graph;
c_lang_id_t         opt_lang;
bool                opt_prompt = true;
bool                opt_read_conf = true;
bool                opt_semicolon = true;
bool                opt_trailing_ret;
bool                opt_typedefs = true;
bool                opt_using = true;
c_ast_kind_t        opt_west_pointer_kinds = K_ANY_FUNCTION_RETURN;

/// @endcond

/**
 * The integer type(s) that `int` shall be printed explicitly for in C/C++
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
 * @remarks Due to non-trivial representation and special cases, this option's
 * variable is `static` and accessible only via `*_explicit_int()` functions.
 *
 * @sa any_explicit_int()
 * @sa is_explicit_int()
 * @sa parse_explicit_int()
 */
static c_tid_t      opt_explicit_int_btids[] = { TB_NONE, TB_NONE };

////////// extern functions ///////////////////////////////////////////////////

bool any_explicit_int( void ) {
  return  opt_explicit_int_btids[0] != TB_NONE ||
          opt_explicit_int_btids[1] != TB_NONE;
}

char const* explicit_ecsu_str( void ) {
  static char buf[5];
  char *s = buf;

  if ( (opt_explicit_ecsu_btids & TB_enum) != TB_NONE )
    *s++ = 'e';
  if ( (opt_explicit_ecsu_btids & TB_class) != TB_NONE )
    *s++ = 'c';
  if ( (opt_explicit_ecsu_btids & TB_struct) != TB_NONE )
    *s++ = 's';
  if ( (opt_explicit_ecsu_btids & TB_union) != TB_NONE )
    *s++ = 'u';
  *s = '\0';

  return buf;
}

char const* explicit_int_str( void ) {
  bool const is_explicit_s   = is_explicit_int( TB_short );
  bool const is_explicit_i   = is_explicit_int( TB_int );
  bool const is_explicit_l   = is_explicit_int( TB_long );
  bool const is_explicit_ll  = is_explicit_int( TB_long_long );

  bool const is_explicit_us  = is_explicit_int( TB_unsigned | TB_short );
  bool const is_explicit_ui  = is_explicit_int( TB_unsigned | TB_int );
  bool const is_explicit_ul  = is_explicit_int( TB_unsigned | TB_long );
  bool const is_explicit_ull = is_explicit_int( TB_unsigned | TB_long_long );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );

  if ( is_explicit_s & is_explicit_i && is_explicit_l && is_explicit_ll ) {
    strbuf_putc( &sbuf, 'i' );
  }
  else {
    if ( is_explicit_s  ) strbuf_putc ( &sbuf, 's'     );
    if ( is_explicit_i  ) strbuf_putc ( &sbuf, 'i'     );
    if ( is_explicit_l  ) strbuf_putc ( &sbuf, 'l'     );
    if ( is_explicit_ll ) strbuf_putsn( &sbuf, "ll", 2 );
  }

  if ( is_explicit_us & is_explicit_ui && is_explicit_ul && is_explicit_ull ) {
    strbuf_putc( &sbuf, 'u' );
  }
  else {
    if ( is_explicit_us  ) strbuf_putsn( &sbuf, "us",  2 );
    if ( is_explicit_ui  ) strbuf_putsn( &sbuf, "ui",  2 );
    if ( is_explicit_ul  ) strbuf_putsn( &sbuf, "ul",  2 );
    if ( is_explicit_ull ) strbuf_putsn( &sbuf, "ull", 3 );
  }

  return empty_if_null( sbuf.str );
}

bool is_explicit_int( c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );

  if ( btids == TB_unsigned ) {
    //
    // Special case: "unsigned" by itself means "unsigned int."
    //
    btids |= TB_int;
  }
  else if ( c_tid_is_any( btids, TB_long_long ) ) {
    //
    // Special case: for long long, its type is always combined with TB_long,
    // i.e., two bits are set.  Therefore, to check for explicit int for long
    // long, we first have to turn off the TB_long bit.
    //
    btids &= c_tid_compl( TB_long );
  }
  bool const is_unsigned = c_tid_is_any( btids, TB_unsigned );
  btids &= c_tid_compl( TB_unsigned );
  return c_tid_is_any( btids, opt_explicit_int_btids[ is_unsigned ] );
}

bool parse_explicit_ecsu( char const *ecsu_format ) {
  assert( ecsu_format != NULL );

  if ( strcmp( ecsu_format, "*" ) == 0 )
    ecsu_format = "ecsu";
  else if ( strcmp( ecsu_format, "-" ) == 0 )
    ecsu_format = "";

  c_tid_t btids = TB_NONE;

  for ( char const *s = ecsu_format; *s != '\0'; ++s ) {
    switch ( tolower( *s ) ) {
      case 'e':
        btids |= TB_enum;
        break;
      case 'c':
        btids |= TB_class;
        break;
      case 's':
        btids |= TB_struct;
        break;
      case 'u':
        btids |= TB_union;
        break;
      default:
        return false;
    } // switch
  } // for

  opt_explicit_ecsu_btids = btids;
  return true;
}

bool parse_explicit_int( char const *ei_format ) {
  assert( ei_format != NULL );

  if ( strcmp( ei_format, "*" ) == 0 )
    ei_format = "iu";
  else if ( strcmp( ei_format, "-" ) == 0 )
    ei_format = "";

  c_tid_t btids = TB_NONE;
  c_tid_t tmp_ei_btids[] = { TB_NONE, TB_NONE };

  for ( char const *s = ei_format; *s != '\0'; ++s ) {
    switch ( tolower( *s ) ) {
      case 'i':
        btids |= TB_int;
        if ( (btids & TB_unsigned) == TB_NONE ) {
          // If only 'i' is specified, it means all signed integer types
          // shall be explicit.
          btids |= TB_short | TB_long | TB_long_long;
        }
        break;
      case 'l':
        if ( s[1] == 'l' || s[1] == 'L' ) {
          btids |= TB_long_long;
          ++s;
        } else {
          btids |= TB_long;
        }
        break;
      case 's':
        btids |= TB_short;
        break;
      case 'u':
        btids |= TB_unsigned;
        if ( s[1] == '\0' || s[1] == ',' ) {
          // If only 'u' is specified, it means all unsigned integer types
          // shall be explicit.
          btids |= TB_short | TB_int | TB_long | TB_long_long;
          break;
        }
        continue;
      case ',':
        break;
      default:
        return false;
    } // switch

    bool const is_unsigned = c_tid_is_any( btids, TB_unsigned );
    tmp_ei_btids[ is_unsigned ] |= btids & c_tid_compl( TB_unsigned );
    btids = TB_NONE;
  } // for

  opt_explicit_int_btids[0] = tmp_ei_btids[0];
  opt_explicit_int_btids[1] = tmp_ei_btids[1];
  return true;
}

bool parse_west_pointer( char const *wp_format ) {
  assert( wp_format != NULL );

  if ( strcmp( wp_format, "*" ) == 0 )
    wp_format = "rt";
  else if ( strcmp( wp_format, "-" ) == 0 )
    wp_format = "";

  unsigned kinds = 0;

  for ( char const *s = wp_format; *s != '\0'; ++s ) {
    switch ( tolower( *s ) ) {
      case 'b':
        kinds |= K_APPLE_BLOCK;
        break;
      case 'f':
        kinds |= K_FUNCTION;
        break;
      case 'l':
        kinds |= K_UDEF_LIT;
        break;
      case 'o':
        kinds |= K_OPERATOR;
        break;
      case 'r':
        kinds |= K_ANY_FUNCTION_RETURN;
        break;
      case 't':
        kinds |= K_ANY_NON_PTR_REF_OBJECT;
        break;
      default:
        return false;
    } // switch
  } // for

  opt_west_pointer_kinds = kinds;
  return true;
}

char const* west_pointer_str( void ) {
  static char buf[6];
  char *s = buf;

  if ( (opt_west_pointer_kinds & K_APPLE_BLOCK) != 0 )
    *s++ = 'b';
  if ( (opt_west_pointer_kinds & K_FUNCTION) != 0 )
    *s++ = 'f';
  if ( (opt_west_pointer_kinds & K_UDEF_LIT) != 0 )
    *s++ = 'l';
  if ( (opt_west_pointer_kinds & K_OPERATOR) != 0 )
    *s++ = 'o';
  if ( (opt_west_pointer_kinds & K_ANY_NON_PTR_REF_OBJECT) != 0 )
    *s++ = 't';

  return buf;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
