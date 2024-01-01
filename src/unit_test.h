/*
**      cdecl -- C gibberish translator
**      src/unit_test.h
**
**      Copyright (C) 2021-2024  Paul J. Lucas
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

#ifndef cdecl_unit_test_H
#define cdecl_unit_test_H

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-value"
#endif /* __GNUC__ */

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints that \a EXPR failed and increments the test failure count.
 *
 * @param EXPR The stringified expression that failed.
 * @return Always returns `false`.
 */
#define FAILED(EXPR) \
  ( EPRINTF( "%s:%d: " EXPR "\n", me, __LINE__ ), !++test_failures )

/**
 * Tests \a EXPR and prints that it failed only if it failed.
 *
 * @param EXPR The expression to evaluate.
 * @return Returns `true` only if \a EXPR is non-zero; `false` only if zero.
 */
#define TEST(EXPR)                ( !!(EXPR) || FAILED( #EXPR ) )

/**
 * Begins a test function.
 *
 * @remarks This should be the first thing inside a test function that must be
 * declared to return `bool`.
 *
 * @sa #TEST_FN_END()
 */
#define TEST_FN_BEGIN() \
  unsigned const test_failures_start = test_failures

/**
 * Ends a test function.
 *
 * @remarks This should be the last thing inside a test function.
 *
 * @sa #TEST_FN_BEGIN()
 */
#define TEST_FN_END() \
  return test_failures == test_failures_start

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_unit_test_H */
/* vim:set et sw=2 ts=2: */
