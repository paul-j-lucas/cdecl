/*
**      cdecl -- C gibberish translator
**      src/util.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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

#ifndef cdecl_util_H
#define cdecl_util_H

/**
 * @file
 * Declares utility constants, macros, and functions.
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */
#ifndef NDEBUG
#include "bit_util.h"
#endif /* NDEBUG */
#include "type_traits.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>                     /* for CHAR_BIT */
#include <stdbool.h>
#include <stdint.h>                     /* for uint*_t */
#include <stdio.h>                      /* for FILE */
#include <stdlib.h>                     /* for atexit(3) */
#include <string.h>                     /* for strspn(3) */
#include <sysexits.h>
#include <time.h>                       /* for strftime(3) */

/// @endcond

/**
 * @defgroup util-group Utility Macros & Functions
 * Utility macros, constants, and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Gets whether the argument(s) contains a comma, that is there are 2 or more
 * arguments.
 *
 * @param ... Zero to 10 arguments, invariably `__VA_ARGS__`.
 * @return Returns `0` for 0 or 1 argument, or `1` for 2 or more arguments.
 */
#define ARGS_HAS_COMMA(...) \
  ARG_11( __VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 )

/// @cond DOXYGEN_IGNORE
#define ARG_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,...) _11
/// @endcond

/**
 * Gets whether there are no arguments.
 *
 * @param ... Zero to 10 arguments, invariably `__VA_ARGS__`.
 * @return Returns `0` for 0 arguments or `1` otherwise.
 *
 * @sa https://stackoverflow.com/a/66556553/99089
 * @sa https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/
 */
#define ARGS_IS_EMPTY(...)                                                  \
  ARGS_IS_EMPTY_CASES(                                                      \
    /*  Case 1: argument with a comma,                                      \
        e.g. "ARG1, ARG2", "ARG1, ...", or ",". */                          \
    ARGS_HAS_COMMA( __VA_ARGS__ ),                                          \
    /*  Case 2: argument within parentheses,                                \
        e.g., "(ARG)", "(...)", or "()". */                                 \
    ARGS_HAS_COMMA( ARGS_IS_EMPTY_COMMA __VA_ARGS__ ),                      \
    /*  Case 3: argument that is a macro that will expand the parentheses,  \
        possibly generating a comma. */                                     \
    ARGS_HAS_COMMA( __VA_ARGS__ () ),                                       \
    /*  Case 4: __VA_ARGS__ doesn't generate a comma by itself, nor with    \
        ARGS_IS_EMPTY_COMMA behind it, nor with () after it.  Therefore,    \
        "ARGS_IS_EMPTY_COMMA __VA_ARGS__ ()" generates a comma only if      \
        __VA_ARGS__ is empty.  So this is the empty __VA_ARGS__ case since  \
        the previous cases are false. */                                    \
    ARGS_HAS_COMMA( ARGS_IS_EMPTY_COMMA __VA_ARGS__ () )                    \
  )

/// @cond DOXYGEN_IGNORE
#define ARGS_IS_EMPTY_CASES(_1,_2,_3,_4) \
  ARGS_HAS_COMMA( NAME5( ARGS_IS_EMPTY_RESULT_, _1, _2, _3, _4 ) )
#define ARGS_IS_EMPTY_COMMA(...)  ,
#define ARGS_IS_EMPTY_RESULT_0001 ,
#define NAME5(A,B,C,D,E)          NAME5_HELPER( A, B, C, D, E )
#define NAME5_HELPER(A,B,C,D,E)   A ## B ## C ## D ## E
/// @endcond

/**
 * Gets a pointer to one element past the last of the given array.
 *
 * @param ARRAY The array to use.
 * @return Returns a pointer to one element past the last of \a ARRAY.
 *
 * @note \a ARRAY _must_ be a statically allocated array.
 *
 * @sa #ARRAY_NEXT()
 * @sa #ARRAY_SIZE()
 * @sa #FOREACH_ARRAY_ELEMENT()
 */
#define ARRAY_END(ARRAY)          ( (ARRAY) + ARRAY_SIZE( (ARRAY) ) )

/**
 * Gets a pointer to either the first or next element of \a ARRAY.
 *
 * @param ARRAY The array to iterate over.
 * @param VAR A pointer to an array element.  It _must_ be NULL initially.
 * Upon return, it is incremented.
 * @return Returns a pointer to the next element of \a ARRAY or NULL if none.
 *
 * @note \a ARRAY _must_ be a statically allocated array.
 *
 * @sa #ARRAY_END()
 * @sa #ARRAY_SIZE()
 * @sa #FOREACH_ARRAY_ELEMENT()
 */
#define ARRAY_NEXT(ARRAY,VAR) \
  ((VAR) == NULL ? (ARRAY) : ++(VAR) < ARRAY_END( (ARRAY) ) ? (VAR) : NULL)

/**
 * Gets the number of elements of the given array.
 *
 * @param ARRAY The array to get the number of elements of.
 * @return Returns the number of elements of \a ARRAY.
 *
 * @note \a ARRAY _must_ be a statically allocated array.
 *
 * @sa #ARRAY_END()
 * @sa #ARRAY_NEXT()
 * @sa #FOREACH_ARRAY_ELEMENT()
 */
#define ARRAY_SIZE(ARRAY) (                                                   \
  STATIC_ASSERT_EXPR( IS_ARRAY_EXPR( (ARRAY) ), #ARRAY " must be an array" )  \
  * sizeof ((ARRAY)) / sizeof 0[ (ARRAY) ] )

/**
 * Like **assert**(3) except can be used in an expression.
 *
 * @param EXPR The expression to evaluate.
 * @return Returns 1 (true) only if \a EXPR is non-zero; if zero, asserts (does
 * not return).
 *
 * @sa #STATIC_ASSERT_EXPR()
 */
#define ASSERT_EXPR(EXPR)         ( assert( (EXPR) ), 1 )

/**
 * Asserts that this line of code is run at most once --- useful in
 * initialization functions that must be called at most once.  For example:
 *
 *      void initialize() {
 *        ASSERT_RUN_ONCE();
 *        // ...
 *      }
 *
 * @sa #RUN_ONCE
 */
