/*
**      cdecl -- C gibberish translator
**      src/p_token.c
**
**      Copyright (C) 2023-2024  Paul J. Lucas
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

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define P_TOKEN_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "p_token.h"
#include "c_lang.h"
#include "c_operator.h"
#include "color.h"
#include "lexer.h"
#include "literals.h"
#include "p_macro.h"
#include "parser.h"
#include "print.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

/**
 * @addtogroup p-token-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

NODISCARD
static bool is_multi_char_punctuator( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * If the last non-#P_PLACEMARKER token of \a token_list, if any, and \a token
 * are both #P_PUNCTUATOR tokens and pasting (concatenating) them together
 * would form a different valid #P_PUNCTUATOR token, appends a #P_SPACE token
 * onto \a token_list to avoid this.
 *
 * @param token_list The \ref p_token_list_t whose last token, if any, to avoid
 * pasting with.
 * @param token The \ref p_token to avoid pasting.  It is _not_ appended to \a
 * token_list.
 *
 * @sa is_multi_char_punctuator()
 * @sa [Token Spacing](https://gcc.gnu.org/onlinedocs/gcc-4.9.3/cppinternals/Token-Spacing.html#Token-Spacing).
 */
static void avoid_paste( p_token_list_t *token_list, p_token_t const *token ) {
  assert( token != NULL );
  assert( token_list != NULL );

  if ( token->kind != P_PUNCTUATOR )
    return;

  //
  // Get the last token of token_list that is not a P_PLACEMARKER, if any.  If
  // said token is not a P_PUNCTUATOR, return.
  //
  p_token_t const *last_token;
  for ( size_t roffset = 0; true; ++roffset ) {
    last_token = slist_atr( token_list, roffset );
    if ( last_token == NULL )
      return;
    if ( last_token->kind == P_PUNCTUATOR )
      break;
    if ( last_token->kind != P_PLACEMARKER )
      return;
  } // for

  char const *const s1 = p_token_str( last_token );
  char const *const s2 = p_token_str( token );

  //
  // It's large enough to hold two of the longest operators of `->*`, `<<=`,
  // `<=>`, or `>>=`, consecutively, plus a terminating `\0`.
  //
  char paste_buf[7];                    // 1112220

  check_snprintf( paste_buf, sizeof paste_buf, "%s%s", s1, s2 );
  if ( is_multi_char_punctuator( paste_buf ) )
    goto append;

  if ( s2[1] != '\0' ) {
    //
    // We also have to check for cases where a partial paste of the token would
    // form a different valid punctuator, e.g.:
    //
    //      cdecl> #define P(X)  -X
    //      cdecl> expand P(->)
    //      P(->) => -X
    //      | X => ->
    //      P(->) => - ->                 // not: -->
    //
    // That would later be parsed as -- > which is wrong.
    //
    check_snprintf( paste_buf, sizeof paste_buf, "%s%c", s1, s2[0] );
    if ( is_multi_char_punctuator( paste_buf ) )
      goto append;
  }

  return;

append:
  slist_push_back( token_list, p_token_new( P_SPACE, /*literal=*/NULL ) );
}

/**
 * Checks whether \a identifier_token will not expand.
 *
 * @remarks
 * @parblock
 * An identifier token will not expand if it's a macro and it's one of:
 *
 *  + Ineligible; or:
 *  + An argument of either #P_CONCAT or #P_STRINGIFY; or:
 *  + Dynamic and not supported in the current language; or:
 *  + A function-like macro not followed by `(`.
 * @endparblock
 *
 * @param identifier_token The #P_IDENTIFIER \ref p_token to check.
 * @param prev_node The non-space \ref p_token_node_t just before \a
 * identifier_token.
 * @param next_node The non-space \ref p_token_node_t just after \a
 * identifier_token.
 * @return Returns `true` only if \a identifier_token will not expand.
 *
 * @note This is a helper function for print_token_list_color() to know whether
 * to print a #P_IDENTIFIER token in the \ref sgr_macro_no_expand color.
 */
