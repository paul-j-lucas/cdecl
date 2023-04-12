/*
**      cdecl -- C gibberish translator
**      src/util.c
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

/**
 * @file
 * Defines utility functions.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_UTIL_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "util.h"
#include "cdecl.h"
#include "slist.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#if HAVE_PWD_H
# include <pwd.h>                       /* for getpwuid() */
#endif /* HAVE_PWD_H */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>                   /* for fstat() */
#include <sysexits.h>
#include <unistd.h>                     /* for geteuid() */

/// @endcond

/**
 * @addtogroup util-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static slist_t free_later_list;         ///< List of stuff to free later.

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for fput_list() that, given a pointer to a pointer to an
 * array of pointer to `char`, returns the pointer to the associated string.
 *
 * @param ppelt A pointer to the pointer to the element to get the string of.
 * On return, it is incremented by the size of the element.
 * @return Returns said string or NULL if none.
 */
NODISCARD
static char const* fput_list_apc_gets( void const **ppelt ) {
  char const *const *const ps = *ppelt;
  *ppelt = ps + 1;
  return *ps;
}

////////// extern functions ///////////////////////////////////////////////////

char const* base_name( char const *path_name ) {
  assert( path_name != NULL );
  char const *const slash = strrchr( path_name, '/' );
  if ( slash != NULL )
    return slash[1] != '\0' ? slash + 1 : slash;
  return path_name;
}

void check_atexit( void (*cleanup_fn)(void) ) {
  assert( cleanup_fn != NULL );
  PERROR_EXIT_IF( atexit( cleanup_fn ) != 0, EX_OSERR );
}

void* check_realloc( void *p, size_t size ) {
  assert( size > 0 );
  p = p != NULL ? realloc( p, size ) : malloc( size );
  PERROR_EXIT_IF( p == NULL, EX_OSERR );
  return p;
}

char* check_strdup( char const *s ) {
  if ( s == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  char *const dup_s = strdup( s );
  PERROR_EXIT_IF( dup_s == NULL, EX_OSERR );
  return dup_s;
}

char* check_strdup_tolower( char const *s ) {
  if ( s == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  char *const dup_s = MALLOC( char, strlen( s ) + 1/*\0*/ );
  PERROR_EXIT_IF( dup_s == NULL, EX_OSERR );
  for ( char *p = dup_s; (*p++ = STATIC_CAST( char, tolower( *s++ ) )); )
    ;
  return dup_s;
}

char* check_strndup( char const *s, size_t n ) {
  if ( s == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  char *const dup_s = strndup( s, n );
  PERROR_EXIT_IF( dup_s == NULL, EX_OSERR );
  return dup_s;
}

#ifdef __GNUC__
  // Silence warning in call to vfprintf().
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif /* __GNUC__ */

void fatal_error( int status, char const *format, ... ) {
  EPRINTF( "%s: ", me );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  _Exit( status );
}

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

bool fd_is_file( int fd ) {
  struct stat fd_stat;
  FSTAT( fd, &fd_stat );
  return S_ISREG( fd_stat.st_mode );
}

#ifndef HAVE_FMEMOPEN
FILE* fmemopen( void *restrict buf, size_t size, char const *restrict mode ) {
  assert( buf != NULL );
  assert( mode != NULL );
  assert( strchr( mode, 'r' ) != NULL );
#ifdef NDEBUG
  (void)mode;
#endif /* NDEBUG */

  if ( unlikely( size == 0 ) )
    return NULL;

  FILE *const temp_file = tmpfile();
  if ( unlikely( temp_file == NULL ) )
    return NULL;

  if ( likely( fwrite( buf, 1, size, temp_file ) == size &&
               fseek( temp_file, 0L, SEEK_SET ) != 0 ) ) {
    return temp_file;
  }

  PJL_IGNORE_RV( fclose( temp_file ) );
  return NULL;
}
#endif /* HAVE_FMEMOPEN */

void fput_list( FILE *out, void const *elt,
                char const* (*gets)( void const** ) ) {
  assert( out != NULL );
  assert( elt != NULL );

  if ( gets == NULL )
    gets = &fput_list_apc_gets;

  char const *s = (*gets)( &elt );
  for ( size_t i = 0; s != NULL; ++i ) {
    char const *const next_s = (*gets)( &elt );
    if ( i > 0 )
      FPUTS( next_s != NULL ? ", " : i > 1 ? ", or " : " or ", out );
    FPUTS( s, out );
    s = next_s;
  } // for
}

void fputs_sp( char const *s, FILE *out ) {
  if ( s[0] != '\0' )
    FPRINTF( out, "%s ", s );
}

void* free_later( void *p ) {
  assert( p != NULL );
  slist_push_back( &free_later_list, p );
  return p;
}

void free_now( void ) {
  slist_cleanup( &free_later_list, &free );
}

char const* home_dir( void ) {
  static char const *home;

  RUN_ONCE {
    home = null_if_empty( getenv( "HOME" ) );
#if HAVE_GETEUID && HAVE_GETPWUID && HAVE_STRUCT_PASSWD_PW_DIR
    if ( home == NULL ) {
      struct passwd *const pw = getpwuid( geteuid() );
      if ( pw != NULL )
        home = null_if_empty( pw->pw_dir );
    }
#endif /* HAVE_GETEUID && && HAVE_GETPWUID && HAVE_STRUCT_PASSWD_PW_DIR */
  }

  return home;
}

bool is_ident_prefix( char const *ident, size_t ident_len, char const *s,
                      size_t s_len ) {
  assert( ident != NULL );
  assert( s != NULL );
  if ( ident_len > s_len || strncmp( s, ident, ident_len ) != 0 )
    return false;
  return !is_ident( s[ ident_len ] );
}

uint32_t ls_bit1_32( uint32_t n ) {
  if ( n != 0 ) {
    for ( uint32_t b = 1; b != 0; b <<= 1 ) {
      if ( (n & b) != 0 )
        return b;
    } // for
  }
  return 0;
}

uint32_t ms_bit1_32( uint32_t n ) {
  if ( n != 0 ) {
    for ( uint32_t b = 0x80000000u; b != 0; b >>= 1 ) {
      if ( (n & b) != 0 )
        return b;
    } // for
  }
  return 0;
}

char const* parse_identifier( char const *s ) {
  assert( s != NULL );
  if ( !(isalpha( s[0] ) || s[0] == '_') )
    return NULL;
  while ( is_ident( *++s ) )
    ;
  return s;
}

bool path_is_file( char const *path ) {
  struct stat path_stat;
  STAT( path, &path_stat );
  return S_ISREG( path_stat.st_mode );
}

bool str_is_prefix( char const *s1, char const *s2 ) {
  assert( s1 != NULL );
  assert( s2 != NULL );
  if ( s1[0] == '\0' )
    return false;
  do {
    if ( *s1++ != *s2++ )
      return false;
  } while ( *s1 != '\0' );
  return true;
}

void strn_rtrim( char const *s, size_t *s_len ) {
  assert( s != NULL );
  assert( s_len != NULL );

  while ( *s_len > 0 && strchr( WS, s[ *s_len - 1 ] ) != NULL )
    --*s_len;
}

size_t strnspn( char const *s, char const *charset, size_t n ) {
  assert( s != NULL );
  assert( charset != NULL );

  char const *const s0 = s;
  while ( n-- > 0 && strchr( charset, *s ) != NULL )
    ++s;
  return STATIC_CAST( size_t, s - s0 );
}

// LCOV_EXCL_START
void perror_exit( int status ) {
  perror( me );
  exit( status );
}
// LCOV_EXCL_STOP

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
