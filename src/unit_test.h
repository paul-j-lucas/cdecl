/*
**      PJL Library
**      src/unit_test.h
**
**      Copyright (C) 2021-2025  Paul J. Lucas
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

#ifndef pjl_unit_test_H
#define pjl_unit_test_H

// standard
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wunused-value"

/**
 * @defgroup unit-test-group Unit Tests
 * Macros, variables, and functions for unit-test programs.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Tests \a EXPR and prints that it failed only if it failed.
 *
 * @param EXPR The expression to evaluate.
 * @return Returns `true` only if \a EXPR is non-zero; `false` only if zero.
 */
#define TEST(EXPR)                ( !!(EXPR) || TEST_FAILED( #EXPR ) )

/**
 * Prints that \a EXPR failed and increments the test failure count.
 *
 * @param EXPR The stringified expression that failed.
 * @return Always returns `false`.
 */
#define TEST_FAILED(EXPR) \
  ( fprintf( stderr, "%s:%d: " EXPR "\n", me, __LINE__ ), !++test_failures )

/**
 * Begins a test function.
 *
 * @remarks This should be the first thing inside a test function that must be
 * declared to return `bool`.
 *
 * @sa #TEST_FUNC_END()
 */
#define TEST_FUNC_BEGIN() \
  unsigned const test_failures_start = test_failures

/**
 * Ends a test function.
 *
 * @remarks This should be the last thing inside a test function.
 *
 * @sa #TEST_FUNC_BEGIN()
 */
#define TEST_FUNC_END() \
  return test_failures == test_failures_start

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern char const  *me;                 ///< Program name.
extern unsigned     test_failures;      ///< Test failure count.

/**
 * Initializes a unit-test program.
 *
 * @note This function must be called exactly once.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 */
void test_prog_init( int argc, char const *const argv[] );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* pjl_unit_test_H */
/* vim:set et sw=2 ts=2: */
