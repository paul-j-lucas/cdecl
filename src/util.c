/*
**      cdecl -- C gibberish translator
**      src/util.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
#include <errno.h>
#ifndef NDEBUG
#include <signal.h>                     /* for raise(3) */
#endif /* NDEBUG */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>                   /* for fstat() */
#include <sysexits.h>

/// @endcond

/**
 * @addtogroup util-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static slist_t free_later_list;         ///< List of stuff to free later.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a s contains only decimal digit characters.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s contains only decimal digits.
 */
NODISCARD
static inline bool str_is_digits( char const *s ) {
  return *SKIP_CHARS( s, "0123456789" ) == '\0';
}

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

/**
 * Checks whether \a s is any one of \a matches, case-insensitive.
 *
 * @param s The null-terminated string to check or null.  May be NULL.
 * @param matches The null-terminated array of values to check against.
 * @return Returns `true` only if \a s is among \a matches.
 */
NODISCARD
static bool str_is_any( char const *s,
                        char const *const matches[const static 2] ) {
  if ( s != NULL ) {
    for ( char const *const *match = matches; *match != NULL; ++match ) {
      if ( strcasecmp( s, *match ) == 0 )
        return true;
    } // for
  }
  return false;
}

////////// extern functions ///////////////////////////////////////////////////

char const* base_name( char const *path_name ) {
  assert( path_name != NULL );
  char const *const slash = strrchr( path_name, '/' );
  if ( slash != NULL )
    return slash[1] != '\0' ? slash + 1 : slash;
  return path_name;
}

char* check_prefix_strdup( char const *prefix, size_t prefix_len,
                           char const *s ) {
  assert( prefix != NULL );
  assert( s != NULL );

  char *const dup_s = MALLOC( char, prefix_len + strlen( s ) + 1/*\0*/ );
  strncpy( dup_s, prefix, prefix_len );
  strcpy( dup_s + prefix_len, s );
  return dup_s;
}

void* check_realloc( void *p, size_t size ) {
  assert( size > 0 );
  p = p != NULL ? realloc( p, size ) : malloc( size );
  PERROR_EXIT_IF( p == NULL, EX_OSERR );
  return p;
}

void check_snprintf( char *buf, size_t buf_size, char const *format, ... ) {
  assert( buf != NULL );
  assert( format != NULL );

  va_list args;
  va_start( args, format );
  int const raw_len = vsnprintf( buf, buf_size, format, args );
  va_end( args );

  PERROR_EXIT_IF( raw_len < 0, EX_OSERR );
  PERROR_EXIT_IF( STATIC_CAST( size_t, raw_len ) >= buf_size, EX_SOFTWARE );
}

