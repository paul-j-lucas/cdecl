/*
**      cdecl -- C gibberish translator
**      src/c_sname.h
**
**      Copyright (C) 2019-2024  Paul J. Lucas
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
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_type.h"
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef C_SNAME_H_INLINE
# define C_SNAME_H_INLINE _GL_INLINE
#endif /* C_SNAME_H_INLINE */

/// @endcond

/**
 * @defgroup sname-group Scoped Names
 * Functions for dealing with "sname" (C++ scoped name) objects, e.g.,
 * `S::T::x`.
 *
 * + An sname also has a type for each scope, one of of #TB_class,
 *   #TB_namespace (and possibly #TS_inline), #TB_SCOPE, #TB_struct, or
 *   #TB_union.
 *
 * + The "local" of an sname is the innermost scope, e.g., `x`.  A non-empty
 *   sname always has a local.
 *
 * + The "scope" of an sname is all but the innermost scope, e.g., `S::T`.  A
 *   non-empty sname may or may not have a scope.
 *
 * @note For C, an sname is simply a single (unscoped) name, e.g., `x`.
 *
 * @sa \ref sglob-group
 * @{
 */

/**
 * Creates a scoped name variable \a VAR on the stack having a local \a NAME.
 *
 * @param VAR The \ref c_sname_t variable.
 * @param NAME The name.
 *
 * @warning c_sname_cleanup() must _not_ be called on \a VAR.
 */
#define SNAME_VAR_INIT_NAME(VAR,NAME) \
  SLIST_VAR_INIT( VAR, (&(c_scope_data_t){ (NAME), T_NONE }) )

/**
 * Convenience macro for iterating over all scopes of an sname.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param SNAME The \ref c_sname_t to iterate over the scopes of.
 *
 * @sa #FOREACH_SNAME_SCOPE_UNTIL()
 */
#define FOREACH_SNAME_SCOPE(VAR,SNAME) \
  FOREACH_SLIST_NODE( VAR, (SNAME) )

/**
 * Convenience macro for iterating over all scopes of an sname up to but not
 * including \a END.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param SNAME The \ref c_sname_t to iterate over the scopes of.
 * @param END The scope to end before; may be NULL.
 *
 * @sa #FOREACH_SNAME_SCOPE()
 */
#define FOREACH_SNAME_SCOPE_UNTIL(VAR,SNAME,END) \
  FOREACH_SLIST_NODE_UNTIL( VAR, (SNAME), (END) )

///////////////////////////////////////////////////////////////////////////////

/**
 * Data for each scope of an \ref c_sname_t.
 */
struct c_scope_data {
  /**
   * The scope's name.
   */
  char const *name;

  /**
   * The scope's type, one of: #TB_class, #TB_struct, #TB_union, [#TS_inline]
   * #TB_namespace, or #TB_SCOPE.
   */
  c_type_t type;
};
typedef struct c_scope_data c_scope_data_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the \ref c_scope_data associated with \a scope.
 *
 * @param scope The scope to get the data of.
 * @return Returns said data.
 */
NODISCARD C_SNAME_H_INLINE
c_scope_data_t const* c_scope_data( c_scope_t const *scope ) {
  return scope->data;
}

/// @cond DOXYGEN_IGNORE
NODISCARD C_SNAME_H_INLINE
c_scope_data_t* nonconst_c_scope_data( c_scope_t *scope ) {
  return CONST_CAST( c_scope_data_t*, c_scope_data( scope ) );
}

#define c_scope_data(SCOPE)       NONCONST_OVERLOAD( c_scope_data, (SCOPE) )
/// @endcond

/**
 * Compares two \ref c_scope_data.
 *
 * @param i_data The first \ref c_scope_data to compare.
 * @param j_data The second \ref c_scope_data to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
NODISCARD
int c_scope_data_cmp( c_scope_data_t const *i_data,
                      c_scope_data_t const *j_data );

/**
 * Duplicates \a data.
 *
 * @param data The \ref c_scope_data to duplicate; may be NULL.
 * @return Returns a duplicate of \a data or NULL only if \a data is NULL.  The
 * caller is responsible for calling c_scope_data_free() on it.
 *
 * @sa c_scope_data_free()
 */
