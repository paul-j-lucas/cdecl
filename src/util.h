/*
**      cdecl -- C gibberish translator
**      src/util.h
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>
#include <stdio.h>                      /* for FILE */
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_UTIL_INLINE
# define C_UTIL_INLINE _GL_INLINE
#endif /* C_UTIL_INLINE */

/// @endcond

/**
 * @defgroup util-group Utility Macros & Functions
 * Declares utility constants, macros, and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Gets the number of elements of the given array.
 *
 * @param A The array to get the number of elements of.
 */
#define ARRAY_SIZE(A)             (sizeof(A) / sizeof(A[0]))

/**
 * Embeds the given statements into a compound statement block.
 *
 * @param ... The statement(s) to embed.
 */
#define BLOCK(...)                do { __VA_ARGS__ } while (0)

/**
 * Explicit C version of C++'s `const_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa REINTERPRET_CAST
 * @sa STATIC_CAST
 */
#define CONST_CAST(T,EXPR)        ((T)(uintptr_t)(EXPR))

/**
 * Calls **ferror**(3) and exits if there was an error on \a STREAM.
 *
 * @param STREAM The `FILE` stream to check for an error.
 */
#define FERROR(STREAM)            IF_EXIT( ferror( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fflush(3)** on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to flush.
 */
#define FFLUSH(STREAM)            IF_EXIT( fflush( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fprintf**(3) on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to print to.
 * @param ... The `fprintf()` arguments.
 */
#define FPRINTF(STREAM,...) \
  IF_EXIT( fprintf( (STREAM), __VA_ARGS__ ) < 0, EX_IOERR )

/**
 * Calls **putc**(3), checks for an error, and exits if there was one.
 *
 * @param C The character to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa FPRINTF
 * @sa FPUTC
 * @sa FPUTS
 */
#define FPUTC(C,STREAM) \
  IF_EXIT( putc( (C), (STREAM) ) == EOF, EX_IOERR )

/**
 * Calls **fputs**(3), checks for an error, and exits if there was one.
 *
 * @param S The C string to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa FPRINTF
 * @sa FPUTC
 */
#define FPUTS(S,STREAM) \
  IF_EXIT( fputs( (S), (STREAM) ) == EOF, EX_IOERR )

/**
 * Frees the given memory.
 *
 * @param PTR The pointer to the memory to free.
 *
 * @remarks
 * This macro exists since free'ing a pointer-to const generates a warning.
 *
 * @sa FREE_STR_LATER
 */
#define FREE(PTR)                 free( CONST_CAST( void*, (PTR) ) )

/**
 * Calls free_later() and casts the result to `char*`.
 *
 * @param PTR The pointer to the C string to free later.
 *
 * @sa FREE_STRDUP_LATER
 */
#define FREE_STR_LATER(PTR)       REINTERPRET_CAST( char*, free_later( PTR ) )

/**
 * Frees the duplicated C string later.
 *
 * @param PTR The pointer to the C string to duplicate and free later.
 *
 * @sa FREE_STR_LATER
 */
#define FREE_STRDUP_LATER(PTR)    FREE_STR_LATER( check_strdup( PTR ) )

/**
 * Calls **fstat**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to stat.
 * @param STAT A pointer to a `struct stat` to receive the result.
 */
#define FSTAT(FD,STAT) \
  IF_EXIT( fstat( (FD), (STAT) ) < 0, EX_IOERR )

/**
 * Evaluates \a EXPR: if it returns `true`, calls perror_exit() with \a ERR.
 *
 * @param EXPR The expression to evaluate.
 * @param ERR The exit status code to use.
 */
#define IF_EXIT(EXPR,ERR) \
  BLOCK( if ( unlikely( EXPR ) ) perror_exit( ERR ); )

/**
 * Prints an error message to standard error and exits in response to an
 * internal error.
 *
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 *
 * @sa PMESSAGE_EXIT
 * @sa UNEXPECTED_INT_VALUE
 * @sa UNEXPECTED_STR_VALUE
 */
#define INTERNAL_ERR(FORMAT,...) \
  PMESSAGE_EXIT( EX_SOFTWARE, "internal error: " FORMAT, __VA_ARGS__ )

