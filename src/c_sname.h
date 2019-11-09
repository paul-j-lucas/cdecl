/*
**      cdecl -- C gibberish translator
**      src/c_sname.h
**
**      Copyright (C) 2019  Paul J. Lucas, et al.
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
 * `S::T::x`. An sname also has a type that is one of #T_NONE, #T_CLASS,
 * #T_NAMESPACE, #T_SCOPE, #T_STRUCT, or #T_UNION.
 *
 * @note For C, an sname is simply a single (unscoped) name, e.g., `x`.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "slist.h"
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <string.h>                     /* for str*(3) */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_SNAME_INLINE
# define CDECL_SNAME_INLINE _GL_INLINE
#endif /* CDECL_SNAME_INLINE */

/// @endcond

/**
 * @defgroup sname-group Scoped Names
 * Functions for accessing and manipulating C++ scoped names.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Appends \a name onto the end of \a sname.
 *
 * @param sname The scoped name to append to.
 * @param name The name to append.
 *
 * @sa c_sname_append_sname()
 * @sa c_sname_prepend_name()
 * @sa c_sname_prepend_sname()
 */
CDECL_SNAME_INLINE void c_sname_append_name( c_sname_t *sname, char *name ) {
  slist_push_tail( sname, name );
}

/**
 * Appends \a src onto the end of \a dst.
 *
 * @param dst The scoped name to append to.
 * @param src The scoped name to append.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_prepend_name()
 * @sa c_sname_prepend_sname()
 */
CDECL_SNAME_INLINE void c_sname_append_sname( c_sname_t *dst, c_sname_t *src ) {
  (void)slist_push_list_tail( dst, src );
}

/**
 * Compares two scoped names.
 *
 * @param sname_i The first scoped name to compare.
 * @param sname_j The second scoped name to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a sname_i is
 * less than, equal to, or greater than \a sname_j, respectively.
 */
CDECL_SNAME_INLINE int c_sname_cmp( c_sname_t const *sname_i,
                                    c_sname_t const *sname_j ) {
  return slist_cmp( sname_i, sname_j, (slist_node_data_cmp_fn_t)&strcmp );
}

/**
 * Gets the number of names of \a sname, e.g., `S::T::x` is 3.
 *
 * @param sname The scoped name to get the number of names of.
 * @return Returns said number of names.
 */
CDECL_SNAME_INLINE size_t c_sname_count( c_sname_t const *sname ) {
  return slist_len( sname );
}

/**
 * Duplicates \a sname.  The caller is responsible for calling c_sname_free on
 * the duplicate.
 *
 * @param sname The scoped name to duplicate.
 * @return Returns a duplicate of \a sname.
 */
CDECL_SNAME_INLINE c_sname_t c_sname_dup( c_sname_t const *sname ) {
  return slist_dup( sname, -1, NULL, (slist_node_data_dup_fn_t)&strdup );
}

/**
 * Gets whether \a name is empty.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname is empty.
 */
CDECL_SNAME_INLINE bool c_sname_empty( c_sname_t const *sname ) {
  return slist_empty( sname );
}

/**
 * Frees all memory associated with \a sname.
 *
 * @param sname The scoped name to free.  If null, does nothing; otherwise,
 * reinitializes it upon completion.
 */
CDECL_SNAME_INLINE void c_sname_free( c_sname_t *sname ) {
  slist_free( sname, NULL, &free );
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
 * @sa c_sname_scope_name
 */
char const* c_sname_full_name( c_sname_t const *sname );

/**
 * Initializes \a sname.  This is not necessary for either global or `static`
 * scoped names.
 *
 * @param sname The scoped name to initialize.
 *
 * @sa c_sname_free()
 */
CDECL_SNAME_INLINE void c_sname_init( c_sname_t *sname ) {
  slist_init( sname );
}

/**
 * Gets whether \a sname is a constructor name, e.g. `S::T::T`.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if the last two names of \a sname match.
 */
bool c_sname_is_ctor( c_sname_t const *sname );

/**
 * Gets the local (last) name of \a sname.
 *
 * @param sname The scoped name to get the local name of.
 * @return Returns said name or the empty string if \a sname is empty.
 *
 * @sa c_sname_full_name
 * @sa c_sname_scope_name
 */
CDECL_SNAME_INLINE char const* c_sname_local_name( c_sname_t const *sname ) {
  return c_sname_empty( sname ) ? "" : SLIST_TAIL( char const*, sname );
}

/**
 * Gets the name at \a offset of \a sname.
 *
 * @param sname The `c_sname_t` to get the name at \a offset of.
 * @param offset The offset (starting at 0) of the name to get.
 * @return Returns the name at \a offset or the empty string if \a offset &gt;=
 * c_sname_count().
 */
CDECL_SNAME_INLINE char const* c_sname_name_at( c_sname_t const *sname,
                                                size_t offset ) {
  char const *const temp = SLIST_PEEK_AT( char const*, sname, offset );
  return temp != NULL ? temp : "";
}

/**
 * Gets the name at \a roffset of \a sname.
 *
 * @param sname The `c_sname_t` to get the name at \a roffset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a roffset or the empty string if \a roffset
 * &gt;= c_sname_count().
 */
CDECL_SNAME_INLINE char const* c_sname_name_atr( c_sname_t const *sname,
                                                 size_t roffset ) {
  char const *const temp = SLIST_PEEK_ATR( char const*, sname, roffset );
  return temp != NULL ? temp : "";
}

/**
 * Prepends \a name onto the beginning of \a sname.
 *
 * @param sname The scoped name to prepend to.
 * @param name The name to prepend.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_append_sname()
 * @sa c_sname_prepend_sname()
 */
CDECL_SNAME_INLINE void c_sname_prepend_name( c_sname_t *sname, char *name ) {
  slist_push_head( sname, name );
}

/**
 * Prepends \a src onto the beginning of \a dst.
 *
 * @param dst The scoped name to prepend to.
 * @param src The name to prepend.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_append_sname()
 * @sa c_sname_prepend_name()
 */
CDECL_SNAME_INLINE void c_sname_prepend_sname( c_sname_t *dst,
                                               c_sname_t *src ) {
  (void)slist_push_list_head( dst, src );
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
 */
char const* c_sname_scope_name( c_sname_t const *sname );

/**
 * Sets the scope type of \a sname.
 *
 * @param sname The scoped name to set the scope type of.
 * @param type_id The scope type.
 *
 * @sa c_sname_type()
 */
CDECL_SNAME_INLINE void c_sname_set_type( c_sname_t *sname,
                                          c_type_id_t type_id ) {
  sname->data = CONST_CAST( void*, type_id );
}

/**
 * Gets the scope type of \a sname.
 *
 * @param sname The scoped name to get the scope type of.
 * @return Returns the scope type.
 *
 * @sa c_sname_set_type()
 */
CDECL_SNAME_INLINE c_type_id_t c_sname_type( c_sname_t const *sname ) {
  return REINTERPRET_CAST( c_type_id_t, sname->data );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_sname_H */
/* vim:set et sw=2 ts=2: */
