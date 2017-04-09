/*
**      cdecl -- C gibberish translator
**      src/common.h
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

#ifndef cdecl_common_H
#define cdecl_common_H

/**
 * @file
 * Declares constants, macros, types, and global variables, that are common to
 * several files that don't really fit anywhere else.
 */

// local
#include "config.h"                     /* must go first */

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

///////////////////////////////////////////////////////////////////////////////

#define CPPDECL               "c++decl"
#define DEBUG_INDENT          2         /* spaces per debug indent level */

typedef struct {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;

#define YYLTYPE_IS_DECLARED   1
#define YYLTYPE_IS_TRIVIAL    1

// extern variables
extern bool         is_input_a_tty;     // is our input from a tty?
extern char const  *me;                 // program name

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_common_H */
/* vim:set et sw=2 ts=2: */