#ifndef NDEBUG
# define ASSERT_RUN_ONCE() BLOCK(       \
    static bool UNIQUE_NAME(run_once);  \
    assert( !UNIQUE_NAME(run_once) );   \
    UNIQUE_NAME(run_once) = true; )
#else
# define ASSERT_RUN_ONCE()        NO_OP
#endif /* NDEBUG */

/**
 * Calls **atexit**(3) and checks for failure.
 *
 * @param FN The pointer to the function to call **atexit**(3) with.
 */
#define ATEXIT(FN) \
  PERROR_EXIT_IF( atexit( (FN) ) != 0, EX_OSERR )

/**
 * Embeds the given statements into a compound statement block.
 *
 * @param ... The statement(s) to embed.
 */
#define BLOCK(...)                do { __VA_ARGS__ } while (0)

/**
 * Macro that "char-ifies" its argument, e.g., <code>%CHARIFY(x)</code> becomes
 * `'x'`.
 *
 * @param X The unquoted character to charify.  It can be only in the set
 * `[0-9_A-Za-z]`.
 *
 * @sa #STRINGIFY()
 */
#define CHARIFY(X)                NAME2(CHARIFY_,X)

/// @cond DOXYGEN_IGNORE
#define CHARIFY_0 '0'
#define CHARIFY_1 '1'
#define CHARIFY_2 '2'
#define CHARIFY_3 '3'
#define CHARIFY_4 '4'
#define CHARIFY_5 '5'
#define CHARIFY_6 '6'
#define CHARIFY_7 '7'
#define CHARIFY_8 '8'
#define CHARIFY_9 '9'
#define CHARIFY_A 'A'
#define CHARIFY_B 'B'
#define CHARIFY_C 'C'
#define CHARIFY_D 'D'
#define CHARIFY_E 'E'
#define CHARIFY_F 'F'
#define CHARIFY_G 'G'
#define CHARIFY_H 'H'
#define CHARIFY_I 'I'
#define CHARIFY_J 'J'
#define CHARIFY_K 'K'
#define CHARIFY_L 'L'
#define CHARIFY_M 'M'
#define CHARIFY_N 'N'
#define CHARIFY_O 'O'
#define CHARIFY_P 'P'
#define CHARIFY_Q 'Q'
#define CHARIFY_R 'R'
#define CHARIFY_S 'S'
#define CHARIFY_T 'T'
#define CHARIFY_U 'U'
#define CHARIFY_V 'V'
#define CHARIFY_W 'W'
#define CHARIFY_X 'X'
#define CHARIFY_Y 'Y'
#define CHARIFY_Z 'Z'
#define CHARIFY__ '_'
#define CHARIFY_a 'a'
#define CHARIFY_b 'b'
#define CHARIFY_c 'c'
#define CHARIFY_d 'd'
#define CHARIFY_e 'e'
#define CHARIFY_f 'f'
#define CHARIFY_g 'g'
#define CHARIFY_h 'h'
#define CHARIFY_i 'i'
#define CHARIFY_j 'j'
#define CHARIFY_k 'k'
#define CHARIFY_l 'l'
#define CHARIFY_m 'm'
#define CHARIFY_n 'n'
#define CHARIFY_o 'o'
#define CHARIFY_p 'p'
#define CHARIFY_q 'q'
#define CHARIFY_r 'r'
#define CHARIFY_s 's'
#define CHARIFY_t 't'
#define CHARIFY_u 'u'
#define CHARIFY_v 'v'
#define CHARIFY_w 'w'
#define CHARIFY_x 'x'
#define CHARIFY_y 'y'
#define CHARIFY_z 'z'
/// @endcond

/**
 * C version of C++'s `const_cast`.
 *
 * @param TYPE The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro can't actually implement C++'s `const_cast` because there's
 * no way to do it in C.  It serves merely as a visual cue for the type of cast
 * meant.
 *
 * @sa #POINTER_CAST()
 * @sa #STATIC_CAST()
 */
#define CONST_CAST(TYPE,EXPR)     ((TYPE)(EXPR))

/**
 * Declares an array of \a N objects of type \a TYPE aligned as \a TYPE with an
 * implementation defined name intended to be used inside a `struct`
 * declaration to add padding.
 *
 * @param TYPE The type of the object.
 * @param N The array size.
 */
#define DECL_UNUSED(TYPE,N)       TYPE UNIQUE_NAME(unused)[ (N) ]

/**
 * Shorthand for printing to standard error.
 *
 * @param ... The `printf()` arguments.
 *
 * @sa #EPUTC()
 * @sa #EPUTS()
 * @sa #FPRINTF()
 * @sa #PRINTF()
 * @sa #PUTC()
 * @sa #PUTS()
 */
#define EPRINTF(...)              fprintf( stderr, __VA_ARGS__ )

/**
 * Shorthand for printing a character to standard error.
 *
 * @param C The character to print.
 *
 * @sa #EPRINTF()
 * @sa #EPUTS()
 * @sa #FPUTC()
 * @sa #PRINTF()
 * @sa #PUTC()
 */
#define EPUTC(C)                  fputc( (C), stderr )

/**
 * Shorthand for printing a C string to standard error.
 *
 * @param S The C string to print.
 *
 * @sa #EPRINTF()
 * @sa #EPUTC()
 * @sa #FPUTS()
 * @sa #PRINTF()
 * @sa #PUTS()
 */
#define EPUTS(S)                  fputs( (S), stderr )

/**
 * Calls **ferror**(3) and exits if there was an error on \a STREAM.
 *
 * @param STREAM The `FILE` stream to check for an error.
 *
 * @sa #PERROR_EXIT_IF()
 */
#define FERROR(STREAM) \
  PERROR_EXIT_IF( ferror( (STREAM) ) != 0, EX_IOERR )

/**
 * Calls **fflush(3)** on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to flush.
 *
 * @sa #PERROR_EXIT_IF()
 */