NODISCARD
static bool ident_will_not_expand( p_token_t const *identifier_token,
                                   p_token_node_t const *prev_node,
                                   p_token_node_t const *next_node ) {
  assert( identifier_token != NULL );
  assert( identifier_token->kind == P_IDENTIFIER );

  if ( identifier_token->ident.ineligible )
    return true;

  p_macro_t const *const macro = p_macro_find( identifier_token->ident.name );
  if ( macro == NULL )
    return false;

  if ( p_is_operator_arg( prev_node, next_node ) )
    return true;
  if ( macro->is_dynamic &&
       !opt_lang_is_any( (*macro->dyn_fn)( /*ptoken=*/NULL ) ) ) {
    return true;
  }
  if ( !p_macro_is_func_like( macro ) )
    return false;
  if ( !p_token_node_is_punct( next_node, '(' ) )
    return true;

  return false;
}

/**
 * Checks whether \a s is a multi-character punctuator.
 *
 * @param s The literal to check.
 * @return Returns `true` only if \a s is a multi-character punctuator.
 *
 * @sa avoid_paste()
 */
NODISCARD
static bool is_multi_char_punctuator( char const *s ) {
  static c_lang_lit_t const MULTI_CHAR_PUNCTUATORS[] = {
    { LANG_ANY,                 "!="  },
    { LANG_ANY,                 "%="  },
    { LANG_ANY,                 "&&"  },
    { LANG_ANY,                 "&="  },
    { LANG_ANY,                 "*="  },
    { LANG_ANY,                 "++"  },
    { LANG_ANY,                 "+="  },
    { LANG_ANY,                 "--"  },
    { LANG_ANY,                 "-="  },
    { LANG_ANY,                 "->"  },
    { LANG_CPP_ANY,             "->*" },
    { LANG_CPP_ANY,             ".*"  },
    { LANG_ANY,                 "/*"  },
    { LANG_SLASH_SLASH_COMMENT, "//"  },
    { LANG_ANY,                 "/="  },
    { LANG_CPP_ANY,             "::"  },
    { LANG_ANY,                 "<<"  },
    { LANG_ANY,                 "<<=" },
    { LANG_ANY,                 "<="  },
    { LANG_LESS_EQUAL_GREATER,  "<=>" },
    { LANG_ANY,                 "=="  },
    { LANG_ANY,                 ">="  },
    { LANG_ANY,                 ">>=" },
    { LANG_ANY,                 "^="  },
    { LANG_ANY,                 "|="  },
    { LANG_ANY,                 "||"  },
  };

  FOREACH_ARRAY_ELEMENT( c_lang_lit_t, punct, MULTI_CHAR_PUNCTUATORS ) {
    if ( !opt_lang_is_any( punct->lang_ids ) )
      continue;
    if ( strcmp( s, punct->literal ) == 0 )
      return true;
  } // for

  return false;
}

/**
 * A predicate function for slist_free_if() that checks whether \a token_node
 * is a #P_SPACE token and precedes another: if so, frees it.
 *
 * @param token_node A pointer to the \ref p_token to possibly free.
 * @param user_data Not used.
 * @return Returns `true` only if \a token_node was freed.
 */
