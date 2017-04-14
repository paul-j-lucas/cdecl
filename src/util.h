/*
**      cdecl -- C gibberish translator
**      src/util.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "config.h"                     /* must go first */

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uintptr_t */
#include <stdio.h>                      /* for FILE */
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_UTIL_INLINE
# define CDECL_UTIL_INLINE _GL_INLINE
#endif /* CDECL_UTIL_INLINE */

///////////////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(A)             (sizeof(A) / sizeof(A[0]))
#define BLOCK(...)                do { __VA_ARGS__ } while (0)
#define CONST_CAST(T,EXPR)        ((T)(EXPR))
#define FREE(PTR)                 free( (void*)(PTR) )
#define NO_OP                     ((void)0)
#define PERROR_EXIT(STATUS)       BLOCK( perror( me ); exit( STATUS ); )
#define PRINT_ERR(...)            fprintf( stderr, __VA_ARGS__ )
#define PUTC_ERR(C)               FPUTC( (C), stderr )
#define PUTC_OUT(C)               FPUTC( (C), stdout )
#define PUTS_ERR(S)               FPUTS( (S), stderr )
#define PUTS_OUT(S)               FPUTS( (S), stdout )
#define REINTERPRET_CAST(T,EXPR)  ((T)(uintptr_t)(EXPR))
#define STRERROR                  strerror( errno )

#define INTERNAL_ERR(FORMAT,...) \
  PMESSAGE_EXIT( EX_SOFTWARE, "internal error: " FORMAT, __VA_ARGS__ )

#define MALLOC(TYPE,N) \
  (TYPE*)check_realloc( NULL, sizeof(TYPE) * (N) )

#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( PRINT_ERR( "%s: " FORMAT, me, __VA_ARGS__ ); exit( STATUS ); )

#define FERROR(STREAM) \
  BLOCK( if ( ferror( STREAM ) ) PERROR_EXIT( EX_IOERR ); )

#define FFLUSH(STREAM) \
  BLOCK( if ( fflush( STREAM ) != 0 ) PERROR_EXIT( EX_IOERR ); )

#define FPRINTF(STREAM,...) \
  BLOCK( if ( fprintf( (STREAM), __VA_ARGS__ ) < 0 ) PERROR_EXIT( EX_IOERR ); )

#define FPUTC(C,STREAM) \
  BLOCK( if ( putc( (C), (STREAM) ) == EOF ) PERROR_EXIT( EX_IOERR ); )

#define FPUTS(S,STREAM) \
  BLOCK( if ( fputs( (S), (STREAM) ) == EOF ) PERROR_EXIT( EX_IOERR ); )

#define FSTAT(FD,STAT) \
  BLOCK( if ( fstat( (FD), (STAT) ) < 0 ) PERROR_EXIT( EX_IOERR ); )

#define REALLOC(PTR,TYPE,N) \
  (PTR) = (TYPE*)check_realloc( (PTR), sizeof(TYPE) * (N) )

///////////////////////////////////////////////////////////////////////////////

typedef struct link link_t;

/**
 * A simple \c struct that serves as a "base class" for an intrusive singly
 * linked list. A "derived class" \e must be a \c struct that has a \c next
 * pointer as its first member.
 */
