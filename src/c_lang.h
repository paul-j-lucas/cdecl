/*
**      cdecl -- C gibberish translator
**      src/c_lang.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

#ifndef cdecl_c_lang_H
#define cdecl_c_lang_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ language versions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "options.h"                    /* for opt_lang */
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_LANG_INLINE
# define C_LANG_INLINE _GL_INLINE
#endif /* C_LANG_INLINE */

/// @endcond

/**
 * @defgroup c-lang-group C/C++ Language Versions
 * Constants, types, and functions for C/C++ language versions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// languages supported
#define LANG_NONE     ((c_lang_id_t)0)  /**< No languages. */
#define LANG_ANY      ((c_lang_id_t)~0) /**< Any supported language. */

#define LANG_C_OLD    LANG_C_KNR        /**< Oldest supported C language. */
#define LANG_C_KNR    (1u << 0)         /**< K&R (pre-ANSI) C. */
#define LANG_C_89     (1u << 1)         /**< C 89 (first ANSI C). */
#define LANG_C_95     (1u << 2)         /**< C 95. */
#define LANG_C_99     (1u << 3)         /**< C 99. */
#define LANG_C_11     (1u << 4)         /**< C 11. */
#define LANG_C_17     (1u << 5)         /**< C 17. */
#define LANG_C_2X     (1u << 6)         /**< C 2X. */
#define LANG_C_NEW    LANG_C_2X         /**< Newest supported C language. */
#define LANG_C_ANY    LANG_MAX(C_NEW)   /**< Any C language. */

#define LANG_CPP_OLD  LANG_CPP_98       /**< Oldest supported C++ language. */
#define LANG_CPP_98   (1u << 9)         /**< C++ 98. */
#define LANG_CPP_03   (1u << 10)        /**< C++ 03. */
#define LANG_CPP_11   (1u << 11)        /**< C++ 11. */
#define LANG_CPP_14   (1u << 12)        /**< C++ 14. */
#define LANG_CPP_17   (1u << 13)        /**< C++ 17. */
#define LANG_CPP_20   (1u << 14)        /**< C++ 20. */
#define LANG_CPP_NEW  LANG_CPP_20       /**< Newest supported C++ language. */
#define LANG_CPP_ANY  LANG_MASK_CPP     /**< Any C++ language. */

/**< Language eXtensions for Embedded C. */
#define LANGX_EMC     (1u << 7)

/**< Language eXtensions for Unified Parallel C. */
#define LANGX_UPC     (1u << 8)

/**
 * Embedded C, or more formally, "Programming languages - C - Extensions to
 * support embedded processors," ISO/IEC TR 18037:2008, which is based on C99,
 * ISO/IEC 9899:1999.
 *
 * @note
 * This is not a distinct language in cdecl, i.e., the user can't set the
 * language to "Embedded C" specifically.  It's used to mark keywords as being
 * available only in the Embedded C extensions to C99 instead of "plain" C99 so
 * that if a user does:
 *
 *      cdecl> declare _Sat as int
 *      9: warning: "_Sat" is a keyword in C99 (with Embedded C extensions)
 *
 * in a language other than C99, they'll get a warning.
 */
#define LANG_C_99_EMC (LANG_C_99 | LANGX_EMC)

/**
 * UPC: Unified Parallel [extension to] C, which is based on C99, ISO/IEC
 * 9899:1999.
 */
#define LANG_C_99_UPC (LANG_C_99 | LANGX_UPC)

// bit masks
#define LANG_MASK_C   0x01FFu           /**< C languages bitmask. */
#define LANG_MASK_CPP 0xFE00u           /**< C++ languages bitmask. */
#define LANGX_MASK    0x0180u           /**< Language extensions bitmask. */

/**
 * All languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 */
#define LANG_MAX(L)               (LANG_ ## L | (LANG_ ## L - 1u))

/**
 * All languages \a L and later.
 *
 * @param L The language _without_ the `LANG_` prefix.
 */
#define LANG_MIN(L)               (~(LANG_ ## L - 1u))

/**
 * C-only languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 */
#define LANG_C_MAX(L)             LANG_MAX( C_ ## L )

/**
 * C-only languages \a L and later.
 *
 * @param L The language _without_ the `LANG_` prefix.
 */
#define LANG_C_MIN(L)             (LANG_MIN( C_ ## L ) & LANG_MASK_C)

/**
 * C++-only languages up to and including \a L.
 *
 * @param L The language _without_ the `LANG_` prefix.
 */
#define LANG_CPP_MAX(L)           (LANG_MAX( CPP_ ## L ) & LANG_MASK_CPP)

/**
 * C++-only languages \a L and later.
 *
 * @param L The language _without_ the `LANG_` prefix.
 */
#define LANG_CPP_MIN(L)           LANG_MIN( CPP_ ## L )

