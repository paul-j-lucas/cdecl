/*
**      cdecl -- C gibberish translator
**      src/util.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>
#include <stdio.h>                      /* for FILE */
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sysexits.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_UTIL_H_INLINE
# define C_UTIL_H_INLINE _GL_INLINE
#endif /* C_UTIL_H_INLINE */

/// @endcond

/**
 * @defgroup util-group Utility Macros & Functions
 * Declares utility constants, macros, and functions.
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

#define CHARIFY_IMPL(X)           CHARIFY_##X
#define STRINGIFY_IMPL(X)         #X

/// @endcond

/**
 * Gets the number of elements of the given array.
 *
 * @param A The array to get the number of elements of.
 */
#define ARRAY_SIZE(A)             (sizeof(A) / sizeof(A[0]))

/**
 * Gets a value where all bits that are greater than or equal to the one bit
 * set in \a N are also set, e.g., <code>%BITS_GE(00010000)</code> =
 * `11110000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GT()
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_GE(N)                (~((N) - 1u))

/**
 * Gets a value where all bits that are greater than the one bit set in \a N
 * are set, e.g., <code>%BITS_GT(00010000)</code> = `11100000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GE()
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_GT(N)                (~((N) | BITS_GE(N)))

/**
 * Gets a value where all bits that are less than or equal to the one bit set
 * in \a N are also set, e.g., <code>%BITS_GE(00010000)</code> = `00011111`.
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
 * set, e.g., <code>%BITS_GT(00010000)</code> = `00001111`.
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
#define CHARIFY(X)                CHARIFY_IMPL(X)

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
 * Shorthand for printing to standard error.
 *
 * @param ... The `printf()` arguments.
 *
 * @sa #EPUTC()
 * @sa #EPUTS()
 * @sa #FPRINTF()
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
 * @sa #PUTS()
 */
#define EPUTS(S)                  fputs( (S), stderr )

/**
 * Prints an error message to standard error and exits with \a STATUS code.
 *
 * @param STATUS The status code to **exit**(3) with.
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 *
 * @sa #INTERNAL_ERR()
 * @sa perror_exit()
 * @sa #UNEXPECTED_INT_VALUE()
 */
#define FATAL_ERR(STATUS,FORMAT,...) \
  BLOCK( EPRINTF( "%s: " FORMAT, me, __VA_ARGS__ ); _Exit( STATUS ); )

/**
 * Calls **ferror**(3) and exits if there was an error on \a STREAM.
 *
 * @param STREAM The `FILE` stream to check for an error.
 *
 * @sa perror_exit_if()
 */