/**
 * Calls **malloc**(3) and casts the result to \a TYPE.
 *
 * @param TYPE The type to cast the pointer returned by **malloc**(3) to.
 * @param N The number of objects of \a TYPE to allocate.
 * @return Returns a pointer to \a N uninitialized objects of \a TYPE.
 *
 * @sa REALLOC
 */
#define MALLOC(TYPE,N) \
  STATIC_CAST( TYPE*, check_realloc( NULL, sizeof(TYPE) * (N) ) )

/**
 * Zeros the memory pointed to by \a PTR.
 *
 * @param PTR The pointer to the memory to zero.  The number of bytes to zero
 * is given by `sizeof *(PTR)`.
 */
#define MEM_ZERO(PTR)             memset( (PTR), 0, sizeof *(PTR) )

/**
 * No-operation statement.  (Useful for a `goto` target.)
 */
#define NO_OP                     ((void)0)

/**
 * Prints an error message to standard error and exits with \a STATUS code.
 *
 * @param STATUS The status code to **exit**(3) with.
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 *
 * @sa INTERNAL_ERR
 * @sa UNEXPECTED_INT_VALUE
 * @sa UNEXPECTED_STR_VALUE
 */
#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( PRINTF_ERR( "%s: " FORMAT, me, __VA_ARGS__ ); exit( STATUS ); )

/**
 * Shorthand for printing to standard error.
 *
 * @param ... The `printf()` arguments.
 *
 * @sa PUTC_ERR
 * @sa PUTS_ERR
 */
#define PRINTF_ERR(...)           fprintf( stderr, __VA_ARGS__ )

/**
 * Shorthand for printing a character to standard error.
 *
 * @param C The character to print.
 *
 * @sa PRINTF_ERR
 * @sa PUTC_OUT
 * @sa PUTS_ERR
 */
#define PUTC_ERR(C)               FPUTC( (C), stderr )

/**
 * Shorthand for printing a character to standard output.
 *
 * @param C The character to print.
 *
 * @sa PUTC_ERR
 * @sa PUTS_OUT
 */
#define PUTC_OUT(C)               FPUTC( (C), stdout )

/**
 * Shorthand for printing a C string to standard error.
 *
 * @param S The C string to print.
 *
 * @sa PRINTF_ERR
 * @sa PUTC_ERR
 * @sa PUTS_OUT
 */
#define PUTS_ERR(S)               FPUTS( (S), stderr )

/**
 * Shorthand for printing a C string to standard output.
 *
 * @param S The C string to print.
 *
 * @sa PUTC_OUT
 * @sa PUTS_ERR
 */
#define PUTS_OUT(S)               FPUTS( (S), stdout )

/**
 * Calls **realloc**(3) and resets \a PTR.
 *
 * @param PTR The pointer to memory to reallocate.  It is set to the newly
 * reallocated memory.
 * @param TYPE The type to cast the pointer returned by **realloc**(3) to.
 * @param N The number of objects of \a TYPE to reallocate.
 *
 * @sa MALLOC
 */
#define REALLOC(PTR,TYPE,N) \
  ((PTR) = STATIC_CAST(TYPE*, check_realloc( (PTR), sizeof(TYPE) * (N) )))

/**
 * Explicit C version of C++'s `reinterpret_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa CONST_CAST
 * @sa STATIC_CAST
 */
#define REINTERPRET_CAST(T,EXPR)  ((T)(uintptr_t)(EXPR))

/**
 * Advances \a S over all \a CHARS.
 *
 * @param S The string pointer to advance.
 * @param CHARS A string containing the characters to skip over.
 * @return Returns the updated \a S.
 */
#define SKIP_CHARS(S,CHARS)       ((S) += strspn( (S), (CHARS) ))

/**
 * Advances \a S over all whitespace.
 *
 * @param S The string pointer to advance.
 * @return Returns the updated \a S.
 */
#define SKIP_WS(S)                SKIP_CHARS( (S), " \f\r\t\v" )

/**
 * Conditionally returns a space or an empty string.
 *
 * @param S The C string to check.
 * @return If \a S is non-empty, returns `" "`; otherwise returns `""`.
 *
 * @sa SP_AFTER
 */
#define SP_IF(S)                  (S[0] != '\0' ? " " : "")

