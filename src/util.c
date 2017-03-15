/*
**      cdecl -- C gibberish translator
**      src/util.c
**
**      Paul J. Lucas
*/

// local
#include "config.h"                     /* must go first */
#include "common.h"
#define CDECL_UTIL_INLINE _GL_EXTERN_INLINE
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>                   /* for fstat() */
#include <sysexits.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif /* HAVE_READLINE_READLINE_H */
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY_H */

///////////////////////////////////////////////////////////////////////////////

/**
 * A node for a singly linked list of pointers to memory to be freed via
 * \c atexit().
 */
struct free_node {
  void             *fn_ptr;
  struct free_node *fn_next;
};
typedef struct free_node free_node_t;

// local variable definitions
static free_node_t *free_head;          // linked list of stuff to free

///////////////////////////////////////////////////////////////////////////////

char const* base_name( char const *path_name ) {
  assert( path_name );
  char const *const slash = strrchr( path_name, '/' );
  if ( slash )
    return slash[1] ? slash + 1 : slash;
  return path_name;
}

void* check_realloc( void *p, size_t size ) {
  //
  // Autoconf, 5.5.1:
  //
  // realloc
  //    The C standard says a call realloc(NULL, size) is equivalent to
  //    malloc(size), but some old systems don't support this (e.g., NextStep).
  //
  if ( !size )
    size = 1;
  void *const r = p ? realloc( p, size ) : malloc( size );
  if ( !r )
    PERROR_EXIT( EX_OSERR );
  return r;
}

char* check_strdup( char const *s ) {
  if ( !s )
    return NULL;
  char *const dup = strdup( s );
  if ( !dup )
    PERROR_EXIT( EX_OSERR );
  return dup;
}

#ifndef HAVE_FMEMOPEN
FILE* fmemopen( void const *buf, size_t size, char const *mode ) {
  assert( buf );
  assert( strchr( mode, 'r' ) );

  FILE *const tmp = tmpfile();
  if ( !tmp )
    PERROR_EXIT( EX_OSERR );
  if ( size > 0 ) {
    if ( fwrite( buf, 1, size, tmp ) != size )
      PERROR_EXIT( EX_OSERR );
    rewind( tmp );
  }
  return tmp;
}
#endif /* HAVE_FMEMOPEN */

void* free_later( void *p ) {
  assert( p );
  free_node_t *const new_node = MALLOC( free_node_t, 1 );
  new_node->fn_ptr = p;
  new_node->fn_next = free_head;
  free_head = new_node;
  return p;
}

void free_now( void ) {
  for ( free_node_t *p = free_head; p; ) {
    free_node_t *const next = p->fn_next;
    FREE( p->fn_ptr );
    FREE( p );
    p = next;
  } // for
  free_head = NULL;
}

bool is_file( int fd ) {
  struct stat fd_stat;
  FSTAT( fd, &fd_stat );
  return S_ISREG( fd_stat.st_mode );
}

void json_print_kv( char const *key, char const *value, FILE *jout ) {
  assert( key );
  if ( value && *value )
    FPRINTF( jout, "\"%s\": \"%s\"", key, value );
  else
    FPRINTF( jout, "\"%s\": null", key  );
}

char* readline_wrapper( char const *prompt ) {
  for (;;) {
    static char *line_read;
#ifdef HAVE_READLINE
    extern void readline_init( void );
    if ( !line_read )
      readline_init();
    else
      free( line_read );

    if ( !(line_read = readline( prompt )) )
      return NULL;
#else
    static size_t line_cap;
    if ( getline( &line_read, &line_cap, stdin ) == -1 ) {
      FERROR( stdin );
      return NULL;
    }
#endif /* HAVE_READLINE */

    if ( !is_blank_line( line_read ) ) {
#ifdef HAVE_READLINE
      add_history( line_read );
      //
      // readline() removes newlines, but we need newlines in the lexer to know
      // when to reset y_token_col, so we have to put a newline back.
      //
      char *const tmp = MALLOC( char, strlen( line_read ) + 1/*\n*/ + 1/*\0*/ );
      size_t const len = strcpy_len( tmp, line_read );
      free( line_read );
      line_read = tmp;
      strcpy( line_read + len, "\n" );
#endif /* HAVE_READLINE */
      return line_read;
    }
  } // for
}

link_t* link_pop( link_t **phead ) {
  assert( phead );
  if ( *phead ) {
    link_t *const popped = (*phead);
    (*phead) = popped->next;
    popped->next = NULL;
    return popped;
  }
  return NULL;
}

void link_push( link_t **phead, link_t *node ) {
  assert( phead );
  assert( node );
  assert( !node->next );
  node->next = (*phead);
  (*phead) = node;
}

size_t strcpy_len( char *dst, char const *src ) {
  assert( dst );
  assert( src );
  char const *const dst0 = dst;
  while ( (*dst++ = *src++) )
    /* empty */;
  return dst - dst0 - 1;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
