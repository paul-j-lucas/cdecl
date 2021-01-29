/*
**      cdecl -- C gibberish translator
**      src/util.h
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
 * Gets a value where all bits that are greater than or equal to the one bit
 * set in \a N are also set, e.g., <code>%BITS_GE(00010000)</code> =
 * `11110000`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_GT()
 * @sa #LSB_SET()
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
 */
#define BITS_GT(N)                (~((N) | BITS_GE(N)))

/**
 * Embeds the given statements into a compound statement block.
 *
 * @param ... The statement(s) to embed.
 */
#define BLOCK(...)                do { __VA_ARGS__ } while (0)

/**
 * Gets a pointer to one past the end of \a BUF.
 *
 * @param BUF The buffer.  It _must_ be an array of known size.
 * @return Returns a pointer to one past the end of \a BUF.
 */
#define BUF_END(BUF)              ((BUF) + sizeof( BUF ))

/**
 * Explicit C version of C++'s `const_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa #REINTERPRET_CAST()
 * @sa #STATIC_CAST()
 */
#define CONST_CAST(T,EXPR)        ((T)(uintptr_t)(EXPR))

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
 * @sa #PUTC()
 * @sa #EPUTS()
 */
#define EPUTC(C)                  FPUTC( (C), stderr )

/**
 * Shorthand for printing a C string to standard error.
 *
 * @param S The C string to print.
 *
 * @sa #EPRINTF()
 * @sa #EPUTC()
 * @sa #PUTS()
 */
#define EPUTS(S)                  FPUTS( (S), stderr )

/**
 * Calls **ferror**(3) and exits if there was an error on \a STREAM.
 *
 * @param STREAM The `FILE` stream to check for an error.
 *
 * @sa #IF_EXIT()
 */
#define FERROR(STREAM)            IF_EXIT( ferror( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fflush(3)** on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to flush.
 *
 * @sa #IF_EXIT()
 */
#define FFLUSH(STREAM)            IF_EXIT( fflush( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fprintf**(3) on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to print to.
 * @param ... The `fprintf()` arguments.
 *
 * @sa #EPRINTF()
 * @sa #IF_EXIT()
 */
#define FPRINTF(STREAM,...) \
  IF_EXIT( fprintf( (STREAM), __VA_ARGS__ ) < 0, EX_IOERR )

/**
 * Calls **putc**(3), checks for an error, and exits if there was one.
 *
 * @param C The character to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPRINTF()
 * @sa #FPUTS()
 * @sa #IF_EXIT()
 */
#define FPUTC(C,STREAM) \
  IF_EXIT( putc( (C), (STREAM) ) == EOF, EX_IOERR )

/**
 * Calls **fputs**(3), checks for an error, and exits if there was one.
 *
 * @param S The C string to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPRINTF()
 * @sa #FPUTC()
 * @sa #IF_EXIT()
 */
#define FPUTS(S,STREAM) \
  IF_EXIT( fputs( (S), (STREAM) ) == EOF, EX_IOERR )

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
 *
 * @sa perror_exit()
 * @sa #PMESSAGE_EXIT()
 * @sa #UNEXPECTED_INT_VALUE()
 * @sa #UNEXPECTED_STR_VALUE()
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
 * @sa perror_exit()
 * @sa #PMESSAGE_EXIT()
 * @sa #UNEXPECTED_INT_VALUE()
 * @sa #UNEXPECTED_STR_VALUE()
 */
#define INTERNAL_ERR(FORMAT,...) \
  PMESSAGE_EXIT( EX_SOFTWARE, "%s:%d: internal error: " FORMAT, __FILE__, __LINE__, __VA_ARGS__ )

/**
 * Gets only the least significant bit of \a N that is set.
 *
 * @param N The integer to get the least significant bit of.
 * @return Returns the value of said bit.
 *
 * @sa #BITS_GE()
 */
#define LSB_SET(N)                ((N) & BITS_GE(N))

/**
 * Convenience macro for calling check_realloc().
 *
 * @param TYPE The type to allocate.
 * @param N The number of objects of \a TYPE to allocate.
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
  __typeof__(PTR) _tmp __attribute__((unused)) = 0;                     \
  memset( (PTR), 0, sizeof *(PTR) ); )
#else
#define MEM_ZERO(PTR)             memset( (PTR), 0, sizeof *(PTR) )
#endif /* HAVE___TYPEOF__ */

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
 * @sa #INTERNAL_ERR()
 * @sa perror_exit()
 * @sa #UNEXPECTED_INT_VALUE()
 * @sa #UNEXPECTED_STR_VALUE()
 */
#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( EPRINTF( "%s: " FORMAT, me, __VA_ARGS__ ); exit( STATUS ); )

/**
 * Shorthand for printing a character to standard output.
 *
 * @param C The character to print.
 *
 * @sa #EPUTC()
 * @sa #PUTS()
 */
#define PUTC(C)                   FPUTC( (C), stdout )

/**
 * Shorthand for printing a C string to standard output.
 *
 * @param S The C string to print.
 *
 * @sa #PUTC()
 * @sa #EPUTS()
 */
#define PUTS(S)                   FPUTS( (S), stdout )

/**
 * Convenience macro for calling check_realloc().
 *
 * @param PTR The pointer to memory to reallocate.  It is set to the newly
 * reallocated memory.
 * @param TYPE The type to cast the pointer returned by **realloc**(3) to.
 * @param N The number of objects of \a TYPE to reallocate.
 *
 * @sa check_realloc()
 * @sa #MALLOC()
 */
#define REALLOC(PTR,TYPE,N) \
  ((PTR) = check_realloc( (PTR), sizeof(TYPE) * (N) ))

