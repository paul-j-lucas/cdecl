/*
**      cdecl -- C gibberish translator
**      src/c_sname.h
**
**      Copyright (C) 2019-2020  Paul J. Lucas, et al.
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

#ifndef cdecl_c_sname_H
#define cdecl_c_sname_H

/**
 * @file
 * Declares functions for dealing with "sname" (C++ scoped name) objects, e.g.,
 * `S::T::x`.
 *
 * + An sname also has a type (one of of #TB_CLASS, #TB_NAMESPACE, #TB_SCOPE,
 *   #TB_STRUCT, or #TB_UNION) for each scope.
 * + The "local" of an sname is the innermost scope, e.g., `x`.  A non-empty
 *   sname always has a local.
 * + The "scope" of an sname is all but the innermost scope, e.g., `S::T`.  A
 *   non-empty sname may or may not have a scope.
 *
 * @note For C, an sname is simply a single (unscoped) name, e.g., `x`.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_type.h"
#include "slist.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <string.h>                     /* for str*(3) */

_GL_INLINE_HEADER_BEGIN
#ifndef C_SNAME_INLINE
# define C_SNAME_INLINE _GL_INLINE
#endif /* C_SNAME_INLINE */

/// @endcond

/**
 * @defgroup sname-group Scoped Names
 * Functions for accessing and manipulating C++ scoped names.
 * @{
 */

/**
 * Creates an sname variable on the stack having \a NAME.
 *
 * @param VAR_NAME The name for the sname variable.
 * @param NAME The name.
 */