/**
 * C-only languages up to and including \a CL; and C++-only languages up to and
 * including \a CPPL.
 *
 * @param CL The C language _without_ the `LANG_` prefix.
 * @param CPPL The C++ language _without_ the `LANG_` prefix.
 */
#define LANG_C_CPP_MAX(CL,CPPL)   (LANG_C_MAX(CL) | LANG_CPP_MAX(CPPL))

/**
 * C-only languages \a CL and later; and C++-only languages \a CPPL and later.
 *
 * @param CL The C language _without_ the `LANG_` prefix.
 * @param CPPL The C++ language _without_ the `LANG_` prefix.
 */
#define LANG_C_CPP_MIN(CL,CPPL)   (LANG_C_MIN(CL) | LANG_CPP_MIN(CPPL))

/**
 * C/C++ language(s)/literal pairs: for the given language(s) only, use the
 * given literal.  This allows different languages to use different literals,
 * e.g., `_Noreturn` for C and `noreturn` for C++.
 */
struct c_lang_lit {
  c_lang_id_t   lang_ids;               ///< Language(s) literal is in.
  char const   *literal;                ///< The literal.
};

/**
 * Convenience macro for specifying a constant array of
 * <code>\ref c_lang_lit</code>.
 *
 * @param ... The array of <code>\ref c_lang_lit</code> elements.
 */
#define C_LANG_LIT(...)           (c_lang_lit_t const[]){ __VA_ARGS__ }

/**
 * Shorthand for the common case of getting whether the current language is
 * among the languages specified by \a LANG_MACRO.
 *
 * @param LANG_MACRO A `LANG_*` macro without the `LANG_` prefix.
 * @return Returns `true` only if the current language is among the languages
 * specified by \a LANG_MACRO.
 */
