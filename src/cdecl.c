/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
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

/**
 * @file
 * Defines main() as well as functions for initialization, clean-up, and
 * parsing user input.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl.h"
#include "c_ast.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "color.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "prompt.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <ctype.h>
#include <errno.h>
#include <limits.h>                     /* for PATH_MAX */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>                     /* for isatty(3) */

#ifdef HAVE_READLINE_READLINE_H
# include <readline/readline.h>
#endif /* HAVE_READLINE_READLINE_H */
#ifdef HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY_H */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

cdecl_command_t const CDECL_COMMANDS[] = {
  { L_CAST,                   CDECL_COMMAND_PROG_NAME,  LANG_ANY          },
  { L_CLASS,                  CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_CONST /* cast */,       CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_DECLARE,                CDECL_COMMAND_PROG_NAME,  LANG_ANY          },
  { L_DEFINE,                 CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_DYNAMIC /* cast */,     CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_ENUM,                   CDECL_COMMAND_FIRST_ARG,  LANG_MIN(C_89)    },
  { L_EXIT,                   CDECL_COMMAND_LANG_ONLY,  LANG_ANY          },
  { L_EXPLAIN,                CDECL_COMMAND_PROG_NAME,  LANG_ANY          },
  { L_HELP,                   CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_INLINE /* namespace */, CDECL_COMMAND_FIRST_ARG,  LANG_CPP_MIN(11)  },
  { L_NAMESPACE,              CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_QUIT,                   CDECL_COMMAND_LANG_ONLY,  LANG_ANY          },
  { L_REINTERPRET /* cast */, CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_SET_COMMAND,            CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_SHOW,                   CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_STATIC /* cast */,      CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_STRUCT,                 CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_TYPEDEF,                CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_UNION,                  CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_USING,                  CDECL_COMMAND_FIRST_ARG,  LANG_CPP_MIN(11)  },
  { NULL,                     CDECL_COMMAND_ANYWHERE,   LANG_NONE         },
};

// extern variable definitions
bool          cdecl_initialized;
cdecl_mode_t  cdecl_mode;
FILE         *fin;
FILE         *fout;
bool          is_input_a_tty;
char const   *me;

// extern functions
extern void parser_cleanup( void );

/**
 * Bison: parse input.
 *
 * @return Returns 0 only if parsing was successful.
 */
PJL_WARN_UNUSED_RESULT
extern int  yyparse( void );

/**
 * Flex: immediately switch to reading \a file.
 *
 * @param in_file The `FILE` to read from.
 */
extern void yyrestart( FILE *in_file );

// local functions
static void cdecl_cleanup( void );

PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_argv( int, char const *const[] );

PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_command_line( char const*, int, char const *const[] );

PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_files( int, char const *const[] );

PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_stdin( void );

static void read_conf_file( void );

PJL_WARN_UNUSED_RESULT
static bool starts_with_token( char const*, char const*, size_t );

////////// main ///////////////////////////////////////////////////////////////

/**
 * The main entry point.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns 0 on success, non-zero on failure.
 */