char* check_strdup( char const *s ) {
  if ( s == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  char *const dup_s = strdup( s );
  PERROR_EXIT_IF( dup_s == NULL, EX_OSERR );
  return dup_s;
}

char* check_strdup_suffix( char const *s, char const *suffix,
                           size_t suffix_len ) {
  assert( s != NULL );
  assert( suffix != NULL );

  size_t const s_len = strlen( s );
  size_t const dup_len = s_len + suffix_len;
  char *const dup_s = MALLOC( char, dup_len + 1/*\0*/ );
  strcpy( dup_s, s );
  strncpy( dup_s + s_len, suffix, suffix_len );
  dup_s[ dup_len ] = '\0';
  return dup_s;
}

char* check_strdup_tolower( char const *s ) {
  if ( s == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  char *const dup_s = MALLOC( char, strlen( s ) + 1/*\0*/ );
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

unsigned long long check_strtoull( char const *s, unsigned long long min,
                                   unsigned long long max ) {
  assert( s != NULL );

  if ( !str_is_digits( s ) )
    return STRTOULL_ERROR;
  errno = 0;
  unsigned long long const rv = strtoull( s, /*endptr=*/NULL, 10 );
  if ( errno == ERANGE || rv < min || rv > max )
    return STRTOULL_ERROR;
  return rv;
}

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif /* __GNUC__ */

void fatal_error( int status, char const *format, ... ) {
  assert( format != NULL );
  EPRINTF( "%s: ", me );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  exit( status );
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
FILE* fmemopen( void *buf, size_t size, char const *mode ) {
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

  fclose( temp_file );
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

void fputs_quoted( char const *s, char quote, FILE *fout ) {
  assert( quote == '\'' || quote == '"' );
  assert( fout != NULL );

  if ( s == NULL ) {
    FPUTS( "null", fout );
    return;
  }

  bool in_quote = false;
  char const other_quote = quote == '\'' ? '"' : '\'';

  FPUTC( quote, fout );
  for ( char prev = '\0'; *s != '\0'; prev = *s++ ) {
    switch ( *s ) {
      case '\b': FPUTS( "\\b", fout ); continue;
      case '\f': FPUTS( "\\f", fout ); continue;
      case '\n': FPUTS( "\\n", fout ); continue;
      case '\r': FPUTS( "\\r", fout ); continue;
      case '\t': FPUTS( "\\t", fout ); continue;
      case '\v': FPUTS( "\\v", fout ); continue;
      case '\\':
        if ( in_quote ) {
          if ( prev != '\\' )
            FPUTS( "\\\\", fout );
          continue;
        }
        break;
    } // switch

    if ( prev != '\\' ) {
      if ( *s == quote ) {
        FPUTC( '\\', fout );
        in_quote = !in_quote;
      }
      else if ( *s == other_quote ) {
        in_quote = !in_quote;
      }
    }

    FPUTC( *s, fout );
  } // for
  FPUTC( quote, fout );
}

void fputs_sp( char const *s, FILE *out ) {
  assert( s != NULL );
  assert( out != NULL );
  if ( s[0] != '\0' )
    FPRINTF( out, "%s ", s );
}

void fputsp_s( char const *s, FILE *out ) {
  assert( s != NULL );
  assert( out != NULL );
  if ( s[0] != '\0' )
    FPRINTF( out, " %s", s );
}

void* free_later( void *p ) {
  assert( p != NULL );
  slist_push_back( &free_later_list, p );
  return p;
}

void free_now( void ) {
  slist_cleanup( &free_later_list, &free );
}

uint32_t ls_bit1_32( uint32_t n ) {
  if ( n != 0 ) {
    for ( uint32_t b = 1; b != 0; b <<= 1 ) {
      if ( (n & b) != 0 )
        return b;
    } // for
  }
  return 0;                             // LCOV_EXCL_LINE
}

uint32_t ms_bit1_32( uint32_t n ) {
  if ( n != 0 ) {
    for ( uint32_t b = 0x80000000u; b != 0; b >>= 1 ) {
      if ( (n & b) != 0 )
        return b;
    } // for
  }
  return 0;                             // LCOV_EXCL_LINE
}

char const* parse_identifier( char const *s ) {
  assert( s != NULL );
  if ( !is_ident_first( s[0] ) )
    return NULL;
  while ( is_ident( *++s ) )
    ;
  return s;
}

bool path_is_file( char const *path ) {
  assert( path != NULL );
  struct stat path_stat;
  STAT( path, &path_stat );
  return S_ISREG( path_stat.st_mode );
}

// LCOV_EXCL_START
void perror_exit( int status ) {
  perror( me );
  exit( status );
}
// LCOV_EXCL_STOP

bool str_is_affirmative( char const *s ) {
  static char const *const AFFIRMATIVES[] = {
    "1",
    "t",
    "true",
    "y",
    "yes",
    NULL
  };
  return str_is_any( s, AFFIRMATIVES );
}

bool str_is_ident_prefix( char const *ident, size_t ident_len, char const *s,
                          size_t s_len ) {
  assert( ident != NULL );
  assert( s != NULL );
  if ( ident_len > s_len || strncmp( s, ident, ident_len ) != 0 )
    return false;
  return !is_ident( s[ ident_len ] );
}

bool str_is_prefix( char const *si, char const *sj ) {
  assert( si != NULL );
  assert( sj != NULL );
  if ( si[0] == '\0' )
    return false;
  do {
    if ( *si++ != *sj++ )
      return false;
  } while ( *si != '\0' );
  return true;
}

char* str_realloc_cat( char *dst, char const *sep, char const *src ) {
  assert( dst != NULL );
  assert( sep != NULL );
  assert( src != NULL );

  size_t const dst_len = strlen( dst );
  size_t const sep_len = strlen( sep );
  size_t const src_len = strlen( src );

  REALLOC( dst, dst_len + sep_len + src_len + 1/*\0*/ );
  strcpy( dst + dst_len, sep );
  strcpy( dst + dst_len + sep_len, src );
  return dst;
}

char* str_realloc_pcat( char const *src, char const *sep, char *dst ) {
  assert( src != NULL );
  assert( sep != NULL );
  assert( dst != NULL );

  size_t const dst_len = strlen( dst );
  size_t const sep_len = strlen( sep );
  size_t const src_len = strlen( src );

  REALLOC( dst, src_len + sep_len + dst_len + 1/*\0*/ );
  // use memmove() due to overlapping ranges
  memmove( dst + src_len + sep_len, dst, dst_len + 1/*\0*/ );
  // use memcpy() so as not to write a \0
  memcpy( dst, src, src_len );
  memcpy( dst + src_len, sep, sep_len );
  return dst;
}

void strn_rtrim( char const *s, size_t *s_len ) {
  assert( s != NULL );
  assert( s_len != NULL );

  while ( *s_len > 0 && strchr( WS_CHARS, s[ *s_len - 1 ] ) != NULL )
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

#ifndef NDEBUG
// LCOV_EXCL_START
void wait_for_debugger_attach( void ) {
  EPRINTF(
    "%s: pid=%d: waiting for debugger to attach...\n",
    me, STATIC_CAST( int, getpid() )
  );
  PERROR_EXIT_IF( raise( SIGSTOP ) == -1, EX_OSERR );
}
// LCOV_EXCL_STOP
#endif /* NDEBUG */

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