#define FFLUSH(STREAM) \
  PERROR_EXIT_IF( fflush( (STREAM) ) != 0, EX_IOERR )

/**
 * Convenience macro for iterating over the elements of a static array.
 *
 * @param TYPE The type of element.
 * @param VAR The element loop variable.
 * @param ARRAY The array to iterate over.
 *
 * @note \a ARRAY _must_ be a statically allocated array.
 *
 * @sa #ARRAY_END()
 * @sa #ARRAY_NEXT()
 * @sa #ARRAY_SIZE()
 * @sa #FOR_N_TIMES()
 */
#define FOREACH_ARRAY_ELEMENT(TYPE,VAR,ARRAY) \
  for ( TYPE const *VAR = (ARRAY); VAR < ARRAY_END( (ARRAY) ); ++VAR )

/**
 * Convenience macro for iterating \a N times.
 *
 * @param N The number of times to iterate.  If of a signed type, it must be
 * &ge; 0.
 *
 * @sa #FOREACH_ARRAY_ELEMENT()
 */
#define FOR_N_TIMES(N)                                                \
  for ( size_t UNIQUE_NAME(i) = STATIC_CAST( size_t,                  \
        STATIC_IF( IS_SIGNED_EXPR( (N) ),                             \
                   ASSERT_EXPR( (N) > 0 || (N) == 0 ) * (N), (N) ) ); \
        UNIQUE_NAME(i) > 0;                                           \
        --UNIQUE_NAME(i) )

/**
 * Calls **fprintf**(3) on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to print to.
 * @param ... The `fprintf()` arguments.
 *
 * @sa #EPRINTF()
 * @sa #FPUTC()
 * @sa #FPUTS()
 * @sa #PERROR_EXIT_IF()
 * @sa #PRINTF()
 * @sa #PUTC()
 * @sa #PUTS()
 */
#define FPRINTF(STREAM,...) \
  PERROR_EXIT_IF( fprintf( (STREAM), __VA_ARGS__ ) < 0, EX_IOERR )

/**
 * Calls **putc**(3), checks for an error, and exits if there was one.
 *
 * @param C The character to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #EPUTC()
 * @sa #FPRINTF()
 * @sa #FPUTS()
 * @sa #PERROR_EXIT_IF()
 * @sa #PRINTF()
 * @sa #PUTC()
 */
#define FPUTC(C,STREAM) \
  PERROR_EXIT_IF( putc( (C), (STREAM) ) == EOF, EX_IOERR )

/**
 * Calls **fputs**(3), checks for an error, and exits if there was one.
 *
 * @param S The C string to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #EPUTS()
 * @sa #FPRINTF()
 * @sa #FPUTC()
 * @sa #FPUTNSP()
 * @sa #PERROR_EXIT_IF()
 * @sa #PRINTF()
 * @sa #PUTS()
 */
#define FPUTS(S,STREAM) \
  PERROR_EXIT_IF( fputs( (S), (STREAM) ) == EOF, EX_IOERR )

/**
 * Prints \a N spaces to \a STREAM.
 *
 * @param N The number of spaces to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPUTS()
 */
#define FPUTNSP(N,STREAM) \
  FPRINTF( (STREAM), "%*s", STATIC_CAST( int, (N) ), "" )

/**
 * Frees the given memory.
 *
 * @remarks This macro exists since free'ing a pointer to `const` generates a
 * warning.
 *
 * @param PTR The pointer to the memory to free.
 */
#define FREE(PTR)                 free( CONST_CAST( void*, (PTR) ) )

/**
 * Calls **fstat**(2), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to stat.
 * @param PSTAT A pointer to a `struct stat` to receive the result.
 */
#define FSTAT(FD,PSTAT) \
  PERROR_EXIT_IF( fstat( (FD), (PSTAT) ) < 0, EX_IOERR )

/**
 * Shorthand for
 * <code>((</code><i>EXPR1</i><code>)</code> <code>?</code>
 * <code>(</code><i>EXPR1</i><code>)</code> <code>:</code>
 * <code>(</code><i>EXPR2</i><code>))</code>.
 *
 * @param EXPR1 The first expression.
 * @param EXPR2 The second expression.
 * @return Returns \a EXPR1 only if it evaluates to non-zero; otherwise returns
 * \a EXPR2.
 *
 * @warning If \a EXPR1 is non-zero, it is evaluated twice.
 */
#define IF_ELSE_EXPR(EXPR1,EXPR2) ( (EXPR1) ? (EXPR1) : (EXPR2) )

/**
 * A special-case of fatal_error() that additionally prints the file and line
 * where an internal error occurred.
 *
 * @param FORMAT The `printf()` format string literal to use.
 * @param ... The `printf()` arguments.
 *
 * @sa fatal_error()
 * @sa perror_exit()
 * @sa #PERROR_EXIT_IF()
 * @sa #UNEXPECTED_INT_VALUE()
 */
#define INTERNAL_ERROR(FORMAT,...)                            \
  fatal_error( EX_SOFTWARE,                                   \
    "%s:%d: internal error: " FORMAT,                         \
    __FILE__, __LINE__ VA_OPT( (,), __VA_ARGS__ ) __VA_ARGS__ \
  )

#ifdef HAVE___BUILTIN_EXPECT

/**
 * Specifies that \a EXPR is _very_ likely (as in 99.99% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * marginally better performance.
 *
 * @param EXPR An expression that can be cast to `bool`.
 *
 * @sa #unlikely()
 * @sa [Memory part 5: What programmers can do](http://lwn.net/Articles/255364/)
 */
#define likely(EXPR)              __builtin_expect( !!(EXPR), 1 )

/**
 * Specifies that \a EXPR is _very_ unlikely (as in .01% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * marginally better performance.
 *
 * @param EXPR An expression that can be cast to `bool`.
 *
 * @sa #likely()
 * @sa [Memory part 5: What programmers can do](http://lwn.net/Articles/255364/)
 */
#define unlikely(EXPR)            __builtin_expect( !!(EXPR), 0 )

