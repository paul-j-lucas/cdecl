/*
**      cdecl -- C gibberish translator
**      src/cdecl_term.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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

#ifndef cdecl_term_H
#define cdecl_term_H

/**
 * @file
 * Declares functions for dealing with the terminal.
 */

// local
#include "pjl_config.h"                 /* must go first */

/**
 * @defgroup terminal-group Terminal Capabilities
 * Function for getting the number of columns of the terminal.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the terminal.
 *
 * @note This function must be called exactly once.
 */
void cdecl_term_init( void );

/**
 * Gets the number of columns of the terminal.
 *
 * @return Returns the number of columns or 0 if can not be determined.
 */
NODISCARD
unsigned term_get_columns( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_term_H */
/* vim:set et sw=2 ts=2: */
