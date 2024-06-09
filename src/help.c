/*
**      cdecl -- C gibberish translator
**      src/help.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas, et al.
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
 * Defines functions for printing help text.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "help.h"
#include "c_lang.h"
#include "cdecl.h"
#include "cdecl_command.h"
#include "color.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/// @endcond

// local functions
static void print_help_name_number( void );
static void print_help_where( void );

/**
 * @addtogroup printing-help-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// local constants
static char const *const HELP_OPTIONS[] = {
  L_commands,
  L_english,
  L_options,
  NULL
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether **cdecl** \a command is \a literal.
 *
 * @param command The **cdecl** command to check.
 * @param literal The literal to compare against.
 * @return Returns `true` only if \a command is either NULL or equal to \a
 * literal.
 *
 * @sa command_is_any()
 */
NODISCARD
static inline bool command_is( cdecl_command_t const *command,
                               char const *literal ) {
  return command == NULL || strcmp( command->literal, literal ) == 0;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether **cdecl** \a command is any of the given literals.
 *
 * @param command The **cdecl** command to check.
 * @param ... A NULL-terminated list of string literal arguments to compare
 * against.
 * @return Returns `true` only if \a command is either NULL or equal to one of
 * \a ... .
 *
 * @sa command_is()
 */
NODISCARD
ATTRIBUTE_SENTINEL()
static bool command_is_any( cdecl_command_t const *command, ... ) {
  if ( command == NULL )
    return true;

  va_list args;
  va_start( args, command );

  bool found = false;
  do {
    char const *const arg = va_arg( args, char const* );
    if ( arg == NULL )
      break;
    found = command->literal == arg;
  } while ( !found );

  va_end( args );
  return found;
}

/**
 * Checks whether the string \a s is a title.
 *
 * @param s The string to check.
 * @return Returns `true` only if \a s is a title string.
 */
NODISCARD
static bool is_title( char const *s ) {
  assert( s != NULL );
  if ( isalpha( *s ) ) {
    while ( *++s != '\0' ) {
      switch ( *s ) {
        case ':':
          return true;
        case '+':
        case '-':
        case '\\':
          continue;
      } // switch
      if ( !isalpha( *s ) )
        break;
    } // while
  }
  return false;
}

/**
 * Possibly maps \a what to another string.
 *
 * @param what What to possibly map to another string.  May be NULL.
 * @return Returns that mapped-to string or \a what if there is no mapping.
 */
NODISCARD
static char const* map_what( char const *what ) {
  if ( what == NULL )
    return L_commands;

  struct str_map {
    char const *from;
    char const *to;
  };
  typedef struct str_map str_map_t;

  static str_map_t const STR_MAP[] = {
    { L_command,          L_commands },

    //
    // Special cases: the cdecl commands are only "const", "dynamic",
    // "reinterpret", and "static" without the "cast", but the user might type
    // "cast" additionally: remove the "cast".
    //
    // Note that the lexer will collapse multiple whitespace characters between
    // words down to a single space.
    //
    { "const cast",       L_const },
    { "dynamic cast",     L_dynamic },
    { "reinterpret cast", L_reinterpret },
    { "static cast",      L_static },

    //
    // Special case: map plain "include" (the original cdecl "include" command)
    // to "#include".
    //
    { L_include,          L_PRE_P_include },

    //
    // Special case: there is no "q" command, only "quit". The lexer maps "q"
    // to "quit" internally, but only when "q" is the only thing on a line (so
    // "q" can be used as a variable name), so we have to map "q" to "quit"
    // here too.
    //
    { "q",                L_quit },
  };

  FOREACH_ARRAY_ELEMENT( str_map_t, map, STR_MAP ) {
    if ( strcmp( what, map->from ) == 0 )
      return map->to;
  } // for

  return what;
}

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_h( char const *line ) {
  assert( line != NULL );

  bool is_escaped = false;              // was preceding char a '\'?
  bool in_title = is_title( line );     // is current char within a title?
  char const *recent_color = NULL;      // most recent color set

  if ( in_title )
    color_start( stdout, recent_color = sgr_help_title );

  for ( char const *s = line; *s != '\0'; ++s ) {
    if ( !is_escaped ) {
      switch ( *s ) {
        case '\\':                      // escapes next char
          is_escaped = true;
          continue;
        case ':':                       // ends a title
          if ( true_clear( &in_title ) )
            color_end( stdout, recent_color );
          break;
        case '<':                       // begins non-terminal
          color_start( stdout, recent_color = sgr_help_nonterm );
          break;
        case '*':                       // other EBNF chars
        case '+':
        case '[':
        case ']':
        case '{':
        case '|':
        case '}':
          color_start( stdout, recent_color = sgr_help_punct );
          FALLTHROUGH;
        case '>':                       // ends non-terminal
          PUTC( *s );
          color_end( stdout, recent_color );
          continue;
      } // switch
    }

    PUTC( *s );
    is_escaped = false;
  } // for
}

/**
 * Prints the help for a command or all commands.
 *
 * @param command
 */
static void print_help_command( cdecl_command_t const *command ) {
  print_h( "command:\n" );

  if ( command_is_any( command, L_cast, L_const, L_dynamic, L_reinterpret,
                       L_static, NULL ) ) {
    print_h( "  " );
    if ( command == NULL ) {
      if ( OPT_LANG_IS( NEW_STYLE_CASTS ) )
        print_h( "[const | dynamic | reinterpret | static] " );
    }
    else if ( command->literal != L_cast ) {
      PRINTF( "%s ", command->literal );
    }
    print_h( "cast [<name>] {as|[in]to} <english>\n" );
  }

  if ( command_is( command, L_declare ) ) {
    print_h( "  declare <name> [, <name>]* as <english> " );
    if ( OPT_LANG_IS( ALIGNMENT ) )
      print_h( "[<declare-option>]\n" );
    else
      print_h( "[width <number> [bits]]\n" );
    if ( OPT_LANG_IS( operator ) )
      print_h( "  declare <operator> as <english>\n" );
    if ( OPT_LANG_IS( LAMBDAS ) )
      print_h( "  declare [<english>] lambda <lambda-english>\n" );
    if ( OPT_LANG_IS( USER_DEF_CONVS ) )
      print_h( "  declare [<english>] user-def[ined] <user-defined-english>\n" );
  }

  if ( command_is( command, L_define ) )
    print_h( "  define <name> as <english>\n" );

  if ( command_is( command, L_PRE_P_define ) )
    print_h( "  #define <name>[([<pp-param> [, <pp-param>]*])] <pp-token>*\n" );

  if ( command_is( command, L_expand ) )
    print_h( "  expand <name>[([<pp-token>* [, <pp-token>*]*])] <pp-token>*\n" );

  if ( command_is( command, L_explain ) )
    print_h( "  explain <gibberish> [, <gibberish>]*\n" );

  if ( command_is( command, L_help ) )
    print_h( "  { help | ? } [command[s] | <command> | english | options]\n" );

  if ( command_is( command, L_PRE_P_include ) )
    print_h( "  [#]include \"<path>\"\n" );

  if ( command_is( command, L_set ) )
    print_h( "  set [<option> [= <value>] | options | <lang>]*\n" );

  if ( command_is( command, L_show ) ) {
    print_h( "  show [<name>|[all] [predefined|user] [<glob>]] [[as] {english|typedef" );
    if ( OPT_LANG_IS( using_DECLS ) )
      print_h( "|using" );
    print_h( "}]\n" );
    print_h( "  show {<name>|[predefined|user] macros}\n" );
  }

  if ( command_is( command, L_typedef ) )
    print_h( "  type[def] <gibberish> [, <gibberish>]*\n" );

  if ( OPT_LANG_IS( SCOPED_NAMES ) &&
       command_is_any( command, L_class, L_inline, L_namespace, L_struct,
                       L_union, NULL ) ) {
    print_h( "  " );
    if ( command == NULL ) {
      print_h( "<scope-c>" );
    } else {
      PUTS( command->literal );
      if ( command->literal == L_inline )
        PRINTF( " %s", L_namespace );
    }
    print_h( " <name>" );
    if ( OPT_LANG_IS( NESTED_TYPES ) ) {
      print_h( " [\\{ [{ <scope-c> | <typedef>" );
      if ( OPT_LANG_IS( using_DECLS ) )
        print_h( " | <using>" );
      print_h( " } ;]* \\}]" );
    }
    print_h( "\n" );
  }

  if ( command_is( command, L_PRE_P_undef ) )
    print_h( "  #undef <name>\n" );

  if ( OPT_LANG_IS( using_DECLS ) && command_is( command, L_using ) )
    print_h( "  using <name> = <gibberish>\n" );

  if ( command_is_any( command, L_exit, L_quit, NULL ) )
    print_h( "  exit | q[uit]\n" );

  if ( OPT_LANG_IS( ALIGNMENT ) && command_is( command, L_declare ) ) {
    print_h( "declare-option:\n" );
    print_h( "  align[ed] [as|to] {<number> [bytes] | <english>}\n" );
    print_h( "  width <number> [bits]\n" );
  }

  if ( command == NULL ) {
    print_h( "gibberish: a C" );
    if ( OPT_LANG_IS( CPP_ANY ) )
      print_h( "\\+\\+" );
    print_h( " declaration, like \"int x\"; or a cast, like \"(int)x\"\n" );
    print_h( "glob: " );
    if ( OPT_LANG_IS( C_ANY ) )
      print_h( "a <name> containing zero or more literal *\n" );
    else
      print_h( "a [[*]::]<name>[::<name>]* containing zero or more literal *\n" );
    print_help_name_number();
  }

  if ( command_is( command, L_PRE_P_define ) ) {
    print_h( "pp-param: a macro parameter <name>" );
    if ( OPT_LANG_IS( VARIADIC_MACROS ) )
      print_h( " or ..." );
    print_h( "\n" );
  }

  if ( command_is_any( command, L_PRE_P_define, L_expand, NULL ) )
    print_h( "pp-token: a preprocessor token\n" );

  if ( command == NULL && OPT_LANG_IS( SCOPED_NAMES ) ) {
    print_h( "scope-c: class | struct | union |" );
    if ( OPT_LANG_IS( inline_namespace ) )
      print_h( " [inline]" );
    print_h( " namespace\n" );
  }

  if ( command == NULL )
    print_help_where();
}

/**
 * Prints help for pseudo-English.
 */
static void print_help_english( void ) {
  print_h( "english:\n" );

  if ( OPT_LANG_IS( C_ANY ) ) {
    print_h( "  <store>*" );
    if ( OPT_LANG_IS( QUALIFIED_ARRAYS ) )
      print_h( " <ar-qual>*" );
    if ( OPT_LANG_IS( VLAS ) ) {
      print_h( " array [<number>|<name>|\\*] of <english>\n" );
      print_h( "  <store>* <ar-qual>* variable [length] array of <english>\n" );
    } else {
      print_h( " array [<number>|<name>] of <english>\n" );
    }
    print_h( "  <store>* function [([<args>])] [returning <english>]\n" );
    print_h( "  <store>*" );
    if ( OPT_LANG_IS( const ) )
      print_h( " <cv-qual>*" );
    print_h( " pointer to <english>\n" );
  }
  else /* C++ */ {
    print_h( "  <store>* <cv-qual>* array [<number>] of <english>\n" );
    print_h( "  <cv-qual>* concept <name> [parameter pack]\n" );
    print_h( "  <store>* constructor [([<args>])]\n" );
    print_h( "  [virtual] destructor [()]\n" );
    print_h( "  <store>* <fn-qual>* [[non-]member] function [([<args>])] [returning <english>]\n" );
    print_h( "  <store>* <fn-qual>* [[non-]member] operator [([<args>])] [returning <english>]\n" );
    print_h( "  <store>* <cv-qual>* pointer to [member of { class | struct } <name>] <english>\n" );
  }

  print_h( "  {" );
  if ( OPT_LANG_IS( enum ) ) {
    print_h( " enum" );
    if ( OPT_LANG_IS( enum_class ) )
      print_h( " [class|struct] [of [type] <english>]" );
    print_h( " |" );
    if ( OPT_LANG_IS( class ) )
      print_h( " class |" );
  }
  print_h( " struct | union } <name>\n" );

  if ( OPT_LANG_IS( C_ANY ) ) {
    print_h( "  block [([<args>])] [returning <english>]\n" );
    print_h( "  <store>* <modifier>* [<C-type>]\n" );

    if ( OPT_LANG_IS( QUALIFIED_ARRAYS ) )
      print_h( "ar-qual: non-empty | const | restrict | volatile\n" );

    print_h( "args: a comma separated list of " );
    if ( OPT_LANG_IS( C_KNR ) )
      print_h( "<name>\n" );
    else if ( OPT_LANG_IS( KNR_FUNC_DEFS ) )
      print_h( "<name>, <english>, or <name> as <english>\n" );
    else
      print_h( "[<name> as] <english>\n" );

    print_h( "C-type: " );
    if ( OPT_LANG_IS( auto_TYPE ) )
      print_h( "auto | " );
    if ( OPT_LANG_IS( _BitInt ) )
      print_h( "_BitInt(<number>) | " );
    if ( OPT_LANG_IS( bool ) )
      print_h( "bool | " );
    else if ( OPT_LANG_IS( _Bool ) )
      print_h( "_Bool | " );
    print_h( "char" );
    if ( OPT_LANG_IS( char16_32_t ) ) {
      print_h( "[{" );
      if ( OPT_LANG_IS( char8_t ) )
        print_h( "8|" );
      print_h( "16|32}_t]" );
    }
    if ( OPT_LANG_IS( wchar_t ) )
      print_h( " | wchar_t" );
    print_h( " | int |" );
    if ( OPT_LANG_IS( auto_TYPE ) || OPT_LANG_IS( _BitInt ) )
      print_h( "\n       " );
    print_h( " float | double" );
    if ( OPT_LANG_IS( void ) )
      print_h( " | void" );
    print_h( "\n" );

    if ( OPT_LANG_IS( const ) ) {
      print_h( "cv-qual:" );
      if ( OPT_LANG_IS( _Atomic ) )
        print_h( " _Atomic |" );
      print_h( " const |" );
      if ( OPT_LANG_IS( restrict ) )
        print_h( " restrict |" );
      print_h( " volatile\n" );
    }

    print_h( "modifier:" );
    print_h( " short | long" );
    if ( OPT_LANG_IS( signed ) )
      print_h( " | signed" );
    print_h( " | unsigned" );
    if ( OPT_LANG_IS( const ) )
      print_h( " | <cv-qual>" );
    print_h( "\n" );

    print_help_name_number();

    print_h( "store: " );
    if ( OPT_LANG_IS( auto_STORAGE ) )
      print_h( "auto | " );
    if ( OPT_LANG_IS( constexpr ) )
      print_h( "constexpr | " );
    print_h( "extern | register | static" );
    if ( OPT_LANG_IS( thread_local ) )
      print_h( " | thread_local" );
    else if ( OPT_LANG_IS( _Thread_local ) )
      print_h( " | _Thread_local" );
    print_h( " | typedef" );
    print_h( "\n" );
  }
  else /* C++ */ {
    print_h( "  <store>*" );
    if ( OPT_LANG_IS( RVALUE_REFERENCES ) )
      print_h( " [rvalue]" );
    print_h( " reference to <english>\n" );

    if ( OPT_LANG_IS( STRUCTURED_BINDINGS ) )
      print_h( "  structured binding\n" );

    print_h( "  <store>* <modifier>* [<C\\+\\+-type>]\n" );

    if ( OPT_LANG_IS( LAMBDAS ) ) {
      print_h( "lambda-english:\n" );
      print_h( "  [[capturing] \\[[<captures>]\\] [([<args>])] [returning <english>]\n" );
    }

    print_h( "user-defined-english:\n" );
    print_h( "  conversion [operator] [of <scope-e> <name>]* returning <english>\n" );

    if ( OPT_LANG_IS( USER_DEF_LITS ) )
      print_h( "  literal [([<args>])] [returning <english>]\n" );

    print_h( "args: a comma separated list of [<name> as] <english>\n" );

    if ( OPT_LANG_IS( LAMBDAS ) ) {
      print_h( "captures: [<capture-default>,] [[&]<name>][,[&]<name>]*\n" );
      print_h( "capture-default: {copy|reference} [by] default | = | &\n" );
    }

    print_h( "C\\+\\+-type: " );
    if ( OPT_LANG_IS( auto_TYPE ) )
      print_h( "auto | " );
    print_h( "bool | char" );
    if ( OPT_LANG_IS( char16_32_t ) ) {
      print_h( "[{" );
      if ( OPT_LANG_IS( char8_t ) )
        print_h( "8|" );
      print_h( "16|32}_t]" );
    }
    print_h( " | wchar_t | int | float | double |" );
    if ( OPT_LANG_IS( char8_t ) && OPT_LANG_IS( auto_TYPE ) )
      print_h( "\n         " );
    if ( OPT_LANG_IS( PARAMETER_PACKS ) )
      print_h( " parameter pack |" );
    print_h( " void\n" );

    print_h( "cv-qual: const | volatile\n" );

    print_h( "fn-qual: <cv-qual>" );
    if ( OPT_LANG_IS( RVALUE_REFERENCES ) )
      print_h( " | [rvalue] reference" );
    print_h( "\n" );

    print_h( "modifier: short | long | signed | unsigned | <cv-qual>\n" );
    print_help_name_number();

    print_h( "scope-e: scope | class | struct | union |" );
    if ( OPT_LANG_IS( inline_namespace ) )
      print_h( " [inline]" );
    print_h( " namespace\n" );

    print_h( "store:" );
    if ( OPT_LANG_IS( auto_STORAGE ) )
      print_h( " auto |" );
    print_h( " const" );
    if ( OPT_LANG_IS( constexpr ) )
      print_h( "[" );
    if ( OPT_LANG_IS( consteval ) )
      print_h( "eval|" );
    if ( OPT_LANG_IS( constexpr ) )
      print_h( "expr" );
    if ( OPT_LANG_IS( constinit ) )
      print_h( "|init" );
    if ( OPT_LANG_IS( constexpr ) )
      print_h( "]" );
    print_h( " | explicit | extern [\"C\" [linkage]] | friend |\n" );
    print_h( "       mutable | static" );
    if ( OPT_LANG_IS( EXPLICIT_OBJ_PARAM_DECLS ) )
      print_h( " | this" );
    if ( OPT_LANG_IS( thread_local ) )
      print_h( " | thread_local" );
    print_h( " | typedef | [pure] virtual\n" );
  }

  print_help_where();
}

/**
 * Prints help for a **cdecl** _name_ and _number_.
 */
static void print_help_name_number( void ) {
  if ( OPT_LANG_IS( C_ANY ) )
    print_h( "name: a C identifier\n" );
  else
    print_h( "name: a C\\+\\+ identifier: <name>[::<name>]* | <name> [of <scope-e> <name>]*\n" );
  print_h( "number: a binary, octal, decimal, or hexadecimal integer\n" );
}

/**
 * Prints help for `set` options.
 */
static void print_help_set_options( void ) {
  print_h( "option:\n" );
  print_h( "  [no]alt-tokens\n" );
#ifdef ENABLE_BISON_DEBUG
  print_h( "  [no]bison-debug\n" );
#endif /* ENABLE_BISON_DEBUG */
  print_h( "  [no]debug[={u|\\*|-}]\n" );
  print_h( "  [no]east-const\n" );
  print_h( "  [no]echo-commands\n" );
  print_h( "  [no]english-types\n" );
  print_h( "  [no]explicit-ecsu[={{e|c|s|u}+|\\*|-}]\n" );
  print_h( "  [no]explicit-int[={<types>|\\*|-}]\n" );
#ifdef ENABLE_FLEX_DEBUG
  print_h( "  [no]flex-debug\n" );
#endif /* ENABLE_FLEX_DEBUG */
  print_h( "  {di|tri|no}graphs\n" );
  print_h( "  [no]infer-command\n" );
  print_h( "  lang=<lang>\n" );
  print_h( "  <lang>\n" );
  print_h( "  [no]prompt\n" );
  print_h( "  [no]semicolon\n" );
  print_h( "  [no]trailing-return\n" );
  print_h( "  [no]using\n" );
  print_h( "  [no]west-decl[={{b|f|l|o|r|s|t}+|\\*|-}]\n" );
  print_h( "lang:\n" );
  print_h( "  K[&|N]R[C] | C[K[&|N]R|78|89|95|99|11|17|23] | C\\+\\+[98|03|11|14|17|20|23]\n" );
  print_h( "types:\n" );
  print_h( "  i|u|[u]{i|s|l[l]}[,[u]{i|s|l[l]}]*\n" );

  print_help_where();
}

static void print_help_where( void ) {
  print_h( "where: [] = 0 or 1; * = 0 or more; + = 1 or more; {} = one of; | = alternate\n" );
}

////////// extern functions ///////////////////////////////////////////////////

char const* const* help_option_next( char const *const *option ) {
  return option == NULL ? HELP_OPTIONS : *++option == NULL ? NULL : option;
}

bool print_help( char const *what, c_loc_t const *what_loc ) {
  assert( what_loc != NULL );

  char const *const mapped_what = map_what( what );

  if ( strcmp( mapped_what, L_commands ) == 0 ) {
    print_help_command( /*command=*/NULL );
    return true;
  }

  if ( strcmp( mapped_what, L_english ) == 0 ) {
    print_help_english();
    return true;
  }

  if ( strcmp( mapped_what, L_options ) == 0 ) {
    print_help_set_options();
    return true;
  }

  //
  // Note that cdecl_command_find() matches strings that _start with_ a
  // command, so we have to check for an exact match if found.
  //
  cdecl_command_t const *const command = cdecl_command_find( mapped_what );
  if ( command == NULL || strcmp( mapped_what, command->literal ) != 0 ) {
    print_error( what_loc, "\"%s\": no such command or option", what );
    print_suggestions( DYM_COMMANDS | DYM_HELP_OPTIONS, what );
    EPUTC( '\n' );
    return false;
  }

  if ( !opt_lang_is_any( command->lang_ids ) ) {
    print_error( what_loc,
      "\"%s\": not supported%s\n",
      what, c_lang_which( command->lang_ids )
    );
    return false;
  }

  print_help_command( command );
  return true;
}

void print_use_help( void ) {
  EPUTS( "; use --help or -h for help\n" );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