/**
 * Conditionally returns \a S followed by a space.
 *
 * @param S The C string to check.
 * @return If \a S is non-empty, returns \a S followed by `" "`; otherwise
 * returns `""` followed by `""`.
 *
 * @sa SP_IF
 */
#define SP_AFTER(S)               S, SP_IF(S)

/**
 * Explicit C version of C++'s `static_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa CONST_CAST
 * @sa REINTERPRET_CAST
 */
#define STATIC_CAST(T,EXPR)       ((T)(EXPR))

/**
 * Convenience macro for calling strcpy_end() and updating \a DST.
 *
 * @param DST A pointer to receive the copy of \a SRC.  It is updated to the
 * new end of \a DST.
 * @param SRC The null-terminated string to copy.
 */
#define STRCAT(DST,SRC)           ((DST) = strcpy_end( (DST), (SRC) ))

/**
 * Shorthand for calling **strerror**(3).
 */
#define STRERROR()                strerror( errno )

#ifdef __GNUC__

/**
 * Specifies that \a EXPR is _very_ likely (as in 99.99% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
 *
 * @param EXPR An expression that can be cast to `bool`.
 *
 * @sa unlikely
 * @sa http://lwn.net/Articles/255364/
 */
#define likely(EXPR)              __builtin_expect( !!(EXPR), 1 )

/**
 * Specifies that \a EXPR is _very_ unlikely (as in .01% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
 *
 * @param EXPR An expression that can be cast to `bool`.
 *
 * @sa likely
 * @sa http://lwn.net/Articles/255364/
 */
#define unlikely(EXPR)            __builtin_expect( !!(EXPR), 0 )

#else
# define likely(EXPR)             (EXPR)
# define unlikely(EXPR)           (EXPR)
#endif /* __GNUC__ */

/**
 * Prints that an integer value was unexpected to standard error and exits.
 *
 * @param EXPR The expression having the unexpected value.
 *
 * @sa INTERNAL_ERR
 * @sa PMESSAGE_EXIT
 * @sa UNEXPECTED_STR_VALUE
 */