struct link {
  link_t *next;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Extracts the base portion of a path-name.
 * Unlike \c basename(3):
 *  + Trailing \c '/' characters are not deleted.
 *  + \a path_name is never modified (hence can therefore be \c const).
 *  + Returns a pointer within \a path_name (hence is multi-call safe).
 *
 * @param path_name The path-name to extract the base portion of.
 * @return Returns a pointer to the last component of \a path_name.
 * If \a path_name consists entirely of '/' characters,
 * a pointer to the string "/" is returned.
 */
char const* base_name( char const *path_name );

/**
 * Calls \c realloc(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If NULL, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 */
void* check_realloc( void *p, size_t size );

/**
 * Calls \c strdup(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or null.
 * @return Returns a copy of \a s or null if \a s is null.
 */
char* check_strdup( char const *s );

/**
 * Checks whether \a s ends with \a c.
 *
 * @param s The string to check.
 * @param s_len The length of \a s.
 * @param c The character to check for.
 * @return Returns \c true only if \a ends with \a c.
 */
CDECL_UTIL_INLINE bool ends_with_chr( char const *s, size_t s_len, char c ) {
  return s_len > 0 && s[ s_len - 1 ] == c;
}

/**
 * Checks the flag: if \c false, sets it to \c true.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if \c false,
 * sets it to \c true.
 * @return Returns \c true only if \c *flag is \c false initially.
 */
CDECL_UTIL_INLINE bool false_set( bool *flag ) {
  return !*flag && (*flag = true);
}

#ifndef HAVE_FMEMOPEN
/**
 * Local implementation of POSIX 2008 fmemopen(3) for systems that don't have
 * it.
 *
 * @param buf A pointer to the buffer to use.  The pointer must remain valid
 * for as along as the FILE is open.
 * @param size The size of \a buf.
 * @param mode The open mode.  It \e must contain \c r.
 * @return Returns a FILE containing the contents of \a buf.
 */
FILE* fmemopen( void *buf, size_t size, char const *mode );
#endif /* HAVE_FMEMOPEN */

/**
 * Adds a pointer to the head of the free-later-list.
 *
 * @param p The pointer to add.
 * @return Returns \a p.
 */
void* free_later( void *p );

/**
 * Frees all the memory pointed to by all the nodes in the free-later-list.
 */
void free_now( void );

#ifdef WITH_TERM_COLUMNS
/**
 * Gets the number of columns of the terminal.
 *
 * @return Returns said number of columns or 0 if it can not be determined.
 */
unsigned get_term_columns( void );
#endif /* WITH_TERM_COLUMNS */

/**
 * Checks whether \a s is a blank line, that is a line consisting only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return Returns \c true only if \a s is a blank line.
 */
CDECL_UTIL_INLINE bool is_blank_line( char const *s ) {
  s += strspn( s, " \t\r\n" );
  return !*s;
}

/**
 * Checks whether the given file descriptor refers to a regular file.
 *
 * @param fd The file descriptor to check.
 * @return Returns \c true only if \a fd refers to a regular file.
 */
bool is_file( int fd );

/**
 * Pops a node from the head of a list.
 *
 * @param phead The pointer to the pointer to the head of the list.
 * @return Returns the popped node or null if the list is empty.
 */
link_t* link_pop( link_t **phead );

/**
 * Convenience macro that pops a node from the head of a list and does the
 * necessary casting.
 *
 * @param NODE_TYPE The type of the node.
 * @param PHEAD A pointer to the pointer of the head of the list.
 * @return Returns the popped node or null if the list is empty.
 * @hideinitializer
 */
#define LINK_POP(NODE_TYPE,PHEAD) \
  (NODE_TYPE*)link_pop( (link_t**)(PHEAD) )

/**
 * Pushes a node onto the front of a list.
 *
 * @param phead The pointer to the pointer to the head of the list.  The head
 * is updated to point to \a node.
 * @param node The pointer to the node to add.  Its \c next pointer is set to
 * the old head of the list.
 */
void link_push( link_t **phead, link_t *node );

/**
 * Convenience macro that pushes a node onto the front of a list and does the
 * necessary casting.
 *
 * @param PHEAD The pointer to the pointer to the head of the list.  The head
 * is updated to point to \a NODE.
 * @param NODE The pointer to the node to add.  Its \c next pointer is set to
 * the old head of the list.
 * @hideinitializer
 */
#define LINK_PUSH(PHEAD,NODE) \
  link_push( (link_t**)(PHEAD), (link_t*)(NODE) )

/**
 * Checks whether only 1 bit is set in the given integer.
 *
 * @param n The number to check.
 * @reeturn Returns \c true only if exactly 1 bit is set.
 */
CDECL_UTIL_INLINE bool only_one_bit_set( unsigned n ) {
  return n && !(n & (n - 1));
}

/**
 * Wraps GNU readline(3) by:
 *
 *  + Adding non-whitespace-only lines to the history.
 *  + Returning only non-whitespace-only lines.
 *  + Stitching multiple lines ending with '\' together.
 *
 * If readline(3) is not compiled in, uses getline(3).
 *
 * @param ps1 The primary prompt to use.
 * @param ps2 The secondary prompt to use.
 * @return Returns the line read or null for EOF.
 */
char* readline_wrapper( char const *ps1, char const *ps2 );

/**
 * A variant of strcpy(3) that returns the number of characters copied.
 *
 * @param dst A pointer to receive the copy of \a src.
 * @param src The null-terminated string to copy.
 * @return Returns the number of characters copied.
 */
size_t strcpy_len( char *dst, char const *src );

/**
 * Checks the flag: if \c false, sets it to \c true.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if \c false,
 * sets it to \c true.
 * @return Returns \c true only if \c *flag is \c true initially.
 */
CDECL_UTIL_INLINE bool true_or_set( bool *flag ) {
  return *flag || !(*flag = true);
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
