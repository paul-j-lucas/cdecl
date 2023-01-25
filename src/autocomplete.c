/*
**      cdecl -- C gibberish translator
**      src/autocomplete.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas, et al.
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
 * Defines functions that implement command-line autocompletion.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_keyword.h"
#include "c_lang.h"
#include "cdecl_command.h"
#include "cdecl_keyword.h"
#include "literals.h"
#include "options.h"
#include "set_options.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>          /* must go last */

#if !HAVE_DECL_RL_COMPLETION_MATCHES
# define rl_completion_matches    completion_matches
#endif /* !HAVE_DECL_RL_COMPLETION_MATCHES */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Either a C/C++ or **cdecl** keyword that is autocompletable.
 */
struct ac_keyword {
  char const         *literal;          ///< C string literal of the keyword.
  c_lang_id_t         ac_lang_ids;      ///< Language(s) autocompletable in.
  bool                ac_in_gibberish;  ///< Autocomplete even for gibberish?
  ac_policy_t         ac_policy;        ///< See \ref cdecl_keyword::ac_policy.
  c_lang_lit_t const *lang_syn;         ///< See \ref cdecl_keyword::lang_syn.
};
typedef struct ac_keyword ac_keyword_t;

// local functions
static char*              command_generator( char const*, int );
static char*              keyword_generator( char const*, int );

NODISCARD
static char const* const* init_set_options( void );

NODISCARD
static bool               is_command( char const*, char const*, size_t );