NODISCARD
c_scope_data_t* c_scope_data_dup( c_scope_data_t const *data );

/**
 * Frees all memory associated with \a data _including_ \a data itself.
 *
 * @param data The \ref c_scope_data to free.  If NULL, does nothing.
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
 * @sa c_sname_set()
 */
void c_sname_append_name( c_sname_t *sname, char const *name );

/**
 * Appends \a src onto the end of \a dst.
 *
 * @param dst The scoped name to append to.
 * @param src The scoped name to append.  Ownership is taken.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_prepend_sname()
 * @sa c_sname_set()
 */
C_SNAME_H_INLINE
void c_sname_append_sname( c_sname_t *dst, c_sname_t *src ) {
  slist_push_list_back( dst, src );
}

/**
 * Checks a scoped name for valid scope order.
 *
 * @param sname The scoped name to check.
 * @param sname_loc The location of \a sname.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
bool c_sname_check( c_sname_t const *sname, c_loc_t const *sname_loc );

/**
 * Cleans-up all memory associated with \a sname but does _not_ free \a sname
 * itself.
 *
 * @param sname The scoped name to clean up.  If NULL, does nothing; otherwise,
 * reinitializes it upon completion.
 *
 * @sa c_sname_init()
 * @sa c_sname_init_name()
 * @sa c_sname_list_cleanup()
 */
void c_sname_cleanup( c_sname_t *sname );

/**
 * Compares two scoped names.
 *
 * @param i_sname The first scoped name to compare.
 * @param j_sname The second scoped name to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_sname is
 * less than, equal to, or greater than \a j_sname, respectively.
 *
 * @sa c_sname_cmp_name()
 */
NODISCARD C_SNAME_H_INLINE
int c_sname_cmp( c_sname_t const *i_sname, c_sname_t const *j_sname ) {
  return slist_cmp(
    i_sname, j_sname, POINTER_CAST( slist_cmp_fn_t, &c_scope_data_cmp )
  );
}

/**
 * Compares a scoped name and a name.
 *
 * @param sname The scoped name to compare.
 * @param name The name to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a sname is
 * less than, equal to, or greater than \a name, respectively.
 *
 * @sa c_sname_cmp()
 */
NODISCARD
int c_sname_cmp_name( c_sname_t const *sname, char const *name );

/**
 * Gets the number of names of \a sname, e.g., `S::T::x` is 3.
 *
 * @param sname The scoped name to get the number of names of.
 * @return Returns said number of names.
 *
 * @note This is named "count" rather than "len" to avoid misinterpretation
 * that "len" would be the total length of the strings and `::` separators.
 */
NODISCARD C_SNAME_H_INLINE
size_t c_sname_count( c_sname_t const *sname ) {
  return slist_len( sname );
}

/**
 * Duplicates \a sname.  The caller is responsible for calling
 * c_sname_cleanup() on the duplicate.
 *
 * @param sname The scoped name to duplicate; may be NULL.
 * @return Returns a duplicate of \a sname or an empty scoped name if \a sname
 * is NULL.
 */
NODISCARD C_SNAME_H_INLINE
c_sname_t c_sname_dup( c_sname_t const *sname ) {
  return slist_dup(
    sname, -1, POINTER_CAST( slist_dup_fn_t, &c_scope_data_dup )
  );
}

/**
 * Gets whether \a sname is empty.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname is empty.
 */
NODISCARD C_SNAME_H_INLINE
bool c_sname_empty( c_sname_t const *sname ) {
  return slist_empty( sname );
}

/**
 * Checks a scoped name for errors.
 *
 * @param sname The scoped name to check.
 * @param sname_loc The location of \a sname.
 * @return Returns `true` only if \a sname contains an error.
 *
 * @sa c_sname_warn()
 */
NODISCARD
bool c_sname_error( c_sname_t const *sname, c_loc_t const *sname_loc );

/**
 * If the local scope-type of \a sname is #TB_namespace, make all scope-types
 * of all enclosing scopes that are either #TB_NONE or #TB_SCOPE also be
 * #TB_namespace since a namespace can only nest within another namespace.
 *
 * @param sname The scoped name to fill in namespaces.
 *
 * @note If there are scope-types that are something other than either #TB_NONE
 * or #TB_SCOPE, this is an error and will be caught by c_sname_check().
 */
