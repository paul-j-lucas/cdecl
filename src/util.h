/*
**      cdecl -- C gibberish translator
**      src/util.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
#include "pjl_config.h"                 /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>
#include <stdio.h>                      /* for FILE */
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_UTIL_H_INLINE
# define C_UTIL_H_INLINE _GL_INLINE
#endif /* C_UTIL_H_INLINE */

/// @endcond

/**
 * @defgroup util-group Utility Macros & Functions
 * Utility macros, constants, and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

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

#define STRINGIFY_HELPER(X)       #X

/// @endcond

/**
 * Gets the number of elements of the given array.
 *
 * @param A The array to get the number of elements of.
 *
 * @note \a A _must_ be a statically allocated array.
 */
#define ARRAY_SIZE(A) (     \
  sizeof(A) / sizeof(0[A])  \
  * STATIC_ASSERT_EXPR( IS_ARRAY(A), #A " must be an array" ))

#ifndef NDEBUG
/**
 * Asserts that this line of code is run at most once &mdash; useful in
 * initialization functions that must be called at most once.  For example:
 *
 *      void initialize() {
 *        ASSERT_RUN_ONCE();
 *        // ...
 *      }
 *
 * @sa #RUN_ONCE
 */
#define ASSERT_RUN_ONCE() BLOCK(    \
  static bool UNIQUE_NAME(called);  \
  assert( !UNIQUE_NAME(called) );   \
  UNIQUE_NAME(called) = true; )
#else
#define ASSERT_RUN_ONCE()         NO_OP
#endif /* NDEBUG */

/**
 * Gets a value where all bits that are greater than or equal to the one bit
 * set in \a N are also set, e.g., <code>%BITS_GE(0b00010000)</code> =
 * `0b11110000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GT()
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_GE(N)                (~BITS_LT(N))

/**
 * Gets a value where all bits that are greater than the one bit set in \a N
 * are set, e.g., <code>%BITS_GT(0b00010000)</code> = `0b11100000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_GT(N)                (~BITS_LE(N))

/**
 * Gets a value where all bits that are less than or equal to the one bit set
 * in \a N are also set, e.g., <code>%BITS_LE(0b00010000)</code> =
 * `0b00011111`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_GT()
 * @sa #BITS_LT()
 */
#define BITS_LE(N)                (BITS_LT(N) | (N))

/**
 * Gets a value where all bits that are less than the one bit set in \a N are
 * set, e.g., <code>%BITS_LT(0b00010000)</code> = `0b00001111`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_GT()
 * @sa #BITS_LE()
 */
#define BITS_LT(N)                ((N) - 1u)

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

/**
 * C version of C++'s `const_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro can't actually implement C++'s `const_cast` because there's
 * no way to do it in C.  It serves merely as a visual cue for the type of cast
 * meant.
 *
 * @sa #INTEGER_CAST()
 * @sa #POINTER_CAST()
 * @sa #STATIC_CAST()
 */
#define CONST_CAST(T,EXPR)        ((T)(EXPR))

/**
 * Declares a object with an unspecified name both aligned and sized as a \a T
 * intended to be used inside a `struct` declaration to add padding.
 *
 * @param T The type of the object.
 */