#define FERROR(STREAM) \
  perror_exit_if( ferror( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fflush(3)** on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to flush.
 *
 * @sa perror_exit_if()
 */
#define FFLUSH(STREAM) \
  perror_exit_if( fflush( STREAM ) != 0, EX_IOERR )

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
 * @sa perror_exit_if()
 */
#define FPRINTF(STREAM,...) \
  perror_exit_if( fprintf( (STREAM), __VA_ARGS__ ) < 0, EX_IOERR )

/**
 * Calls **putc**(3), checks for an error, and exits if there was one.
 *
 * @param C The character to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #EPUTC()
 * @sa #FPRINTF()
 * @sa #FPUTS()
 * @sa perror_exit_if()
 */
#define FPUTC(C,STREAM) \
  perror_exit_if( putc( (C), (STREAM) ) == EOF, EX_IOERR )

/**
 * Calls **fputs**(3), checks for an error, and exits if there was one.
 *
 * @param S The C string to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #EPUTS()
 * @sa #FPRINTF()
 * @sa #FPUTC()
 * @sa perror_exit_if()
 */
#define FPUTS(S,STREAM) \
  perror_exit_if( fputs( (S), (STREAM) ) == EOF, EX_IOERR )

/**
 * Frees the given memory.
 *
 * @param PTR The pointer to the memory to free.
 *
 * @remarks
 * This macro exists since free'ing a pointer to `const` generates a warning.
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
  perror_exit_if( fstat( (FD), (PSTAT) ) < 0, EX_IOERR )

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
 * A special-case of #FATAL_ERR that additionally prints the file and line
 * where an internal error occurred.
 *
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 *
 * @sa #FATAL_ERR()
 * @sa perror_exit()
 * @sa #UNEXPECTED_INT_VALUE()
 */
#define INTERNAL_ERR(FORMAT,...) \
  FATAL_ERR( EX_SOFTWARE, "%s:%d: internal error: " FORMAT, __FILE__, __LINE__, __VA_ARGS__ )

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
#define MALLOC(TYPE,N)            check_realloc( NULL, sizeof(TYPE) * (N) )

/**
 * Zeros the memory pointed to by \a PTR.  The number of bytes to zero is given
 * by `sizeof *(PTR)`.
 *
 * @param PTR The pointer to the start of memory to zero.  \a PTR must be a
 * pointer.  If it's an array, it'll generate a compile-time error.
 */
#ifdef HAVE___TYPEOF__
#define MEM_ZERO(PTR) BLOCK(                                            \
  /* "error: array initializer must be an initializer list" if array */ \
  MAYBE_UNUSED __typeof__(PTR) _tmp = 0;                                \
  memset( (PTR), 0, sizeof *(PTR) ); )
#else
#define MEM_ZERO(PTR)             memset( (PTR), 0, sizeof *(PTR) )
#endif /* HAVE___TYPEOF__ */

/**
 * No-operation statement.  (Useful for a `goto` target.)
 */
#define NO_OP                     ((void)0)

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
 * Shorthand for printing a C string to standard output.
 *
 * @param S The C string to print.
 *
 * @sa #EPUTS()
 * @sa #FPUTS()
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
  perror_exit_if( stat( (PATH), (PSTAT) ) < 0, EX_IOERR )

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
#define STRINGIFY(X)              STRINGIFY_IMPL(X)

#ifdef __GNUC__

/**
 * Specifies that \a EXPR is _very_ likely (as in 99.99% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
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
 * magrinally better performance.
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
 * A special-case of #INTERNAL_ERR() that prints an unexpected integer value.
 *
 * @param EXPR The expression having the unexpected value.
 *
 * @sa #FATAL_ERR()
 * @sa #INTERNAL_ERR()
 * @sa perror_exit()
 */
#define UNEXPECTED_INT_VALUE(EXPR) \
  INTERNAL_ERR( "%lld (0x%llX): unexpected value for " #EXPR "\n", (long long)(EXPR), (unsigned long long)(EXPR) )

/**
 * Whitespace characters.
 */
#define WS                        " \f\n\r\t\v"

////////// extern functions ///////////////////////////////////////////////////

/**
 * Extracts the base portion of a path-name.
 * Unlike **basename**(3):
 *  + Trailing `/` characters are not deleted.
 *  + \a path_name is never modified (hence can therefore be `const`).
 *  + Returns a pointer within \a path_name (hence is multi-call safe).
 *
 * @param path_name The path-name to extract the base portion of.
 * @return Returns a pointer to the last component of \a path_name.
 * If \a path_name consists entirely of `/` characters, a pointer to the string
 * `/` is returned.
 */
NODISCARD
char const* base_name( char const *path_name );

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
 * Local implementation of POSIX 2008 **fmemopen**(3) for systems that don't
 * have it.
 *
 * @param buf A pointer to the buffer to use.  The pointer must remain valid
 * for as along as the `FILE` is open.
 * @param size The size of \a buf.
 * @param mode The open mode.  It _must_ contain `r`.
 * @return Returns a `FILE*` containing the contents of \a buf or NULL upon
 * error.
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
 * remain valid at least until after the _next_ call to fprint_list(), that is
 * upon return, the previously returned string pointer must still be valid
 * also.
 */
void fprint_list( FILE *out, void const *elt,
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
NODISCARD
void* free_later( void *p );

/**
 * Frees all the memory pointed to by all the nodes in the free-later-list.
 *
 * @sa free_later()
 */
void free_now( void );

#ifdef ENABLE_TERM_SIZE
/**
 * Gets the number of columns and/or lines of the terminal.
 *
 * @param ncolumns If not NULL, receives the number of columns or 0 if can not
 * be determined.
 * @param nlines If not NULL, receives the number of lines or 0 if it can not
 * be determined.
 */
void get_term_columns_lines( unsigned *ncolumns, unsigned *nlines );
#endif /* ENABLE_TERM_SIZE */

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
  return is_1_bit( n ) && (n & set) != 0;
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
 */
NODISCARD C_UTIL_H_INLINE
bool is_ident( char c ) {
  return isalnum( c ) || c == '_';
}

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
 * @param s The string to parse.
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
 * @sa #FATAL_ERR()
 * @sa #INTERNAL_ERR()
 * @sa perror_exit_if()
 */
noreturn void perror_exit( int status );

/**
 * If \a expr is `true`, prints an error message for `errno` to standard error
 * and exits.
 *
 * @param expr The expression.
 * @param status The exit status code.
 *
 * @sa #FATAL_ERR()
 * @sa #INTERNAL_ERR()
 * @sa perror_exit()
 */
C_UTIL_H_INLINE
void perror_exit_if( bool expr, int status ) {
  if ( unlikely( expr ) )
    perror_exit( status );
}

/**
 * Checks whether \a s is a blank line, that is either an empty string or a
 * line consisting only of whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s is a blank line.
 *
 * @sa null_if_empty()
 */
NODISCARD C_UTIL_H_INLINE
bool str_is_empty( char const *s ) {
  SKIP_WS( s );
  return *s == '\0';
}

/**
 * Checks whether \a s is null, an empty string, or consists only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns \a s only if it's neither the empty string nor only
 * whitespace; otherwise returns NULL.
 *
 * @sa str_is_empty()
 */
NODISCARD C_UTIL_H_INLINE
char const* null_if_empty( char const *s ) {
  return s != NULL && str_is_empty( s ) ? NULL : s;
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
 * Decrements \a *s_len as if to trim whitespace, if any, from the end of \a s.
 *
 * @param s The null-terminated string to trim.
 * @param s_len A pointer to the length of \a s.
 */
void str_rtrim_len( char const *s, size_t *s_len );

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
 * @param sep_flag A pointer to a flag to know whether to print \a sep.  The
 * flag should be `false` initially.
 * @param sout The `FILE` to print to.
 */
C_UTIL_H_INLINE
void fprint_sep( FILE *sout, char const *sep, bool *sep_flag ) {
  if ( true_or_set( sep_flag ) )
    FPUTS( sep, sout );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