#else
# define likely(EXPR)             (EXPR)
# define unlikely(EXPR)           (EXPR)
#endif /* HAVE___BUILTIN_EXPECT */

/**
 * Convenience macro for calling check_realloc().
 *
 * @param TYPE The type to allocate.
 * @param N The number of objects of \a TYPE to allocate.  It _must_ be &gt; 0.
 * @return Returns a pointer to \a N uninitialized objects of \a TYPE.
 *
 * @sa check_realloc()
 * @sa #REALLOC()
 */
#define MALLOC(TYPE,N) \
  check_realloc( /*p=*/NULL, sizeof(TYPE) * STATIC_CAST( size_t, (N) ) )

/**
 * Gets the number of characters needed to represent the largest magnitide
 * value of the integral \a TYPE in decimal.
 *
 * @remarks
 * @parblock
 * The number of decimal digits _d_ required to represent a binary number with
 * _b_ bits is:
 *
 *      d = ceil(b * log10(2))
 *
 * where _log10(2)_ &asymp; .30102999; hence multiply _b_ by .30102999.  Since
 * the compiler can't do floating-point math at compile-time, that has to be
 * simulated using only integer math.
 *
 * The expression 1233 / 4096 = .30102539 is a close approximation of
 * .30102999.  Integer division by 4096 is the same as right-shifting by 12.
 * The number of bits _b_ = <code>sizeof(</code><i>TYPE</i><code>)</code> *
 * `CHAR_BIT`.  Therefore, multiply that by 1233, then right-shift by 12.
 *
 * The `STATIC_ASSERT_EXPR` (if true) adds 1 that rounds up since shifting
 * truncates.  The `IS_SIGNED_TYPE` (if true) adds another 1 to accomodate the
 * possible `-` (minus sign) for a negative number if \a TYPE is signed.
 * @endparblock
 *
 * @param TYPE The integral type.
 *
 * @sa https://stackoverflow.com/a/13546502/99089
 */
