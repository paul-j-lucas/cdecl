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
#define TEST(EXPR)                test_expr( !!(EXPR), #EXPR, __LINE__ )

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
extern unsigned     test_failures;      ///< Test failure count.

/**
 * Helper function for the #TEST() macro that, if \a expr_is_true is `false`,
 * prints that the test failed and increments the failure count.
 *
 * @param expr_is_true True only if the expression for the test evaluated to
 * `true`.
 * @param expr The stringified expression.
 * @param line The line number of the test.
 * @return Returns \a expr_is_true.
 *
 * @note This function isn't normally called directly; use the #TEST() macro
 * instead.
 *
 * @sa #TEST()
 */
bool test_expr( bool expr_is_true, char const *expr, int line );

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