#define SNAME_VAR_INIT(VAR_NAME,NAME)                   \
  c_scope_data_t VAR_NAME##_data = { (NAME), T_NONE };  \
  SLIST_VAR_INIT( VAR_NAME, NULL, &VAR_NAME##_data )

/**
 * Gets the data associated with \a SCOPE.
 * @param SCOPE The `c_scope_t` to get the data of.
 *
 * @note This is a macro instead of an inline function so it'll work with
 * either `const` or non-`const` \a SCOPE.
 */
#define c_scope_data(SCOPE) \
  REINTERPRET_CAST( c_scope_data_t*, (SCOPE)->data )

///////////////////////////////////////////////////////////////////////////////

/**
 * Data for each scope of an sname.
 */
struct c_scope_data {
  /**
   * The scope's name.
   */
  char const *name;

  /**
   * The scope's type, one of: #TB_CLASS, #TB_STRUCT, #TB_UNION, [#TS_INLINE]
   * #TB_NAMESPACE, or #TB_SCOPE.
   */
  c_type_t type;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Compares two scope datas.
 *
 * @param i_data The first scope data to compare.
 * @param j_data The second scope data to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
PJL_WARN_UNUSED_RESULT
int c_scope_data_cmp( c_scope_data_t *i_data, c_scope_data_t *j_data );

/**
 * Duplicates \a data.  The caller is responsible for calling
 * c_scope_data_free() on the duplicate.
 *
 * @param data The scope data to duplicate.
 * @return Returns a duplicate of \a data.
 */
PJL_WARN_UNUSED_RESULT
c_scope_data_t* c_scope_data_dup( c_scope_data_t const *data );

/**
 * Frees all memory associated with \a data.
 *
 * @param data The scope data to free.  If null, does nothing.
 */
void c_scope_data_free( c_scope_data_t *data );

/**
 * Appends \a name onto the end of \a sname.
 *
 * @param sname The scoped name to append to.
 * @param name The name to append.  Ownership is taken.
 *
 * @sa c_sname_append_sname()
 * @sa c_sname_prepend_sname()
 */
void c_sname_append_name( c_sname_t *sname, char *name );

/**
 * Appends \a src onto the end of \a dst.
 *
 * @param dst The scoped name to append to.
 * @param src The scoped name to append.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_prepend_sname()
 */
C_SNAME_INLINE
void c_sname_append_sname( c_sname_t *dst, c_sname_t *src ) {
  slist_push_list_tail( dst, src );
}

/**
 * Compares two scoped names.
 *
 * @param i_sname The first scoped name to compare.
 * @param j_sname The second scoped name to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_sname is
 * less than, equal to, or greater than \a j_sname, respectively.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
int c_sname_cmp( c_sname_t const *i_sname, c_sname_t const *j_sname ) {
  return slist_cmp(
    i_sname, j_sname, (slist_node_data_cmp_fn_t)&c_scope_data_cmp
  );
}

/**
 * Gets the number of names of \a sname, e.g., `S::T::x` is 3.
 *
 * @param sname The scoped name to get the number of names of.
 * @return Returns said number of names.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
size_t c_sname_count( c_sname_t const *sname ) {
  return slist_len( sname );
}

/**
 * Duplicates \a sname.  The caller is responsible for calling c_sname_free()
 * on the duplicate.
 *
 * @param sname The scoped name to duplicate.
 * @return Returns a duplicate of \a sname.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
c_sname_t c_sname_dup( c_sname_t const *sname ) {
  return slist_dup(
    sname, -1, NULL, (slist_node_data_dup_fn_t)&c_scope_data_dup
  );
}

/**
 * Gets whether \a name is empty.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname is empty.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
bool c_sname_empty( c_sname_t const *sname ) {
  return slist_empty( sname );
}

/**
 * Frees all memory associated with \a sname.
 *
 * @param sname The scoped name to free.  If null, does nothing; otherwise,
 * reinitializes it upon completion.
 */
C_SNAME_INLINE
void c_sname_free( c_sname_t *sname ) {
  slist_free( sname, NULL, (slist_data_free_fn_t)&c_scope_data_free );
}

/**
 * Gets the fully scoped name of \a sname.
 *
 * @param sname The `c_sname_t` to get the full name of.
 * @return Returns said name or the empty string if \a sname is empty.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_local_name
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name
 */
PJL_WARN_UNUSED_RESULT
char const* c_sname_full_name( c_sname_t const *sname );

/**
 * Initializes \a sname.  This is not necessary for either global or `static`
 * scoped names.
 *
 * @param sname The scoped name to initialize.
 *
 * @sa c_sname_free()
 */
C_SNAME_INLINE
void c_sname_init( c_sname_t *sname ) {
  slist_init( sname );
}

/**
 * Gets whether \a sname is a constructor name, e.g. `S::T::T`.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if the last two names of \a sname match.
 */
PJL_WARN_UNUSED_RESULT
bool c_sname_is_ctor( c_sname_t const *sname );

/**
 * Gets the local (last) name of \a sname.
 *
 * @param sname The scoped name to get the local name of.
 * @return Returns said name or the empty string if \a sname is empty.
 *
 * @sa c_sname_full_name
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
char const* c_sname_local_name( c_sname_t const *sname ) {
  return c_sname_empty( sname ) ?
    "" : SLIST_PEEK_TAIL( c_scope_data_t*, sname )->name;
}

/**
 * Gets the scope type of \a sname (which is the type of the innermost scope).
 *
 * @param sname The scoped name to get the scope type of.
 * @return Returns the scope type.
 *
 * @sa c_sname_scope_type()
 * @sa c_sname_set_local_type()
 */
c_type_t const* c_sname_local_type( c_sname_t const *sname );

/**
 * Gets the name at \a roffset of \a sname.
 *
 * @param sname The `c_sname_t` to get the name at \a roffset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a roffset or the empty string if \a roffset
 * &gt;= c_sname_count().
 *
 * @sa c_sname_full_name
 * @sa c_sname_scope_name
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
char const* c_sname_name_atr( c_sname_t const *sname, size_t roffset ) {
  c_scope_data_t const *const data =
    SLIST_PEEK_ATR( c_scope_data_t*, sname, roffset );
  return data != NULL ? data->name : "";
}

/**
 * Prepends \a src onto the beginning of \a dst.
 *
 * @param dst The scoped name to prepend to.
 * @param src The name to prepend.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_append_sname()
 */
C_SNAME_INLINE
void c_sname_prepend_sname( c_sname_t *dst, c_sname_t *src ) {
  slist_push_list_head( dst, src );
}

/**
 * Gets just the scope name of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns the empty string.
 *
 * @param sname The `c_sname_t` to get the scope name of.
 * @return Returns said name or the empty string if \a sname is empty or not
 * within a scope.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_full_name
 * @sa c_sname_local_name
 * @sa c_sname_name_atr()
 */
PJL_WARN_UNUSED_RESULT
char const* c_sname_scope_name( c_sname_t const *sname );

/**
 * Gets the scope type of the scope of \a sname.
 *
 * @param sname The scoped name to get the scope type of the scope of.
 * @return Returns said type or #T_NONE if \a sname is empty or not within a
 * scope.
 * @sa c_sname_local_type()
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
c_type_t const* c_sname_scope_type( c_sname_t const *sname ) {
  c_scope_data_t const *const data =
    SLIST_PEEK_ATR( c_scope_data_t*, sname, 1 );
  return data != NULL ? &data->type : &T_NONE;
}

/**
 * Sets the scope type of \a sname (which is the type of the innermost scope).
 *
 * @param sname The scoped name to set the scope type of.
 * @param type The type.
 *
 * @sa c_sname_local_type()
 */
C_SNAME_INLINE
void c_sname_set_local_type( c_sname_t *sname, c_type_t const *type ) {
  c_scope_data( sname->tail )->type = *type;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_sname_H */
/* vim:set et sw=2 ts=2: */