#define UNEXPECTED_INT_VALUE(EXPR) \
  INTERNAL_ERR( "%lld (0x%llX): unexpected value for " #EXPR "\n", (long long)(EXPR), (long long)(EXPR) )

/**
 * Prints that a string value was unexpected to standard error and exits.
 *
 * @sa INTERNAL_ERR
 * @sa PMESSAGE_EXIT
 * @sa UNEXPECTED_INT_VALUE
 */
#define UNEXPECTED_STR_VALUE(EXPR) \
  INTERNAL_ERR( "\"%s\": unexpected value for " #EXPR "\n", (char const*)(EXPR) )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks whether at most 1 bit is set in the given integer.
 *
 * @param n The number to check.
 * @return Returns `true` only if at most 1 bit is set.
 *
 * @sa exactly_one_bit_set()
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool at_most_one_bit_set( uint64_t n ) {
  return (n & (n - 1)) == 0;
}

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
C_WARN_UNUSED_RESULT
char const* base_name( char const *path_name );

/**
 * Calls **realloc**(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If null, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 */
C_WARN_UNUSED_RESULT
void* check_realloc( void *p, size_t size );

/**
 * Calls **strdup**(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or null.
 * @return Returns a copy of \a s or null if \a s is null.
 */
C_WARN_UNUSED_RESULT
char* check_strdup( char const *s );

/**
 * Checks whether \a s ends with \a c.
 *
 * @param s The string to check.
 * @param s_len The length of \a s.
 * @param c The character to check for.
 * @return Returns `true` only if \a ends with \a c.
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool ends_with_chr( char const *s, size_t s_len, char c ) {
  return s_len > 0 && s[ s_len - 1 ] == c;
}

/**
 * Checks whether exactly 1 bit is set in the given integer.
 *
 * @param n The number to check.
 * @return Returns `true` only if exactly 1 bit is set.
 *
 * @sa at_most_one_bit_set()
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool exactly_one_bit_set( uint64_t n ) {
  return n != 0 && at_most_one_bit_set( n );
}

/**
 * Checks \a flag: if `false`, sets it to `true`.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if `false`,
 * sets it to `true`.
 * @return Returns `true` only if `*flag` is `false` initially.
 *
 * @sa true_or_set()
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool false_set( bool *flag ) {
  return !*flag && (*flag = true);
}

#ifndef HAVE_FMEMOPEN
/**
 * Local implementation of POSIX 2008 **fmemopen**(3) for systems that don't
 * have it.
 *
 * @param buf A pointer to the buffer to use.  The pointer must remain valid
 * for as along as the `FILE` is open.
 * @param size The size of \a buf.
 * @param mode The open mode.  It _must_ contain `r`.
 * @return Returns a `FILE` containing the contents of \a buf.
 */
C_WARN_UNUSED_RESULT
FILE* fmemopen( void *buf, size_t size, char const *mode );
#endif /* HAVE_FMEMOPEN */

/**
 * Adds a pointer to the head of the free-later-list.
 *
 * @param p The pointer to add.
 * @return Returns \a p.
 */
C_WARN_UNUSED_RESULT
void* free_later( void *p );

/**
 * Frees all the memory pointed to by all the nodes in the free-later-list.
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
 * @return Returns said directory or null if it is not obtainable.
 */
C_WARN_UNUSED_RESULT
char const* home_dir( void );

/**
 * Checks whether \a s is a blank line, that is a line consisting only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s is a blank line.
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool is_blank_line( char const *s ) {
  SKIP_WS( s );
  return *s == '\0';
}

/**
 * Checks whether the given file descriptor refers to a regular file.
 *
 * @param fd The file descriptor to check.
 * @return Returns `true` only if \a fd refers to a regular file.
 */
C_WARN_UNUSED_RESULT
bool is_file( int fd );

/**
 * Checks whether \a c is an identifier character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphanumeric or `_`
 * character.
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool is_ident( char c ) {
  return isalnum( c ) || c == '_';
}

/**
 * Checks whether the only bits set in \a bits are among the bits set in \a
 * set.
 *
 * @param bits The bits to check.
 * @param set The set of bits to check against.
 * @return Returns `true` only if the bits set in \a bits are among the bits
 * set in \a set.
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool only_bits_in( uint64_t bits, uint64_t set ) {
  return bits != 0 && (bits & set) == bits;
}

/**
 * Appends a component to a path ensuring that exactly one `/` separates them.
 *
 * @param path The path to append to.
 * The buffer pointed to must be big enough to hold the new path.
 * @param component The component to append.
 */
void path_append( char *path, char const *component );

/**
 * Prints an error message for `errno` to standard error and exits.
 *
 * @param status The exit status code.
 */
C_NORETURN
void perror_exit( int status );

/**
 * Reads an input line:
 *
 *  + Returns only non-whitespace-only lines.
 *  + Stitches multiple lines ending with `\` together.
 *
 * If GNU **readline**(3) is compiled in, also:
 *
 *  + Adds non-whitespace-only lines to the history.
 *
 * @param ps1 The primary prompt to use.
 * @param ps2 The secondary prompt to use for a continuation line (a line after
 * ones ending with `\`).
 * @return Returns the line read or null for EOF.
 */
C_WARN_UNUSED_RESULT
char* read_input_line( char const *ps1, char const *ps2 );

/**
 * Copies a character to \a dst and appends a null.
 *
 * @param dst A pointer to receive \a c.
 * @param c The character to copy.
 * @return Returns a pointer to the new end of \a dst.
 */
C_WARN_UNUSED_RESULT
char* chrcpy_end( char *dst, char c );

/**
 * A variant of **strcpy**(3) that returns the pointer to the new end of \a
 * dst.
 *
 * @param dst A pointer to receive the copy of \a src.
 * @param src The null-terminated string to copy.
 * @return Returns a pointer to the new end of \a dst.
 */
C_WARN_UNUSED_RESULT
char* strcpy_end( char *dst, char const *src );

/**
 * Checks \a flag: if `false`, sets it to `true`.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if `false`, sets
 * it to `true`.
 * @return Returns `true` only if `*flag` is `true` initially.
 *
 * @sa false_set()
 */
C_UTIL_INLINE C_WARN_UNUSED_RESULT
bool true_or_set( bool *flag ) {
  return *flag || !(*flag = true);
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