#define MAX_DEC_INT_DIGITS(TYPE)                              \
  (((sizeof(TYPE) * CHAR_BIT * 1233) >> 12)                   \
    + STATIC_ASSERT_EXPR( IS_INTEGRAL_TYPE(TYPE),             \
                          #TYPE " must be an integral type" ) \
    + IS_SIGNED_TYPE(TYPE))

/**
 * Concatenate \a A and \a B together to form a single token.
 *
 * @remarks This macro is needed instead of simply using `##` when either
 * argument needs to be expanded first, e.g., `__LINE__`.
 *
 * @param A The first token.
 * @param B The second token.
 */
#define NAME2(A,B)                NAME2_HELPER(A,B)

/// @cond DOXYGEN_IGNORE
#define NAME2_HELPER(A,B)         A ## B
/// @endcond

/**
 * No-operation statement.
 *
 * @remarks This is useful for do-nothing statements.
 */
#define NO_OP                     ((void)0)

/**
 * Overloads \a FN such that if \a PTR is a pointer to:
 *  + `const`, \a FN is called; or:
 *  + Non-`const`, `nonconst_`\a FN is called.
 *
 * @remarks
 * @parblock
 * Sometimes for a given function `f`, you want `f` to return:
 *
 *  + `R const*` when passed a `T const*`; or:
 *  + `R*` when passed a `T*`.
 *
 * That is you want the `const`-ness of `R` to match that of `T`.  In C++,
 * you'd simply overload `f`:
 *
 *      R const*  f( T const* );        // C++ only
 *      inline R* f( T *t ) {
 *        return const_cast<R*>( f( const_cast<T const*>( t ) ) );
 *      }
 *
 * In C, you'd need two differently named functions:
 *
 *      R const*  f( T const* );        // C
 *      inline R* nonconst_f( T *t ) {
 *        return (R*)f( t );
 *      }
 *
 * This macro allows `f` to be "overloaded" in C such that only `f` ever needs
 * to be called explicitly and either `f` or `nonconst_f` is actually called
 * depending on the `const`-ness of \a PTR.
 *
 * To use this macro:
 *
 *  1. Declare `f` as a function that takes a `T const*` and returns an `R
 *     const*`.
 *
 *  2. Declare `nonconst_f` as an `inline` function that takes a `T*` and
 *     returns an `R*` by calling `f` and casting the result to `R*`.
 *
 *  3. Define a macro also named `f` that expands into `NONCONST_OVERLOAD`
 *     like:
 *
 *          #define f(P,A2,A3)    NONCONST_OVERLOAD( f, (P), (A2), (A3) )
 *
 *     where <code>A</code><i>n</i> are additional arguments for `f`.
 *
 *  4. Define the function `f` with an extra set of `()` to prevent the macro
 *     `f` from expanding:
 *
 *          R const* (f)( T const *t, A2 a2, A3 a3 ) {
 *            // ...
 *          }
 * @endparblock
 *
 * @param FN The name of the function to overload.
 * @param PTR A pointer. If it's a pointer to:
 *  + `const`, \a FN is called; or:
 *  + Non-`const`, `nonconst_`\a FN is called.
 *
 * @param ... Additional arguments passed to \a FN.
 */
#define NONCONST_OVERLOAD(FN,PTR,...)       \
  STATIC_IF( IS_PTR_TO_CONST_EXPR( (PTR) ), \
    FN,                                     \
    NAME2(nonconst_,FN)                     \
  )( (PTR) VA_OPT( (,), __VA_ARGS__ ) __VA_ARGS__ )

/**
 * If \a EXPR is `true`, prints an error message for `errno` to standard error
 * and exits with status \a STATUS.
 *
 * @param EXPR The expression.
 * @param STATUS The exit status code.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 * @sa #UNEXPECTED_INT_VALUE()
 */
#define PERROR_EXIT_IF(EXPR,STATUS) \
  BLOCK( if ( unlikely( (EXPR) ) ) perror_exit( (STATUS) ); )

/**
 * Cast either from or to a pointer type --- similar to C++'s
 * `reinterpret_cast`, but for pointers only.
 *
 * @param TYPE The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro silences a "cast to pointer from integer of different size"
 * warning.  In C++, this would be done via `reinterpret_cast`, but it's not
 * possible to implement that in C that works for both pointers and integers.
 *
 * @sa #CONST_CAST()
 * @sa #STATIC_CAST()
 */
#define POINTER_CAST(TYPE,EXPR)   ((TYPE)(uintptr_t)(EXPR))

/**
 * Calls #FPRINTF() with `stdout`.
 *
 * @param ... The `fprintf()` arguments.
 *
 * @sa #EPRINTF()
 * @sa #FPRINTF()
 * @sa #PUTC()
 * @sa #PUTS()
 */
#define PRINTF(...)               FPRINTF( stdout, __VA_ARGS__ )

/**
 * Calls #FPUTC() with `stdout`.
 *
 * @param C The character to print.
 *
 * @sa #EPUTC()
 * @sa #FPUTC()
 * @sa #PRINTF()
 * @sa #PUTS()
 */
#define PUTC(C)                   FPUTC( (C), stdout )

/**
 * Calls #FPUTS() with `stdout`.
 *
 * @param S The C string to print.
 *
 * @note Unlike **puts**(3), does _not_ print a newline.
 *
 * @sa #EPUTS()
 * @sa #FPUTS()
 * @sa #PRINTF()
 * @sa #PUTC()
 */
#define PUTS(S)                   FPUTS( (S), stdout )

/**
 * Convenience macro for calling check_realloc().
 *
 * @param PTR The pointer to memory to reallocate.  It is set to the newly
 * reallocated memory.
 * @param N The number of objects to reallocate.
 *
 * @sa check_realloc()
 * @sa #MALLOC()
 */
#define REALLOC(PTR,N) \
  ((PTR) = check_realloc( (PTR), sizeof *(PTR) * (N) ))

/**
 * Runs a statement at most once even if control passes through it more than
 * once.  For example:
 *
 *      RUN_ONCE initialize();
 *
 * or:
 *
 *      RUN_ONCE {
 *        // ...
 *      }
 *
 * @sa #ASSERT_RUN_ONCE()
 */
#define RUN_ONCE                      \
  static bool UNIQUE_NAME(run_once);  \
  if ( likely( true_or_set( &UNIQUE_NAME(run_once) ) ) ) ; else

/**
 * Advances \a S over all \a CHARS.
 *
 * @param S The string pointer to advance.
 * @param CHARS A string containing the characters to skip over.
 * @return Returns the updated \a S.
 *
 * @sa #SKIP_WS()
 */
#define SKIP_CHARS(S,CHARS)       ((S) += strspn( (S), (CHARS) ))

/**
 * Advances \a S over all whitespace.
 *
 * @param S The string pointer to advance.
 * @return Returns the updated \a S.
 *
 * @sa #SKIP_CHARS()
 */
#define SKIP_WS(S)                SKIP_CHARS( (S), WS_CHARS )

/**
 * C23-like version of single-argument form of `static_assert`.
 *
 * @param EXPR The expression to use.
 */
#define STATIC_ASSERT(EXPR)       static_assert( (EXPR), #EXPR )

/**
 * C version of C++'s `static_cast`.
 *
 * @param TYPE The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro can't actually implement C++'s `static_cast` because
 * there's no way to do it in C.  It serves merely as a visual cue for the type
 * of cast meant.
 *
 * @sa #CONST_CAST()
 * @sa #POINTER_CAST()
 */
#define STATIC_CAST(TYPE,EXPR)    ((TYPE)(EXPR))

/**
 * Shorthand for calling **strerror**(3) with `errno`.
 */
#define STRERROR()                strerror( errno )

/**
 * Calls **strftime**(3) and checks for failure.
 *
 * @param BUF The destination buffer to print into.
 * @param SIZE The size of \a buf.
 * @param FORMAT The `strftime()` style format string.
 * @param TM A pointer to the time to format.
 */
#define STRFTIME(BUF,SIZE,FORMAT,TM) \
  PERROR_EXIT_IF( strftime( (BUF), (SIZE), (FORMAT), (TM) ) == 0, EX_SOFTWARE )

/**
 * Macro that "string-ifies" its argument, e.g., <code>%STRINGIFY(x)</code>
 * becomes `"x"`.
 *
 * @param X The unquoted string to stringify.
 *
 * @note This macro is sometimes necessary in cases where it's mixed with uses
 * of `##` by forcing re-scanning for token substitution.
 *
 * @sa #CHARIFY()
 */
#define STRINGIFY(X)              STRINGIFY_HELPER(X)

/// @cond DOXYGEN_IGNORE
#define STRINGIFY_HELPER(X)       #X
/// @endcond

/**
 * Strips the enclosing parentheses from \a ARG.
 *
 * @param ARG The argument.  It _must_ be enclosed within parentheses.
 * @return Returns \a ARG without enclosing parentheses.
 */
#define STRIP_PARENS(ARG)         STRIP_PARENS_HELPER ARG

/// @cond DOXYGEN_IGNORE
#define STRIP_PARENS_HELPER(...)  __VA_ARGS__
/// @endcond

/**
 * Gets the length of \a S.
 *
 * @param S The C string literal to get the length of.
 * @return Returns said length.
 */
#define STRLITLEN(S) \
  (ARRAY_SIZE( (S) ) \
   - STATIC_ASSERT_EXPR( IS_C_STR_EXPR( (S) ), #S " must be a C string literal" ))

/**
 * Calls **strncmp**(3) with #STRLITLEN(\a LIT) for the third argument.
 *
 * @param S The string to compare.
 * @param LIT The string literal to compare against.
 * @return Returns a number less than 0, 0, or greater than 0 if \a S is
 * less than, equal to, or greater than \a LIT, respectively.
 */
#define STRNCMPLIT(S,LIT)         strncmp( (S), (LIT), STRLITLEN( (LIT) ) )

/**
 * Synthesises a name prefixed by \a PREFIX unique to the line on which it's
 * used.
 *
 * @param PREFIX The prefix of the synthesized name.
 *
 * @warning All uses for a given \a PREFIX that refer to the same name _must_
 * be on the same line.  This is not a problem within macro definitions, but
 * won't work outside of them since there's no way to refer to a previously
 * used unique name.
 */
#define UNIQUE_NAME(PREFIX)       NAME2(NAME2(PREFIX,_),__LINE__)

/**
 * A special-case of #INTERNAL_ERROR() that prints an unexpected integer value.
 *
 * @param EXPR The expression having the unexpected value.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 * @sa #PERROR_EXIT_IF()
 */
#define UNEXPECTED_INT_VALUE(EXPR)                      \
  INTERNAL_ERROR(                                       \
    "%lld (0x%llX): unexpected value for " #EXPR "\n",  \
    STATIC_CAST( long long, (EXPR) ),                   \
    STATIC_CAST( unsigned long long, (EXPR) )           \
  )

/**
 * Pre-C23/C++20 substitution for `__VA_OPT__`, that is returns \a TOKENS only
 * if 1 or more additional arguments are passed.
 *
 * @remarks
 * @parblock
 * For compilers that don't yet support `__VA_OPT__`, instead of doing
 * something like:
 *
 *      ARG __VA_OPT__(,) __VA_ARGS__               // C23/C++20 way
 *
 * do this instead:
 *
 *      ARG VA_OPT( (,), __VA_ARGS__ ) __VA_ARGS__  // substitute way
 *
 * (It's unfortunately necessary to specify `__VA_ARGS__` twice.)
 * @endparblock
 *
 * @param TOKENS The token(s) possibly to be returned.  They _must_ be enclosed
 * within parentheses.
 * @param ... Zero to 10 arguments, invariably `__VA_ARGS__`.
 * @return Returns \a TOKENS (with enclosing parentheses stripped) followed by
 * `__VA_ARGS__` only if 1 or more additional arguments are passed; returns
 * nothing otherwise.
 */
#ifdef HAVE___VA_OPT__
# define VA_OPT(TOKENS,...) \
    __VA_OPT__( STRIP_PARENS( TOKENS ) )
#else
# define VA_OPT(TOKENS,...) \
    NAME2( VA_OPT_EMPTY_, ARGS_IS_EMPTY( __VA_ARGS__ ) )( TOKENS, __VA_ARGS__ )

  /// @cond DOXYGEN_IGNORE
# define VA_OPT_EMPTY_0(TOKENS,...) STRIP_PARENS(TOKENS)
# define VA_OPT_EMPTY_1(TOKENS,...) /* nothing */
  /// @endcond
#endif /* HAVE___VA_OPT__ */

////////// extern variables ///////////////////////////////////////////////////

/**
 * Identifier characters.
 *
 * @sa is_ident()
 * @sa is_ident_first()
 * @sa str_is_ident_prefix()
 */
extern char const IDENT_CHARS[];

/**
 * Whitespace characters.
 */
extern char const WS_CHARS[];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Extracts the base portion of a path-name.
 *
 * @remarks Unlike **basename**(3):
 *  + Trailing `/` characters are not deleted.
 *  + \a path_name is never modified (hence can therefore be `const`).
 *  + Returns a pointer within \a path_name (hence is multi-call safe).
 *
 * @param path_name The path-name to extract the base portion of.
 * @return Returns a pointer to the last component of \a path_name.  If \a
 * path_name consists entirely of `/` characters, a pointer to the string `/`
 * is returned.
 */
NODISCARD
char const* base_name( char const *path_name );

/**
 * Duplicates \a s prefixed by \a prefix.
 * If duplication fails, prints an error message and exits.
 *
 * @param prefix The null-terminated prefix string to duplicate.
 * @param prefix_len The length of \a prefix.
 * @param s The null-terminated string to duplicate.
 * @return Returns a copy of \a s prefixed by \a prefix.
 *
 * @sa check_strdup_suffix()
 */
NODISCARD
char* check_prefix_strdup( char const *prefix, size_t prefix_len,
                           char const *s );

/**
 * Calls **realloc**(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If NULL, new memory is allocated.
 * @param size The number of bytes to allocate.  It _must_ be &gt; 0.
 * @return Returns a pointer to the allocated memory.
 *
 * @sa #MALLOC()
 * @sa #REALLOC()
 */
NODISCARD
void* check_realloc( void *p, size_t size );

/**
 * Calls **snprintf**(3) and checks for failure.
 *
 * @param buf The destination buffer to print into.
 * @param buf_size The size of \a buf.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 */
PJL_PRINTF_LIKE_FUNC(3)
void check_snprintf( char *buf, size_t buf_size, char const *format, ... );

/**
 * Calls **strdup**(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or NULL.
 * @return Returns a copy of \a s or NULL if \a s is NULL.
 *
 * @sa check_strdup_tolower()
 * @sa check_strndup()
 */
NODISCARD
char* check_strdup( char const *s );

/**
 * Duplicates \a s suffixed by \a suffix.
 * If duplication fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate.
 * @param suffix The null-terminated suffix string to duplicate.
 * @param suffix_len The length of \a suffix.
 * @return Returns a copy of \a s suffixed by \a suffix.
 *
 * @sa check_prefix_strdup()
 */
NODISCARD
char* check_strdup_suffix( char const *s, char const *suffix,
                           size_t suffix_len );

/**
 * Duplicates \a s and checks for failure, but converts all characters to
 * lower-case.  If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or NULL.
 * @return Returns a copy of \a s with all characters converted to lower-case
 * or NULL if \a s is NULL.
 *
 * @sa check_strdup()
 * @sa check_strndup()
 */
NODISCARD
char* check_strdup_tolower( char const *s );

/**
 * Calls **strndup**(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or NULL.
 * @param n The number of characters of \a s to duplicate.
 * @return Returns a copy of \a n characters of \a s or NULL if \a s is NULL.
 *
 * @sa check_strdup()
 * @sa check_strdup_tolower()
 */
NODISCARD
char* check_strndup( char const *s, size_t n );

/**
 * Calls **strtoull**(3) and:
 *
 *  + Ensures \a s contains only decimal digits; and:
 *  + Checks `errno` for failture; and:
 *  + Ensures the value &ge; min; and:
 *  + Ensures the value &le; max.
 *
 * @param s The null-terminated string to convert.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @return Returns \a s convervted to an unsigned integer upon success or
 * `ULLONG_MAX` upon failure.
 * @par
 * Additionally, `errno` is set to:
 *  + `EILSEQ` if \a s contains non-decimal digits; or:
 *  + `ERANGE` if the unsigned integer is &lt; \a min or &gt; \a max.
 */
NODISCARD
unsigned long long check_strtoull( char const *s, unsigned long long min,
                                   unsigned long long max );

/**
 * Checks whether \a s is null: if so, returns the empty string.
 *
 * @param s The pointer to check.
 * @return If \a s is null, returns the empty string; otherwise returns \a s.
 *
 * @sa null_if_empty()
 */
NODISCARD
inline char const* empty_if_null( char const *s ) {
  return s == NULL ? "" : s;
}

/**
 * Checks \a flag: if `false`, sets it to `true`.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if `false`,
 * sets it to `true`.
 * @return Returns `true` only if \a flag was `false` initially.
 *
 * @sa true_clear()
 * @sa true_or_set()
 */
NODISCARD
inline bool false_set( bool *flag ) {
  return !*flag && (*flag = true);
}

/**
 * Prints an error message to standard error and exits with \a status code.
 *
 * @param status The status code to exit with.
 * @param format The `printf()` format string literal to use.
 * @param ... The `printf()` arguments.
 *
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 * @sa #PERROR_EXIT_IF()
 * @sa #UNEXPECTED_INT_VALUE()
 */
PJL_PRINTF_LIKE_FUNC(2)
_Noreturn void fatal_error( int status, char const *format, ... );

/**
 * Checks whether \a fd refers to a regular file.
 *
 * @param fd The file descriptor to check.
 * @return Returns `true` only if \a fd refers to a regular file.
 */
NODISCARD
bool fd_is_file( int fd );

/**
 * Prints a zero-or-more element list of strings where for:
 *
 *  + A zero-element list, nothing is printed;
 *  + A one-element list, the string for the element is printed;
 *  + A two-element list, the strings for the elements are printed separated by
 *    `or`;
 *  + A three-or-more element list, the strings for the first N-1 elements are
 *    printed separated by `,` and the N-1st and Nth elements are separated by
 *    `, or`.
 *
 * @param out The `FILE` to print to.
 * @param elt A pointer to the first element to print.
 * @param gets A pointer to a function to call to get the string for the
 * element `**ppelt`: if the function returns NULL, it signals the end of the
 * list; otherwise, the function returns the string for the element and must
 * increment `*ppelt` to the next element.  If \a gets is NULL, it is assumed
 * that \a elt points to the first element of an array of `char*` and that the
 * array ends with NULL.
 *
 * @warning The string pointer returned by \a gets for a given element _must_
 * remain valid at least until after the _next_ call to fput_list(), that is
 * upon return, the previously returned string pointer must still be valid
 * also.
 */
void fput_list( FILE *out, void const *elt,
                char const* (*gets)( void const **ppelt ) );

/**
 * Possibly prints the list separator \a sep based on \a sep_flag.
 *
 * @param sep The separator to print.
 * @param sep_flag If `true`, prints \a sep; if `false`, prints nothing, but
 * sets it to `true`.  The flag should be `false` initially.
 * @param fout The `FILE` to print to.
 */
void fput_sep( char const *sep, bool *sep_flag, FILE *fout );

/**
 * Prints \a s as a quoted string with escaped characters.
 *
 * @param s The string to put.  If NULL, prints `null` (unquoted).
 * @param quote The quote character to use, either <tt>'</tt> or <tt>"</tt>.
 * @param fout The `FILE` to print to.
 *
 * @sa strbuf_puts_quoted()
 */
void fputs_quoted( char const *s, char quote, FILE *fout );

/**
 * If \a s is not empty, prints \a s followed by a space to \a out; otherwise
 * does nothing.
 *
 * @param s The string to print.
 * @param out The `FILE` to print to.
 *
 * @sa fputsp_s()
 */
void fputs_sp( char const *s, FILE *out );

/**
 * If \a s is not empty, prints a space followed by \a s to \a out; otherwise
 * does nothing.
 *
 * @param s The string to print.
 * @param out The `FILE` to print to.
 *
 * @sa fputs_sp()
 */
void fputsp_s( char const *s, FILE *out );

/**
 * Checks whether \a c is an identifier character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphanumeric or `_`
 * character.
 *
 * @sa IDENT_CHARS
 * @sa is_ident_first()
 */
NODISCARD
inline bool is_ident( char c ) {
  return isalnum( c ) || c == '_';
}

/**
 * Checks whether \a c is an identifier first character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphabetic or `_`
 * character.
 *
 * @sa IDENT_CHARS
 * @sa is_ident()
 */
NODISCARD
inline bool is_ident_first( char c ) {
  return isalpha( c ) || c == '_';
}

/**
 * Checks whether \a s is null, an empty string, or consists only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return If \a s is either null or the empty string, returns NULL; otherwise
 * returns a pointer to the first non-whitespace character in \a s.
 *
 * @sa empty_if_null()
 * @sa str_is_empty()
 */
NODISCARD
inline char const* null_if_empty( char const *s ) {
  return s != NULL && *SKIP_WS( s ) == '\0' ? NULL : s;
}

/**
 * Parses a C/C++ identifier.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns a pointer to the first character that is not an identifier
 * character only if an identifier was parsed or NULL if an identifier was not
 * parsed.
 */
NODISCARD
char const* parse_identifier( char const *s );

/**
 * Prints an error message for `errno` to standard error and exits.
 *
 * @param status The exit status code.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa #PERROR_EXIT_IF()
 * @sa #UNEXPECTED_INT_VALUE()
 */
_Noreturn void perror_exit( int status );

/**
 * Rounds \a n up to a multiple of \a multiple.
 *
 * @param n The number to round up.  Must be &gt; 0.
 * @param multiple The multiple to round up to.  It _must_ be a power of 2.
 * @return Returns \a n rounded up to a multiple of \a multiple.
 */
NODISCARD
inline size_t round_up_pow_2( size_t n, size_t multiple ) {
  assert( is_1_bit( multiple ) );
  return (n + multiple - 1) & ~(multiple - 1);
}

/**
 * Compares two strings for equality.
 *
 * @param is The first string.
 * @param js The second string.
 * @return Returns `true` only if \a is equals \a js.
 */
NODISCARD
bool str_equal( char const *is, char const *js );

/**
 * Checks whether \a s is an affirmative value.  An affirmative value is one of
 * 1, t, true, y, or yes, case-insensitive.
 *
 * @param s The null-terminated string to check.  May be NULL.
 * @return Returns `true` only if \a s is affirmative.
 */
NODISCARD
bool str_is_affirmative( char const *s );

/**
 * Checks whether \a s is either an empty string or a line consisting only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s is either an empty string or a line
 * consisting only of whitespace.
 *
 * @sa null_if_empty()
 */
NODISCARD
inline bool str_is_empty( char const *s ) {
  return *SKIP_WS( s ) == '\0';
}

/**
 * Checks whether \a ident is a prefix of \a s.
 *
 * @remarks If \a s_len &gt; \a ident_len, then the first character past the
 * end of \a s must _not_ be an identifier character.  For example, if \a ident
 * is "foo", then \a s must be "foo" exactly; or \a s must be "foo" followed by
 * a whitespace or punctuation character, but _not_ an identifier character:
 *
 *      "foo"   match
 *      "foo "  match
 *      "foo("  match
 *      "foob"  no match
 *
 * @param ident The identidier to check for.
 * @param ident_len The length of \a ident.
 * @param s The string to check.  Leading whitespace _must_ have been skipped.
 * @param s_len The length of \a s.
 * @return Returns `true` only if it is.
 *
 * @sa IDENT_CHARS
 * @sa str_is_prefix()
 */
NODISCARD
bool str_is_ident_prefix( char const *ident, size_t ident_len, char const *s,
                          size_t s_len );

/**
 * Checks whether \a si is a prefix of (or equal to) \a sj.
 *
 * @param si The candidate prefix string.
 * @param sj The larger string.
 * @return Returns `true` only if \a si is not the empty string and is a prefix
 * of (or equal to) \a sj.
 *
 * @sa str_is_ident_prefix()
 */
NODISCARD
bool str_is_prefix( char const *si, char const *sj );

/**
 * Compares two string pointers by comparing the string pointed to.
 *
 * @param psi The first string pointer to compare.
 * @param psj The first string pointer to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a *psi is
 * less than, equal to, or greater than \a *psj, respectively.
 */
NODISCARD
int str_ptr_cmp( char const **psi, char const **psj );

/**
 * Concatenates \a sep and \a src onto the end of \a dst.
 *
 * @param dst The string onto which \a sep and \a src are appended.
 * @param sep The string to append to \a dst before appending \a src.
 * @param src The string to append to \a dst after appending \a sep.
 * @return Returns the concatenated string, aka, \a dst.
 *
 * @warning \a dst _must_ have been dynamically allocated.
 *
 * @sa str_realloc_pcat()
 */
PJL_DISCARD
char* str_realloc_cat( char *dst, char const *sep, char const *src );

/**
 * Prepends \a src and \a sep to \a dst.
 *
 * @param src The string to prepend.
 * @param sep The string to prepend to \a dst before prepending \a src.
 * @param dst The string to prepend onto.
 * @return Returns the concatenated string, aka, \a dst.
 *
 * @warning \a dst _must_ have been dynamically allocated.
 *
 * @sa str_realloc_cat()
 */
PJL_DISCARD
char* str_realloc_pcat( char const *src, char const *sep, char *dst );

/**
 * Like **strncmp**(3), but ignores characters _not_ in \a charset.
 *
 * @param si The first string.
 * @param sj The second string.
 * @param n The maximum number of characters to check.
 * @param charset The string containing the set of characters to consider.
 * @return Returns a number less than 0, 0, or greater than 0 if \a si is less
 * than, equal to (ignoring characters not in \a charset), or greater than \a
 * sj, respectively.
 * @par
 * When a non-zero value is returned, it will have the same sign as what
 * <code>strncmp(</code> \a si<code>,</code> \a sj<code>,</code> \a n
 * <code>)</code> would return.
 */
NODISCARD
int strncmp_in_set( char const *si, char const *sj, size_t n,
                    char const *charset );

/**
 * Decrements \a *s_len as if to trim whitespace, if any, from the end of \a s.
 *
 * @param s The null-terminated string to trim.
 * @param s_len A pointer to the length of \a s.
 */
void strn_rtrim( char const *s, size_t *s_len );

/**
 * Like **strspn**(3) except it limits its scan to at most \a n characters.
 *
 * @param s The string to span.
 * @param charset The set of allowed characters.
 * @param n The number of characters at most to check.
 * @return Returns the number of characters spanned.
 */
NODISCARD
size_t strnspn( char const *s, char const *charset, size_t n );

/**
 * Checks \a flag: if `false`, sets it to `true`.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if `false`, sets
 * it to `true`.
 * @return Returns `true` only if \a flag was `true` initially.
 *
 * @sa false_set()
 * @sa true_clear()
 */
NODISCARD
inline bool true_or_set( bool *flag ) {
  return *flag || !(*flag = true);
}

/**
 * Checks \a flag: if `true`, sets it to `false`.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if `true`, sets
 * it to `false`.
 * @return Returns `true` only if \a flag was `true` initially.
 *
 * @sa false_set()
 * @sa true_or_set()
 */
NODISCARD
inline bool true_clear( bool *flag ) {
  return *flag && !(*flag = false);
}

#ifndef NDEBUG
/**
 * Suspends process execution until a debugger attaches.
 */
void wait_for_debugger_attach( void );
#else
# define wait_for_debugger_attach() NO_OP
#endif /* NDEBUG */

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