void c_sname_fill_in_namespaces( c_sname_t *sname );

/**
 * Gets the first scope-type of \a sname (which is the type of the outermost
 * scope).
 *
 * @param sname The scoped name to get the first scope-type of.
 * @return Returns the first scope-type or #T_NONE if \a sname is empty.
 *
 * @sa c_sname_local_type()
 * @sa c_sname_scope_type()
 * @sa c_sname_set_first_type()
 */
NODISCARD C_SNAME_H_INLINE
c_type_t const* c_sname_first_type( c_sname_t const *sname ) {
  return c_sname_empty( sname ) ? &T_NONE : &c_scope_data( sname->head )->type;
}

/**
 * Frees all memory associated with \a sname _including_ \a sname itself.
 *
 * @param sname The scoped name to free.  If NULL, does nothing.
 *
 * @sa c_sname_cleanup()
 * @sa c_sname_init()
 * @sa c_sname_init_name()
 * @sa c_sname_list_cleanup()
 */
void c_sname_free( c_sname_t *sname );

/**
 * Gets the fully scoped name of \a sname.
 *
 * @param sname The scoped name to get the full name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty or NULL.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_english()
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name()
 * @sa c_sname_scope_sname()
 */
NODISCARD
char const* c_sname_full_name( c_sname_t const *sname );

/**
 * Initializes \a sname.
 *
 * @param sname The scoped name to initialize.
 *
 * @note This need not be called for either global or `static` scoped names.
 *
 * @sa c_sname_cleanup()
 * @sa c_sname_free()
 * @sa c_sname_init_name()
 * @sa c_sname_move()
 */
C_SNAME_H_INLINE
void c_sname_init( c_sname_t *sname ) {
  slist_init( sname );
}

/**
 * Initializes \a sname with \a name.
 *
 * @param sname The scoped name to initialize.
 * @param name The name to set to.  Ownership is taken.
 *
 * @sa c_sname_cleanup()
 * @sa c_sname_free()
 * @sa c_sname_init()
 */
C_SNAME_H_INLINE
void c_sname_init_name( c_sname_t *sname, char const *name ) {
  slist_init( sname );
  c_sname_append_name( sname, name );
}

/**
 * Gets whether \a sname is a constructor name, i.e., whether the last two
 * names match, for example `S::T::T`.
 *
 * @note This can also be used to check for destructor names since **cdecl**
 * elides the `~` when parsing them.  (An AST's kind is #K_DESTRUCTOR.)
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname has at least two names and the last
 * two names match.
 */
NODISCARD
bool c_sname_is_ctor( c_sname_t const *sname );

/**
 * Cleans-up all memory associated with \a list but does _not_ free \a list
 * itself.
 *
 * @param list The list of scoped names to clean up.  If NULL, does nothing;
 * otherwise, reinitializes \a list upon completion.
 *
 * @sa c_sname_cleanup()
 * @sa c_sname_free()
 */
void c_sname_list_cleanup( slist_t *list );

/**
 * Gets the local name of \a sname (which is the name of the last scope), for
 * example the local name of `S::T::x` is `x`.
 *
 * @param sname The scoped name to get the local name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty or NULL.
 *
 * @sa c_sname_full_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name()
 * @sa c_sname_scope_sname()
 */
NODISCARD
char const* c_sname_local_name( c_sname_t const *sname );

/**
 * Gets the local scope-type of \a sname (which is the type of the innermost
 * scope).
 *
 * @param sname The scoped name to get the local scope-type of.
 * @return Returns the local scope-type or #T_NONE if \a sname is empty.
 *
 * @sa c_sname_first_type()
 * @sa c_sname_scope_type()
 * @sa c_sname_set_local_type()
 */
NODISCARD C_SNAME_H_INLINE
c_type_t const* c_sname_local_type( c_sname_t const *sname ) {
  return c_sname_empty( sname ) ? &T_NONE : &c_scope_data( sname->tail )->type;
}

