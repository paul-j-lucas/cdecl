/*
**      cdecl -- C gibberish translator
**      src/decl_flags.h
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

#ifndef cdecl_decl_flags_H
#define cdecl_decl_flags_H

/**
 * @file
 * Declares macros for both denoting how a type was declared and how to print
 * a declaration or cast either in pseudo-English or gibberish (aka, a C/C++
 * declaration).
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * @ingroup printing-english-group
 * @defgroup english-flags English Flags
 * Flags for c_ast_english() that control how pseudo-English is printed.
 *
 * @sa \ref gibberish-flags
 * @{
 */

/**
 * For \ref c_typedef::decl_flags "decl_flags", denotes that a type was
 * declared via pseudo-English.
 *
 * @sa #C_GIB_TYPEDEF
 * @sa #C_GIB_USING
 */
#define C_ENG_DECL                (1u << 0)

/**
 * Flag for c_ast_english() to omit the `declare` _name_ `as` part and print
 * only the type in pseudo-English.
 *
 * @sa c_ast_english()
 */
#define C_ENG_OPT_OMIT_DECLARE    (1u << 1)

/**
 * Pseudo-English-only declaration flags.
 *
 * @sa #C_GIB_ANY
 */
#define C_ENG_ANY                 0x00FFu

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @ingroup printing-gibberish-group
 * @defgroup gibberish-flags Gibberish Flags
 * Flags for c_ast_gibberish() and c_typedef_gibberish() that control how
 * gibberish is printed.
 *
 * @sa \ref english-flags
 * @{
 */

/**
 * Flag for c_ast_gibberish() to print as a C/C++ cast instead of a
 * declaration.
 *
 * @note May _not_ be used in combination with any other flags.
 *
 * @sa c_ast_gibberish()
 * @sa #C_GIB_PRINT_DECL
 */
#define C_GIB_PRINT_CAST          (1u << 8)

/**
 * Flag for c_ast_gibberish() to print as an ordinary declaration instead of a
 * `typedef` or `using` declaration or C/C++ cast.
 *
 * @note May be used _only_ in combination with:
 *  + #C_GIB_OPT_MULTI_DECL
 *  + #C_GIB_OPT_OMIT_TYPE
 *  + #C_GIB_OPT_SEMICOLON
 *
 * @sa c_ast_gibberish()
 * @sa #C_GIB_OPT_MULTI_DECL
 * @sa #C_GIB_OPT_OMIT_TYPE
 * @sa #C_GIB_PRINT_CAST
 * @sa #C_GIB_TYPEDEF
 * @sa #C_GIB_USING
 */
#define C_GIB_PRINT_DECL          (1u << 9)

/**
 * Flag for c_ast_gibberish() to indicate that the declaration is of multiple
 * objects for the same base type, for example:
 *
 *      int *x, *y;
 *
 * @note Unlike #C_GIB_OPT_OMIT_TYPE, `C_GIB_OPT_MULTI_DECL` _must_ be used for
 * the entire declaration.
 *
 * @note May be used _only_ in combination with:
 *  + #C_GIB_OPT_OMIT_TYPE
 *  + #C_GIB_PRINT_DECL
 *
 * @sa c_ast_gibberish()
 * @sa #C_GIB_OPT_OMIT_TYPE
 * @sa #C_GIB_PRINT_DECL
 */
#define C_GIB_OPT_MULTI_DECL      (1u << 11)

/**
 * Flag for c_ast_gibberish() to omit the type name when printing gibberish for
 * the _second_ and subsequent objects when printing multiple objects in the
 * same declaration.  For example, when printing:
 *
 *      int *x, *y;
 *
 * the gibberish for `y` _must not_ print the `int` again.
 *
 * @note May be used _only_ in combination with:
 *  + #C_GIB_OPT_MULTI_DECL
 *  + #C_GIB_PRINT_DECL
 *
 * @sa c_ast_gibberish()
 * @sa #C_GIB_OPT_MULTI_DECL
 * @sa #C_GIB_PRINT_DECL
 */
#define C_GIB_OPT_OMIT_TYPE       (1u << 12)

/**
 * Flag for c_ast_gibberish() or c_typedef_gibberish() to print the final
 * semicolon after a type declaration.
 *
 * @note May be used in combination with any other `C_GIB_*` flags _except_
 * #C_GIB_PRINT_CAST.
 *
 * @sa c_ast_gibberish()
 * @sa c_typedef_gibberish()
 */
#define C_GIB_OPT_SEMICOLON       (1u << 10)

/**
 * Dual purpose:
 *
 *  1. For \ref c_typedef::decl_flags "decl_flags", denotes that a type was
 *     declared via a `typedef` declaration (as opposed to a `using`
 *     declaration).
 *
 *  2. When printing gibberish, c_typedef_gibberish() will print as a `typedef`
 *     declaration (as opposed to a `using` declaration).
 *
 * @note May be used _only_ in combination with #C_GIB_OPT_SEMICOLON.
 *
 * @sa c_typedef_gibberish()
 * @sa #C_GIB_USING
 */
#define C_GIB_TYPEDEF             (1u << 13)

/**
 * Dual purpose:
 *
 *  1. For \ref c_typedef::decl_flags "decl_flags", denotes that a type was
 *     declared via a `using` declaration (as opposed to a `typedef`
 *     declaration).
 *
 *  2. When printing gibberish:
 *
 *      + c_ast_gibberish() will print only the right-hand side of a `using`
 *        declaration (the type).
 *
 *      + c_typedef_gibberish() will print as a whole `using` declaration.
 *
 *     For example, given:
 *
 *          using RI = int&
 *
 *     then:
 *
 *      + c_ast_gibberish() will print only `int&` whereas:
 *      + c_typedef_gibberish() will print `using RI = int&`.
 *
 * @note When used for the second purpose, may be used _only_ in combination
 * with #C_GIB_OPT_SEMICOLON.
 *
 * @sa c_ast_gibberish()
 * @sa #C_ENG_DECL
 * @sa #C_GIB_TYPEDEF
 * @sa c_typedef_gibberish()
 * @sa print_ast_type_aka()
 */
#define C_GIB_USING               (1u << 14)

/**
 * Gibberish-only declaration flags.
 *
 * @sa #C_ENG_ANY
 */
#define C_GIB_ANY                 0xFF00u

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * Convenience macro that is the bitwise-or of the two ways in which a type
 * can be declared in gibberish:
 *
 *  1. Via `typedef`.
 *  2. Via `using`.
 *
 * @sa #C_ENG_DECL
 * @sa #C_TYPE_DECL_ANY
 */
#define C_GIB_DECL_ANY            ( C_GIB_TYPEDEF | C_GIB_USING )

/**
 * Convenience macro that is the bitwise-or of the three ways in which a type
 * can be declared:
 *
 *  1. Pseudo-English.
 *  2. Gibberish via `typedef`.
 *  3. Gibberish via `using`.
 *
 * @sa #C_ENG_DECL
 * @sa #C_GIB_DECL_ANY
 */
#define C_TYPE_DECL_ANY           ( C_ENG_DECL | C_GIB_DECL_ANY )

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_decl_flags_H */
/* vim:set et sw=2 ts=2: */
