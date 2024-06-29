/*
**      cdecl -- C gibberish translator
**      src/options.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas, et al.
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
#include <stddef.h>                     /* for NULL */

/// @endcond

/**
 * @addtogroup cdecl-options-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern option variables
bool                opt_alt_tokens;
cdecl_debug_t       opt_cdecl_debug;
color_when_t        opt_color_when = COLOR_NOT_FILE;
char const         *opt_conf_path;
bool                opt_east_const;
bool                opt_echo_commands;
bool                opt_english_types = true;
c_tid_t             opt_explicit_ecsu_btids = TB_struct | TB_union;
char const         *opt_file = "-";
c_graph_t           opt_graph;
bool                opt_infer_command;
c_lang_id_t         opt_lang_id;
unsigned            opt_lineno;
bool                opt_prompt = true;
bool                opt_read_conf = true;
bool                opt_semicolon = true;
bool                opt_trailing_ret;
bool                opt_typedefs = true;
bool                opt_using = true;
c_ast_kind_t        opt_west_decl_kinds = K_ANY_FUNCTION_RETURN;

// extern constants
char const          OPT_CDECL_DEBUG_ALL[] = "u";
char const          OPT_ECSU_ALL[]        = "ecsu";
char const          OPT_WEST_DECL_ALL[]   = "bflost";

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
 * @sa is_explicit_int()
 * @sa parse_explicit_int()
 */
static c_tid_t      opt_explicit_int_btids[] = { TB_NONE, TB_NONE };

////////// local functions ////////////////////////////////////////////////////

/**
 * If \a *pformat is:
 *
 *  + `"*"`: sets \a *pformat to \a all_value.
 *  + `"-"`: sets \a *pformat to `""` (the empty string).
 *
 * Otherwise does nothing.
 *
 * @param pformat A pointer to the format string to possibly set.
 * @param all_value The "all" value for when \a *pformat is `"*"`.
 */
static void set_all_or_none( char const **pformat, char const *all_value ) {
  assert( pformat != NULL );
  assert( *pformat != NULL );
  assert( all_value != NULL );
  assert( all_value[0] != '\0' );

  if ( strcmp( *pformat, "*" ) == 0 )
    *pformat = all_value;
  else if ( strcmp( *pformat, "-" ) == 0 )
    *pformat = "";
}

////////// extern functions ///////////////////////////////////////////////////

char const* cdecl_debug_str( void ) {
  static char buf[ ARRAY_SIZE( OPT_CDECL_DEBUG_ALL ) ];
  char *s = buf;

  if ( (opt_cdecl_debug & CDECL_DEBUG_OPT_AST_UNIQUE_ID) != 0 )
    *s++ = 'u';                         // LCOV_EXCL_LINE -- unique_ids vary

  assert( s < buf + ARRAY_SIZE( buf ) );
  *s = '\0';
  return buf;
}

char const* explicit_ecsu_str( void ) {
  static char buf[ ARRAY_SIZE( OPT_ECSU_ALL ) ];
  char *s = buf;

  if ( (opt_explicit_ecsu_btids & TB_enum) != TB_NONE )
    *s++ = 'e';
  if ( (opt_explicit_ecsu_btids & TB_class) != TB_NONE )
    *s++ = 'c';
  if ( (opt_explicit_ecsu_btids & TB_struct) != TB_NONE )
    *s++ = 's';
  if ( (opt_explicit_ecsu_btids & TB_union) != TB_NONE )
    *s++ = 'u';

  assert( s < buf + ARRAY_SIZE( buf ) );
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

bool parse_cdecl_debug( char const *debug_format ) {
  if ( debug_format == NULL ) {
    opt_cdecl_debug = CDECL_DEBUG_NO;
    return true;
  }

  set_all_or_none( &debug_format, OPT_CDECL_DEBUG_ALL );
  cdecl_debug_t cdecl_debug = CDECL_DEBUG_YES;

  for ( char const *s = debug_format; *s != '\0'; ++s ) {
    switch ( tolower( *s ) ) {
      case 'u':
        // LCOV_EXCL_START -- unique_ids vary
        cdecl_debug |= CDECL_DEBUG_OPT_AST_UNIQUE_ID;
        break;
        // LCOV_EXCL_STOP
      default:
        return false;
    } // switch
  } // for

  opt_cdecl_debug = cdecl_debug;
  return true;
}

bool parse_explicit_ecsu( char const *ecsu_format ) {
  set_all_or_none( &ecsu_format, OPT_ECSU_ALL );
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
  set_all_or_none( &ei_format, "iu" );
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

  opt_explicit_int_btids[0] = c_tid_check( tmp_ei_btids[0], C_TPID_BASE );
  opt_explicit_int_btids[1] = c_tid_check( tmp_ei_btids[1], C_TPID_BASE );
  return true;
}

bool parse_west_decl( char const *wd_format ) {
  set_all_or_none( &wd_format, OPT_WEST_DECL_ALL );
  unsigned kinds = 0;

  for ( char const *s = wd_format; *s != '\0'; ++s ) {
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
      case 's':
        kinds |= K_STRUCTURED_BINDING;
        break;
      case 't':
        kinds |= K_ANY_NON_PTR_REF_OBJECT;
        break;
      default:
        return false;
    } // switch
  } // for

  opt_west_decl_kinds = kinds;
  return true;
}

char const* west_decl_str( void ) {
  static char buf[ ARRAY_SIZE( OPT_WEST_DECL_ALL ) ];
  char *s = buf;

  if ( (opt_west_decl_kinds & K_APPLE_BLOCK) != 0 )
    *s++ = 'b';
  if ( (opt_west_decl_kinds & K_FUNCTION) != 0 )
    *s++ = 'f';
  if ( (opt_west_decl_kinds & K_UDEF_LIT) != 0 )
    *s++ = 'l';
  if ( (opt_west_decl_kinds & K_OPERATOR) != 0 )
    *s++ = 'o';
  if ( (opt_west_decl_kinds & K_STRUCTURED_BINDING) != 0 )
    *s++ = 's';
  if ( (opt_west_decl_kinds & K_ANY_NON_PTR_REF_OBJECT) != 0 )
    *s++ = 't';

  assert( s < buf + ARRAY_SIZE( buf ) );
  *s = '\0';
  return buf;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