static bool p_token_free_if_consec_space( p_token_node_t *token_node,
                                          user_data_t user_data ) {
  assert( token_node != NULL );
  (void)user_data;
  p_token_t *const token = token_node->data;
  assert( token != NULL );

  if ( token->kind != P_SPACE )
    return false;

  p_token_node_t const *const next_node =
    p_token_node_not( token_node->next, P_PLACEMARKER );
  if ( p_token_node_is_any( next_node, P_ANY_OPAQUE ) )
    return false;

  p_token_free( token );
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

char const* p_kind_name( p_token_kind_t kind ) {
  switch ( kind ) {
    case P_CHAR_LIT   : return "char_lit";
    case P_CONCAT     : return "##";
    case P_IDENTIFIER : return "identifier";
    case P_NUM_LIT    : return "num_lit";
    case P_OTHER      : return "other";
    case P_PLACEMARKER: return "placemarker";
    case P_PUNCTUATOR : return "punctuator";
    case P_SPACE      : return " ";
    case P_STRINGIFY  : return "#";
    case P_STR_LIT    : return "str_lit";
    case P___VA_ARGS__: return L___VA_ARGS__;
    case P___VA_OPT__ : return L___VA_OPT__;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

p_token_t* p_token_dup( p_token_t const *token ) {
  if ( token == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  p_token_t *const dup_token = MALLOC( p_token_t, 1 );
  dup_token->kind = token->kind;
  dup_token->loc = token->loc;
  dup_token->is_substituted = token->is_substituted;
  switch ( token->kind ) {
    case P_CHAR_LIT:
    case P_NUM_LIT:
    case P_STR_LIT:
      dup_token->lit.value = check_strdup( token->lit.value );
      break;
    case P_IDENTIFIER:
      dup_token->ident.ineligible = token->ident.ineligible;
      dup_token->ident.name = check_strdup( token->ident.name );
      break;
    case P_OTHER:
      dup_token->other.value = token->other.value;
      break;
    case P_PUNCTUATOR:
      strcpy( dup_token->punct.value, token->punct.value );
      break;
    case P_CONCAT:
    case P_PLACEMARKER:
    case P_SPACE:
    case P_STRINGIFY:
    case P___VA_ARGS__:
    case P___VA_OPT__:
      // nothing to do
      break;
  } // switch
  return dup_token;
}

void p_token_free( p_token_t *token ) {
  if ( token == NULL )
    return;                             // LCOV_EXCL_LINE
  switch ( token->kind ) {
    case P_CHAR_LIT:
    case P_NUM_LIT:
    case P_STR_LIT:
      FREE( token->lit.value );
      break;
    case P_IDENTIFIER:
      FREE( token->ident.name );
      break;
    case P_CONCAT:
    case P_OTHER:
    case P_PLACEMARKER:
    case P_PUNCTUATOR:
    case P_SPACE:
    case P_STRINGIFY:
    case P___VA_ARGS__:
    case P___VA_OPT__:
      // nothing to do
      break;
  } // switch
  free( token );
}

p_token_t* p_token_lex( c_loc_t const *loc, strbuf_t *sbuf ) {
  assert( loc != NULL );
  assert( sbuf != NULL );

  if ( sbuf->len == 0 )
    return p_token_new_loc( P_PLACEMARKER, loc, /*literal=*/NULL );

  // Preprocessor lines must end in a newline.
  strbuf_putc( sbuf, '\n' );
  lexer_push_string( sbuf->str, sbuf->len );

  p_token_t *token = NULL;
  int y_token_id = yylex();

  switch ( y_token_id ) {
    case '!':
    case '#':                           // ordinary '#', not P_STRINGIFY
    case '%':
    case '&':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '[':
    case ']':
    case '^':
    case '{':
    case '|':
    case '}':
    case '~':
    case Y_AMPER_AMPER:
    case Y_AMPER_EQUAL:
    case Y_CARET_EQUAL:
    case Y_COLON_COLON_STAR:
    case Y_DOT_DOT_DOT:
    case Y_EQUAL_EQUAL:
    case Y_EXCLAM_EQUAL:
    case Y_GREATER_EQUAL:
    case Y_GREATER_GREATER:
    case Y_GREATER_GREATER_EQUAL:
    case Y_LESS_EQUAL:
    case Y_LESS_LESS:
    case Y_LESS_LESS_EQUAL:
    case Y_MINUS_EQUAL:
    case Y_MINUS_GREATER:
    case Y_MINUS_MINUS:
    case Y_PERCENT_EQUAL:
    case Y_PIPE_EQUAL:
    case Y_PIPE_PIPE:
    case Y_PLUS_EQUAL:
    case Y_PLUS_PLUS:
    case Y_SLASH_EQUAL:
    case Y_STAR_EQUAL:
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, lexer_token );
      break;

    case Y_COLON_COLON:
    case Y_DOT_STAR:
    case Y_MINUS_GREATER_STAR:
      //
      // Special case: the lexer isn't language-sensitive (which would be hard
      // to do) so these tokens are always recognized.  But if the current
      // language isn't C++, consider them as two tokens (which is a
      // concatenation error).
      //
      if ( !OPT_LANG_IS( CPP_ANY ) )
        goto done;
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, lexer_token );
      break;

    case Y_LESS_EQUAL_GREATER:
      //
      // Special case: same as above tokens.
      //
      if ( !OPT_LANG_IS( LESS_EQUAL_GREATER ) )
        goto done;
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, lexer_token );
      break;

    case Y_CHAR_LIT:
      token = p_token_new_loc( P_CHAR_LIT, &yylloc, yylval.str_val );
      break;

    case Y_FLOAT_LIT:
    case Y_INT_LIT:
      token =
        p_token_new_loc( P_NUM_LIT, &yylloc, check_strdup( lexer_token ) );
      break;

    case Y_NAME:
      token = p_token_new_loc( P_IDENTIFIER, &yylloc, yylval.name );
      break;

    case Y_STR_LIT:
      token = p_token_new_loc( P_STR_LIT, &yylloc, yylval.str_val );
      break;

    case Y_P_CONCAT:
      //
      // Given:
      //
      //      #define hash_hash # ## #
      //
      // when expanding hash_hash, the concat operator produces a new token
      // consisting of two adjacent sharp signs, but this new token is NOT the
      // concat operator.
      //
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, "##" );
      break;

    case Y_P_SPACE:                     // can't result from concatenation
      UNEXPECTED_INT_VALUE( y_token_id );

    case Y_P___VA_ARGS__:
      //
      // Given:
      //
      //      cdecl> #define M(...)   __VA ## _ARGS__
      //      cdecl> expand M(x)
      //      M(x) => __VA_ARGS__
      //
      // when expanding M, the concat operator produces a new __VA_ARGS__
      // token, but this new token is NOT the normal __VA_ARGS__.
      //
      token = p_token_new_loc(
        P_IDENTIFIER, &yylloc, check_strdup( L___VA_ARGS__ )
      );
      token->ident.ineligible = true;
      break;

    case Y_P___VA_OPT__:
      //
      // Given:
      //
      //      cdecl> #define M(...)   __VA_ARGS__ __VA ## _OPT__(y)
      //      cdecl> expand M(x)
      //      M(x) => x __VA_OPT__(y)
      //
      // when expanding M, the concat operator produces a new __VA_OPT__ token,
      // but this new token is NOT the normal __VA_OPT__.
      //
      token = p_token_new_loc(
        P_IDENTIFIER, &yylloc, check_strdup( L___VA_OPT__ )
      );
      token->ident.ineligible = true;
      break;

    case '$':
    case '@':
    case '`':
      token = p_token_new_loc( P_OTHER, &yylloc, lexer_token );
      break;

    case Y_LEXER_ERROR:
      goto done;

    default:
      UNEXPECTED_INT_VALUE( y_token_id );
  } // switch

  //
  // We've successfully lex'd a token: now try to lex another one to see if
  // there is another one.
  //
  y_token_id = yylex();

done:
  lexer_pop_string();
  sbuf->str[ --sbuf->len ] = '\0';      // remove newline

  switch ( y_token_id ) {
    case Y_END:                         // exactly one token: success
      return token;
    case Y_LEXER_ERROR:
      break;
    default:                            // more than one token: failure
      print_error( loc,
        "\"%s\": concatenation formed invalid token\n", sbuf->str
      );
  } // switch

  p_token_free( token );
  return NULL;
}

