/*
**      cdecl -- C gibberish translator
**      src/help.h
**
**      Copyright (C) 2021  Paul J. Lucas
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

#ifndef cdecl_help_H
#define cdecl_help_H

/**
 * @file
 * Declares functions for printing help text.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints a help message.
 *
 * @param help The type of help to print.
 */
void print_help( cdecl_help_t help );

/**
 * Prints `; use --help or -h for help` to stderr.
 */
void print_use_help( void );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_help_H */
/* vim:set et sw=2 ts=2: */