/**
 * Checks whether \a sname matches \a sglob where \a sglob is glob-like in that
 * `*` matches zero or more characters; however, `*` matches only within a
 * single scope.  Examples:
 *
 *  + `foo*` matches all names starting with `foo` in the global scope.
 *
 *  + `s::&zwj;*foo` matches all names ending with `foo` only within the top-
 *    level scope `s`.
 *
 *  + `s*::&zwj;foo` matches all names equal to `foo` in all top-level scopes
 *    starting with `s`.
 *
 *  + `s::*::&zwj;foo` matches all names equal to `foo` in any scope within the
 *    top-level scope `s`.
 *
 * Additionally, a leading `**` is used to match within any scope.  Examples:
 *
 *  + `**::&zwj;foo` matches all names equal to `foo` in any scope.
 *
 * @param sname The scoped name to match against.
 * @param sglob The scoped glob to match.
 * @return Returns `true` only if \a sname matches \a sglob.
 */
NODISCARD
bool c_sname_match( c_sname_t const *sname, c_sglob_t const *sglob );

/**
 * Reinitializes \a sname and returns its former value so that it can be
 * "moved" into another scoped name via assignment.  For example:
 *
 * ```c
 * c_sname_t new_sname = c_sname_move( old_sname );
 * ```
 *
 * @remarks In many cases, a simple assignment would be fine; however, if
 * there's code that modifies `old_sname` afterwards, it would interfere with
 * `new_sname` since both point to the same underlying data.
 *
 * @param sname The scoped name to move.
 * @return Returns the former value of \a sname.
 *
 * @warning The recipient scoped name _must_ be either uninitialized or empty.
 *
 * @sa c_sname_init()
 * @sa c_sname_set()
 */
NODISCARD C_SNAME_H_INLINE
c_sname_t c_sname_move( c_sname_t *sname ) {
  return slist_move( sname );
}

/**
 * Gets the name at \a roffset of \a sname.
 *
 * @param sname The scoped name to get the name at \a roffset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a roffset or the empty string if \a roffset
 * &ge; c_sname_count().
 *
 * @sa c_sname_full_name()
 * @sa c_sname_local_name()
 * @sa c_sname_scope_name()
 * @sa c_sname_scope_sname()
 */
NODISCARD C_SNAME_H_INLINE
char const* c_sname_name_atr( c_sname_t const *sname, size_t roffset ) {
  c_scope_data_t const *const data = slist_atr( sname, roffset );
  return data != NULL ? data->name : "";
}

/**
 * Parses a scoped name, for example `a::b::c`.
 *
 * @param s The string to parse.
 * @param rv_sname The scoped name to parse into.
 * @return Returns the number of characters of \a s that were successfully
 * parsed.
 *
 * @sa c_sname_parse_dtor()
 */
NODISCARD
size_t c_sname_parse( char const *s, c_sname_t *rv_sname );

/**
 * Parses a scoped destructor name, for example `S::T::~T`.
 *
 * @param s The string to parse.
 * @param rv_sname The scoped name to parse into.
 * @return Returns `true` only if the scoped destructor name was successfully
 * parsed.
 *
 * @sa c_sname_parse()
 */
NODISCARD
bool c_sname_parse_dtor( char const *s, c_sname_t *rv_sname );

/**
 * Prepends \a src onto the beginning of \a dst.
 *
 * @param dst The scoped name to prepend to.
 * @param src The name to prepend.  Ownership is taken.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_append_sname()
 */
C_SNAME_H_INLINE
void c_sname_prepend_sname( c_sname_t *dst, c_sname_t *src ) {
  slist_push_list_front( dst, src );
}

/**
 * Gets just the scope name of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns the empty string.
 *
 * @param sname The scoped name to get the scope name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty, NULL, or
 * not within a scope.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_full_name()
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_sname()
 */
NODISCARD
char const* c_sname_scope_name( c_sname_t const *sname );

/**
 * Gets just the scope sname of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns an empty scoped name.
 *
 * @param sname The scoped name to get the scope name of; may be NULL.
 * @return Returns said scoped name or an empty scoped name if \a sname is
 * empty, NULL, or not within a scope.
 *
 * @sa c_sname_full_name()
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name()
 */
