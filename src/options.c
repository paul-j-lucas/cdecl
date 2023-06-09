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

// extern option variables
bool                opt_alt_tokens;
#ifdef ENABLE_CDECL_DEBUG
bool                opt_cdecl_debug;
#endif /* ENABLE_CDECL_DEBUG */
color_when_t        opt_color_when = COLOR_WHEN_DEFAULT;
char const         *opt_conf_path;
bool                opt_east_const;
bool                opt_echo_commands;
bool                opt_english_types = true;
bool                opt_explain;
c_tid_t             opt_explicit_ecsu_btids = TB_STRUCT | TB_UNION;
c_graph_t           opt_graph;
c_lang_id_t         opt_lang;
bool                opt_prompt = true;
bool                opt_read_conf = true;
bool                opt_semicolon = true;
bool                opt_trailing_ret;
bool                opt_typedefs = true;
bool                opt_using = true;
c_ast_kind_t        opt_west_pointer_kinds = K_ANY_FUNCTION_RETURN;

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
        btids |= TB_INT;
        if ( (btids & TB_UNSIGNED) == TB_NONE ) {
          // If only 'i' is specified, it means all signed integer types
          // shall be explicit.
          btids |= TB_SHORT | TB_LONG | TB_LONG_LONG;
        }
        break;
      case 'l':
        if ( s[1] == 'l' || s[1] == 'L' ) {
          btids |= TB_LONG_LONG;
          ++s;
        } else {
          btids |= TB_LONG;
        }
        break;
      case 's':
        btids |= TB_SHORT;
        break;
      case 'u':
        btids |= TB_UNSIGNED;
        if ( s[1] == '\0' || s[1] == ',' ) {
          // If only 'u' is specified, it means all unsigned integer types
          // shall be explicit.
          btids |= TB_SHORT | TB_INT | TB_LONG | TB_LONG_LONG;
          break;
        }
        continue;
      case ',':
        break;
      default:
        return false;
    } // switch

    bool const is_unsigned = c_tid_is_any( btids, TB_UNSIGNED );
    tmp_ei_btids[ is_unsigned ] |= btids & c_tid_compl( TB_UNSIGNED );
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
        kinds |= K_USER_DEF_LITERAL;
        break;
      case 'o':
        kinds |= K_OPERATOR;
        break;
      case 'r':
        kinds |= K_ANY_FUNCTION_RETURN;
        break;
      case 't':
        kinds |= K_NON_PTR_REF_OBJECT;
        break;
      default:
        return false;
    } // switch
  } // for

  opt_west_pointer_kinds = kinds;
  return true;
}

void print_explicit_ecsu( FILE *out ) {
  if ( (opt_explicit_ecsu_btids & TB_ENUM) != TB_NONE )
    FPUTC( 'e', out );
  if ( (opt_explicit_ecsu_btids & TB_CLASS) != TB_NONE )
    FPUTC( 'c', out );
  if ( (opt_explicit_ecsu_btids & TB_STRUCT) != TB_NONE )
    FPUTC( 's', out );
  if ( (opt_explicit_ecsu_btids & TB_UNION) != TB_NONE )
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

void print_west_pointer( FILE *out ) {
  if ( (opt_west_pointer_kinds & K_APPLE_BLOCK) != 0 )
    FPUTC( 'b', out );
  if ( (opt_west_pointer_kinds & K_FUNCTION) != 0 )
    FPUTC( 'f', out );
  if ( (opt_west_pointer_kinds & K_USER_DEF_LITERAL) != 0 )
    FPUTC( 'l', out );
  if ( (opt_west_pointer_kinds & K_OPERATOR) != 0 )
    FPUTC( 'o', out );
  if ( (opt_west_pointer_kinds & K_NON_PTR_REF_OBJECT) != 0 )
    FPUTC( 't', out );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
