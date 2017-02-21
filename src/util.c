/*
**      cdecl -- C gibberish translator
**      src/util.c
**
**      Paul J. Lucas
*/

// local
#include "config.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

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

char* cat( char const *s1, ... ) {
  va_list args;
  size_t len = 0;

  // first, find the length that needs to be allocated
  va_start( args, s1 );
  for ( char const *s = s1; s; s = va_arg( args, char const* ) )
    len += strlen( s );
  va_end( args );

  char *const combined = MALLOC( char, len + 1/*null*/ );
  combined[0] = '\0';

  va_start( args, s1 );
  for ( char const *s = s1; s; s = va_arg( args, char const* ) ) {
    strcat( combined, s );
    FREE( s );
  } // for
  va_end( args );

  return combined;
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
  assert( s );
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

char const* printable_char( char c ) {
  switch( c ) {
    case '\0': return "\\0";
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    case '\v': return "\\v";
  } // switch

  static char buf[5];                   // \xHH + NULL
  if ( isprint( c ) )
    buf[0] = c, buf[1] = '\0';
  else
    snprintf( buf, sizeof buf, "\\x%02X", (unsigned)c );
  return buf;
}

size_t strcpy_len( char *dst, char const *src ) {
  assert( dst );
  assert( src );
  char const *const dst0 = dst;
  while ( (*dst++ = *src++) )
    /* empty */;
  return dst - dst0 - 1;
}

char* trim_ws( char *s ) {
  assert( s );
  s += strspn( s, " \t\r" );
  for ( size_t len = strlen( s ); len > 0 && isspace( s[ --len ] ); )
    s[ len ] = '\0';
  return s;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