int main( int argc, char const *argv[] ) {
  atexit( cdecl_cleanup );
  options_init( &argc, &argv );
  c_typedef_init();
  lexer_reset( true );                  // resets line number

  if ( !opt_no_conf )
    read_conf_file();
  opt_conf_file = NULL;                 // don't print in errors any more
  cdecl_initialized = true;

  bool const ok = parse_cdecl_argv( argc, argv );
  exit( ok ? EX_OK : EX_DATAERR );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Reads an input line interactively.
 *
 *  + Returns only non-whitespace-only lines.
 *  + Stitches multiple lines ending with `\` together.
 *
 * If GNU **readline**(3) is compiled in, also:
 *
 *  + Adds non-whitespace-only lines to the history.
 *
 * @param sbuf The strbuf to use.
 * @param ps1 The primary prompt to use.
 * @param ps2 The secondary prompt to use for a continuation line (a line after
 * ones ending with `\`).
 * @return Returns `false` only if encountered EOF.
 */
PJL_WARN_UNUSED_RESULT
static bool cdecl_read_line( strbuf_t *sbuf, char const *ps1,
                             char const *ps2 ) {
  assert( sbuf != NULL );
  assert( ps1 != NULL );
  assert( ps2 != NULL );

  bool is_cont_line = false;

  strbuf_init( sbuf );

  for (;;) {
    static char *line;
    bool got_line;

#ifdef WITH_READLINE
    extern void readline_init( FILE*, FILE* );
    static bool called_readline_init;
    if ( false_set( &called_readline_init ) )
      readline_init( fin, fout );
    free( line );
    got_line = (line = readline( is_cont_line ? ps2 : ps1 )) != NULL;
#else
    static size_t line_cap;
    FPUTS( is_cont_line ? ps2 : ps1, fout );
    FFLUSH( fout );
    got_line = getline( &line, &line_cap, fin ) != -1;
#endif /* WITH_READLINE */

    if ( !got_line ) {
      FERROR( fout );
      return false;
    }

    if ( is_blank_line( line ) ) {
      if ( is_cont_line ) {
        //
        // If we've been accumulating continuation lines, a blank line ends it.
        //
        break;
      }
      continue;                         // otherwise, ignore blank lines
    }

    size_t const line_len = strlen( line );
    is_cont_line = ends_with_chr( line, line_len, '\\' );
    strbuf_catsn( sbuf, line, line_len - is_cont_line /* don't copy '\' */ );

    if ( !is_cont_line )
      break;
  } // for

  assert( sbuf->str != NULL );
  assert( sbuf->str[0] != '\0' );
#ifdef WITH_READLINE
  add_history( sbuf->str );
#endif /* WITH_READLINE */
  return true;
}

/**
 * Checks whether \a s is a cdecl command.
 *
 * @param s The null-terminated string to check.
 * @param command_kind The kind of commands to check against.
 * @return Returns `true` only if \a s is a command.
 */
PJL_WARN_UNUSED_RESULT
static bool is_command( char const *s, cdecl_command_kind_t command_kind ) {
  assert( s != NULL );
  SKIP_WS( s );

  FOREACH_COMMAND( c ) {
    if ( c->kind >= command_kind ) {
      size_t const literal_len = strlen( c->literal );
      if ( !starts_with_token( s, c->literal, literal_len ) )
        continue;
      if ( c->literal == L_CONST || c->literal == L_STATIC ) {
        //
        // When in explain-by-default mode, a special case has to be made for
        // const and static since explain is implied only when NOT followed by
        // "cast":
        //
        //      const int *p                      // Implies explain.
        //      const cast p into pointer to int  // Does NOT imply explain.
        //
        char const *p = s + literal_len;
        if ( !isspace( *p ) )
          break;
        SKIP_WS( p );
        if ( !starts_with_token( p, L_CAST, 4 ) )
          break;
        p += 4;
        if ( !isspace( *p ) )
          break;
      }
      return true;
    }
  } // for
  return false;
}

/**
 * Cleans up cdecl data.
 */
static void cdecl_cleanup( void ) {
  free_now();
  c_typedef_cleanup();
  parser_cleanup();                     // must go before c_ast_cleanup()
  c_ast_cleanup();
}

/**
 * Parses \a argv to figure out what kind of arguments were given.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_argv( int argc, char const *const argv[] ) {
  if ( argc == 0 )                      // cdecl
    return parse_cdecl_stdin();
  if ( is_command( me, CDECL_COMMAND_PROG_NAME ) )
    return parse_cdecl_command_line( me, argc, argv );

  //
  // Note that options_init() adjusts argv such that argv[0] becomes the first
  // argument (and no longer the program name).
  //
  if ( is_command( argv[0], CDECL_COMMAND_FIRST_ARG ) )
    return parse_cdecl_command_line( NULL, argc, argv );

  if ( opt_explain )
    return parse_cdecl_command_line( L_EXPLAIN, argc, argv );

  // assume arguments are file names
  return parse_cdecl_files( argc, argv );
}

/**
 * Parses a cdecl command from the command-line.
 *
 * @param command The value of main()'s `argv[0]` if it's a cdecl command; NULL
 * otherwise and `argv[1]` is a cdecl command.
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_command_line( char const *command, int argc,
                                      char const *const argv[] ) {
  strbuf_t sbuf;
  bool space;

  strbuf_init( &sbuf );
  if ( (space = command != NULL) )
    strbuf_cats( &sbuf, command );
  for ( int i = 0; i < argc; ++i )
    strbuf_sepc_cats( &sbuf, ' ', &space, argv[i] );

  bool const ok = parse_cdecl_string( sbuf.str, sbuf.len );
  strbuf_cleanup( &sbuf );
  return ok;
}

/**
 * Parses cdecl commands from a file.
 *
 * @param file The FILE to read from.
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_file( FILE *file ) {
  assert( file != NULL );
  bool ok = true;

  // We don't just call yyrestart( file ) and yyparse() directly because
  // parse_cdecl_string() also inserts "explain " for opt_explain.

  for ( char buf[ 1024 ]; fgets( buf, sizeof buf, file ) != NULL; ) {
    if ( !parse_cdecl_string( buf, strlen( buf ) ) )
      ok = false;
  } // for
  FERROR( file );

  return ok;
}

/**
 * Parses cdecl commands from one or more files.
 *
 * @param num_files The length of \a files.
 * @param files An array of file names.
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_files( int num_files, char const *const files[] ) {
  bool ok = true;

  for ( int i = 0; i < num_files && ok; ++i ) {
    if ( strcmp( files[i], "-" ) == 0 ) {
      ok = parse_cdecl_stdin();
    }
    else {
      FILE *const file = fopen( files[i], "r" );
      if ( unlikely( file == NULL ) )
        PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", files[i], STRERROR() );
      if ( !parse_cdecl_file( file ) )
        ok = false;
      PJL_IGNORE_RV( fclose( file ) );
    }
  } // for

  return ok;
}

/**
 * Parses cdecl commands from standard input.
 *
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_cdecl_stdin( void ) {
  bool ok = true;
  is_input_a_tty = isatty( fileno( fin ) );

  if ( is_input_a_tty || opt_interactive ) {
    if ( opt_prompt )
      FPRINTF( fout, "Type \"%s\" or \"?\" for help\n", L_HELP );
    ok = true;
    for (;;) {
      static strbuf_t sbuf;
      strbuf_reset( &sbuf );
      if ( !cdecl_read_line( &sbuf, cdecl_prompt[0], cdecl_prompt[1] ) )
        break;
      ok = parse_cdecl_string( sbuf.str, sbuf.len );
    } // for
  } else {
    ok = parse_cdecl_file( fin );
  }

  is_input_a_tty = false;
  return ok;
}

/**
 * Reads the configuration file, if any.
 */
static void read_conf_file( void ) {
  bool const is_explicit_conf_file = (opt_conf_file != NULL);

  if ( !is_explicit_conf_file ) {       // no explicit conf file: use default
    char const *const home = home_dir();
    if ( home == NULL )
      return;
    static char conf_path_buf[ PATH_MAX ];
    strcpy( conf_path_buf, home );
    path_append( conf_path_buf, CONF_FILE_NAME_DEFAULT );
    opt_conf_file = conf_path_buf;
  }

  FILE *const conf_file = fopen( opt_conf_file, "r" );
  if ( conf_file == NULL ) {
    if ( is_explicit_conf_file )
      PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", opt_conf_file, STRERROR() );
    return;
  }

  //
  // Before reading the configuration file, temporarily set the language to the
  // maximum supported C++ so "using" declarations, if any, won't cause the
  // parser to error out.
  //
  c_lang_id_t const orig_lang = opt_lang;
  opt_lang = LANG_CPP_NEW;
  PJL_IGNORE_RV( parse_cdecl_file( conf_file ) );
  opt_lang = orig_lang;

  PJL_IGNORE_RV( fclose( conf_file ) );
}

/**
 * Checks whether \a s starts with a token.  If so, the character following the
 * token in \a s also _must not_ be an identifier character, i.e., whitespace,
 * punctuation, or the null byte.
 *
 * @param s The null-terminated string to check.
 * @param token The token to check against.
 * @param token_len The length of \a token.
 * @return Returns `true` only if \a s starts with \a token.
 */
PJL_WARN_UNUSED_RESULT
static bool starts_with_token( char const *s, char const *token,
                               size_t token_len ) {
  return  strncmp( s, token, token_len ) == 0 &&
          !is_ident( token[ token_len ] );
}

////////// extern functions ///////////////////////////////////////////////////

bool parse_cdecl_string( char const *s, size_t s_len ) {
  assert( s != NULL );

  // The code in print.c relies on command_line being set, so set it.
  print_params.command_line = s;
  print_params.command_line_len = s_len;

  strbuf_t explain_buf;
  bool const insert_explain =
    opt_explain && !is_command( s, CDECL_COMMAND_ANYWHERE );

  if ( insert_explain ) {
    //
    // The string doesn't start with a command: insert "explain " and set
    // inserted_len so the print_*() functions subtract it from the error
    // column to get the correct column within the original string.
    //
    static char const EXPLAIN_SP[] = "explain ";
    print_params.inserted_len = ARRAY_SIZE( EXPLAIN_SP ) - 1/*\0*/;
    strbuf_init( &explain_buf );
    strbuf_reserve( &explain_buf, print_params.inserted_len + s_len );
    strbuf_catsn( &explain_buf, EXPLAIN_SP, print_params.inserted_len );
    strbuf_catsn( &explain_buf, s, s_len );
    s = explain_buf.str;
    s_len = explain_buf.len;
  }

  FILE *const temp_file = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  IF_EXIT( temp_file == NULL, EX_IOERR );
  yyrestart( temp_file );
  bool const ok = yyparse() == 0;
  PJL_IGNORE_RV( fclose( temp_file ) );

  if ( insert_explain ) {
    strbuf_cleanup( &explain_buf );
    print_params.inserted_len = 0;
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
