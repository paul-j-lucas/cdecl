/*
**      cdecl -- C gibberish translator
**      src/c_kind.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
 * Defines functions for kinds of things in C/C++ declarations.
 */

// local
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_KIND_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_kind.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

char const* c_kind_name( c_kind_t kind ) {
  switch ( kind ) {
    case K_ARRAY            : return "array";
    case K_BLOCK            : return "block";
    case K_BUILTIN          : return "built-in type";
    case K_FUNCTION         : return "function";
    case K_NAME             : return "name";
    case K_NONE             : return "none";
    case K_PLACEHOLDER      : return "placeholder";
    case K_OPERATOR         : return "operator";
    case K_POINTER          : return "pointer";
    case K_POINTER_TO_MEMBER: return "pointer-to-member";
    case K_REFERENCE        : return "reference";
    case K_RVALUE_REFERENCE : return "rvalue reference";
    case K_TYPEDEF          : return "typedef";
    case K_VARIADIC         : return "variadic";

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( opt_lang >= LANG_CPP_OLD )
        return "enum, class, struct, or union";
      if ( opt_lang >= LANG_C_89 )
        return "enum, struct, or union";
      assert( opt_lang == LANG_C_KNR );
      return "struct or union";

    default:
      INTERNAL_ERR( "unexpected value (%d) for c_kind\n", (int)kind );
      return NULL;                      // suppress warning
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