void p_token_list_cleanup( p_token_list_t *list ) {
  slist_cleanup( list, POINTER_CAST( slist_free_fn_t, &p_token_free ) );
}

size_t p_token_list_relocate( p_token_list_t *token_list,
                              size_t first_column ) {
  assert( token_list != NULL );

  bool relocated_opaque = false;

  FOREACH_SLIST_NODE( token_node, token_list ) {
    p_token_t *const token = token_node->data;
    switch ( token->kind ) {
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !relocated_opaque )
          continue;                     // don't do leading spaces
        if ( p_token_node_emptyish( token_node->next ) )
          goto done;                    // don't do trailing spaces either
        FALLTHROUGH;
      default:
        token->loc.first_column = STATIC_CAST( c_loc_num_t, first_column );
        first_column += strlen( p_token_str( token ) );
        token->loc.last_column = STATIC_CAST( c_loc_num_t, first_column - 1 );
        relocated_opaque = true;
        break;
    } // switch
  } // for

done:
  return first_column;
}

void p_token_list_push_back( p_token_list_t *token_list, p_token_t *token ) {
  avoid_paste( token_list, token );
  slist_push_back( token_list, token );
}

char const* p_token_list_str( p_token_list_t const *token_list ) {
  assert( token_list != NULL );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );

  bool stringified_opaque = false;

  FOREACH_SLIST_NODE( token_node, token_list ) {
    p_token_t const *const token = token_node->data;
    switch ( token->kind ) {
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !stringified_opaque )
          continue;                     // don't do leading spaces
        if ( p_token_node_emptyish( token_node->next ) )
          goto done;                    // don't do trailing spaces either
        FALLTHROUGH;
      default:
        strbuf_puts( &sbuf, p_token_str( token ) );
        stringified_opaque = true;
        break;
    } // switch
  } // for