#define DECL_UNUSED(T) \
  _Alignas(T) char UNIQUE_NAME(unused)[ sizeof(T) ]

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
  PERROR_EXIT_IF( ferror( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fflush(3)** on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to flush.
 *
 * @sa #PERROR_EXIT_IF()
 */
#define FFLUSH(STREAM) \
  PERROR_EXIT_IF( fflush( STREAM ) != 0, EX_IOERR )

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
 *
 * @sa #STAT()
 */
#define FSTAT(FD,PSTAT) \
  PERROR_EXIT_IF( fstat( (FD), (PSTAT) ) < 0, EX_IOERR )

/**
 * Shorthand for `((EXPR1) ? (EXPR1) : (EXPR2))`.
 *
 * @param EXPR1 The first expression.
 * @param EXPR2 The second expression.
 * @return Returns \a EXPR1 only if it evaluates to non-zero; otherwise returns
 * \a EXPR2.
 *
 * @warning If \a EXPR1 is non-zero, it is evaluated twice.
 */
#define IF_ELSE(EXPR1,EXPR2)      ( (EXPR1) ? (EXPR1) : (EXPR2) )

/**
 * Cast either from or to an integral type &mdash; similar to C++'s
 * `reinterpret_cast`, but for integers only.
 *
 * @param T The integral type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note In C++, this would be done via `reinterpret_cast`, but it's not
 * possible to implement that in C that works for both pointers and integers.
 *
 * @sa #CONST_CAST()
 * @sa #POINTER_CAST()
 * @sa #STATIC_CAST()
 */
#define INTEGER_CAST(T,EXPR)      ((T)(uintmax_t)(EXPR))

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
#define INTERNAL_ERROR(FORMAT,...) \
  fatal_error( EX_SOFTWARE, "%s:%d: internal error: " FORMAT, __FILE__, __LINE__, __VA_ARGS__ )

/**
 * Checks (at compile-time) whether \a A is an array.
 *
 * @param A The alleged array to check.
 * @return Returns 1 (true) only if \a A is an array; 0 (false) otherwise.
 *
 * @sa #IS_POINTER()
 */
#define IS_ARRAY(A)               !IS_SAME_TYPE( (A), &(A)[0] )

/**
 * Checks (at compile-time) whether \a P is a pointer.
 *
 * @param P The alleged pointer to check.
 * @return Returns 1 (true) only if \a P is a pointer; 0 (false) otherwise.
 *
 * @sa #IS_ARRAY()
 */
#define IS_POINTER(P)             IS_SAME_TYPE( (P), &(P)[0] )

/**
 * Checks (at compile-time) whether \a T1 and \a T2 are the same type.
 *
 * @param T1 The first type or expression.
 * @param T2 The second type or expression.
 * @return Returns 1 (true) only if \a T1 and \a T2 are the same type; 0
 * (false) otherwise.
 */
#if defined(HAVE___BUILTIN_TYPES_COMPATIBLE_P) && defined(HAVE___TYPEOF__)
# define IS_SAME_TYPE(T1,T2) \
    __builtin_types_compatible_p( __typeof__(T1), __typeof__(T2) )
#else
# define IS_SAME_TYPE(T1,T2)      1
#endif

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
  check_realloc( NULL, sizeof(TYPE) * STATIC_CAST( size_t, (N) ) )

/// @cond DOXYGEN_IGNORE
#define NAME2_HELPER(A,B)         A##B
/// @endcond

/**
 * Concatenate \a A and \a B together to form a single token.
 *
 * @remarks This macro is needed instead of simply using `##` when either
 * argument needs to be expanded first, e.g., `__LINE__`.
 *
 * @param A The first name.
 * @param B The second name.
 */
#define NAME2(A,B)                NAME2_HELPER(A,B)

/**
 * Zeros the memory pointed to by \a PTR.  The number of bytes to zero is given
 * by `sizeof *(PTR)`.
 *
 * @param PTR The pointer to the start of memory to zero.  \a PTR must be a
 * pointer.  If it's an array, it'll generate a compile-time error.
 */
#define MEM_ZERO(PTR) BLOCK(                                    \
  static_assert( IS_POINTER(PTR), #PTR " must be a pointer" );  \
  memset( (PTR), 0, sizeof *(PTR) ); )

/**
 * No-operation statement.  (Useful for a `goto` target.)
 */
#define NO_OP                     ((void)0)

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
#define PERROR_EXIT_IF( EXPR, STATUS ) \
  BLOCK( if ( unlikely( EXPR ) ) perror_exit( STATUS ); )

/**
 * Cast either from or to a pointer type &mdash; similar to C++'s
 * `reinterpret_cast`, but for pointers only.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro silences a "cast to pointer from integer of different size"
 * warning.  In C++, this would be done via `reinterpret_cast`, but it's not
 * possible to implement that in C that works for both pointers and integers.
 *
 * @sa #CONST_CAST()
 * @sa #INTEGER_CAST()
 * @sa #STATIC_CAST()
 */
#define POINTER_CAST(T,EXPR)      ((T)(uintptr_t)(EXPR))

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
 * @param TYPE The type of object to reallocate.
 * @param N The number of objects of \a TYPE to reallocate.
 *
 * @sa check_realloc()
 * @sa #MALLOC()
 */
#define REALLOC(PTR,TYPE,N) \
  ((PTR) = check_realloc( (PTR), sizeof(TYPE) * (N) ))

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
#define SKIP_WS(S)                SKIP_CHARS( (S), WS )

/**
 * Calls **stat**(2), checks for an error, and exits if there was one.
 *
 * @param PATH The path to stat.
 * @param PSTAT A pointer to a `struct stat` to receive the result.
 *
 * @sa #FSTAT()
 */
#define STAT(PATH,PSTAT) \
  PERROR_EXIT_IF( stat( (PATH), (PSTAT) ) < 0, EX_IOERR )

/**
 * Like C11's `_Static_assert()` except that is can be used in an expression.
 *
 * @param EXPR The expression to check.
 * @param MSG The string literal of the error message to print only if \a EXPR
 * evaluates to 0 (false).
 * @return Always returns 1.
 */
#define STATIC_ASSERT_EXPR(EXPR,MSG) \
  (!!sizeof( struct { static_assert( (EXPR), MSG ); int required; } ))

/**
 * C version of C++'s `static_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro can't actually implement C++'s `static_cast` because
 * there's no way to do it in C.  It serves merely as a visual cue for the type
 * of cast meant.
 *
 * @sa #CONST_CAST()
 * @sa #INTEGER_CAST()
 * @sa #POINTER_CAST()
 */
#define STATIC_CAST(T,EXPR)       ((T)(EXPR))

/**
 * Shorthand for calling **strerror**(3) with `errno`.
 */
#define STRERROR()                strerror( errno )

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

#ifdef __GNUC__

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
#endif /* __GNUC__ */

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
#define UNEXPECTED_INT_VALUE(EXPR) \
  INTERNAL_ERROR( "%lld (0x%llX): unexpected value for " #EXPR "\n", (long long)(EXPR), (unsigned long long)(EXPR) )

/**
 * Whitespace characters.
 */
#define WS                        " \n\t\r\f\v"

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
 * Calls **atexit**(3) and checks for failure.
 *
 * @param cleanup_fn The pointer to the function to call **atexit**(3) with.
 */
void check_atexit( void (*cleanup_fn)(void) );

/**
 * Calls **dup2**(2) and checks for failure.
 *
 * @param old_fd The old file descriptor to duplicate.
 * @param new_fd The new file descriptor to duplicate to.
 */
void check_dup2( int old_fd, int new_fd );

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
 * Checks whether \a s is null: if so, returns the empty string.
 *
 * @param s The pointer to check.
 * @return If \a s is null, returns the empty string; otherwise returns \a s.
 *
 * @sa null_if_empty()
 */
NODISCARD C_UTIL_H_INLINE
char const* empty_if_null( char const *s ) {
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
NODISCARD C_UTIL_H_INLINE
bool false_set( bool *flag ) {
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
 *
 * @sa path_is_file()
 */
NODISCARD
bool fd_is_file( int fd );

#ifndef HAVE_FMEMOPEN
/**
 * Partial implementation of POSIX 2008 **fmemopen**(3) for systems that don't
 * have it to read at most \a size bytes of memory starting at \a buf as if it
 * were a `FILE`.
 *
 * @param buf A pointer to the buffer to use.  Unlike the standard
 * **fmemopen**, \a buf can not be NULL.
 * @param size The size of \a buf.
 * @param mode The open mode.  Unlike the standard **fmemopen**, it _must_
 * contain `r` and `b` (if given) is ignored.
 * @return Returns a `FILE*` containing the contents of \a buf or NULL if
 * either \a size is 0 or upon error.
 *
 * @warning \a buf must remain valid for as along as the `FILE` is open.
 */
NODISCARD
FILE* fmemopen( void *buf, size_t size, char const *mode );
#endif /* HAVE_FMEMOPEN */

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
 * If \a s is not empty, prints \a s followed by a space to \a out; otherwise
 * does nothing.
 *
 * @param s The string to print.
 * @param out the `FILE` to print to.
 */
void fputs_sp( char const *s, FILE *out );

/**
 * Adds a pointer to the head of the free-later-list.
 *
 * @param p The pointer to add.
 * @return Returns \a p.
 *
 * @sa free_now()
 */
PJL_DISCARD
void* free_later( void *p );

/**
 * Frees all the memory pointed to by all the nodes in the free-later-list.
 *
 * @sa free_later()
 */
void free_now( void );

/**
 * Gets the full path of the user's home directory.
 *
 * @return Returns said directory or NULL if it is not obtainable.
 */
NODISCARD
char const* home_dir( void );

/**
 * Checks whether \a n has either 0 or 1 bits set.
 *
 * @param n The number to check.
 * @return Returns `true` only if \a n has either 0 or 1 bits set.
 *
 * @sa is_01_bit_only_in_set()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_01_bit( uint64_t n ) {
  return (n & (n - 1)) == 0;
}

/**
 * Checks whether there are 0 or more bits set in \a n that are only among the
 * bits set in \a set.
 *
 * @param n The bits to check.
 * @param set The bits to check against.
 * @return Returns `true` only if there are 0 or more bits set in \a n that are
 * only among the bits set in \a set.
 *
 * @sa is_01_bit()
 * @sa is_01_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_0n_bit_only_in_set( uint64_t n, uint64_t set ) {
  return (n & set) == n;
}

/**
 * Checks whether \a n has exactly 1 bit is set.
 *
 * @param n The number to check.
 * @return Returns `true` only if \a n has exactly 1 bit set.
 *
 * @sa is_01_bit()
 * @sa is_01_bit_only_in_set()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_1_bit( uint64_t n ) {
  return n != 0 && is_01_bit( n );
}

/**
 * Checks whether \a n has exactly 1 bit set in \a set.
 *
 * @param n The number to check.
 * @param set The set of bits to check against.
 * @return Returns `true` only if \a n has exactly 1 bit set in \a set.
 *
 * @note There may be other bits set in \a n that are not in \a set.
 *
 * @sa is_01_bit()
 * @sa is_01_bit_only_in_set()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_1_bit_in_set( uint64_t n, uint64_t set ) {
  return is_1_bit( n & set );
}

/**
 * Checks whether \a n has exactly 1 bit set only in \a set.
 *
 * @param n The number to check.
 * @param set The set of bits to check against.
 * @return Returns `true` only if \a n has exactly 1 bit set only in \a set.
 *
 * @sa is_01_bit()
 * @sa is_01_bit_only_in_set()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_1_bit_only_in_set( uint64_t n, uint64_t set ) {
  return is_1_bit( n ) && is_1_bit_in_set( n, set );
}

/**
 * Checks whether \a n is zero or has exactly 1 bit set only in \a set.
 *
 * @param n The bits to check.
 * @param set The bits to check against.
 * @return Returns `true` only if either \a n is zero or has exactly 1 bit set
 * only in \a set.
 *
 * @sa is_01_bit()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 * @sa is_1n_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_01_bit_only_in_set( uint64_t n, uint64_t set ) {
  return n == 0 || is_1_bit_only_in_set( n, set );
}

/**
 * Checks whether \a n has one or more bits set that are only among the bits
 * set in \a set.
 *
 * @param n The bits to check.
 * @param set The bits to check against.
 * @return Returns `true` only if \a n has one or more bits set that are only
 * among the bits set in \a set.
 *
 * @sa is_01_bit()
 * @sa is_01_bit_only_in_set()
 * @sa is_0n_bit_only_in_set()
 * @sa is_1_bit()
 * @sa is_1_bit_in_set()
 * @sa is_1_bit_only_in_set()
 */
NODISCARD C_UTIL_H_INLINE
bool is_1n_bit_only_in_set( uint64_t n, uint64_t set ) {
  return n != 0 && is_0n_bit_only_in_set( n, set );
}

/**
 * Checks whether \a c is an identifier character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphanumeric or `_`
 * character.
 *
 * @sa is_ident_first()
 */
NODISCARD C_UTIL_H_INLINE
bool is_ident( char c ) {
  return isalnum( c ) || c == '_';
}

/**
 * Checks whether \a c is an identifier first character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphabetic or `_`
 * character.
 *
 * @sa is_ident()
 */
NODISCARD C_UTIL_H_INLINE
bool is_ident_first( char c ) {
  return isalpha( c ) || c == '_';
}

/**
 * Checks whether \a ident is a prefix if \a s.
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
 * @sa str_is_prefix()
 */
NODISCARD
bool is_ident_prefix( char const *ident, size_t ident_len, char const *s,
                      size_t s_len );

/**
 * Gets the value of the least significant bit that's a 1 in \a n.
 * For example, for \a n of 12, returns 4.
 *
 * @param n The number to use.
 * @return Returns said value or 0 if \a n is 0.
 *
 * @sa ms_bit1_32()
 */
NODISCARD
uint32_t ls_bit1_32( uint32_t n );

/**
 * Gets the value of the most significant bit that's a 1 in \a n.
 * For example, for \a n of 12, returns 8.
 *
 * @param n The number to use.
 * @return Returns said value or 0 if \a n is 0.
 *
 * @sa ls_bit1_32()
 */
NODISCARD
uint32_t ms_bit1_32( uint32_t n );

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
 * Checks whether \a s is either an empty string or a line consisting only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s is either an empty string or a line
 * consisting only of whitespace.
 *
 * @sa null_if_empty()
 */
NODISCARD C_UTIL_H_INLINE
bool str_is_empty( char const *s ) {
  return *SKIP_WS( s ) == '\0';
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
NODISCARD C_UTIL_H_INLINE
char const* null_if_empty( char const *s ) {
  return s != NULL && *SKIP_WS( s ) == '\0' ? NULL : s;
}

/**
 * Checks whether \a path refers to a regular file.
 *
 * @param path The path to check.
 * @return Returns `true` only if \a path refers to a regular file.
 *
 * @sa fd_is_file()
 */
NODISCARD
bool path_is_file( char const *path );

/**
 * Checks whether \a s1 is a prefix of (or equal to) \a s2.
 *
 * @param s1 The candidate prefix string.
 * @param s2 The larger string.
 * @return Returns `true` only if \a s1 is not the empty string and is a prefix
 * of (or equal to) \a s2.
 *
 * @sa is_ident_prefix()
 */
NODISCARD
bool str_is_prefix( char const *s1, char const *s2 );

/**
 * Concatenates \a sep and \a src onto the end of \a dst.
 *
 * @param dst The string onto which \a sep and \a src are appended.
 * @param sep The string to append to \a dst before appending \a src.
 * @param src The string to append to \a dst after appending \a sep.
 * @return Returns the concatenated string.
 *
 * @warning \a dst _must_ have been dynamically allocated.
 */
NODISCARD
char* str_realloc_cat( char *dst, char const *sep, char const *src );

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
NODISCARD C_UTIL_H_INLINE
bool true_or_set( bool *flag ) {
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
NODISCARD C_UTIL_H_INLINE
bool true_clear( bool *flag ) {
  return *flag && !(*flag = false);
}

/**
 * Possibly prints the list separator \a sep based on \a sep_flag.
 *
 * @param sep The separator to print.
 * @param sep_flag If `true`, prints \a sep; if `false`, prints nothing, but
 * sets it to `true`.  The flag should be `false` initially.
 * @param sout The `FILE` to print to.
 */
C_UTIL_H_INLINE
void fput_sep( char const *sep, bool *sep_flag, FILE *sout ) {
  if ( true_or_set( sep_flag ) )
    FPUTS( sep, sout );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
