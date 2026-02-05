/*
**      cdecl -- C gibberish translator
**      src/p_kind.c
**
**      Copyright (C) 2023-2026  Paul J. Lucas
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

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "p_kind.h"
#include "literals.h"
#include "util.h"

////////// extern functions ///////////////////////////////////////////////////

char const* p_kind_name( p_token_kind_t kind ) {
  switch ( kind ) {
    case P_CHAR_LIT   : return "char_lit";
    case P_CONCAT     : return "##";
    case P_IDENTIFIER : return "identifier";
    case P_NUM_LIT    : return "num_lit";
    case P_OTHER      : return "other";
    case P_PLACEMARKER: return "placemarker";
    case P_PUNCTUATOR : return "punctuator";
    case P_SPACE      : return " ";
    case P_STRINGIFY  : return "#";
    case P_STR_LIT    : return "str_lit";
    case P___VA_ARGS__: return L_PRE___VA_ARGS__;
    case P___VA_OPT__ : return L_PRE___VA_OPT__;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