#define OPT_LANG_IS(LANG_MACRO) \
  ((opt_lang & LANG_ ## LANG_MACRO) != LANG_NONE)

///////////////////////////////////////////////////////////////////////////////

/**
 * A mapping between a language name and its corresponding `c_lang_id_t`.
 */
struct c_lang {
  char const   *name;                   ///< Language name.
  bool          is_alias;               ///< Alias for another language name?
  c_lang_id_t   lang_id;                ///< Language bit.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets all the language(s) \a lang_id and newer.
 *
 * @param lang_id The language.  Exactly one language must be set.
 * @return Returns the bitwise-or of all language(s) \a lang_id and newer.
 *
 * @sa c_lang_oldest()
 * @sa c_lang_newer()
 * @sa c_lang_newest()
 * @sa opt_lang_and_newer()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
c_lang_id_t c_lang_and_newer( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( exactly_one_bit_set( lang_id ) );
  return BITS_GE( lang_id );
}

/**
 * Gets the <code>\ref c_lang_id_t</code> corresponding to the given string
 * (case insensitive).
 *
 * @param name The language name to get the corresponding <code>\ref
 * c_lang_id_t</code> for.
 * @return Returns said language or #LANG_NONE if \a name doesn't correspond to
 * any supported language.
 */
PJL_WARN_UNUSED_RESULT
c_lang_id_t c_lang_find( char const *name );

/**
 * Gets whether \a lang_ids is any version of C.
 *
 * @param lang_ids The bitwise-or of language(s) to check.
 * @return Returns `true` only if any one of \a lang_ids is a version of C.
 *
 * @sa c_lang_is_cpp()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
bool c_lang_is_c( c_lang_id_t lang_ids ) {
  return (lang_ids & LANG_MASK_C) != LANG_NONE;
}

/**
 * Gets whether \a lang_ids is any version of C++.
 *
 * @param lang_ids The bitwise-of of language(s) to check.
 * @return Returns `true` only if any one of \a lang_ids is a version of C++.
 *
 * @sa c_lang_is_c()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
bool c_lang_is_cpp( c_lang_id_t lang_ids ) {
  return (lang_ids & LANG_MASK_CPP) != LANG_NONE;
}

/**
 * Gets the "coarse" name of \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return If:
 *  + \a lang_ids contains any version of both C and C++, returns NULL;
 *    otherwise:
 *  + \a lang_ids contains any version of C, returns `"C"`.
 *  + \a lang_ids contains any version of C++, returns `"C++"`.
 *
 * @sa c_lang_name()
 * @sa c_lang_oldest_name()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
char const* c_lang_coarse_name( c_lang_id_t lang_ids ) {
  return c_lang_is_c( lang_ids ) ?
    (c_lang_is_cpp( lang_ids ) ? NULL : "C") :
    "C++";
}

/**
 * Gets the literal appropriate for the current language.
 *
 * @param lang_lit A <code>\ref c_lang_lit</code> array.  The last element
 * _must_ always have a `lang_ids` value of #LANG_ANY.  If the corresponding
 * `literal` value is NULL, it means there is no appropriate literal for the
 * current language.
 * @return Returns said literal or NULL if there is no appropriate literal for
 * the current language.
 */
PJL_WARN_UNUSED_RESULT
char const* c_lang_literal( c_lang_lit_t const *lang_lit );

/**
 * Gets the printable name of \a lang_id.
 *
 * @param lang_id The language to get the name of.  Exactly one language must
 * be set.
 * @return Returns said name.
 *
 * @sa c_lang_coarse_name()
 * @sa c_lang_oldest_name()
 */
PJL_WARN_UNUSED_RESULT
char const* c_lang_name( c_lang_id_t lang_id );

/**
 * Gets the bitwise-or of language(s) that are newer than \a lang_id.
 *
 * @param lang_id The language.  Exactly one language must be set.
 * @return Returns the bitwise-or of languages \a lang_id or newer; or
 * #LANG_NONE if no language(s) are newer.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newest()
 * @sa c_lang_oldest()
 * @sa opt_lang_newer()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
c_lang_id_t c_lang_newer( c_lang_id_t lang_id ) {
  lang_id &= ~LANGX_MASK;
  assert( exactly_one_bit_set( lang_id ) );
  return BITS_GT( lang_id );
}

/**
 * Gets the newest language among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns said language.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newer()
 * @sa c_lang_oldest()
 * @sa c_lang_oldest_name()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
c_lang_id_t c_lang_newest( c_lang_id_t lang_ids ) {
  return ms_bit1_32( lang_ids & ~LANGX_MASK );
}

/**
 * Iterates to the next C/C++ language.
 *
 * @param lang A pointer to the previous language. For the first iteration,
 * NULL should be passed.
 * @return Returns the next C/C++ language or NULL for none.
 *
 * @sa #FOREACH_LANG()
 */
PJL_WARN_UNUSED_RESULT
c_lang_t const* c_lang_next( c_lang_t const *lang );

/**
 * Convenience macro for iterating over all languages.
 *
 * @param LANG The `c_lang` loop variable.
 *
 * @sa c_lang_next()
 */
#define FOREACH_LANG(LANG) \
  for ( c_lang_t const *LANG = NULL; (LANG = c_lang_next( LANG )) != NULL; )

/**
 * Gets the oldest language among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns said language.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newer()
 * @sa c_lang_newest()
 * @sa c_lang_oldest_name()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
c_lang_id_t c_lang_oldest( c_lang_id_t lang_ids ) {
  return ls_bit1_32( lang_ids & ~LANGX_MASK );
}

/**
 * Gets the printable name of the oldest language among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns said name.
 *
 * @sa c_lang_coarse_name()
 * @sa c_lang_name()
 * @sa c_lang_oldest()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
char const* c_lang_oldest_name( c_lang_id_t lang_ids ) {
  return c_lang_name( c_lang_oldest( lang_ids ) );
}

/**
 * Sets the current language and the corresponding prompt.
 *
 * @param lang_id The language to set.  Exactly one language must be set.
 */
void c_lang_set( c_lang_id_t lang_id );

/**
 * Gets a string specifying when a particular language feature won't be legal
 * until or has been illegal since, if ever.  It is presumed to follow `"...
 * not supported"` (with no trailing space).
 *
 * @param lang_ids The bitwise-or of legal language(s).
 * @return If:
 *  + \a lang_ids is #LANG_NONE, returns the empty string.
 *  + The current language is C and \a lang_ids does not contain any version of
 *    C, returns `" in C"`.
 *  + The current language is C++ and \a lang_ids does not contain any version
 *    of C++, returns `" in C++"`.
 *  + If the current language is older than oldest language in \a lang_ids,
 *    returns `" until "` followed by the name of the oldest C version (if the
 *    current language is C) or the name of the oldest C++ version (if the
 *    current language is C++).
 *  + Otherwise returns `" since "` followed by the name of the newest C
 *    version (if the current language is C) or the name of the newest C++
 *    version (if the current language is C++).
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 */
char const* c_lang_which( c_lang_id_t lang_ids );

/**
 * Convenience function for calling c_lang_and_newer() with opt_lang.
 *
 * @return Returns the bitwise-or all language(s) opt_lang and newer.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newer()
 * @sa c_lang_newest()
 * @sa opt_lang_newer()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
c_lang_id_t opt_lang_and_newer( void ) {
  return c_lang_and_newer( opt_lang );
}

/**
 * Convenience function for calling c_lang_newer() with opt_lang.
 *
 * @return Returns the bitwise-or of languages \a lang_id or newer; or
 * #LANG_NONE if no language(s) are newer.
 *
 * @sa c_lang_and_newer()
 * @sa c_lang_newer()
 * @sa c_lang_newest()
 * @sa opt_lang_and_newer()
 */
C_LANG_INLINE PJL_WARN_UNUSED_RESULT
c_lang_id_t opt_lang_newer( void ) {
  return c_lang_newer( opt_lang );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_lang_H */
/* vim:set et sw=2 ts=2: */
