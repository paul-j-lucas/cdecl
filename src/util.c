/*
**      cdecl -- C gibberish translator
**      src/util.c
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

/**
 * @file
 * Defines utility functions.
 */

// local
#include "config.h"                     /* must go first */
#include "common.h"
#define CDECL_UTIL_INLINE _GL_EXTERN_INLINE
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>                   /* for fstat() */
#include <sysexits.h>

#ifdef HAVE_READLINE_READLINE_H
# include <readline/readline.h>
#endif /* HAVE_READLINE_READLINE_H */
#ifdef HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY_H */

#ifdef WITH_TERM_COLUMNS
# include <fcntl.h>                     /* for open(2) */
# if defined(HAVE_CURSES_H)
#   define _BOOL                        /* prevents clash of bool on Solaris */
#   include <curses.h>
#   undef _BOOL
# elif defined(HAVE_NCURSES_H)
#   include <ncurses.h>
# endif
# include <term.h>                      /* for setupterm(3) */
# include <unistd.h>                    /* for close(2) */
#endif /* WITH_TERM_COLUMNS */

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
  assert( path_name != NULL );
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
FILE* fmemopen( void *buf, size_t size, char const *mode ) {
  assert( buf != NULL );
  assert( strchr( mode, 'r' ) != NULL );

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
  assert( p != NULL );
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

#ifdef WITH_TERM_COLUMNS
unsigned get_term_columns( void ) {
  unsigned    cols = 0;
  int         cterm_fd = -1;
  char        reason_buf[ 128 ];
  char const *reason = NULL;

  char const *const cterm_path = ctermid( NULL );
  if ( !cterm_path || !*cterm_path ) {
    reason = "ctermid(3) failed to get controlling terminal";
    goto error;
  }

  if ( (cterm_fd = open( cterm_path, O_RDWR )) == -1 ) {
    reason = STRERROR;
    goto error;
  }

  char const *const term = getenv( "TERM" );
  if ( !term ) {
    reason = "TERM environment variable not set";
    goto error;
  }

  int setupterm_err;
  if ( setupterm( (char*)term, cterm_fd, &setupterm_err ) == ERR ) {
    reason = reason_buf;
    switch ( setupterm_err ) {
      case -1:
        reason = "terminfo database not found";
        break;
      case 0:
        snprintf(
          reason_buf, sizeof reason_buf,
          "TERM=%s not found in database or too generic", term
        );
        break;
      case 1:
        reason = "terminal is harcopy";
        break;
      default:
        snprintf(
          reason_buf, sizeof reason_buf,
          "setupterm(3) returned error code %d", setupterm_err
        );
    } // switch
    goto error;
  }

  int const ti_cols = tigetnum( (char*)"cols" );
  if ( ti_cols < 0 ) {
    snprintf(
      reason_buf, sizeof reason_buf,
      "tigetnum(\"cols\") returned error code %d", ti_cols
    );
    goto error;
  }

  cols = (unsigned)ti_cols;

error:
  if ( cterm_fd != -1 )
    close( cterm_fd );
  if ( reason )
    PMESSAGE_EXIT( EX_UNAVAILABLE,
      "failed to determine number of columns in terminal: %s\n",
      reason
    );

  return cols;
}
#endif /* WITH_TERM_COLUMNS */

bool is_file( int fd ) {
  struct stat fd_stat;
  FSTAT( fd, &fd_stat );
  return S_ISREG( fd_stat.st_mode );
}

char* readline_wrapper( char const *ps1, char const *ps2 ) {
  static char *buf;
  size_t buf_len = 0;

  free( buf );
  buf = NULL;

  for (;;) {
    static char *line;
#ifdef HAVE_READLINE
    extern void readline_init( void );
    static bool called_readline_init;

    if ( !called_readline_init ) {
      readline_init();
      called_readline_init = true;
    }

    free( line );
    if ( !(line = readline( buf ? ps2 : ps1 )) )
      return NULL;
#else
    static size_t line_cap;
    PUTS_OUT( buf ? ps2 : ps1 );
    FFLUSH( stdout );
    if ( getline( &line, &line_cap, stdin ) == -1 ) {
      FERROR( stdin );
      return NULL;
    }
#endif /* HAVE_READLINE */

    if ( is_blank_line( line ) ) {
      if ( buf ) {
        //
        // If we've been accumulating continuation lines, a blank line ends it.
        //
        break;
      }
      continue;
    }

    size_t line_len = strlen( line );
    bool const is_continuation = line_len >= 1 && line[ line_len - 1 ] == '\\';

    if ( is_continuation )
      line[ --line_len ] = '\0';        // get rid of '\'

    if ( !buf ) {
      buf = check_strdup( line );
      buf_len = line_len;
    } else {
      size_t const new_len = buf_len + line_len;
      REALLOC( buf, char, new_len + 1/*null*/ );
      strcpy( buf + buf_len, line );
      buf_len = new_len;
    }

    if ( !is_continuation )
      break;
  } // for

  assert( buf != NULL );
  assert( *buf != '\0' );
#ifdef HAVE_READLINE
  add_history( buf );
#endif /* HAVE_READLINE */
  return buf;
}

link_t* link_pop( link_t **phead ) {
  assert( phead != NULL );
  if ( *phead ) {
    link_t *const popped = (*phead);
    (*phead) = popped->next;
    popped->next = NULL;
    return popped;
  }
  return NULL;
}

void link_push( link_t **phead, link_t *node ) {
  assert( phead != NULL );
  assert( node != NULL );
  assert( node->next == NULL );
  node->next = (*phead);
  (*phead) = node;
}

size_t strcpy_len( char *dst, char const *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  char const *const dst0 = dst;
  while ( (*dst++ = *src++) )
    /* empty */;
  return dst - dst0 - 1;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