/**
 * Explicit C version of C++'s `reinterpret_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa #CONST_CAST()
 * @sa #STATIC_CAST()
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
 * @sa #SP_AFTER()
 */
#define SP_IF(S)                  (S[0] != '\0' ? " " : "")

/**
 * Conditionally returns \a S followed by a space.
 *
 * @param S The C string to check.
 * @return If \a S is non-empty, returns \a S followed by `" "`; otherwise
 * returns `""` followed by `""`.
 *
 * @sa #SP_IF()
 */
#define SP_AFTER(S)               S, SP_IF(S)

/**
 * Explicit C version of C++'s `static_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa #CONST_CAST()
 * @sa #REINTERPRET_CAST()
 */
#define STATIC_CAST(T,EXPR)       ((T)(EXPR))

/**
 * Convenience macro for calling strcpy_end() and updating \a DST.
 *
 * @param DST A pointer to receive the copy of \a SRC.  It is updated to the
 * new end of \a DST.
 * @param SRC The null-terminated string to copy.
 *
 * @sa strcpy_end()
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
 * Prints that an integer value was unexpected to standard error and exits.
 *
 * @param EXPR The expression having the unexpected value.
 *
 * @sa #INTERNAL_ERR()
 * @sa #PMESSAGE_EXIT()
 * @sa #UNEXPECTED_STR_VALUE()
 */
#define UNEXPECTED_INT_VALUE(EXPR) \
  INTERNAL_ERR( "%lld (0x%llX): unexpected value for " #EXPR "\n", (long long)(EXPR), (long long)(EXPR) )

/**
 * Prints that a string value was unexpected to standard error and exits.
 *
 * @sa #INTERNAL_ERR()
 * @sa #PMESSAGE_EXIT()
 * @sa #UNEXPECTED_INT_VALUE()
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
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
char const* base_name( char const *path_name );

/**
 * Calls **realloc**(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If null, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 *
 * @sa #MALLOC()
 * @sa #REALLOC()
 */
PJL_WARN_UNUSED_RESULT
void* check_realloc( void *p, size_t size );

/**
 * Calls **strdup**(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or null.
 * @return Returns a copy of \a s or null if \a s is null.
 *
 * @sa check_strdup_tolower()
 */
PJL_WARN_UNUSED_RESULT
char* check_strdup( char const *s );

/**
 * Duplicates \a s and checks for failure, but converts all characters to
 * lower-case.  If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or null.
 * @return Returns a copy of \a s with all characters converted to lower-case
 * or null if \a s is null.
 *
 * @sa check_strdup()
 */
PJL_WARN_UNUSED_RESULT
char* check_strdup_tolower( char const *s );

/**
 * Checks whether \a s ends with any character in \a set.
 *
 * @param s The string to check.
 * @param s_len The length of \a s.
 * @param set The set of characters to check for.
 * @return Returns `true` only if \a ends with \a c.
 *
 * @sa ends_with_chr()
 */
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
bool ends_with_any_chr( char const *s, size_t s_len, char const *set ) {
  return s_len > 0 && strchr( set, s[ s_len - 1 ] ) != NULL;
}

/**
 * Checks whether \a s ends with \a c.
 *
 * @param s The string to check.
 * @param s_len The length of \a s.
 * @param c The character to check for.
 * @return Returns `true` only if \a s ends with \a c.
 *
 * @sa ends_with_any_chr()
 */
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
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
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
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
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
FILE* fmemopen( void *buf, size_t size, char const *mode );
#endif /* HAVE_FMEMOPEN */

/**
 * Adds a pointer to the head of the free-later-list.
 *
 * @param p The pointer to add.
 * @return Returns \a p.
 */
PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
char const* home_dir( void );

/**
 * Checks whether \a s is a blank line, that is a line consisting only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s is a blank line.
 */
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
bool is_file( int fd );

/**
 * Checks whether \a c is an identifier character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphanumeric or `_`
 * character.
 */
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
bool is_ident( char c ) {
  return isalnum( c ) || c == '_';
}

/**
 * Checks whether the bits set in \a bits are only among the bits set in \a
 * allowed_bits, i.e., there is no bit set in \a bits that is not also set in
 * \a allowed_bits.
 *
 * @param bits The bits to check.
 * @param allowed_bits The bits to check against.
 * @return Returns `true` only if \a bits is not zero and the bits set are only
 * among the bits set in \a allowed_bits.
 */
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
bool only_bits_set( uint64_t bits, uint64_t allowed_bits ) {
  return bits != 0 && (bits & allowed_bits) == bits;
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
 *
 * @sa #IF_EXIT()
 * @sa #INTERNAL_ERR()
 * @sa #PMESSAGE_EXIT()
 */
noreturn void perror_exit( int status );

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
 * @warning The pointer returned is to a static buffer and every calls resets
 * it.
 */
PJL_WARN_UNUSED_RESULT
char* read_input_line( char const *ps1, char const *ps2 );

/**
 * Copies a character to \a dst and appends a null.
 *
 * @param dst A pointer to receive \a c.
 * @param c The character to copy.
 * @return Returns a pointer to the new end of \a dst.
 *
 * @sa strcpy_end()
 */
PJL_WARN_UNUSED_RESULT
char* chrcpy_end( char *dst, char c );

/**
 * A variant of **strcpy**(3) that returns the pointer to the new end of \a
 * dst.
 *
 * @param dst A pointer to receive the copy of \a src.
 * @param src The null-terminated string to copy.
 * @return Returns a pointer to the new end of \a dst.
 *
 * @sa chrcpy_end()
 * @sa #STRCAT()
 */
PJL_WARN_UNUSED_RESULT
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
C_UTIL_INLINE PJL_WARN_UNUSED_RESULT
bool true_or_set( bool *flag ) {
  return *flag || !(*flag = true);
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
