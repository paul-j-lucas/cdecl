/*
**      cdecl -- C gibberish translator
**      src/util.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_util_H
#define cdecl_util_H

// local
#include "config.h"                     /* must go first */

// standard
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for FILE */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(A)             (sizeof(A) / sizeof(A[0]))
#define BLOCK(...)                do { __VA_ARGS__ } while (0)
#define PERROR_EXIT(STATUS)       BLOCK( perror( me ); exit( STATUS ); )
#define PRINT_ERR(...)            fprintf( stderr, __VA_ARGS__ )
#define STRERROR                  strerror( errno )

#define INTERNAL_ERR(FORMAT,...) \
  PMESSAGE_EXIT( EX_SOFTWARE, "internal error: " FORMAT, __VA_ARGS__ )

#define MALLOC(TYPE,N) \
  (TYPE*)check_realloc( NULL, sizeof(TYPE) * (N) )

#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( PRINT_ERR( "%s: " FORMAT, me, __VA_ARGS__ ); exit( STATUS ); )

// extern variable definitions
extern char const  *me;                 // executable name

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
 * Concatenates all strings.
 *
 * @param s1 The first null-terminated C string.
 * @return Returns a null-terminated C string that is the concatenation of all
 * arguments.  The called is responsible for freeing the string.
 */
char* cat( char const *s1, ... );

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
 * @param s The null-terminated string to duplicate.
 * @return Returns a copy of \a s.
 */
char* check_strdup( char const *s );

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
FILE* fmemopen( void const *buf, size_t size, char const *mode );
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

/**
 * TODO
 *
 * @param c TODO
 * @return TODO
 */
char const* visible( int c );

/**
 * A variant of strcpy(3) that returns the number of characters copied.
 *
 * @param dst A pointer to receive the copy of \a src.
 * @param src The null-terminated string to copy.
 * @return Returns the number of characters copied.
 */
size_t strcpy_len( char *dst, char const *src );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_util_H */
/* vim:set et sw=2 ts=2: */
