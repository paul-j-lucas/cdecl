/*
**      cdecl -- C gibberish translator
**      src/p_macro.h
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

#ifndef cdecl_p_macro_H
#define cdecl_p_macro_H

/**
 * @file
 * Declares types, macros, and functions for C preprocessor macros.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup p-macro-group C Preprocessor Macros
 * Types and functions for C Preprocessor macros.
 * @{
 */

////////// typedefs ///////////////////////////////////////////////////////////

/**
 * The signature for a function to generate a macro's value dynamically.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Returns the bitwise-or of languages the macro has a value in.
 */
typedef c_lang_id_t (*p_macro_dyn_fn_t)( p_token_t **ptoken );

/**
 * The signature for a function passed to p_macro_visit().
 *
 * @param macro The \ref p_macro to visit.
 * @param visit_data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and a pointer to the
 * \ref p_macro the visitor stopped on to be returned to the caller of
 * p_macro_visit().
 */
typedef bool (*p_macro_visit_fn_t)( p_macro_t const *macro, void *visit_data );

////////// structs ////////////////////////////////////////////////////////////

/**
 * C preprocessor macro parameter.
 */
struct p_param {
  char const *name;                     ///< Parameter name.
  c_loc_t     loc;                      ///< Source location.
};

/**
 * C preprocessor macro.
 */
struct p_macro {
  char const         *name;             ///< Macro name.

  /**
   * Is value dynamically generated?
   *
   * @remarks All predefined macros generate their values dynamically, so this
   * is synonymous with a macro being predefined.
   */
  bool                is_dynamic;

  /**
   * Additional data based on whether \ref is_dynamic is `true` or `false`.
   */
  union {
    p_macro_dyn_fn_t  dyn_fn;           ///< Dynamic value function.

    /**
     * Static macro members.
     */
    struct {
      /**
       * Parameter(s), if any.
       *
       * @remarks This is a pointer to a \ref p_param_list_t rather than a \ref
       * p_param_list_t so that NULL can distinguish an object-like macro from
       * a function-like macro that has zero parameters.
       */
      p_param_list_t *param_list;

      p_token_list_t  replace_list;     ///< Replacement tokens, if any.
    };
  };
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks whether \a name is a predefined macro or `__VA_ARGS__` or
 * `__VA_OPT__`.
 *
 * @param name The name to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_token_is_macro()
 */
NODISCARD
bool macro_is_predefined( char const *name );

/**
 * Frees all memory used by \a arg_list but _not_ \a arg_list itself.
 *
 * @param arg_list The macro argument list to cleanup.
 */
void p_arg_list_cleanup( p_arg_list_t *arg_list );

/**
 * Defines a new \ref p_macro.
 *
 * @param name The name of the macro to define.  Ownership is taken only if the
 * macro is defined successfully.
 * @param name_loc The source location of \a name.
 * @param param_list The parameter list, if any.  Parameters are moved out of
 * the list only if the macro is defined successfully.
 * @param replace_list The replacement token list, if any.  Tokens are move out
 * of the list only if the macro is defined successfully.
 * @return Returns a pointer to the new macro or NULL if unsuccessful.
 *
 * @sa p_macro_undef()
 */
NODISCARD
p_macro_t* p_macro_define( char *name, c_loc_t const *name_loc,
                           p_param_list_t *param_list,
                           p_token_list_t *replace_list );

/**
 * Expands a macro named \a name using \a arg_list.
 *
 * @param name The name of the macro to expand.
 * @param name_loc The source location of the macro's name.
 * @param arg_list The list of macro argument tokens, if any.
 * @param extra_list The list of extra tokens at the end of the `expand`
 * command, if any.
 * @param fout The `FILE` to print to.
 * @return Returns `true` only if the macro expanded successfully.
 */
NODISCARD
bool p_macro_expand( char const *name, c_loc_t const *name_loc,
                     p_arg_list_t *arg_list, p_token_list_t *extra_list,
                     FILE *fout );

/**
 * Gets the \ref p_macro having \a name.
 *
 * @param name The name of the macro to find.
 * @return Returns a pointer to the \ref p_macro having \a name or NULL if it's
 * not defined.
 */
NODISCARD
p_macro_t const* p_macro_find( char const *name );

/**
 * Checks whether \a macro is a function-like macro.
 *
 * @param macro The \ref p_macro to check.
 * @return Returns `true` only if it is.
 */
NODISCARD
inline bool p_macro_is_func_like( p_macro_t const *macro ) {
  return !macro->is_dynamic && macro->param_list != NULL;
}

/**
 * Undefines a macro having \a name.
 *
 * @param name The name of the macro to undefine.
 * @param name_loc The source location of \a name.
 * @return Returns `true` only if a macro having \a name was undefined.
 *
 * @sa p_macro_define()
 */
NODISCARD
bool p_macro_undef( char const *name, c_loc_t const *name_loc );

/**
 * Does an in-order traversal of all \ref p_macro.
 *
 * @param visit_fn The visitor function to use.
 * @param visit_data Optional data passed to \a visit_fn.
 */
void p_macro_visit( p_macro_visit_fn_t visit_fn, void *visit_data );

/**
 * Initializes all C preprocessor macro data.
 *
 * @note This function must be called exactly once.
 */
void p_macros_init( void );

/**
 * Frees all memory used by \a param _including_ \a param itself.
 *
 * @param param The \ref p_param to free.  If NULL, does nothing.
 *
 * @sa p_param_list_cleanup()
 */
void p_param_free( p_param_t *param );

/**
 * Cleans-up \a param_list by freeing only its nodes but _not_ \a param_list
 * itself.
 *
 * @param param_list The list of \ref p_param to free.
 *
 * @sa p_param_free()
 */
void p_param_list_cleanup( p_param_list_t *param_list );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_p_macro_H */
/* vim:set et sw=2 ts=2: */