done:
  return empty_if_null( sbuf.str );
}

void p_token_list_trim( p_token_list_t *token_list ) {
  assert( token_list != NULL );

  while ( !slist_empty( token_list ) ) {
    p_token_t *const token = slist_front( token_list );
    if ( token->kind != P_SPACE )
      break;
    p_token_free( slist_pop_front( token_list ) );
  } // while

  while ( !slist_empty( token_list ) ) {
    p_token_t *const token = slist_back( token_list );
    if ( token->kind != P_SPACE )
      break;
    p_token_free( slist_pop_back( token_list ) );
  } // while

  slist_free_if( token_list, &p_token_free_if_consec_space, USER_DATA_ZERO );
}

p_token_t* p_token_new_loc( p_token_kind_t kind, c_loc_t const *loc,
                            char const *literal ) {
  p_token_t *const token = MALLOC( p_token_t, 1 );
  *token = (p_token_t){ .kind = kind };
  if ( loc != NULL )
    token->loc = *loc;
  switch ( kind ) {
    case P_CHAR_LIT:
    case P_NUM_LIT:
    case P_STR_LIT:
      assert( literal != NULL );
      token->lit.value = literal;
      break;
    case P_IDENTIFIER:
      assert( literal != NULL );
      token->ident.name = literal;
      break;
    case P_OTHER:
      assert( literal != NULL );
      assert( literal[0] != '\0' );
      assert( literal[1] == '\0' );
      token->other.value = literal[0];
      break;
    case P_PUNCTUATOR:
      assert( literal != NULL );
      assert( literal[0] != '\0' );
      assert( literal[1] == '\0' || literal[2] == '\0' || literal[3] == '\0' );
      strcpy( token->punct.value, literal );
      break;
    case P_CONCAT:
    case P_PLACEMARKER:
    case P_SPACE:
    case P_STRINGIFY:
    case P___VA_ARGS__:
    case P___VA_OPT__:
      assert( literal == NULL );
      break;
  } // switch
  return token;
}