NODISCARD
c_sname_t c_sname_scope_sname( c_sname_t const *sname );

/**
 * Gets the scope scope-type of \a sname (which is the type of the next
 * innermost scope).
 *
 * @param sname The scoped name to get the scope scope-type of.
 * @return Returns the scope-type #T_NONE if \a sname is empty or not within a
 * scope.
 *
 * @sa c_sname_local_type()
 * @sa c_sname_set_scope_type()
 */
NODISCARD C_SNAME_H_INLINE
c_type_t const* c_sname_scope_type( c_sname_t const *sname ) {
  c_scope_data_t const *const data = slist_atr( sname, 1 );
  return data != NULL ? &data->type : &T_NONE;
}

/**
 * Sets \a dst_sname to \a src_sname.
 *
 * @param dst_sname The scoped name to set.
 * @param src_sname The scoped name to set \a dst_sname to. Ownership is taken.
 *
 * @note If \a dst_sname `==` \a src_name, does nothing.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_append_sname()
 * @sa c_sname_move()
 * @sa c_sname_prepend_sname()
 */
void c_sname_set( c_sname_t *dst_sname, c_sname_t *src_sname );

/**
 * Sets all the scope-types (except that of the local scope) of \a sname to the
 * types found in `typedef`'s names.
 *
 * For example, given:
 *
 *      class C { struct S; };
 *
 * and an \a sname of `C::S::x`, set `C`'s scope-type to #TB_class and `S`'s
 * scope-type to #TB_struct; the scope-type of `x` is not changed.
 *
 * If there is no `typedef` for a partial scoped name, then that scope-type is
 * set to \a default_type.
 *
 * As a special case, if the first scope's name is `std`, sets that scope-type
 * to #TB_namespace.
 *
 * @param sname The scoped name to set all the scope-types of.
 * @param default_type The default type.  If NULL, then the default type is
 * #TB_SCOPE.
 *
 * @sa c_sname_set_first_type()
 * @sa c_sname_set_local_type()
 */
void c_sname_set_all_types( c_sname_t *sname, c_type_t const *default_type );

/**
 * Sets the first scope-type of \a sname (which is the type of the outermost
 * scope).
 *
 * @param sname The scoped name to set the first scope-type of.
 * @param type The type.
 *
 * @sa c_sname_first_type()
 * @sa c_sname_set_all_types()
 * @sa c_sname_set_local_type()
 * @sa c_sname_set_scope_type()
 */
C_SNAME_H_INLINE
void c_sname_set_first_type( c_sname_t *sname, c_type_t const *type ) {
  c_scope_data( sname->head )->type = *type;
}

/**
 * Sets the local scope-type of \a sname (which is the type of the innermost
 * scope).
 *
 * @param sname The scoped name to set the local scope-type of.
 * @param type The type.
 *
 * @sa c_sname_local_type()
 * @sa c_sname_set_all_types()
 * @sa c_sname_set_first_type()
 * @sa c_sname_set_scope_type()
 */
C_SNAME_H_INLINE
void c_sname_set_local_type( c_sname_t *sname, c_type_t const *type ) {
  c_scope_data( sname->tail )->type = *type;
}

/**
 * Sets the scope scope-type of \a sname (which is the type of the next
 * innermost scope) or does nothing if \a sname has no scope.
 *
 * @param sname The scoped name to set the scope scope-type of.
 * @param type The type.
 *
 * @sa c_sname_scope_type()
 * @sa c_sname_set_first_type()
 * @sa c_sname_set_local_type()
 */
C_SNAME_H_INLINE
void c_sname_set_scope_type( c_sname_t *sname, c_type_t const *type ) {
  c_scope_data_t *const data = slist_atr( sname, 1 );
  if ( data != NULL )
    data->type = *type;
}

/**
 * Checks a scoped name for warnings.
 *
 * @param sname The scoped name to check.
 * @param sname_loc The location of \a sname.
 *
 * @sa c_name_error()
 */
void c_sname_warn( c_sname_t const *sname, c_loc_t const *sname_loc );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_sname_H */
/* vim:set et sw=2 ts=2: */