NODISCARD
static char const*        str_prev_token( char const*, size_t, size_t* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a literal is a C/C++ keyword.
 *
 * @param literal The literal to check.
 * @return Returns `true` only of \a literal is a C/C++ keyword.
 *
 * @sa c_keyword_find()
 */
NODISCARD
static inline bool is_c_keyword( char const *literal ) {
  return c_keyword_find( literal, LANG_ANY, C_KW_CTX_DEFAULT ) != NULL;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Autocompletion wrapper around cdecl_command_next() that returns only
 * autocompletable **cdecl** commands.
 *
 * @param command A pointer to the previous command. For the first iteration,
 * NULL should be passed.
 * @return Returns the next autocompletable command or NULL for none.
 */
NODISCARD static
cdecl_command_t const* ac_cdecl_command_next( cdecl_command_t const *command ) {
  do {
    command = cdecl_command_next( command );
  } while ( command != NULL && command->ac_lang_ids == LANG_NONE );
  return command;
}

/**
 * Compares two \ref ac_keyword objects.
 *
 * @param i_keyword The first \ref ac_keyword to compare.
 * @param j_keyword The second \ref ac_keyword to compare.
 * Returns a number less than 0, 0, or greater than 0 if \a i_keyword is
 * less than, equal to, or greater than \a j_keyword, respectively.
 */
NODISCARD
static int ac_keyword_cmp( ac_keyword_t const *i_keyword,
                           ac_keyword_t const *j_keyword ) {
  return strcmp( i_keyword->literal, j_keyword->literal );
}

/**
 * Gets a specific list of keywords to autocomplete after \a command, if any.
 *
 * @param command The command to get the specific list of autocomplete keywords
 * for.
 * @return Returns a NULL-terminated array of keywords for \a command or NULL
 * for none.
 */
NODISCARD
static char const *const* command_ac_keywords( char const *command ) {
  if ( command == L_const || command == L_static ) {
    //
    // This needs to be here instead of in CDECL_KEYWORDS because `const` and
    // `static` as cdecl commands can only be followed by `cast` -- that isn't
    // true when `const` and `static` are used as C/C++ keywords.
    //
    static char const *const cast_keywords[] = {
      L_cast,
      NULL
    };
    return cast_keywords;
  }

  if ( command == L_help ) {
    //
    // This needs to be here instead of in CDECL_KEYWORDS because
    // str_prev_token() wouldn't match `?` as `help`.
    //
    static char const *const help_keywords[] = {
      L_commands,
      L_english,
      L_options,
      NULL
    };
    return help_keywords;
  }

  if ( command == L_set ) {
    //
    // This needs to be here instead of in CDECL_KEYWORDS because the list of
    // keywords is generated (not static).
    //
    static char const *const *set_options;
    if ( set_options == NULL )
      set_options = init_set_options();
    return set_options;
  }

  if ( command == L_show ) {
    //
    // This needs to be here instead of in CDECL_KEYWORDS because `using` is a
    // language-senstive C++ keyword.
    //
    static char const *const show_keywords[] = {
      L_all,
      L_english,
      L_predefined,
      L_typedef,
      L_user,
      NULL
    };
    static char const *const show_keywords_with_using[] = {
      L_all,
      L_english,
      L_predefined,
      L_typedef,
      L_user,
      L_using,
      NULL
    };
    return OPT_LANG_IS( using_DECLARATION ) ?
      show_keywords_with_using : show_keywords;
  }

  return NULL;
}

/**
 * Creates and initializes an array of all autocompletable keywords composed of
 * C/C++ keywords and **cdecl** keywords.
 *
 * @return Returns a pointer to said array.
 */
NODISCARD
static ac_keyword_t const* init_ac_keywords( void ) {
  size_t n = 0;

  // pre-flight to calculate array size
  FOREACH_C_KEYWORD( k )
    n += k->ac_lang_ids != LANG_NONE;
  FOREACH_CDECL_KEYWORD( k )
    n += k->ac_lang_ids != LANG_NONE && !is_c_keyword( k->literal );

  ac_keyword_t *const ac_keywords = free_later( MALLOC( ac_keyword_t, n + 1 ) );
  ac_keyword_t *p = ac_keywords;

  FOREACH_C_KEYWORD( k ) {
    if ( k->ac_lang_ids != LANG_NONE ) {
      *p++ = (ac_keyword_t){
        .literal = k->literal,
        .ac_lang_ids = k->ac_lang_ids,
        .ac_in_gibberish = true,
        .ac_policy = AC_POLICY_DEFAULT,
        .lang_syn = NULL
      };
    }
  } // for

  FOREACH_CDECL_KEYWORD( k ) {
    if ( k->ac_lang_ids != LANG_NONE && !is_c_keyword( k->literal ) ) {
      *p++ = (ac_keyword_t){
        .literal = k->literal,
        .ac_lang_ids = k->ac_lang_ids,
        .ac_in_gibberish = k->always_find,
        .ac_policy = k->ac_policy,
        .lang_syn = k->lang_syn
      };
    }
  } // for

  MEM_ZERO( p );

  //
  // Sort so C/C++ keywords come before their pseudo-English synonyms (e.g.,
  // `enum` before `enumeration`).  This matters when attempting to match
  // (almost) any keyword in keyword_generator().
  //
  qsort(
    ac_keywords, n, sizeof( ac_keyword_t ),
    POINTER_CAST( qsort_cmp_fn_t, &ac_keyword_cmp )
  );

  return ac_keywords;
}

/**
 * Creates and initializes an array of all `set` option strings to be used for
 * autocompletion for the `set` command.
 *
 * @return Returns a pointer to said array.
 */
NODISCARD
static char const* const* init_set_options( void ) {
  size_t n = 1;                         // for "options"

  // pre-flight to calculate array size
  FOREACH_SET_OPTION( opt )
    n += 1u + (opt->kind == SET_OPTION_TOGGLE /* "no" */);
  FOREACH_LANG( lang )
    n += !lang->is_alias;

  char **const set_options = free_later( MALLOC( char*, n + 1 ) );
  char **p = set_options;

  *p++ = CONST_CAST( char*, L_options );

  FOREACH_SET_OPTION( opt ) {
    switch ( opt->kind ) {
      case SET_OPTION_AFF_ONLY:
        *p++ = CONST_CAST( char*, opt->name );
        break;

      case SET_OPTION_TOGGLE:
        *p++ = CONST_CAST( char*, opt->name );
        FALLTHROUGH;

      case SET_OPTION_NEG_ONLY:
        *p = free_later(
          MALLOC( char, 2/*no*/ + strlen( opt->name ) + 1/*\0*/ )
        );
        strcpy( *p + 0, "no" );
        strcpy( *p + 2, opt->name );
        ++p;
        break;
    } // switch
  } // for
  FOREACH_LANG( lang ) {
    if ( !lang->is_alias )
      *p++ = free_later( check_strdup_tolower( lang->name ) );
  } // for

  *p = NULL;

  return (char const*const*)set_options;
}

/**
 * Checks whether \a s is a particular **cdecl** command.
 *
 * @param command The command to check for.
 * @param s The string to check.  Leading whitespace must have been skipped.
 * @param s_len The length of \a s.
 * @return Returns `true` only if it is.
 */
NODISCARD
static bool is_command( char const *command, char const *s, size_t s_len ) {
  assert( command != NULL );
  assert( s != NULL );
  size_t const command_len = strlen( command );
  if ( command_len > s_len || strncmp( s, command, command_len ) != 0 )
    return false;
  if ( command_len == s_len )
    return true;
  //
  // If s_len > command_len, then the first character past the end of the
  // command must not be an identifier character.  For example, if command is
  // "foo", then it must be "foo" exactly (above); or "foo" followed by a
  // whitespace or punctuation character, but not an identifier character:
  //
  //      "foo"   match
  //      "foo "  match
  //      "foo("  match
  //      "foob"  no match
  //
  return !is_ident( s[ command_len ] );
}

/**
 * Gets whether \a command is an English command, that is followed by pseudo-
 * English instead of gibberish.
 *
 * @param command The **cdecl** command to check.
 * @return Returns `true` only if \a command is an English command.
 */
NODISCARD
static bool is_english_command( char const *command ) {
  assert( command != NULL );
  return  command == L_cast     ||
          command == L_declare  ||
          command == L_define   ||
          command == L_help     ||
          command == L_set;
}

/**
 * Attempts to find the previous **cdecl** keyword in \a s relative to \a pos.
 *
 * @remarks This function exists to find the previous **cdecl** keyword for
 * autocompletion skipping over non-kewywords.  For example, given:
 * ```
 * cdecl> declare x as int width 4 <tab>
 * ```
 * hitting _tab_ finds the previous word `width` skipping over `4` since it's
 * not a keyword.  (The next autocompletion word for `width` can therefore
 * specify `bits` even though it's not adjacent.)
 *
 * @param s The string to find the previous keyword in.
 * @param pos The position within \a s to start looking before.
 * @return Returns the previous keyword or NULL if none.
 *
 * @sa str_prev_token()
 */
NODISCARD
static cdecl_keyword_t const* prev_cdecl_keyword( char const *s, size_t pos ) {
  for (;;) {
    size_t token_len;
    char const *const token = str_prev_token( s, pos, &token_len );
    if ( token == NULL )
      return NULL;
    static strbuf_t token_buf;
    strbuf_reset( &token_buf );
    strbuf_putsn( &token_buf, token, token_len );
    cdecl_keyword_t const *const k = cdecl_keyword_find( token_buf.str );
    if ( k != NULL )
      return k;
    pos = (size_t)(token - s);
  } // for
}

/**
 * Attempts to find the start of the previous token in \a s relative to \a pos.
 * For example, given the string and position:
 *
 *      Lorem ipsum
 *             ^
 *
 * will return a pointer to `L` and a length of 5.
 *
 * @param s The string to find the previous token in.
 * @param pos The position within \a s to start looking before.
 * @param token_len If a previous token has been found, receives its length.
 * @return Returns a pointer to the start of the previous token or NULL if
 * none.
 */
NODISCARD
static char const* str_prev_token( char const *s, size_t pos,
                                   size_t *token_len ) {
  assert( s != NULL );
  assert( token_len != NULL );

  if ( pos == 0 )
    return NULL;

  char const *p = s + pos - 1;

  // Back up over current token.
  while ( !isspace( *p ) ) {
    if ( --p == s )
      return NULL;
  } // while

  // Back up over whitespace between previous and current tokens.
  while ( isspace( *p ) ) {
    if ( --p == s )
      return NULL;
  } // while

  // Back up to the start of the previous token.
  for ( char const *const last = p--; ; --p ) {
    if ( p > s ) {
      if ( !isspace( *p ) )
        continue;
      ++p;
    }
    *token_len = STATIC_CAST( size_t, last - p + 1 );
    return p;
  } // for

  return NULL;
}

////////// readline callback functions ////////////////////////////////////////

/**
 * Attempts command completion for readline().
 *
 * @param text The text read so far, without leading whitespace, if any, to
 * match against.
 * @param start The starting character position of \a text.
 * @param end The ending character position of \a text.
 * @return Returns an array of C strings of possible matches.
 */
static char** cdecl_rl_completion( char const *text, int start, int end ) {
  assert( text != NULL );
  (void)end;

  rl_attempted_completion_over = 1;     // don't do filename completion

  //
  // Determine whether we should complete a cdecl command (the first word on
  // the line) vs. a non-command keyword: if start is zero or all characters in
  // the readline buffer before start are whitespace, then complete a command.
  // Having two generator functions makes the logic simpler in each.
  //
  size_t const n = STATIC_CAST( size_t, start );
  bool const is_command = strnspn( rl_line_buffer, " ", n ) == n;
  return rl_completion_matches(
    text, is_command ? command_generator : keyword_generator
  );
}

/**
 * Attempts to match a **cdecl** command.
 *
 * @param text The text read (so far) to match against.
 * @param state If 0, restart matching from the beginning; if non-zero,
 * continue to next match, if any.
 * @return Returns a copy of the command or NULL if none.
 */
static char* command_generator( char const *text, int state ) {
  assert( text != NULL );

  static cdecl_command_t const *match_command;
  static size_t text_len;

  if ( state == 0 ) {                   // new word? reset
    match_command = ac_cdecl_command_next( NULL );
    text_len = strlen( text );
  }

  while ( match_command != NULL ) {
    cdecl_command_t const *const c = match_command;
    match_command = ac_cdecl_command_next( match_command );
    if ( !opt_lang_is_any( c->lang_ids ) )
      continue;
    if ( strncmp( c->literal, text, text_len ) == 0 )
      return check_strdup( c->literal );
  } // while

  return NULL;
}

/**
 * Attempts to match a **cdecl** keyword (that is not a command).
 *
 * @param text The text read (so far) to match against.
 * @param state If 0, restart matching from the beginning; if non-zero,
 * continue to next match, if any.
 * @return Returns a copy of the keyword or NULL if none.
 */
static char* keyword_generator( char const *text, int state ) {
  assert( text != NULL );

  static char const *command;           // current command

  if ( state == 0 ) {                   // new word? reset
    size_t const rl_size = STATIC_CAST( size_t, rl_end );
    size_t const leading_spaces = strnspn( rl_line_buffer, " ", rl_size );
    size_t const buf_len = rl_size - leading_spaces;
    if ( buf_len == 0 )
      return NULL;
    char const *const buf = rl_line_buffer + leading_spaces;

    command = NULL;

    //
    // Retroactively figure out what the current command is so we can do some
    // command-sensitive autocompletion.  We can't just set the command in
    // command_generator() since it may never be called: the user could type an
    // entire command, then hit <tab> sometime later, e.g.:
    //
    //      cdecl> set <tab>
    //
    if ( is_command( "?", buf, buf_len ) )
      command = L_help;
    else {
      FOREACH_CDECL_COMMAND( c ) {
        if ( !opt_lang_is_any( c->lang_ids ) )
          continue;
        if ( is_command( c->literal, buf, buf_len ) ) {
          command = c->literal;
          break;
        }
      } // for
    }
    if ( command == NULL && opt_explain )
      command = L_explain;
  }

  if ( command == NULL ) {
    //
    // We haven't at least matched a command yet, so don't match any other
    // keywords.
    //
    return NULL;
  }

  static size_t match_index;
  static size_t text_len;
  static char const *const *specific_ac_keywords;

  if ( state == 0 ) {
    match_index = 0;
    text_len = strlen( text );

    //
    // Special case: for certain commands, complete using specific keywords for
    // that command.
    //
    specific_ac_keywords = command_ac_keywords( command );

    if ( specific_ac_keywords == NULL ) {
      //
      // Special case: for certain keywords, complete using specific keywords
      // for that keyword.
      //
      assert( rl_point >= 0 );
      cdecl_keyword_t const *const k =
        prev_cdecl_keyword( rl_line_buffer, STATIC_CAST( size_t, rl_point ) );
      specific_ac_keywords = k != NULL ? k->ac_next_keywords : NULL;
    }
  }

  if ( specific_ac_keywords != NULL ) {
    //
    // There's a special-case command having specific keywords in effect:
    // attempt to match against only those.
    //
    for ( char const *s; (s = specific_ac_keywords[ match_index ]) != NULL; ) {
      ++match_index;
      cdecl_keyword_t const *const k = cdecl_keyword_find( s );
      if ( k != NULL && !opt_lang_is_any( k->ac_lang_ids ) )
        continue;
      if ( strncmp( s, text, text_len ) == 0 )
        return check_strdup( s );
    } // for
    return NULL;
  }

  //
  // Otherwise, just attempt to match (almost) any keyword (if any text has
  // been typed).
  //
  if ( text_len == 0 )
    return NULL;

  static ac_keyword_t const *ac_keywords, *no_other_k;
  static bool is_gibberish;
  static bool returned_any;

  if ( state == 0 ) {
    if ( ac_keywords == NULL )
      ac_keywords = init_ac_keywords();
    is_gibberish = !is_english_command( command );
    no_other_k = NULL;
    returned_any = false;
  }

  for ( ac_keyword_t const *k;
        (k = ac_keywords + match_index)->literal != NULL; ) {
    ++match_index;

    //
    // If we're deciphering gibberish into pseudo-English, but the current
    // keyword shouldn't be autocompleted in gibberish, skip it.
    //
    if ( is_gibberish && !k->ac_in_gibberish )
      continue;

    if ( !opt_lang_is_any( k->ac_lang_ids ) )
      continue;
    if ( strncmp( k->literal, text, text_len ) != 0 )
      continue;

    if ( k->lang_syn != NULL ) {
      //
      // If this keyword is a synonym for another keyword and the text typed so
      // far is a prefix of the synonym, skip this keyword because the synonym
      // was previously returned and we don't want to return this keyword and
      // its synonym since it's redundant.
      //
      // For example, if this keyword is "character" (a synonym for "char"),
      // and the text typed so far is "char", skip "character" since it would
      // be redundant with "char".
      //
      char const *const synonym = c_lang_literal( k->lang_syn );
      if ( synonym != NULL && str_is_prefix( text, synonym ) )
        continue;
    }

    switch ( k->ac_policy ) {
      case AC_POLICY_DEFAULT:
        returned_any = true;
        return check_strdup( k->literal );
      case AC_POLICY_IN_NEXT_ONLY:
        continue;
      case AC_POLICY_NO_OTHER:
        no_other_k = k;
        continue;
    } // switch
    UNEXPECTED_INT_VALUE( k->ac_policy );
  } // for

  if ( no_other_k != NULL && !returned_any ) {
    returned_any = true;
    return check_strdup( no_other_k->literal );
  }

  return NULL;
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Initializes readline.
 *
 * @param prog_name The name of the running program to parse from `~/.inputrc`.
 * @param rin The `FILE` to read from.
 * @param rout The `FILE` to write to.
 */
void readline_init( char const *prog_name, FILE *rin, FILE *rout ) {
  assert( rin != NULL );
  assert( rout != NULL );

  // Allow almost any non-identifier character to break a word -- except '-'
  // since we use that as part of hyphenated keywords.
  rl_basic_word_break_characters =
    CONST_CAST( char*, "\t\n \"!#$%&'()*+,./:;<=>?@[\\]^`{|}" );

  rl_attempted_completion_function = cdecl_rl_completion;
  rl_instream = rin;
  rl_outstream = rout;
  rl_readline_name = CONST_CAST( char*, prog_name );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