bool p_token_node_is_any( p_token_node_t const *token_node,
                          p_token_kind_t kinds ) {
  if ( token_node == NULL )
    return false;
  p_token_t const *const token = token_node->data;
  return (token->kind & kinds) != 0;
}

bool p_token_node_is_punct( p_token_node_t const *token_node, char punct ) {
  return token_node != NULL && p_token_is_punct( token_node->data, punct );
}

p_token_node_t* p_token_node_not( p_token_node_t *token_node,
                                  p_token_kind_t kinds ) {
  for ( ; token_node != NULL; token_node = token_node->next ) {
    p_token_t const *const token = token_node->data;
    if ( (token->kind & kinds) == 0 )
      break;
  } // for
  return token_node;
}

char const* p_token_str( p_token_t const *token ) {
  assert( token != NULL );

  static char other_str[2];
  static strbuf_t sbuf;

  switch ( token->kind ) {
    case P_CHAR_LIT:
      strbuf_reset( &sbuf );
      strbuf_puts_quoted( &sbuf, '\'', token->lit.value );
      return sbuf.str;
    case P_CONCAT:
      return "##";
    case P_IDENTIFIER:
      return token->ident.name;
    case P_NUM_LIT:
      return token->lit.value;
    case P_OTHER:
      other_str[0] = token->other.value;
      return other_str;
    case P_PLACEMARKER:
      return "";
    case P_PUNCTUATOR:
      return token->punct.value;
    case P_SPACE:
      return " ";
    case P_STRINGIFY:
      return "#";
    case P_STR_LIT:
      strbuf_reset( &sbuf );
      strbuf_puts_quoted( &sbuf, '"', token->lit.value );
      return sbuf.str;
    case P___VA_ARGS__:
      return L___VA_ARGS__;
    case P___VA_OPT__:
      return L___VA_OPT__;
  } // switch

  UNEXPECTED_INT_VALUE( token->kind );
}

void print_token_list( p_token_list_t const *token_list, FILE *fout ) {
  assert( token_list != NULL );
  assert( fout != NULL );

  bool printed_opaque = false;

  FOREACH_SLIST_NODE( token_node, token_list ) {
    p_token_t const *const token = token_node->data;
    switch ( token->kind ) {
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !printed_opaque )
          continue;                     // don't print leading spaces
        if ( p_token_node_emptyish( token_node->next ) )
          return;                       // don't print trailing spaces either
        break;
      default:
        break;
    } // switch

    FPUTS( p_token_str( token ), fout );
    printed_opaque = true;
  } // for
}

void print_token_list_color( p_token_list_t const *token_list, FILE *fout ) {
  assert( token_list != NULL );
  assert( fout != NULL );

  bool printed_opaque = false;

  p_token_node_t const *prev_node = NULL;
  FOREACH_SLIST_NODE( token_node, token_list ) {
    char const *color = NULL;
    p_token_t const *const token = token_node->data;

    p_token_node_t const *const next_node =
      p_token_node_not( token_node->next, P_ANY_TRANSPARENT );

    switch ( token->kind ) {
      case P_IDENTIFIER:
        if ( ident_will_not_expand( token, prev_node, next_node ) ) {
          color = sgr_macro_no_expand;
          break;
        }
        FALLTHROUGH;
      default:
        if ( token->is_substituted )
          color = sgr_macro_subst;
        break;
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !printed_opaque )
          continue;                     // don't print leading spaces
        if ( next_node == NULL )
          return;                       // don't print trailing spaces either
        break;
    } // switch

    color_start( fout, color );
    FPUTS( p_token_str( token ), fout );
    color_end( fout, color );
    printed_opaque = true;

    if ( token->kind != P_SPACE )
      prev_node = token_node;
  } // for
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
