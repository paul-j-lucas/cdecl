/*
**      cdecl -- C gibberish translator
**      src/autocomplete.h
**
**      Copyright (C) 2023-2024  Paul J. Lucas
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

#ifndef cdecl_autocomplete_H
#define cdecl_autocomplete_H

/**
 * @file
 * Declares types and functions for command-line autocompletion.
 */

// local
#include "pjl_config.h"                 /* must go first */
#ifndef WITH_READLINE
#error "This file should not be included unless WITH_READLINE is defined."
#endif /* WITH_READLINE */

// standard
#include <stdio.h>                      /* for FILE */
#include <readline/readline.h>          /* must go last */

/**
 * @defgroup autocompletion-group Autocompletion
 * Types and functions for command-line autocompletion.
 * @{
 */

/**
 * Defined to an expression that evaluates to `true` only if we're running
 * genuine GNU **readline**(3) and not some other library emulating it.
 *
 * @remarks Some readline emulators, e.g., editline, have a bug that makes
 * color prompts not work correctly.  So, unless we know we're using genuine
 * GNU readline, use this to disable color prompts.
 *
 * @return Evaluates to `true` only if we're running genuine GNU readline.
 *
 * @sa [The GNU Readline
 * Library](https://tiswww.case.edu/php/chet/readline/rltop.html)
 * @sa http://stackoverflow.com/a/31333315/99089
 */
#if HAVE_DECL_RL_GNU_READLINE_P
# define HAVE_GENUINE_GNU_READLINE  (rl_gnu_readline_p == 1)
#else
# define HAVE_GENUINE_GNU_READLINE  0
#endif /* HAVE_DECL_RL_GNU_READLINE_P */

///////////////////////////////////////////////////////////////////////////////

/**
 * Autocompletion policy for a particular \ref cdecl_keyword.
 */
enum ac_policy {
  /**
   * No special autocompletion policy.
   */
  AC_POLICY_DEFAULT,

  /**
   * Do not autocomplete: defer to another keyword, e.g., `align` should defer
   * to `aligned`, `const` should defer to its C keyword counterpart, etc.
   */
  AC_POLICY_DEFER,

  /**
   * Autocomplete only when the keyword is explicitly listed in the \ref
   * cdecl_keyword::ac_next_keywords "ac_next_keywords" of some other keyword.
   *
   * For example, the `bytes` keyword should be autocompleted only when it
   * follows `aligned`.
   */
  AC_POLICY_IN_NEXT_ONLY,

  /**
   * Autocomplete only if no other keyword matches.
   *
   * For example, the **cdecl** `boolean` keyword is a synonym for either
   * `_Bool` in C or `bool` in C++.  However, `boolean` should _not_ be offered
   * as an autocompletion choice initially since it would be ambiguous with
   * `bool` which is redundant:
   *
   *      cdecl> declare x as bo<tab>
   *      bool boolean
   *
   * Instead, `boolean` should be offered only if the user typed enough as to
   * make it unambiguous (no other keyword matches):
   *
   *      cdecl> declare x as boole<tab>
   */
  AC_POLICY_NO_OTHER,

  /**
   * Do not autocomplete: the keyword is too short, e.g., `as`, `mbr`, `no`,
   * `of`, `ptr`, `q`, etc.
   *
   * @note The keyword can still be autocompleted if it's explicitly listed in
   * some other keyword's \ref cdecl_keyword::ac_next_keywords
   * "ac_next_keywords".
   */
  AC_POLICY_TOO_SHORT
};
typedef enum ac_policy ac_policy_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Initializes GNU **readline**(3).
 *
 * @param prog_name The name of the running program to parse from `~/.inputrc`
 * or NULL for none.
 * @param rin The `FILE` to read from.
 * @param rout The `FILE` to write to.
 *
 * @note This function _must_ be called once before calling **readline**(3);
 * however, calling it more than once is harmless.
 */
void readline_init( char const *prog_name, FILE *rin, FILE *rout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_autocomplete_H */
/* vim:set et sw=2 ts=2: */
