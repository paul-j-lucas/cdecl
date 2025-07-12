/*
**      cdecl -- C gibberish translator
**      src/c_ast_check.c
**
**      Copyright (C) 2017-2025  Paul J. Lucas
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
 * Defines functions for checking an AST for semantic errors.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast_check.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_ast_warn.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_operator.h"
#include "c_sname.h"
#include "c_type.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "gibberish.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <string.h>

/// @endcond

/**
 * Storage-class-like types that are _not_ legal with `constexpr` in C only.
 */
#define TS_NOT_constexpr_C_ONLY   ( TS__Atomic | TS_restrict | TS_volatile )

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints an error: `<kind> not supported[ {in|since|unless|until} <lang>]`.
 *
 * @param AST The AST having the unsupported kind.
 * @param LANG_IDS The bitwise-or of legal language(s).
 */
#define error_kind_not_supported(AST,LANG_IDS)              \
  print_error( &(AST)->loc,                                 \
    "%s not supported%s\n",                                 \
    c_kind_name( (AST)->kind ), c_lang_which( (LANG_IDS) )  \
  )

/**
 * Prints an error: `<kind> can not be <type>`.
 *
 * @param AST The AST .
 * @param TID The bad type.
 * @param LANG_IDS The language(s) that this is legal in, if any.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_not_tid(AST,TID,LANG_IDS,END_STR_LIT)  \
  print_error( &(AST)->loc,                               \
    "%s can not be \"%s\"%s" END_STR_LIT,                 \
    c_kind_name( (AST)->kind ), c_tid_error( (TID) ),     \
    c_lang_which( (LANG_IDS) )                            \
  )

/**
 * Prints an error: `<kind> of <kind> is illegal`.
 *
 * @param AST1 The AST having the bad kind.
 * @param AST2 The AST having the other kind.
 */
#define error_kind_of_kind(AST1,AST2) BLOCK(                          \
  print_error( &(AST1)->loc, "%s of ", c_kind_name( (AST1)->kind ) ); \
  print_ast_kind_aka( (AST2), stderr );                               \
  EPUTS( " is illegal" ); )

/**
 * Prints an error: `<kind> to <type> is illegal`.
 *
 * @param AST The AST having the bad kind.
 * @param TID The bad type.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_to_tid(AST,TID,END_STR_LIT)        \
  print_error( &(AST)->loc,                           \
    "%s to \"%s\" is illegal" END_STR_LIT,            \
    c_kind_name( (AST)->kind ), c_tid_error( (TID) )  \
  )

///////////////////////////////////////////////////////////////////////////////

/**
 * State maintained by c_ast_check_visitor().
 */
struct c_ast_check_state {
  /**
   * If the current AST node is the \ref c_typedef_ast::for_ast "for_ast" of a
   * #K_TYPEDEF AST, store that #K_TYPEDEF AST here.
   *
   * We need to know if `tdef_ast` is the \ref c_ptr_ref_ast::to_ast "to_ast"
   * of a #K_POINTER AST for a case like:
   *
   *      typedef void V;               // typedef AST1 AST2
   *      explain V *p;                 // explain AST2 AST3
   *
   * That is, if AST3 (`p`) is a pointer to AST2 (`V`) that is a `typedef` of
   * AST1 (`void`), then AST1 is a "pointee" because it is pointed to from AST3
   * (indirectly via AST2).
   *
   * This is needed only for a pointer to a `typedef` of `void` since:
   *
   *  + A variable of `void` is illegal; but:
   *  + A `typedef` of `void` is legal; and:
   *  + A pointer to `void` is also legal; therefore:
   *  + A pointer to a `typedef` of `void` is also legal.
   */
  c_ast_t const  *tdef_ast;
};
typedef struct c_ast_check_state c_ast_check_state_t;

// local constants

/// Convenience return value for \ref c_ast_visit_fn_t functions.
static bool const VISITOR_ERROR_FOUND     = true;

/// Convenience return value for \ref c_ast_visit_fn_t functions.
static bool const VISITOR_ERROR_NOT_FOUND = false;

// local functions
NODISCARD
static bool         c_ast_check_emc( c_ast_t const* ),
                    c_ast_check_func_default_delete( c_ast_t const* ),
                    c_ast_check_func_main( c_ast_t const* ),
                    c_ast_check_func_main_char_ptr_param( c_ast_t const* ),
                    c_ast_check_func_params_knr( c_ast_t const* ),
                    c_ast_check_func_params_redef( c_ast_t const* ),
                    c_ast_check_lambda_captures( c_ast_t const* ),
                    c_ast_check_lambda_captures_redef( c_ast_t const* ),
                    c_ast_check_op_minus_minus_plus_plus_params( c_ast_t const* ),
                    c_ast_check_op_relational_default( c_ast_t const* ),
                    c_ast_check_oper_default( c_ast_t const* ),
                    c_ast_check_oper_params( c_ast_t const* ),
                    c_ast_check_upc( c_ast_t const* ),
                    c_ast_has_escu_param( c_ast_t const* ),
                    c_ast_visitor_error( c_ast_t const*, user_data_t ),
                    c_ast_visitor_type( c_ast_t const*, user_data_t ),
                    c_op_is_new_delete( c_op_id_t );

NODISCARD
static char const*  c_ast_member_or_nonmember_str( c_ast_t const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks an entire AST for semantic errors using \a check_fn.
 *
 * @param ast The AST to check.
 * @param check_fn The check function to use.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static inline bool c_ast_check_visitor( c_ast_t const *ast,
                                        c_ast_visit_fn_t check_fn ) {
  return NULL == c_ast_visit(
    ast, C_VISIT_DOWN, check_fn,
    (user_data_t){ .pc = &(c_ast_check_state_t){ 0 } }
  );
}

/**
 * Gets whether \a ast is a lambda capture for either `this` or `*this`.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if it is.
 */
NODISCARD
static inline bool c_ast_is_capture_this( c_ast_t const *ast ) {
  return  ast->capture.kind == C_CAPTURE_THIS ||
          ast->capture.kind == C_CAPTURE_STAR_THIS;
}

/**
 * Gets the location of the first parameter of \a ast, if any.
 *
 * @param ast The AST to get the location of its first parameter of, if any.
 * @return Returns the location of either the first parameter of \a ast or \a
 * ast if \a ast has no parameters.
 */
NODISCARD
static inline c_loc_t const* c_ast_params_loc( c_ast_t const *ast ) {
  c_ast_t const *const param_ast = c_param_ast( c_ast_params( ast ) );
  return &IF_ELSE_EXPR( param_ast, ast )->loc;
}

/**
 * Returns an `"s"` or not based on \a n to pluralize a word.
 *
 * @param n A quantity.
 * @return Returns the empty string only if \a n == 1; otherwise returns `"s"`.
 */
NODISCARD
static inline char const* plural_s( uint64_t n ) {
  return n == 1 ? "" : "s";
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks the \ref c_ast::align "align" of an AST for errors.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_alignas( c_ast_t const *ast ) {
  assert( ast != NULL );

  if ( ast->align.kind == C_ALIGNAS_NONE )
    return true;

  if ( c_tid_is_any( ast->type.stids, TS_typedef ) ) {
    print_error( &ast->align.loc, "types can not be aligned\n" );
    return false;
  }

  if ( c_ast_is_register( ast ) ) {
    print_error( &ast->align.loc,
      "\"%s\" can not be combined with \"register\"\n", alignas_name()
    );
    return false;
  }

  c_ast_t const *const raw_ast = c_ast_untypedef( ast );

  if ( (raw_ast->kind & K_ANY_OBJECT) == 0 ) {
    print_error( &ast->align.loc, "%s", "" );
    print_ast_kind_aka( ast, stderr );
    EPUTS( " can not be aligned\n" );
    return false;
  }

  if ( (raw_ast->kind & K_ANY_BIT_FIELD) != 0 &&
        ast->bit_field.bit_width > 0 ) {
    print_error( &ast->align.loc, "bit fields can not be aligned\n" );
    return false;
  }

  if ( (raw_ast->kind & K_CLASS_STRUCT_UNION) != 0 &&
       !OPT_LANG_IS( ALIGNED_CSUS ) ) {
    print_error( &ast->align.loc, "%s", "" );
    print_ast_kind_aka( ast, stderr );
    EPRINTF( " can not be aligned%s\n", C_LANG_WHICH( ALIGNED_CSUS ) );
    return false;
  }

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      unreachable();
    case C_ALIGNAS_BYTES:
      if ( !is_01_bit( ast->align.bytes ) ) {
        print_error( &ast->align.loc,
          "\"%u\": alignment must be a power of 2\n", ast->align.bytes
        );
        return false;
      }
      break;
    case C_ALIGNAS_SNAME:
      // nothing to do
      break;
    case C_ALIGNAS_TYPE:
      return c_ast_check( ast->align.type_ast );
  } // switch

  return true;
}

/**
 * Checks a #K_ARRAY AST for errors.
 *
 * @param ast The #K_ARRAY AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_array( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );

  if ( c_tid_is_any( ast->type.stids, TS__Atomic ) ) {
    error_kind_not_tid( ast, TS__Atomic, LANG_NONE, "\n" );
    return false;
  }

  switch ( ast->array.kind ) {
    case C_ARRAY_SIZE_NONE:
      if ( c_tid_is_any( ast->type.stids, TS_NON_EMPTY_ARRAY ) ) {
        //
        // When deciphering gibberish into pseudo-English, this situation is
        // impossible because the C grammar requires that `static` is
        // immediately followed by an array size, e.g.:
        //
        //      void f( int x[static 2] )
        //
        // so omitting the size would result in a syntax error.
        //
        // However, this situation can be true when converting pseudo-English
        // into gibberish because the pseudo-English grammar has `non-empty`
        // before `array` and the optional size after:
        //
        //      declare f as function ( x as non-empty array of int )
        //
        // so we have to check for it.
        //
        print_error( &ast->loc, "\"non-empty\" requires an array dimension\n" );
        return false;
      }
      break;

    case C_ARRAY_SIZE_INT:
      if ( ast->array.size_int == 0 ) {
        print_error( &ast->loc, "array dimension must be > 0\n" );
        return false;
      }
      break;

    case C_ARRAY_SIZE_NAME:
      if ( !c_ast_is_param( ast ) )
        break;
      c_ast_t const *const size_param_ast =
        c_ast_find_param_named( ast->param_of_ast, ast->array.size_name, ast );
      if ( size_param_ast == NULL )
        break;
      // At this point, we know it's a VLA.
      if ( !c_ast_is_integral( size_param_ast ) ) {
        print_error( &ast->loc, "invalid array dimension type " );
        print_ast_type_aka( size_param_ast, stderr );
        EPUTS( "; must be integral\n" );
        return false;
      }
      FALLTHROUGH;

    case C_ARRAY_SIZE_VLA:
      if ( !OPT_LANG_IS( VLAS ) ) {
        print_error( &ast->loc,
          "variable length arrays not supported%s\n",
          C_LANG_WHICH( VLAS )
        );
        return false;
      }
      break;
  } // switch

  if ( c_tid_is_any( ast->type.stids, TS_ANY_ARRAY_QUALIFIER ) &&
       !OPT_LANG_IS( QUALIFIED_ARRAYS ) ) {
    print_error( &ast->loc,
      "\"%s\" arrays not supported%s\n",
      c_tid_error( ast->type.stids ),
      C_LANG_WHICH( QUALIFIED_ARRAYS )
    );
    return false;
  }

  c_ast_t const *const of_ast = ast->array.of_ast;
  c_ast_t const *const raw_of_ast = c_ast_untypedef( of_ast );

  switch ( raw_of_ast->kind ) {
    case K_ARRAY:
      if ( of_ast->array.kind == C_ARRAY_SIZE_NONE ) {
        print_error( &of_ast->loc, "array dimension required\n" );
        return false;
      }
      break;

    case K_BUILTIN:
      if ( c_ast_is_builtin_any( raw_of_ast, TB_void ) ) {
        print_error( &ast->loc, "array of \"%s\"", c_tid_error( TB_void ) );
        if ( is_english_to_gibberish() )
          print_hint( "array of \"pointer to void\"" );
        else
          print_hint( "array of \"void*\"" );
        return false;
      }
      break;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
      error_kind_of_kind( ast, of_ast );
      print_hint( "array of pointer to function" );
      return false;

    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      error_kind_of_kind( ast, of_ast );
      if ( is_english_to_gibberish() ) {
        print_hint( "%s to array", c_kind_name( raw_of_ast->kind ) );
      } else {
        print_hint( "(%s%s)[]",
          other_token_c( raw_of_ast->kind == K_REFERENCE ? "&" : "&&" ),
          c_sname_gibberish( c_ast_find_name( ast, C_VISIT_DOWN ) )
        );
      }
      return false;

    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_ENUM:
    case K_NAME:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
      // nothing to do
      break;

    case K_CAPTURE:
    case K_CAST:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_LAMBDA:
    case K_OPERATOR:
    case K_PLACEHOLDER:
    case K_STRUCTURED_BINDING:
    case K_TYPEDEF:                     // impossible after c_ast_untypedef()
    case K_UDEF_CONV:
    case K_UDEF_LIT:
    case K_VARIADIC:
      UNEXPECTED_INT_VALUE( raw_of_ast->kind );
  } // switch

  return true;
}

/**
 * Checks a #K_BUILTIN AST for errors.
 *
 * @param ast The #K_BUILTIN AST to check.
 * @param tdef_ast The #K_TYPEDEF AST \a ast is a `typedef` for, if any.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_builtin( c_ast_t const *ast, c_ast_t const *tdef_ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );
  assert( tdef_ast == NULL || tdef_ast->kind == K_TYPEDEF );

  if ( ast->type.btids == TB_NONE && !OPT_LANG_IS( IMPLICIT_int ) &&
       !c_ast_parent_is_kind_any( ast, K_UDEF_CONV ) ) {
    print_error( &ast->loc,
      "implicit \"%s\" is illegal%s\n",
      c_tid_error( TB_int ),
      C_LANG_WHICH( IMPLICIT_int )
    );
    return false;
  }

  if ( c_tid_is_any( ast->type.stids, TS_inline ) &&
       !OPT_LANG_IS( inline_VARIABLES ) ) {
    print_error( &ast->loc,
      "inline variables not supported%s\n",
      C_LANG_WHICH( inline_VARIABLES )
    );
    return false;
  }

  if ( c_tid_is_any( ast->type.btids, TB__BitInt ) ) {
    unsigned const min_bits =
      1u + !c_tid_is_any( ast->type.btids, TB_unsigned );
    if ( ast->builtin.BitInt.width < min_bits ) {
      print_error( &ast->loc,
        "%s must be at least %u bit%s\n",
        c_type_error( &ast->type ), min_bits, plural_s( min_bits )
      );
      return false;
    }
    if ( ast->builtin.BitInt.width > C_BITINT_MAXWIDTH ) {
      print_error( &ast->loc,
        "%s can be at most %u bits\n",
        c_type_error( &ast->type ), C_BITINT_MAXWIDTH
      );
      return false;
    }
  }
  else if ( ast->builtin.bit_width > 0 ) {
    if ( c_sname_count( &ast->sname ) > 1 ) {
      print_error( &ast->loc, "scoped names can not have bit-field widths\n" );
      return false;
    }
    if ( c_tid_is_any( ast->type.atids, TA_no_unique_address ) ) {
      print_error( &ast->loc,
        "\"%s\" %ss can not have bit-field widths\n",
        c_tid_error( TA_no_unique_address ),
        c_kind_name( ast->kind )
      );
      return false;
    }
    if ( ast->type.stids != TS_NONE ) {
      print_error( &ast->loc,
        "\"%s\" can not have bit-field widths\n",
        c_tid_error( ast->type.stids )
      );
      return false;
    }
  }

  if ( c_ast_is_builtin_any( ast, TB_void ) &&
       //
       // If we're of type void and:
       //
       //   + Not: int f(void)     // not a zero-parameter function; and:
       //   + Not: (void)x         // not a cast to void; and:
       //   + Not: typedef void V  // not a typedef of void; and:
       //   + Not: extern void V   // not an extern void (in C); and:
       //   + Not: V *p            // not a pointer to typedef of void; then:
       //
       // it means we must be a variable of void which is an error.
       //
       ast->parent_ast == NULL &&
       ast->kind != K_CAST &&
       !c_tid_is_any( ast->type.stids, TS_typedef ) &&
       !(OPT_LANG_IS( extern_void ) &&
         c_tid_is_any( ast->type.stids, TS_extern )) &&
       (tdef_ast == NULL ||
        !c_ast_parent_is_kind_any( tdef_ast, K_POINTER )) ) {
    print_error( &ast->loc, "variable of \"%s\"", c_tid_error( TB_void ) );
    if ( is_english_to_gibberish() )
      print_hint( "\"pointer to void\"" );
    else
      print_hint( "\"void*\"" );
    return false;
  }

  return c_ast_check_emc( ast ) && c_ast_check_upc( ast );
}

/**
 * Checks a #K_CAST AST for errors.
 *
 * @param ast The #K_CAST AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_CAST );

  if ( ast->cast.kind != C_CAST_C && !OPT_LANG_IS( NEW_STYLE_CASTS ) ) {
    print_error( &ast->loc,
      "%s not supported%s\n", c_cast_gibberish( ast->cast.kind ),
      C_LANG_WHICH( NEW_STYLE_CASTS )
    );
    return false;
  }

  c_ast_t const *const to_ast = ast->cast.to_ast;
  c_ast_t const *const storage_ast = c_ast_find_type_any(
    to_ast, C_VISIT_DOWN, &C_TYPE_LIT_S( TS_ANY_STORAGE )
  );

  if ( storage_ast != NULL ) {
    print_error( &to_ast->loc,
      "can not cast into \"%s\"\n",
      c_tid_error( storage_ast->type.stids & TS_ANY_STORAGE )
    );
    return false;
  }

  c_ast_t const *const leaf_ast = c_ast_leaf( ast );
  if ( c_ast_is_tid_any( leaf_ast, TB_auto ) ) {
    print_error( &leaf_ast->loc,
      "can not cast into \"%s\"\n",
      c_type_error( &leaf_ast->type )
    );
    return false;
  }

  c_ast_t const *const raw_to_ast = c_ast_untypedef( to_ast );

  switch ( raw_to_ast->kind ) {
    case K_ARRAY:
      if ( !c_sname_empty( &ast->sname ) ) {
        print_error( &to_ast->loc, "arithmetic or pointer type expected\n" );
        return false;
      }
      break;
    case K_FUNCTION:
      print_error( &to_ast->loc, "can not cast into " );
      print_ast_kind_aka( to_ast, stderr );
      print_hint( "cast into pointer to function" );
      return false;
    default:
      /* suppress warning */;
  } // switch

  switch ( ast->cast.kind ) {
    case C_CAST_CONST:
      if ( (raw_to_ast->kind & K_ANY_POINTER_OR_REFERENCE) == 0 ) {
        print_error( &to_ast->loc, "invalid const_cast type " );
        print_ast_type_aka( to_ast, stderr );
        EPRINTF(
          "; must be a pointer, pointer to member, %s reference\n",
          OPT_LANG_IS( RVALUE_REFERENCES ) ? "reference, or rvalue" : "or"
        );
        return false;
      }
      break;

    case C_CAST_DYNAMIC:
      if ( !c_ast_is_ptr_to_kind_any( raw_to_ast, K_CLASS_STRUCT_UNION ) &&
           !c_ast_is_ref_to_kind_any( raw_to_ast, K_CLASS_STRUCT_UNION ) ) {
        print_error( &to_ast->loc, "invalid dynamic_cast type " );
        print_ast_type_aka( to_ast, stderr );
        EPUTS(
          "; must be a pointer or reference to a class, struct, or union\n"
        );
        return false;
      }
      break;

    case C_CAST_REINTERPRET:
      if ( c_ast_is_builtin_any( to_ast, TB_void ) ) {
        print_error( &to_ast->loc, "invalid reinterpret_cast type " );
        print_ast_type_aka( to_ast, stderr );
        EPUTC( '\n' );
        return false;
      }
      break;

    case C_CAST_C:
      // A C-style cast can cast to any type, so nothing to check.
    case C_CAST_STATIC:
      // A static cast can cast to any type; but cdecl doesn't know the type of
      // the object being cast, so nothing to check.
      break;
  } // switch

  return true;
}

/**
 * Checks a #K_CONCEPT AST for errors.
 *
 * @param ast the #K_CONCEPT AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_concept( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_CONCEPT );

  c_tid_t const not_concept_stids = ast->type.stids & c_tid_compl( TS_CONCEPT );
  if ( not_concept_stids != TS_NONE ) {
    error_kind_not_tid( ast, not_concept_stids, LANG_NONE, "\n" );
    return false;
  }

  c_sname_t const *const concept_sname = &ast->concept.concept_sname;

  FOREACH_SNAME_SCOPE_UNTIL( scope, concept_sname, concept_sname->tail ) {
    c_type_t const *const scope_type = &c_scope_data( scope )->type;
    if ( scope_type->btids != TB_namespace ) {
      c_sname_t scope_sname = c_sname_scope_sname( concept_sname );
      print_error( &ast->loc,
        "concept \"%s\" may only be within a namespace; "
        "\"%s\" was previously declared as \"%s\"\n",
        c_sname_local_name( concept_sname ),
        c_sname_gibberish( &scope_sname ),
        c_tid_error( scope_type->btids )
      );
      c_sname_cleanup( &scope_sname );
      return false;
    }
  } // for

  return true;
}

/**
 * Checks a #K_CONSTRUCTOR or #K_DESTRUCTOR AST for errors.
 *
 * @param ast The #K_CONSTRUCTOR or #K_DESTRUCTOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_ctor_dtor( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_CONSTRUCTOR | K_DESTRUCTOR ) );

  if ( !OPT_LANG_IS( CONSTRUCTORS ) ) {
    print_error( &ast->loc,
      "%ss not supported%s\n",
      c_kind_name( ast->kind ),
      C_LANG_WHICH( CONSTRUCTORS )
    );
    return false;
  }

  bool const is_definition = c_sname_count( &ast->sname ) > 1;

  if ( is_definition && !c_sname_is_ctor( &ast->sname ) ) {
    print_error( &ast->loc,
      "\"%s\", \"%s\": %s and %s names don't match\n",
      c_sname_name_atr( &ast->sname, 1 ), c_sname_local_name( &ast->sname ),
      c_type_error( c_sname_local_type( &ast->sname ) ),
      c_kind_name( ast->kind )
    );
    return false;
  }

  bool const is_constructor = ast->kind == K_CONSTRUCTOR;

  c_tid_t const ok_stids = is_constructor ?
    (is_definition ? TS_CONSTRUCTOR_DEF : TS_CONSTRUCTOR_DECL) :
    (is_definition ? TS_DESTRUCTOR_DEF  : TS_DESTRUCTOR_DECL ) ;

  c_tid_t const not_ok_stids = ast->type.stids & c_tid_compl( ok_stids );
  if ( not_ok_stids != TS_NONE ) {
    print_error( &ast->loc,
      "%s%s can not be \"%s\"\n",
      c_kind_name( ast->kind ),
      is_definition ? " definitions" : "s",
      c_tid_error( not_ok_stids )
    );
    return false;
  }

  return true;
}

/**
 * Checks a #K_BUILTIN Embedded C type AST for errors.
 *
 * @param ast The #K_BUILTIN AST to check.
 * @return Returns `true` only if all checks passed.
 *
 * @sa [Information Technology — Programming languages - C - Extensions to support embedded processors](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1169.pdf)
 */
NODISCARD
static bool c_ast_check_emc( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );

  if ( c_tid_is_any( ast->type.btids, TB_EMC__Sat ) &&
      !c_tid_is_any( ast->type.btids, TB_ANY_EMC ) ) {
    print_error( &ast->loc,
      "\"%s\" requires either \"%s\" or \"%s\"\n",
      c_tid_error( TB_EMC__Sat ),
      c_tid_error( TB_EMC__Accum ),
      c_tid_error( TB_EMC__Fract )
    );
    return false;
  }

  return true;
}

/**
 * Checks a #K_ENUM AST for errors.
 *
 * @param ast The #K_ENUM AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_enum( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_ENUM );

  if ( is_gibberish_to_english() &&
       c_tid_is_any( ast->type.btids, TB_struct | TB_class ) &&
      !c_tid_is_any( ast->type.stids, TS_typedef ) ) {
    print_error( &ast->loc,
      "\"%s\": enum classes must just use \"enum\"\n",
      c_type_error( &ast->type )
    );
    return false;
  }

  if ( ast->enum_.bit_width > 0 && !OPT_LANG_IS( enum_BITFIELDS ) ) {
    print_error( &ast->loc,
      "%s bit-fields not supported%s\n",
      c_tid_error( TB_enum ),
      C_LANG_WHICH( enum_BITFIELDS )
    );
    return false;
  }

  c_ast_t const *const of_ast = ast->enum_.of_ast;
  if ( of_ast != NULL ) {
    if ( !OPT_LANG_IS( FIXED_TYPE_enum ) ) {
      print_error( &of_ast->loc,
        "%s with underlying type not supported%s\n",
        c_tid_error( TB_enum ),
        C_LANG_WHICH( FIXED_TYPE_enum )
      );
      return false;
    }

    if ( !c_ast_is_builtin_any( of_ast, TB_ANY_INTEGRAL ) ) {
      print_error( &of_ast->loc,
        "invalid %s underlying type ",
        c_tid_error( TB_enum )
      );
      print_ast_type_aka( of_ast, stderr );
      EPUTS( "; must be integral\n" );
      return false;
    }
  }

  return true;
}

/**
 * Checks an entire AST for semantic errors.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_errors( c_ast_t const *ast ) {
  assert( ast != NULL );
  // check in major-to-minor error order
  return  c_ast_check_visitor( ast, &c_ast_visitor_error ) &&
          c_ast_check_visitor( ast, &c_ast_visitor_type );
}

/**
 * Checks a function-like AST for errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );

  if ( ast->kind == K_FUNCTION &&
       c_sname_cmp( &ast->sname, &C_SNAME_LIT( "main" ) ) == 0 &&
      ( //
        // Perform extra checks on a function named "main" if either:
        //
        //  + The current language is C; or:
        //
        //  + The current language is C++ and the function does not have any
        //    storage-like type that can't be used with the program's main().
        //    (Otherwise assume it's just a member function named "main".)
        //
        OPT_LANG_IS( C_ANY ) ||
        !c_tid_is_any( ast->type.stids, c_tid_compl( TS_MAIN_FUNC_CPP ) ) ) &&
      !c_ast_check_func_main( ast ) ) {
    return false;
  }

  if ( OPT_LANG_IS( C_ANY ) )
    return true;

  c_ast_t const *param_ast = c_param_ast( c_ast_params( ast ) );
  if ( param_ast != NULL && c_ast_is_tid_any( param_ast, TS_this ) &&
       c_ast_is_tid_any( ast, TS_FUNC_LIKE_NOT_EXPLICIT_OBJ_PARAM ) ) {
    print_error( &param_ast->loc,
      "%s with \"this\" parameter can not be \"%s\"\n",
      c_kind_name( ast->kind ),
      c_tid_error( ast->type.stids & TS_FUNC_LIKE_NOT_EXPLICIT_OBJ_PARAM )
    );
    return false;
  }

  if ( c_tid_is_any( ast->type.stids, TS_constinit ) ) {
    error_kind_not_tid( ast, TS_constinit, LANG_NONE, "\n" );
    return false;
  }

  if ( c_tid_is_any( ast->type.stids, TS_ANY_REFERENCE ) ) {
    if ( !OPT_LANG_IS( REF_QUALIFIED_FUNCS ) ) {
      print_error( &ast->loc,
        "reference qualified %ss not supported%s\n",
        c_kind_name( ast->kind ),
        C_LANG_WHICH( REF_QUALIFIED_FUNCS )
      );
      return false;
    }
    if ( c_tid_is_any( ast->type.stids, TS_ANY_LINKAGE ) ) {
      print_error( &ast->loc,
        "reference qualified %ss can not be \"%s\"\n",
        c_kind_name( ast->kind ),
        c_tid_error( ast->type.stids & TS_ANY_LINKAGE )
      );
      return false;
    }
  }

  c_tid_t const member_func_stids = ast->type.stids & TS_MEMBER_FUNC_ONLY;

  if ( member_func_stids != TS_NONE &&
       c_tid_is_any( ast->type.stids, TS_ANY_LINKAGE ) ) {
    print_error( &ast->loc,
      "\"%s\" %ss can not be \"%s\"\n",
      c_tid_error( ast->type.stids & TS_ANY_LINKAGE ),
      c_kind_name( ast->kind ),
      c_tid_error( member_func_stids )
    );
    return false;
  }

  switch ( ast->func.member ) {
    case C_FUNC_MEMBER:
      NO_OP;
      //
      // Member functions can't have linkage -- except the new & delete
      // operators may have static explicitly specified.
      //
      c_tid_t linkage_stids = TS_extern | TS_extern_C;
      if ( ast->kind == K_OPERATOR &&
           !c_op_is_new_delete( ast->oper.operator->op_id ) ) {
        linkage_stids |= TS_static;
      }
      if ( c_tid_is_any( ast->type.stids, linkage_stids ) ) {
        print_error( &ast->loc,
          "member %ss can not be \"%s\"\n",
          c_kind_name( ast->kind ),
          c_tid_error( ast->type.stids & linkage_stids )
        );
        return false;
      }
      break;

    case C_FUNC_NON_MEMBER:
      if ( member_func_stids != TS_NONE ) {
        print_error( &ast->loc,
          "non-member %ss can not be \"%s\"\n",
          c_kind_name( ast->kind ),
          c_tid_error( member_func_stids )
        );
        return false;
      }
      break;

    case C_FUNC_UNSPECIFIED:
      // nothing to do
      break;
  } // switch

  if ( !c_ast_check_func_default_delete( ast ) )
    return false;

  c_tid_t const not_func_atids = ast->type.atids & c_tid_compl( TA_FUNC );
  if ( not_func_atids != TA_NONE ) {
    error_kind_not_tid( ast, not_func_atids, LANG_NONE, "\n" );
    return false;
  }

  if ( c_tid_is_any( ast->type.stids, TS_virtual ) ) {
    if ( c_sname_count( &ast->sname ) > 1 ) {
      print_error( &ast->loc,
        "\"%s\": \"%s\" can not be used in file-scoped %ss\n",
        c_sname_gibberish( &ast->sname ),
        c_tid_error( TS_virtual ),
        c_kind_name( ast->kind )
      );
      return false;
    }
  }
  else if ( c_tid_is_any( ast->type.stids, TS_PURE_virtual ) ) {
    print_error( &ast->loc,
      "non-virtual %s can not be pure\n",
      c_kind_name( ast->kind )
    );
    return false;
  }

  return true;
}

/**
 * Checks a function-like AST that is marked with either #TS_default or
 * #TS_delete.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func_default_delete( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );

  if ( !c_tid_is_any( ast->type.stids, TS_default | TS_delete ) )
    return true;

  c_ast_t const *param_ast = c_param_ast( c_ast_params( ast ) );

  switch ( ast->kind ) {
    case K_CONSTRUCTOR:
      switch ( slist_len( &ast->ctor.param_ast_list ) ) {
        case 0:                         // C()
          break;
        case 1:                         // C(C const&)
          if ( c_ast_is_ref_to_class_sname( param_ast, &ast->sname ) )
            break;
          FALLTHROUGH;
        default:
          //
          // This isn't correct since copy constructors can have more than one
          // parameter if the additional ones all have default arguments; but
          // cdecl doesn't support default arguments.
          //
          goto only_special;
      } // switch
      break;

    case K_FUNCTION:
    case K_UDEF_CONV:
      if ( c_tid_is_any( ast->type.stids, TS_default ) )
        goto only_special;
      break;

    case K_OPERATOR:
      switch ( ast->oper.operator->op_id ) {
        case C_OP_EQUAL:                // C& operator=(C const&)
          NO_OP;
          //
          // For C& operator=(C const&), the parameter and the return type must
          // both be a reference to the same class, struct, or union.
          //
          c_ast_t const *const ret_ast =
            c_ast_is_ref_to_tid_any( ast->oper.ret_ast, TB_ANY_CLASS );
          if ( ret_ast == NULL ||
                slist_len( &ast->oper.param_ast_list ) != 1 ) {
            goto only_special;
          }
          param_ast = c_ast_is_ref_to_tid_any( param_ast, TB_ANY_CLASS );
          if ( !c_ast_equal( param_ast, ret_ast ) )
            goto only_special;
          break;

        case C_OP_EQUAL_EQUAL:
        case C_OP_EXCLAMATION_EQUAL:
        case C_OP_GREATER:
        case C_OP_GREATER_EQUAL:
        case C_OP_LESS:
        case C_OP_LESS_EQUAL:
        case C_OP_LESS_EQUAL_GREATER:
          if ( c_tid_is_any( ast->type.stids, TS_delete ) )
            goto only_special;
          //
          // Detailed checks for defaulted overloaded relational operators are
          // done in c_ast_check_op_relational_default().
          //
          break;

        default:
          goto only_special;
      } // switch
      break;

    case K_APPLE_BLOCK:
    case K_ARRAY:
    case K_BUILTIN:
    case K_CAPTURE:
    case K_CAST:
    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_DESTRUCTOR:
    case K_ENUM:
    case K_LAMBDA:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_STRUCTURED_BINDING:
    case K_TYPEDEF:
    case K_UDEF_LIT:
    case K_VARIADIC:
      //
      // The grammar allows only functions, operators, constructors,
      // destructors, and user-defined conversion operators to be either
      // `default` or `delete`.  This function isn't called for destructors and
      // the others have cases above.
      //
      UNEXPECTED_INT_VALUE( ast->kind );
  } // switch

  return true;

only_special:
  print_error( &ast->loc,
    "\"%s\" can be used only for special member functions%s\n",
    c_type_error( &ast->type ),
    OPT_LANG_IS( default_RELOPS ) &&
    c_tid_is_any( ast->type.stids, TS_default ) ?
      " and relational operators" : ""
  );
  return false;
}

/**
 * Checks the return type and parameters for main().
 *
 * @param ast The main function AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func_main( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_FUNCTION );

  if ( OPT_LANG_IS( C_ANY ) &&
       c_tid_is_any( ast->type.stids, c_tid_compl( TS_MAIN_FUNC_C ) ) ) {
    print_error( &ast->loc,
      "main() can not be \"%s\" in C\n",
      c_tid_error( ast->type.stids )
    );
    return false;
  }

  c_ast_t const *const ret_ast = ast->func.ret_ast;
  if ( !c_ast_is_builtin_any( ret_ast, TB_int ) ) {
    print_error( &ret_ast->loc, "invalid main() return type " );
    print_ast_type_aka( ret_ast, stderr );
    EPRINTF( "; must be \"%s\" or a typedef thereof\n", c_tid_error( TB_int ) );
    return false;
  }

  size_t const n_params = slist_len( &ast->func.param_ast_list );
  c_param_t const *param = c_ast_params( ast );
  c_ast_t const *param_ast;

  switch ( n_params ) {
    case 0:                             // main()
      break;

    case 1:                             // main(void)
      param_ast = c_param_ast( param );
      if ( !OPT_LANG_IS( PROTOTYPES ) ) {
        print_error( &param_ast->loc,
          "main() must have 0, 2, or 3 parameters in %s\n",
          c_lang_name( LANG_C_KNR )
        );
        return false;
      }
      if ( !c_ast_is_builtin_any( param_ast, TB_void ) ) {
        print_error( &param_ast->loc,
          "a single parameter for main() must be \"%s\"\n",
          c_tid_error( TB_void )
        );
        return false;
      }
      break;

    case 2:                             // main(int, char *argv[])
    case 3:                             // main(int, char *argv[], char *envp[])
      if ( !OPT_LANG_IS( PROTOTYPES ) )
        break;

      param_ast = c_param_ast( param );
      if ( !c_ast_is_builtin_any( param_ast, TB_int ) ) {
        print_error( &param_ast->loc,
          "invalid main() first parameter type "
        );
        print_ast_type_aka( param_ast, stderr );
        EPRINTF(
          "; must be \"%s\" or a typedef thereof\n",
          c_tid_error( TB_int )
        );
        return false;
      }

      param = param->next;
      param_ast = c_param_ast( param );
      if ( !c_ast_check_func_main_char_ptr_param( param_ast ) )
        return false;

      if ( n_params == 3 ) {          // char *envp[]
        param = param->next;
        param_ast = c_param_ast( param );
        if ( !c_ast_check_func_main_char_ptr_param( param_ast ) )
          return false;
      }
      break;

    default:
      print_error( c_ast_params_loc( ast ),
        "main() must have %s parameters\n",
        OPT_LANG_IS( PROTOTYPES ) ? "0-3" : "0, 2, or 3"
      );
      return false;
  } // switch

  return true;
}

/**
 * Checks that an AST of a main() parameter is either `char*[]` or `char**`
 * optionally including `const`.
 *
 * @param param_ast The parameter AST to check.
 * @return Returns `true` only if \a parameter_ast is of either type.
 */
NODISCARD
static bool c_ast_check_func_main_char_ptr_param( c_ast_t const *param_ast ) {
  c_ast_t const *const raw_param_ast = c_ast_untypedef( param_ast );
  switch ( raw_param_ast->kind ) {
    case K_ARRAY:                       // char *argv[]
    case K_POINTER:                     // char **argv
      if ( !c_ast_is_ptr_to_type_any( param_ast->parent.of_ast,
              &C_TYPE_LIT( TB_ANY, c_tid_compl( TS_const ), TA_ANY ),
              &C_TYPE_LIT_B( TB_char ) ) ) {
        print_error( &param_ast->loc, "invalid main() parameter type " );
        print_ast_type_aka( param_ast, stderr );
        EPUTS( "; must be " );
        if ( is_english_to_gibberish() ) {
          EPRINTF( "\"%s %s pointer to %s\"\n",
            c_kind_name( param_ast->kind ),
            param_ast->kind == K_ARRAY ? "of" : "to",
            c_tid_error( TB_char )
          );
        }
        else {
          EPRINTF( "\"char*%s\"\n",
            param_ast->kind == K_ARRAY ? other_token_c( "[]" ) : "*"
          );
        }
        return false;
      }
      break;
    default:                            // ???
      print_error( &param_ast->loc, "invalid main() parameter type " );
      print_ast_type_aka( param_ast, stderr );
      EPUTS( "; must be " );
      if ( is_english_to_gibberish() )
        EPRINTF( "\"array of pointer to %s\"\n", c_tid_error( TB_char ) );
      else
        EPRINTF( "\"char*%s\"\n", other_token_c( "[]" ) );
      return false;
  } // switch
  return true;
}

/**
 * Checks all function-like parameters for semantic errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func_params( c_ast_t const *ast ) {
  if ( !OPT_LANG_IS( PROTOTYPES ) )
    return c_ast_check_func_params_knr( ast );

  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );

  c_ast_t const *void_ast = NULL;
  unsigned n_params = 0;

  FOREACH_AST_FUNC_PARAM( param, ast ) {
    c_ast_t const *const param_ast = c_param_ast( param );

    if ( ++n_params > 1 ) {
      if ( c_ast_is_tid_any( param_ast, TS_this ) ) {
        print_error( &param_ast->loc,
          "\"this\" can be only first parameter\n"
        );
        return false;
      }
      if ( void_ast != NULL )
        goto only_void;                 // R f(void, T)
    }

    if ( c_sname_count( &param_ast->sname ) > 1 ) {
      print_error( &param_ast->loc, "parameter names can not be scoped\n" );
      return false;
    }

    c_ast_t const *const raw_param_ast = c_ast_untypedef( param_ast );

    if ( raw_param_ast->kind != K_ARRAY ) {
      c_tid_t const param_stids =
        TS_ANY_STORAGE & param_ast->type.stids &
        c_tid_compl( TS_FUNC_LIKE_PARAM );
      if ( param_stids != TS_NONE ) {
        print_error( &param_ast->loc,
          "%s parameters can not be \"%s\"\n",
          c_kind_name( ast->kind ),
          c_tid_error( param_stids )
        );
        return false;
      }
    }

    switch ( raw_param_ast->kind ) {
      case K_BUILTIN:
        if ( c_tid_is_any( raw_param_ast->type.btids, TB_auto ) &&
             !OPT_LANG_IS( auto_PARAMS ) ) {
          print_error( &param_ast->loc,
            "\"%s\" parameters not supported%s\n",
            c_tid_error( TB_auto ),
            C_LANG_WHICH( auto_PARAMS )
          );
          return false;
        }
        if ( c_ast_is_builtin_any( raw_param_ast, TB_void ) ) {
          //
          // Ordinarily, void parameters are invalid; but a single void
          // function "parameter" is valid (as long as it has neither a name
          // nor qualifiers).
          //
          if ( !c_sname_empty( &param_ast->sname ) ) {
            print_error( &param_ast->loc,
              "\"%s\" parameters can not have a name\n",
              c_tid_error( TB_void )
            );
            return false;
          }
          c_tid_t qual_stids;
          if ( c_ast_is_tid_any_qual( param_ast, TS_CV, &qual_stids ) ) {
            print_error( &param_ast->loc,
              "\"%s\" parameters can not be \"%s\"\n",
              c_tid_error( TB_void ),
              c_tid_error( qual_stids )
            );
            return false;
          }

          assert( void_ast == NULL );
          void_ast = param_ast;
          if ( n_params > 1 )
            goto only_void;             // R f(T, void)
          continue;
        }
        if ( param_ast->builtin.bit_width > 0 ) {
          print_error( &param_ast->loc,
            "invalid parameter: can not have bit-field width\n"
          );
          return false;
        }
        break;

      case K_NAME:
        if ( !OPT_LANG_IS( KNR_FUNC_DEFS ) && c_ast_is_untyped( param_ast ) ) {
          //
          // C23 finally forbids old-style K&R function definitions:
          //
          //      strlen(s)
          //        char *s             // illegal in C23
          //
          print_error( &param_ast->loc,
            "type specifier required%s\n",
            C_LANG_WHICH( KNR_FUNC_DEFS )
          );
          return false;
        }
        break;

      case K_VARIADIC:
        if ( ast->kind == K_OPERATOR &&
             ast->oper.operator->op_id != C_OP_PARENTHESES ) {
          print_error( &param_ast->loc,
            "operator \"%s\" can not have \"...\" parameter\n",
            ast->oper.operator->literal
          );
          return false;
        }
        if ( param->next != NULL ) {
          print_error( &param_ast->loc, "\"...\" must be last\n" );
          return false;
        }
        if ( !OPT_LANG_IS( VARIADIC_ONLY_PARAMS ) && n_params == 1 ) {
          print_error( &param_ast->loc,
            "\"...\" as only parameter not supported%s\n",
            C_LANG_WHICH( VARIADIC_ONLY_PARAMS )
          );
          return false;
        }
        continue;

      case K_ARRAY:
      case K_CLASS_STRUCT_UNION:
      case K_CONCEPT:
      case K_ENUM:
      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        // nothing to do
        break;

      case K_APPLE_BLOCK:
      case K_CAPTURE:
      case K_CAST:
      case K_CONSTRUCTOR:
      case K_DESTRUCTOR:
      case K_FUNCTION:
      case K_LAMBDA:
      case K_OPERATOR:
      case K_PLACEHOLDER:
      case K_STRUCTURED_BINDING:
      case K_TYPEDEF:                   // impossible after c_ast_untypedef()
      case K_UDEF_CONV:
      case K_UDEF_LIT:
        UNEXPECTED_INT_VALUE( raw_param_ast->kind );
    } // switch

    if ( !c_ast_check_errors( param_ast ) )
      return false;
  } // for

  return c_ast_check_func_params_redef( ast );

only_void:
  print_error( &void_ast->loc,
    "\"%s\" must be only parameter if specified\n",
    c_tid_error( TB_void )
  );
  return false;
}

/**
 * Checks all function parameters for semantic errors in #LANG_C_KNR.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func_params_knr( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_APPLE_BLOCK | K_FUNCTION ) );
  assert( !OPT_LANG_IS( PROTOTYPES ) );

  FOREACH_AST_FUNC_PARAM( param, ast ) {
    c_ast_t const *const param_ast = c_param_ast( param );
    if ( param_ast->kind != K_NAME ) {
      print_error( &param_ast->loc,
        "function prototypes not supported%s\n",
        C_LANG_WHICH( PROTOTYPES )
      );
      return false;
    }
  } // for

  return c_ast_check_func_params_redef( ast );
}

/**
 * Checks function-like parameters for redefinition (duplicate names).
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func_params_redef( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );

  FOREACH_AST_FUNC_PARAM( param, ast ) {
    c_ast_t const *const param_ast = c_param_ast( param );
    if ( c_sname_empty( &param_ast->sname ) )
      continue;
    FOREACH_SLIST_NODE_UNTIL( prev_param, &ast->func.param_ast_list, param ) {
      c_ast_t const *const prev_param_ast = c_param_ast( prev_param );
      if ( c_sname_empty( &prev_param_ast->sname ) )
        continue;
      if ( c_sname_cmp( &param_ast->sname, &prev_param_ast->sname ) == 0 ) {
        print_error( &param_ast->loc,
          "\"%s\": redefinition of parameter\n",
          c_sname_gibberish( &param_ast->sname )
        );
        return false;
      }
    } // for
  } // for

  return true;
}

/**
 * Checks the return type of a function-like AST for errors.
 *
 * @param ast The function-like AST to check the return type of.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_func_ret_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );

  c_ast_t const *const ret_ast = ast->func.ret_ast;
  if ( ret_ast == NULL )
    return true;

  char const *const kind_name = c_kind_name( ast->kind );
  c_ast_t const *const raw_ret_ast = c_ast_untypedef( ret_ast );

  switch ( raw_ret_ast->kind ) {
    case K_ARRAY:
      print_error( &ret_ast->loc, "%s returning array", kind_name );
      print_hint( "%s returning pointer", kind_name );
      return false;

    case K_BUILTIN:
      if ( c_tid_is_any( raw_ret_ast->type.btids, TB_auto ) &&
           !OPT_LANG_IS( auto_RETURN_TYPES ) ) {
        print_error( &ret_ast->loc,
          "%s returning \"%s\" not supported%s\n",
          kind_name,
          c_tid_error( TB_auto ),
          C_LANG_WHICH( auto_RETURN_TYPES )
        );
        return false;
      }
      break;

    case K_CLASS_STRUCT_UNION:
      if ( !OPT_LANG_IS( CSU_RETURN_TYPES ) ) {
        print_error( &ret_ast->loc, "%s returning ", kind_name );
        print_ast_kind_aka( ret_ast, stderr );
        EPRINTF( " not supported%s\n", C_LANG_WHICH( CSU_RETURN_TYPES ) );
        return false;
      }
      break;

    case K_FUNCTION:
      print_error( &ret_ast->loc, "%s returning ", kind_name );
      print_ast_kind_aka( ret_ast, stderr );
      EPUTS( " is illegal" );
      print_hint( "%s returning pointer to function", kind_name );
      return false;

    case K_STRUCTURED_BINDING:
      print_error( &ret_ast->loc,
        "%s returning %s is illegal\n",
        kind_name, c_kind_name( ret_ast->kind )
      );
      return false;

    case K_APPLE_BLOCK:
    case K_ENUM:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
      // nothing to check
      break;

    case K_CAPTURE:
    case K_CAST:
    case K_CONCEPT:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_LAMBDA:
    case K_NAME:
    case K_OPERATOR:
    case K_PLACEHOLDER:
    case K_UDEF_CONV:
    case K_UDEF_LIT:
    case K_VARIADIC:
      UNEXPECTED_INT_VALUE( raw_ret_ast->kind );
  } // switch

  if ( c_tid_is_any( ast->type.stids, TS_explicit ) ) {
    c_lang_id_t which_lang_ids = LANG_NONE;
    switch ( ast->kind ) {
      case K_UDEF_CONV:
        if ( OPT_LANG_IS( explicit_USER_DEF_CONVS ) )
          break;
        which_lang_ids = LANG_explicit_USER_DEF_CONVS;
        FALLTHROUGH;
      default:
        error_kind_not_tid( ast, TS_explicit, which_lang_ids, "\n" );
        return false;
    } // switch
  }

  return true;
}

/**
 * Checks a #K_LAMBDA AST for semantic errors.
 *
 * @param ast The #K_LAMBDA AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_lambda( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );

  if ( !OPT_LANG_IS( LAMBDAS ) ) {
    print_error( &ast->loc,
      "lambdas not supported%s\n",
      C_LANG_WHICH( LAMBDAS )
    );
    return false;
  }

  c_tid_t const stids = ast->type.stids & c_tid_compl( TS_LAMBDA );
  if ( stids != TS_NONE ) {
    print_error( &ast->loc,
      "%s can not be \"%s\"\n",
      c_kind_name( ast->kind ),
      c_tid_error( stids )
    );
    return false;
  }

  return c_ast_check_lambda_captures( ast ) &&
         c_ast_check_lambda_captures_redef( ast );
}

/**
 * Checks #K_LAMBDA captures for semantic errors.
 *
 * @param ast The #K_LAMBDA AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_lambda_captures( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );

  c_ast_t const *default_capture_ast = NULL;
  unsigned n_captures = 0;

  FOREACH_AST_LAMBDA_CAPTURE( capture, ast ) {
    c_ast_t const *const capture_ast = c_capture_ast( capture );
    assert( capture_ast->kind == K_CAPTURE );
    ++n_captures;

    switch ( capture_ast->capture.kind ) {
      case C_CAPTURE_COPY:
        assert( c_sname_empty( &capture_ast->sname ) );
set_default_capture:
        if ( default_capture_ast != NULL ) {
          print_error( &capture_ast->loc,
            "default capture previously specified\n"
          );
          return false;
        }
        if ( n_captures > 1 ) {
          print_error( &capture_ast->loc,
            "default capture must be specified first\n"
          );
          return false;
        }
        assert( default_capture_ast == NULL );
        default_capture_ast = capture_ast;
        break;

      case C_CAPTURE_REFERENCE:
        if ( c_sname_empty( &capture_ast->sname ) )
          goto set_default_capture;
        if ( default_capture_ast != NULL &&
             default_capture_ast->capture.kind == C_CAPTURE_REFERENCE ) {
          print_error( &capture_ast->loc,
            "default capture is already by reference\n"
          );
          return false;
        }
        break;

      case C_CAPTURE_STAR_THIS:
        if ( !OPT_LANG_IS( CAPTURE_STAR_THIS ) ) {
          print_error( &capture_ast->loc,
            "capturing \"*this\" not supported%s\n",
            C_LANG_WHICH( CAPTURE_STAR_THIS )
          );
          return false;
        }
        FALLTHROUGH;
      case C_CAPTURE_THIS:
        assert( c_sname_empty( &capture_ast->sname ) );
        break;

      case C_CAPTURE_VARIABLE:
        assert( !c_sname_empty( &capture_ast->sname ) );
        break;
    } // switch
  } // for

  return true;
}

/**
 * Checks #K_LAMBDA captures for redefinition (duplicate names, `this`, or
 * `*this`).
 *
 * @param ast The #K_LAMBDA AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_lambda_captures_redef( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );

  FOREACH_AST_LAMBDA_CAPTURE( capture, ast ) {
    c_ast_t const *const capture_ast = c_capture_ast( capture );
    assert( capture_ast->kind == K_CAPTURE );
    FOREACH_AST_LAMBDA_CAPTURE_UNTIL( prev_capture, ast, capture ) {
      c_ast_t const *const prev_capture_ast = c_capture_ast( prev_capture );
      if ( c_ast_is_capture_this( capture_ast ) &&
           c_ast_is_capture_this( prev_capture_ast ) ) {
        print_error( &capture_ast->loc, "\"this\" previously captured\n" );
        return false;
      }
      if ( c_sname_empty( &prev_capture_ast->sname ) )
        continue;
      if ( c_sname_cmp( &capture_ast->sname, &prev_capture_ast->sname ) == 0 ) {
        print_error( &capture_ast->loc,
          "\"%s\" previously captured\n",
          c_sname_gibberish( &capture_ast->sname )
        );
        return false;
      }
    } // for
  } // for

  return true;
}

/**
 * Checks an AST's name(s) for errors.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if all checks passed.
 *
 * @sa c_ast_warn_name()
 */
static bool c_ast_check_name( c_ast_t const *ast ) {
  assert( ast != NULL );

  if ( !c_sname_check( &ast->sname, &ast->loc ) )
    return false;

  if ( ast->align.kind == C_ALIGNAS_SNAME &&
       !c_sname_check( &ast->align.sname, &ast->align.loc ) ) {
    return false;
  }

  if ( (ast->kind & K_ANY_NAME) != 0 &&
       !c_sname_check( &ast->name.sname, &ast->loc ) ) {
    return false;
  }

  return true;
}

/**
 * Checks a #K_OPERATOR AST for errors.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  c_operator_t const *const op = ast->oper.operator;

  if ( op->overload == C_OVERLOAD_NONE ) {
    print_error( &ast->loc,
      "operator \"%s\" can not be overloaded\n", op->literal
    );
    return false;
  }

  if ( !opt_lang_is_any( op->lang_ids ) ) {
    print_error( &ast->loc,
      "overloading operator \"%s\" not supported%s\n",
      op->literal, c_lang_which( op->lang_ids )
    );
    return false;
  }

  if ( !c_ast_op_mbr_matches( ast, op ) ) {
    //
    // The user explicitly specified either member or non-member, but the
    // operator can't be that.
    //
    print_error( &ast->loc,
      "operator \"%s\" can only be a %s\n",
      op->literal, op->overload == C_OVERLOAD_MEMBER ? L_member : H_non_member
    );
    return false;
  }

  if ( op->overload == C_OVERLOAD_MEMBER &&
       c_tid_is_any( ast->type.stids, TS_static ) ) {
    c_lang_id_t ok_lang_ids = LANG_NONE;
    switch ( op->op_id ) {
      case C_OP_PARENTHESES:
        if ( OPT_LANG_IS( static_OP_PARENS ) )
          break;
        ok_lang_ids = LANG_static_OP_PARENS;
        FALLTHROUGH;
      default:
        print_error( &ast->loc,
          "operator \"%s\" can not be \"%s\"%s\n",
          op->literal,
          c_tid_error( TS_static ),
          c_lang_which( ok_lang_ids )
        );
        return false;
    } // switch
  }

  if ( c_op_is_new_delete( op->op_id ) &&
       c_tid_is_any( ast->type.stids, c_tid_compl( TS_NEW_DELETE_OP ) ) ) {
    //
    // Special case for operators new, new[], delete, and delete[] that can
    // only have specific types.
    //
    print_error( &ast->loc,
      "operator \"%s\" can not be \"%s\"\n",
      op->literal, c_type_error( &ast->type )
    );
    return false;
  }

  c_ast_t const *const ret_ast = ast->oper.ret_ast;

  switch ( op->op_id ) {
    case C_OP_MINUS_GREATER:
      //
      // Special case for operator-> that must return a pointer to a struct,
      // union, or class.
      //
      if ( !c_ast_is_ptr_to_kind_any( ret_ast, K_CLASS_STRUCT_UNION ) ) {
        print_error( &ret_ast->loc,
          "invalid operator \"%s\" return type ",
          op->literal
        );
        print_ast_type_aka( ret_ast, stderr );
        EPUTS( "; must be a pointer to struct, union, or class\n" );
        return false;
      }
      break;

    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for operators delete and delete[] that must return void.
      //
      if ( !c_ast_is_builtin_any( ret_ast, TB_void ) ) {
        print_error( &ret_ast->loc,
          "invalid operator \"%s\" return type ",
          op->literal
        );
        print_ast_type_aka( ret_ast, stderr );
        EPRINTF( "; must be \"%s\"\n", c_tid_error( TB_void ) );
        return false;
      }
      break;

    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
      //
      // Special case for operators new and new[] that must return pointer to
      // void.
      //
      if ( !c_ast_is_ptr_to_tid_any( ret_ast, TB_void ) ) {
        print_error( &ret_ast->loc,
          "invalid operator \"%s\" return type ",
          op->literal
        );
        print_ast_type_aka( ret_ast, stderr );
        EPUTS( "; must be " );
        if ( is_english_to_gibberish() )
          EPUTS( "\"pointer to void\"\n" );
        else
          EPUTS( "\"void*\"\n" );
        return false;
      }
      break;

    default:
      /* suppress warning */;
  } // switch

  if ( c_tid_is_any( ast->type.stids, TS_default ) &&
       !c_ast_check_oper_default( ast ) ) {
    return false;
  }

  return c_ast_check_oper_params( ast );
}

/**
 * Checks #K_OPERATOR `delete` and `delete[]` AST parameters for semantic
 * errors.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_op_delete_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( ast->oper.operator->op_id == C_OP_DELETE ||
          ast->oper.operator->op_id == C_OP_DELETE_ARRAY );

  // minimum number of parameters checked in c_ast_check_oper_params()

  c_param_t const *const param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *const param_ast = c_param_ast( param );

  if ( !c_ast_is_ptr_to_tid_any( param_ast, TB_void | TB_ANY_CLASS ) ) {
    print_error( &param_ast->loc,
      "invalid operator \"%s\" parameter type ",
      ast->oper.operator->literal
    );
    print_ast_type_aka( param_ast, stderr );
    EPUTS( "; must be a pointer to void, class, struct, or union\n" );
    return false;
  }

  return true;
}

/**
 * Checks the return type of a #K_OPERATOR `<=>` AST for semantic errors.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_op_less_equal_greater_ret_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  c_operator_t const *const op = ast->oper.operator;
  assert( op->op_id == C_OP_LESS_EQUAL_GREATER );

  static c_ast_t const *std_partial_ordering_ast;
  static c_ast_t const *std_strong_ordering_ast;
  static c_ast_t const *std_weak_ordering_ast;

  if ( std_partial_ordering_ast == NULL ) {
    std_partial_ordering_ast =
      c_typedef_find_name( "std::partial_ordering" )->ast;
    std_strong_ordering_ast =
      c_typedef_find_name( "std::strong_ordering" )->ast;
    std_weak_ordering_ast =
      c_typedef_find_name( "std::weak_ordering" )->ast;
  }

  c_ast_t const *const ret_ast = ast->oper.ret_ast;
  c_ast_t const *const raw_ret_ast = c_ast_untypedef( ret_ast );

  if ( c_ast_is_builtin_any( ret_ast, TB_auto ) ||
       c_ast_equal( raw_ret_ast, std_partial_ordering_ast ) ||
       c_ast_equal( raw_ret_ast, std_strong_ordering_ast ) ||
       c_ast_equal( raw_ret_ast, std_weak_ordering_ast ) ) {
    return true;
  }

  print_error( &ret_ast->loc,
    "invalid operator \"%s\" return type ", op->literal
  );
  print_ast_type_aka( ret_ast, stderr );
  EPRINTF(
    "; must be "
    "\"%s\", "
    "\"std::partial_ordering\", "
    "\"std::strong_ordering\", "
    "or "
    "\"std::weak_ordering\"\n",
    c_tid_error( TB_auto )
  );

  return false;
}

/**
 * Checks #K_OPERATOR `--` and `++` AST parameters for semantic errors.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_op_minus_minus_plus_plus_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( ast->oper.operator->op_id == C_OP_MINUS_MINUS ||
          ast->oper.operator->op_id == C_OP_PLUS_PLUS );

  //
  // Ensure the dummy parameter for postfix -- or ++ is type int (or a typedef
  // for int).
  //
  c_param_t const *param = c_ast_params( ast );
  if ( param == NULL )                  // member prefix
    return true;
  c_func_member_t const member = c_ast_op_overload( ast );
  if ( member == C_FUNC_NON_MEMBER ) {
    param = param->next;
    if ( param == NULL )                // non-member prefix
      return true;
  }

  c_operator_t const *const op = ast->oper.operator;

  //
  // At this point, it's either member or non-member postfix: operator++(int)
  // or operator++(S&,int).
  //
  c_ast_t const *const param_ast = c_param_ast( param );
  if ( !c_ast_is_builtin_any( param_ast, TB_int ) ) {
    print_error( &param_ast->loc,
      "invalid postfix %soperator \"%s\" parameter type ",
      c_ast_member_or_nonmember_str( ast ), op->literal
    );
    print_ast_type_aka( param_ast, stderr );
    EPRINTF(
      "; must be \"%s\" or a typedef thereof\n",
      c_tid_error( TB_int )
    );
    return false;
  }

  return true;
}

/**
 * Checks #K_OPERATOR `new` and `new[]` AST parameters for semantic errors.
 *
 * @param ast The #K_OPERATOR `new` AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_op_new_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( ast->oper.operator->op_id == C_OP_NEW ||
          ast->oper.operator->op_id == C_OP_NEW_ARRAY );

  // minimum number of parameters checked in c_ast_check_oper_params()

  c_param_t const *const param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *const param_ast = c_param_ast( param );

  if ( !c_ast_is_size_t( param_ast ) ) {
    print_error( &param_ast->loc,
      "invalid operator \"%s\" parameter type ",
      ast->oper.operator->literal
    );
    print_ast_type_aka( param_ast, stderr );
    EPUTS( "; must be \"std::size_t\" (or equivalent)\n" );
    return false;
  }

  return true;
}

/**
 * Checks a relational #K_OPERATOR AST that is marked `= default`.
 *
 * @param ast The defaulted relational #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_op_relational_default( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( c_tid_is_any( ast->type.stids, TS_default ) );

  // number of parameters checked in c_ast_check_oper_params()

  c_operator_t const *const op = ast->oper.operator;

  if ( !OPT_LANG_IS( default_RELOPS ) ) {
    print_error( &ast->loc,
      "default operator \"%s\" not supported%s\n",
      op->literal, C_LANG_WHICH( default_RELOPS )
    );
    return false;
  }

  c_param_t const *const param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *const param_ast = c_param_ast( param );

  switch ( c_ast_op_overload( ast ) ) {
    case C_FUNC_NON_MEMBER: {
      if ( !c_tid_is_any( ast->type.stids, TS_friend ) ) {
        print_error( &ast->loc,
          "default non-member operator \"%s\" must also be \"%s\"\n",
          op->literal,
          c_tid_error( TS_friend )
        );
        return false;
      }

      //
      // Default non-member relational operators must take two of the same
      // class by either value or reference-to-const.
      //
      bool param1_is_ref_to_class = false;
      c_ast_t const *param1_ast = c_ast_is_tid_any( param_ast, TB_ANY_CLASS );
      if ( param1_ast == NULL ) {
        param1_ast = c_ast_is_ref_to_type_any( param_ast, &T_ANY_const_CLASS );
        if ( param1_ast == NULL )
          goto must_take_2_params_same_class;
        param1_is_ref_to_class = true;
      }

      c_ast_t const *param2_ast = c_param_ast( param->next );
      param2_ast = param1_is_ref_to_class ?
        c_ast_is_ref_to_type_any( param2_ast, &T_ANY_const_CLASS ) :
        c_ast_is_tid_any( param2_ast, TB_ANY_CLASS );
      if ( param2_ast == NULL || !c_ast_equal( param1_ast, param2_ast ) )
        goto must_take_2_params_same_class;
      break;
    }

    case C_FUNC_MEMBER: {
      if ( !c_tid_is_any( ast->type.stids, TS_const ) ) {
        print_error( &ast->loc,
          "default member operator \"%s\" must also be \"%s\"\n",
          op->literal,
          c_tid_error( TS_const )
        );
        return false;
      }

      //
      // Default member relational operators must take one class parameter by
      // either value or reference-to-const.
      //
      c_ast_t const *param1_ast = c_ast_is_tid_any( param_ast, TB_ANY_CLASS );
      if ( param1_ast == NULL ) {
        param1_ast = c_ast_is_ref_to_type_any( param_ast, &T_ANY_const_CLASS );
        if ( param1_ast == NULL ) {
          print_error( c_ast_params_loc( ast ),
            "default member relational operators must take one "
            "value or reference-to-const parameter to a class "
            "or a typedef thereof\n"
          );
          return false;
        }
      }
      break;
    }

    case C_FUNC_UNSPECIFIED:
      // nothing to do
      break;
  } // switch

  if ( op->op_id == C_OP_LESS_EQUAL_GREATER ) {
    if ( !c_ast_check_op_less_equal_greater_ret_type( ast ) )
      return false;
  }
  else {
    c_ast_t const *const ret_ast = ast->oper.ret_ast;
    if ( !c_ast_is_builtin_any( ret_ast, TB_bool ) ) {
      print_error( &ret_ast->loc,
        "invalid operator \"%s\" return type ",
        op->literal
      );
      print_ast_type_aka( ret_ast, stderr );
      EPRINTF(
        "; must be \"%s\" or a typedef thereof\n",
        c_tid_error( TB_bool )
      );
      return false;
    }
  }

  return true;

must_take_2_params_same_class:
  print_error( c_ast_params_loc( ast ),
    "default non-member relational operators must take two "
    "value or reference-to-const parameters of the same class "
    "or a typedef thereof\n"
  );
  return false;
}

/**
 * Checks a #K_OPERATOR AST that is marked `= default`.
 *
 * @param ast The defaulted #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_default( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( c_tid_is_any( ast->type.stids, TS_default ) );

  switch ( ast->oper.operator->op_id ) {
    case C_OP_EQUAL:
      //
      // Detailed checks for defaulted assignment operators are done in
      // c_ast_check_func().
      //
      break;

    case C_OP_EQUAL_EQUAL:
    case C_OP_EXCLAMATION_EQUAL:
    case C_OP_GREATER:
    case C_OP_GREATER_EQUAL:
    case C_OP_LESS:
    case C_OP_LESS_EQUAL:
    case C_OP_LESS_EQUAL_GREATER:
      return c_ast_check_op_relational_default( ast );

    default:
      print_error( &ast->loc,
        "only operator \"=\"%s operators can be \"%s\"\n",
        OPT_LANG_IS( default_RELOPS ) ? " and relational" : "",
        c_tid_error( TS_default )
      );
      return false;
  } // switch

  return true;
}

/**
 * Checks that a #K_OPERATOR AST is valid when either a member or non-member.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 *
 * @sa c_ast_check_oper_params()
 */
NODISCARD
static bool c_ast_check_oper_member( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  switch ( c_ast_op_overload( ast ) ) {
    case C_FUNC_NON_MEMBER:
      if ( c_op_is_new_delete( ast->oper.operator->op_id ) )
        break;                          // checks don't apply for new & delete

      //
      // Ensure non-member operators (except new, new[], delete, and delete[])
      // have at least one enum, class, struct, or union parameter.
      //
      if ( !c_ast_has_escu_param( ast ) ) {
        print_error( c_ast_params_loc( ast ),
          "at least 1 parameter of a non-member operator must be an "
          "enum, class, struct, or union"
          ", or a reference thereto"
          ", or a typedef thereof\n"
        );
        return false;
      }
      break;

    case C_FUNC_MEMBER:
      //
      // Ensure member operators are not friend, e.g.:
      //
      //      friend bool operator!()   // error
      //
      // Note that if an operator has a scoped name, e.g.:
      //
      //      friend bool S::operator!()
      //
      // then it's a member of S and not a member of the class that it's
      // presumably being declared within.
      //
      if ( c_tid_is_any( ast->type.stids, TS_friend ) &&
           c_sname_empty( &ast->sname ) ) {
        print_error( &ast->loc,
          "member operators can not be \"%s\"\n",
          c_tid_error( TS_friend )
        );
        return false;
      }
      break;

    case C_FUNC_UNSPECIFIED:
      // nothing to do
      break;
  } // switch

  return true;
}

/**
 * Checks that a #K_OPERATOR AST has the correct number of parameters.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 *
 * @sa c_ast_check_oper_params()
 */
NODISCARD
static bool c_ast_check_oper_num_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  c_operator_t const *const op = ast->oper.operator;

  unsigned params_min, params_max;
  c_ast_op_params_min_max( ast, &params_min, &params_max );

  size_t const n_params = slist_len( &ast->oper.param_ast_list );
  if ( n_params < params_min ) {
    if ( params_min == params_max )
      goto must_have_exactly_n_params;
    print_error( c_ast_params_loc( ast ),
      "%soperator \"%s\" must have at least %u parameter%s\n",
      c_ast_member_or_nonmember_str( ast ), op->literal,
      params_min, plural_s( params_min )
    );
    return false;
  }
  if ( n_params > params_max ) {
    if ( op->params_min == params_max )
      goto must_have_exactly_n_params;
    print_error( c_ast_params_loc( ast ),
      "%soperator \"%s\" can have at most %u parameter%s\n",
      c_ast_member_or_nonmember_str( ast ), op->literal,
      op->params_max, plural_s( op->params_max )
    );
    return false;
  }

  return true;

must_have_exactly_n_params:
  print_error( c_ast_params_loc( ast ),
    "%soperator \"%s\" must have exactly %u parameter%s\n",
    c_ast_member_or_nonmember_str( ast ), op->literal,
    params_min, plural_s( params_min )
  );
  return false;
}

/**
 * Checks all #K_OPERATOR AST parameters for semantic errors.
 *
 * @param ast The #K_OPERATOR AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_params( c_ast_t const *ast ) {
  if ( !c_ast_check_oper_num_params( ast ) )
    return false;
  if ( !c_ast_check_oper_member( ast ) )
    return false;

  // Perform additional checks for certain operators.
  switch ( ast->oper.operator->op_id ) {
    case C_OP_MINUS_MINUS:
    case C_OP_PLUS_PLUS:
      return c_ast_check_op_minus_minus_plus_plus_params( ast );

    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      return c_ast_check_op_delete_params( ast );

    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
      return c_ast_check_op_new_params( ast );

    default:
      /* suppress warning */;
  } // switch

  return true;
}

/**
 * Checks an AST that is a parameter pack for errors.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_param_pack( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->is_param_pack );

  if ( c_ast_parent_is_kind_any( ast, K_ANY_FUNCTION_LIKE ) ) {
    print_error( &ast->loc,
      "%s can not return parameter pack\n",
      c_kind_name( ast->parent_ast->kind )
    );
    return false;
  }

  //
  // For a parameter pack like:
  //
  //      auto &...x
  //
  // where the AST is:
  //
  //      {
  //        sname: { string: "x", scopes: "none" },
  //        is_param_pack: true,
  //        kind: { value: 0x1000, string: "reference" },
  //        ...
  //        ptr_ref: {
  //          to_ast: {
  //            ...
  //            is_param_pack: false,
  //            kind: { value: 0x2, string: "built-in type" },
  //            ...
  //            type: { btid: 0x0000000000000021, string: "auto" },
  //            ...
  //          }
  //        }
  //      }
  //
  // it's the reference that's a parameter pack, but we have to ensure the
  // type of the AST the reference is to (the leaf AST) is "auto".
  //
  c_ast_t const *const leaf_ast = c_ast_leaf( ast );
  if ( leaf_ast->kind == K_BUILTIN && leaf_ast->type.btids != TB_auto ) {
    print_error( &leaf_ast->loc,
      "parameter pack type must be \"%s\"\n",
      c_tid_error( TB_auto )
    );
    return false;
  }

  return true;
}

/**
 * Checks a #K_POINTER or #K_POINTER_TO_MEMBER AST for errors.
 *
 * @param ast The #K_POINTER or #K_POINTER_TO_MEMBER AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_pointer( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_POINTER ) );

  c_ast_t const *const to_ast = ast->ptr_ref.to_ast;
  c_ast_t const *const raw_to_ast = c_ast_untypedef( to_ast );

  switch ( raw_to_ast->kind ) {
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      print_error( &ast->loc, "%s to ", c_kind_name( ast->kind ) );
      print_ast_kind_aka( to_ast, stderr );
      EPUTS( " is illegal" );
      if ( raw_to_ast == to_ast ) {
        if ( is_english_to_gibberish() )
          print_hint( "\"reference to pointer\"" );
        else
          print_hint( "\"*&\"" );
      } else {
        EPUTC( '\n' );
      }
      return false;

    case K_BUILTIN:
      if ( c_tid_is_any( raw_to_ast->type.btids, TB_auto ) &&
           !OPT_LANG_IS( auto_POINTER_TYPES ) ) {
        print_error( &ast->loc,
          "\"%s\" with pointer declarator not supported%s\n",
          c_tid_error( TB_auto ),
          C_LANG_WHICH( auto_POINTER_TYPES )
        );
        return false;
      }
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_ARRAY:
    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_FUNCTION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_TYPEDEF:
      if ( c_tid_is_any( ast->type.atids, TA_ANY_MSC_CALL ) ) {
        print_error( &ast->loc,
          "\"%s\" can be used only for functions and pointers to function\n",
          c_tid_error( ast->type.atids )
        );
        return false;
      }
      break;

    case K_STRUCTURED_BINDING:
      print_error( &to_ast->loc, "pointer to structured binding is illegal\n" );
      return false;

    case K_CONCEPT:
    case K_NAME:
      // nothing to do
      break;

    case K_CAPTURE:
    case K_CAST:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_LAMBDA:
    case K_OPERATOR:
    case K_PLACEHOLDER:
    case K_UDEF_CONV:
    case K_UDEF_LIT:
    case K_VARIADIC:
      UNEXPECTED_INT_VALUE( raw_to_ast->kind );
  } // switch

  if ( c_ast_is_register( to_ast ) ) {
    error_kind_to_tid( ast, TS_register, "\n" );
    return false;
  }

  return true;
}

/**
 * Checks a #K_REFERENCE or #K_RVALUE_REFERENCE AST for errors.
 *
 * @param ast The #K_REFERENCE or #K_RVALUE_REFERENCE AST to check.
 * @param tdef_ast
 * @parblock
 * A #K_TYPEDEF AST whose \ref c_typedef_ast::for_ast "for_ast"
 * is \a ast or NULL otherwise.
 * Given:
 *
 *      using rint = int&;
 *
 * we need to distinguish two cases:
 *
 *  1. `int &const x`
 *      <br/>
 *      **error**: `const` may not be applied to a reference (directly).
 *
 *  2. `const rint x`
 *      <br/>
 *      **warning**: `const` on reference type has no effect.
 *
 * This function checks for case 1 (along with `volatile`);
 * c_ast_visitor_warning() checks for case 2.
 * @endparblock
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_reference( c_ast_t const *ast,
                                   c_ast_t const *tdef_ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_REFERENCE ) );
  assert( tdef_ast == NULL || tdef_ast->kind == K_TYPEDEF );

  if ( tdef_ast == NULL && c_tid_is_any( ast->type.stids, TS_CV ) ) {
    c_tid_t const qual_stids = ast->type.stids & TS_ANY_QUALIFIER;
    error_kind_not_tid( ast, qual_stids, LANG_NONE, "" );
    print_hint(
      is_english_to_gibberish() ? "\"reference to %s\"" : "\"%s&\"",
      c_tid_error( qual_stids )
    );
    return false;
  }

  c_ast_t const *const to_ast = ast->ptr_ref.to_ast;

  if ( c_ast_is_builtin_any( to_ast, TB_void ) ) {
    error_kind_to_tid( ast, TB_void, "" );
    if ( is_english_to_gibberish() )
      print_hint( "\"pointer to void\"" );
    else
      print_hint( "\"void*\"" );
    return false;
  }

  return true;
}

/**
 * Checks an AST whose type is #TS_restrict for errors.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if all checks passed.
 *
 * @sa c_ast_visitor_type()
 */
NODISCARD
static bool c_ast_check_restrict( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( c_ast_is_tid_any( ast, TS_restrict ) );

  c_ast_t const *const raw_ast = c_ast_untypedef( ast );
  switch ( raw_ast->kind ) {
    case K_ARRAY:                     // legal in C; __restrict legal in C++
      if ( !c_ast_is_param( ast ) ) {
        print_error( &ast->loc,
          "%s can not be \"%s\" except as function parameter\n",
          c_kind_name( raw_ast->kind ),
          c_tid_error( TS_restrict )
        );
        return false;
      }
      break;

    case K_FUNCTION:
    case K_OPERATOR:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_UDEF_CONV:
      //
      // These being declared "restrict" is already made an error by checks
      // elsewhere.
      //
      break;

    case K_POINTER:
      if ( !c_ast_is_ptr_to_kind_any( raw_ast, K_ANY_OBJECT ) ) {
        print_error( &ast->loc,
          "pointer to %s can not be \"%s\"\n",
          c_kind_name( c_ast_unpointer( ast )->kind ),
          c_tid_error( TS_restrict )
        );
        return false;
      }
      break;

    case K_BUILTIN:
    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_ENUM:
    case K_POINTER_TO_MEMBER:
      error_kind_not_tid( raw_ast, TS_restrict, LANG_NONE, "\n" );
      return false;

    case K_APPLE_BLOCK:
    case K_CAPTURE:
    case K_CAST:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_LAMBDA:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_STRUCTURED_BINDING:
    case K_TYPEDEF:
    case K_UDEF_LIT:
    case K_VARIADIC:
      UNEXPECTED_INT_VALUE( raw_ast->kind );
  } // switch

  return true;
}

/**
 * Checks a #K_STRUCTURED_BINDING AST for errors.
 *
 * @param ast The #K_STRUCTURED_BINDING AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_structured_binding( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_STRUCTURED_BINDING );

  if ( c_tid_is_any( ast->type.stids, c_tid_compl( TS_STRUCTURED_BINDING ) ) ) {
    print_error( &ast->loc,
      "structured binding may not be \"%s\"\n",
      c_tid_error( ast->type.stids )
    );
    return false;
  }

  FOREACH_SLIST_NODE( sname_node, &ast->struct_bind.sname_list ) {
    c_sname_t const *const sname = sname_node->data;
    if ( c_sname_count( sname ) > 1 ) {
      print_error( &ast->loc,
        "\"%s\": structured binding names may not be scoped\n",
        c_sname_gibberish( sname )
      );
      return false;
    }
    FOREACH_SLIST_NODE_UNTIL( prev_sname_node, &ast->struct_bind.sname_list,
                              sname_node ) {
      c_sname_t const *const prev_sname = prev_sname_node->data;
      if ( c_sname_cmp( sname, prev_sname ) == 0 ) {
        print_error( &ast->loc,
          "\"%s\": redefinition of structured binding\n",
          c_sname_local_name( prev_sname )
        );
        return false;
      }
    } // for
  } // for

  return true;
}

/**
 * Checks a #K_UDEF_CONV AST for errors.
 *
 * @param ast The #K_UDEF_CONV AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_udef_conv( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_UDEF_CONV );

  if ( c_tid_is_any( ast->type.stids, c_tid_compl( TS_USER_DEF_CONV ) ) ) {
    error_kind_not_tid( ast, ast->type.stids, LANG_NONE, "\n" );
    return false;
  }
  if ( c_tid_is_any( ast->type.stids, TS_friend ) &&
       c_sname_empty( &ast->sname ) ) {
    print_error( &ast->loc,
      "friend user-defined conversion operator must use qualified name\n"
    );
    return false;
  }
  c_ast_t const *const to_ast = ast->udef_conv.to_ast;
  c_ast_t const *const raw_to_ast = c_ast_untypedef( to_ast );
  if ( raw_to_ast->kind == K_ARRAY ) {
    print_error( &to_ast->loc,
      "user-defined conversion operator return type "
    );
    print_ast_type_aka( to_ast, stderr );
    EPUTS( " can not be an array" );
    print_hint( "pointer to array" );
    return false;
  }

  return  c_ast_check_func_ret_type( ast ) &&
          c_ast_check_func( ast ) &&
          c_ast_check_func_params( ast );
}

/**
 * Checks all #K_UDEF_LIT parameters for semantic errors.
 *
 * @param ast The #K_UDEF_LIT AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_udef_lit_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_UDEF_LIT );

  c_param_t const *param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *param_ast = c_param_ast( param );
  c_ast_t const *raw_param_ast = c_ast_untypedef( param_ast );
  c_ast_t const *ptr_to_ast = NULL;

  size_t const n_params = slist_len( &ast->udef_lit.param_ast_list );
  switch ( n_params ) {
    case 0:
      // the grammar requires at least one parameter
      UNEXPECTED_INT_VALUE( n_params );

    case 1:
      switch ( raw_param_ast->type.btids ) {
        case TB_char:
        case TB_char8_t:
        case TB_char16_t:
        case TB_char32_t:
        case TB_wchar_t:
        case TB_unsigned | TB_long | TB_long_long:
        case TB_unsigned | TB_long | TB_long_long | TB_int:
        case TB_long | TB_double:
          break;
        default:                        // check for: char const*
          if ( !c_ast_is_ptr_to_type_any( param_ast, &T_ANY, &T_const_char ) ) {
            print_error( &param_ast->loc,
              "invalid user-defined literal parameter type "
            );
            print_ast_type_aka( param_ast, stderr );
            EPRINTF( "; must be \"%s\", ",
                     c_tid_error( TB_unsigned | TB_long | TB_long_long ) );
            EPRINTF( "\"%s\", ", c_tid_error( TB_long | TB_double ) );
            EPRINTF( "\"%s\", ", c_tid_error( TB_char ) );
            EPRINTF(
              is_english_to_gibberish() ? "\"pointer to %s\", " : "\"%s*\", ",
              c_type_error( &T_const_char )
            );
            if ( OPT_LANG_IS( char8_t ) )
              EPRINTF( "\"%s\", ", c_tid_error( TB_char8_t ) );
            EPRINTF( "\"%s\", ", c_tid_error( TB_char16_t ) );
            EPRINTF( "\"%s\", ", c_tid_error( TB_char32_t ) );
            EPUTS( "or " );
            EPRINTF( "\"%s\"\n", c_tid_error( TB_wchar_t ) );
            return false;
          }
      } // switch
      break;

    case 2:
      ptr_to_ast = c_ast_unpointer( raw_param_ast );
      if ( ptr_to_ast == NULL ||
          !(c_ast_is_tid_any( ptr_to_ast, TS_const ) &&
            c_ast_is_tid_any( ptr_to_ast, TB_ANY_CHAR )) ) {
        print_error( &param_ast->loc,
          "invalid user-defined literal parameter type "
        );
        print_ast_type_aka( param_ast, stderr );
        EPRINTF( "; must be "
          "const (char%s|char16_t|char32_t|wchar_t)*\n",
          OPT_LANG_IS( char8_t ) ? "|char8_t" : ""
        );
        return false;
      }
      param_ast = c_param_ast( param->next );
      if ( param_ast == NULL || !c_ast_is_size_t( param_ast ) ) {
        print_error( &param_ast->loc,
          "invalid user-defined literal parameter type "
        );
        print_ast_type_aka( param_ast, stderr );
        EPUTS( "; must be \"std::size_t\" (or equivalent)\n" );
        return false;
      }
      break;

    default:
      param_ast = c_param_ast( param->next->next );
      print_error( &param_ast->loc,
        "user-defined literal may have at most 2 parameters\n"
      );
      return false;
  } // switch

  return true;
}

/**
 * Checks a #K_BUILTIN Unified Parallel C type AST for errors.
 *
 * @param ast The #K_BUILTIN AST to check.
 * @return Returns `true` only if all checks passed.
 *
 * @sa [Unified Parallel C](http://upc-lang.org/)
 */
NODISCARD
static bool c_ast_check_upc( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );

  if ( c_tid_is_any( ast->type.stids, TS_UPC_relaxed | TS_UPC_strict ) &&
      !c_tid_is_any( ast->type.stids, TS_UPC_shared ) ) {
    print_error( &ast->loc,
      "\"%s\" requires \"%s\"\n",
      c_type_error( &ast->type ),
      c_tid_error( TS_UPC_shared )
    );
    return false;
  }

  return true;
}

/**
 * Checks whether a function-like AST has at least one `enum,` `class`,
 * `struct`, or `union` parameter or reference thereto.
 *
 * @param ast The #K_ANY_FUNCTION_LIKE AST to check.
 * @return Returns `true` only if \a ast has at least one `enum`, `class`,
 * `struct`, or `union` parameter or reference thereto.
 */
NODISCARD
static bool c_ast_has_escu_param( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );

  FOREACH_AST_FUNC_PARAM( param, ast ) {
    c_ast_t const *param_ast = c_ast_unreference_any( c_param_ast( param ) );
    if ( (param_ast->kind & K_ANY_ECSU) != 0 )
      return true;
  } // for

  return false;
}

/**
 * Gets the string `"member "` or `"non-member "` depending on whether \a ast
 * is a member or non-member operator.
 *
 * @param ast The #K_OPERATOR AST to get the string for.
 * @return Returns either `"member "` or `"non-member "` including a trailing
 * space; or the empty string if unspecified.
 */
NODISCARD
static char const* c_ast_member_or_nonmember_str( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  c_func_member_t const member = c_ast_op_overload( ast );
  return  member == C_FUNC_MEMBER     ? "member "     :
          member == C_FUNC_NON_MEMBER ? "non-member " :
          "";
}

/**
 * Visitor function that checks an AST for semantic errors.
 *
 * @param ast The AST to check.
 * @param user_data The data to use.
 * @return Returns \ref VISITOR_ERROR_FOUND if an error was found;
 * \ref VISITOR_ERROR_NOT_FOUND if not.
 *
 * @sa c_type_ast_visitor_error()
 */
NODISCARD
static bool c_ast_visitor_error( c_ast_t const *ast, user_data_t user_data ) {
  assert( ast != NULL );
  c_ast_check_state_t const *const check = user_data.pc;
  assert( check != NULL );

  if ( !c_ast_check_name( ast ) )
    return VISITOR_ERROR_FOUND;

  if ( !c_ast_check_alignas( ast ) )
    return VISITOR_ERROR_FOUND;

  switch ( ast->kind ) {
    case K_ARRAY:
      if ( !c_ast_check_array( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_BUILTIN:
      if ( !c_ast_check_builtin( ast, check->tdef_ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_CAST:
      if ( !c_ast_check_cast( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_CLASS_STRUCT_UNION:
      // nothing to check
      break;

    case K_CONCEPT:
      if ( !c_ast_check_concept( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_ENUM:
      if ( !c_ast_check_enum( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_OPERATOR:
      if ( !c_ast_check_oper( ast ) )
        return VISITOR_ERROR_FOUND;
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
      if ( !c_ast_check_func_ret_type( ast ) )
        return VISITOR_ERROR_FOUND;
      FALLTHROUGH;

    case K_CONSTRUCTOR:
      if ( !(c_ast_check_func( ast ) && c_ast_check_func_params( ast )) )
        return VISITOR_ERROR_FOUND;
      FALLTHROUGH;

    case K_DESTRUCTOR:
      if ( (ast->kind & (K_CONSTRUCTOR | K_DESTRUCTOR)) != 0 &&
           !c_ast_check_ctor_dtor( ast ) ) {
        return VISITOR_ERROR_FOUND;
      }

      c_tid_t const not_func_stids =
        ast->type.stids &
        c_tid_compl( OPT_LANG_IS( C_ANY ) ? TS_FUNC_C : TS_FUNC_LIKE_CPP );
      if ( not_func_stids != TS_NONE ) {
        error_kind_not_tid( ast, not_func_stids, LANG_NONE, "\n" );
        return VISITOR_ERROR_FOUND;
      }

      if ( c_tid_is_any( ast->type.stids, TS_throw ) &&
           !OPT_LANG_IS( throw ) ) {
        print_error( &ast->loc,
          "\"throw\" not supported%s",
          C_LANG_WHICH( throw )
        );
        print_hint( "\"noexcept\"" );
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_LAMBDA:
      if ( !(c_ast_check_lambda( ast ) &&
             c_ast_check_func_params( ast ) &&
             c_ast_check_func_ret_type( ast )) ) {
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_NAME:
      // nothing to check
      break;

    case K_POINTER_TO_MEMBER:
      if ( !OPT_LANG_IS( POINTERS_TO_MEMBER ) ) {
        error_kind_not_supported( ast, LANG_POINTERS_TO_MEMBER );
        return VISITOR_ERROR_FOUND;
      }
      FALLTHROUGH;
    case K_POINTER:
      if ( !c_ast_check_pointer( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_RVALUE_REFERENCE:
      if ( !OPT_LANG_IS( RVALUE_REFERENCES ) ) {
        error_kind_not_supported( ast, LANG_RVALUE_REFERENCES );
        return VISITOR_ERROR_FOUND;
      }
      FALLTHROUGH;
    case K_REFERENCE:
      if ( !OPT_LANG_IS( REFERENCES ) ) {
        error_kind_not_supported( ast, LANG_REFERENCES );
        return VISITOR_ERROR_FOUND;
      }
      if ( !c_ast_check_reference( ast, check->tdef_ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_STRUCTURED_BINDING:
      if ( !c_ast_check_structured_binding( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_TYPEDEF:
      NO_OP;
      //
      // K_TYPEDEF isn't a "parent" kind since it's not a parent "of" the
      // underlying type, but instead a synonym "for" it.  Hence, we have to
      // recurse into it manually.
      //
      c_ast_t const temp_ast = c_ast_sub_typedef( ast );
      user_data.pc = &(c_ast_check_state_t){ .tdef_ast = ast };
      return c_ast_visitor_error( &temp_ast, user_data );

    case K_UDEF_CONV:
      if ( !c_ast_check_udef_conv( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_UDEF_LIT:
      if ( !(c_ast_check_func_ret_type( ast ) &&
             c_ast_check_func( ast ) &&
             c_ast_check_udef_lit_params( ast )) ) {
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_CAPTURE:               // checked in c_ast_check_lambda_captures()
    case K_VARIADIC:              // checked in c_ast_check_func_params()
      unreachable();

    case K_PLACEHOLDER:
      UNEXPECTED_INT_VALUE( ast->kind );
  } // switch

  if ( ast->kind != K_FUNCTION &&
       c_tid_is_any( ast->type.stids, TS_consteval ) ) {
    print_error( &ast->loc,
      "only functions can be \"%s\"\n",
      c_tid_error( TS_consteval )
    );
    return VISITOR_ERROR_FOUND;
  }

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for type errors.
 *
 * @param ast The AST to visit.
 * @param user_data Not used.
 * @return Returns \ref VISITOR_ERROR_FOUND if an error was found;
 * \ref VISITOR_ERROR_NOT_FOUND if not.
 */
NODISCARD
static bool c_ast_visitor_type( c_ast_t const *ast, user_data_t user_data ) {
  assert( ast != NULL );
  (void)user_data;

  c_lang_id_t const ok_lang_ids = c_type_check( &ast->type );
  if ( ok_lang_ids != LANG_ANY ) {
    c_lang_id_t const one_lang_ids = c_lang_is_one( ok_lang_ids );
    if ( one_lang_ids != LANG_NONE && !opt_lang_is_any( one_lang_ids ) ) {
      //
      // The language(s) ast->type is legal in is only either C or C++ and the
      // current language isn't one of those languages: just say it's illegal
      // (regardless of kind) in the current language (otherwise it can imply
      // it's legal for some other kind in the current language).
      //
      print_error( &ast->loc,
        "\"%s\" is illegal%s\n",
        c_type_error( &ast->type ),
        c_lang_which( ok_lang_ids )
      );
    } else {
      print_error( &ast->loc,
        "\"%s\" is illegal for %s%s\n",
        c_type_error( &ast->type ),
        c_kind_name( ast->kind ),
        c_lang_which( ok_lang_ids )
      );
    }
    return VISITOR_ERROR_FOUND;
  }

  if ( ast->is_param_pack && !c_ast_check_param_pack( ast ) )
    return VISITOR_ERROR_FOUND;

  if ( (ast->kind & K_ANY_FUNCTION_LIKE) != 0 ) {
    if ( c_tid_is_any( ast->type.stids, TS_constexpr ) &&
         !OPT_LANG_IS( constexpr_RETURN_TYPES ) &&
         c_ast_is_builtin_any( ast->func.ret_ast, TB_void ) ) {
      print_error( &ast->loc,
        "\"%s %s\" is illegal%s\n",
        c_tid_error( ast->type.stids ),
        c_tid_error( ast->func.ret_ast->type.btids ),
        C_LANG_WHICH( constexpr_RETURN_TYPES )
      );
      return VISITOR_ERROR_FOUND;
    }
  }
  else {
    if ( ast->kind != K_ARRAY &&
         c_tid_is_any( ast->type.stids, TS_NON_EMPTY_ARRAY ) ) {
      // Can't use error_kind_not_tid() here because we need to call
      // c_tid_english() for TS_NON_EMPTY_ARRAY, not c_tid_error().
      print_error( &ast->loc,
        "%s can not be \"%s\"\n",
        c_kind_name( ast->kind ),
        c_tid_english( TS_NON_EMPTY_ARRAY )
      );
      return VISITOR_ERROR_FOUND;
    }

    if ( c_tid_is_any( ast->type.stids, TS_constexpr ) &&
         OPT_LANG_IS( C_ANY ) &&
         c_tid_is_any( ast->type.stids, TS_NOT_constexpr_C_ONLY ) ) {
      print_error( &ast->loc,
        "\"%s %s\" is illegal in C\n",
        c_tid_error( TS_constexpr ),
        c_tid_error( ast->type.stids & TS_NOT_constexpr_C_ONLY )
      );
      return VISITOR_ERROR_FOUND;
    }

    c_tid_t const not_object_atids = ast->type.atids & c_tid_compl( TA_OBJECT );
    if ( not_object_atids != TA_NONE ) {
      error_kind_not_tid( ast, not_object_atids, LANG_NONE, "\n" );
      return VISITOR_ERROR_FOUND;
    }
  }

  if ( c_ast_is_tid_any( ast, TS_restrict ) && !c_ast_check_restrict( ast ) )
    return VISITOR_ERROR_FOUND;

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Checks whether \a op_id is one of #C_OP_NEW, #C_OP_NEW_ARRAY, #C_OP_DELETE,
 * or #C_OP_DELETE_ARRAY.
 *
 * @param op_id The ID of the c_operator to check.
 * @return Returns `true` only of \a op_id is one of said operators.
 */
bool c_op_is_new_delete( c_op_id_t op_id ) {
  switch ( op_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      return true;
    default:
      return false;
  } // switch
}

/**
 * Visitor function that checks a type AST for additional semantic errors.
 *
 * @param ast The AST of a type to check.
 * @param user_data Not used.
 * @return Returns \ref VISITOR_ERROR_FOUND if an error was found;
 * \ref VISITOR_ERROR_NOT_FOUND if not.
 *
 * @sa c_ast_visitor_error()
 * @sa c_type_ast_check()
 * @sa c_type_ast_visitor_warning()
 */
NODISCARD
static bool c_type_ast_visitor_error( c_ast_t const *ast,
                                      user_data_t user_data ) {
  assert( ast != NULL );
  (void)user_data;

  if ( !c_ast_check_name( ast ) )
    return false;

  switch ( ast->kind ) {
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_FUNCTION:
      FOREACH_AST_FUNC_PARAM( param, ast ) {
        if ( !c_type_ast_check( c_param_ast( param ) ) )
          return VISITOR_ERROR_FOUND;
      } // for
      break;

    case K_BUILTIN:
      if ( c_ast_is_tid_any( ast, TB_auto ) ) {
        print_error( &ast->loc,
          "\"%s\" illegal in type definition\n",
          c_tid_error( TB_auto )
        );
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_CONCEPT:
      print_error( &ast->loc,
        "\"%s\" illegal in type definition\n",
        c_kind_name( ast->kind )
      );
      return VISITOR_ERROR_FOUND;

    case K_ARRAY:
    case K_CAPTURE:
    case K_CAST:
    case K_CLASS_STRUCT_UNION:
    case K_DESTRUCTOR:
    case K_ENUM:
    case K_NAME:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_STRUCTURED_BINDING:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_LAMBDA:
    case K_OPERATOR:
    case K_UDEF_CONV:
    case K_UDEF_LIT:
      // even though these have parameters, they can't be used in a typedef
    case K_PLACEHOLDER:
      UNEXPECTED_INT_VALUE( ast->kind );
  } // switch

  return VISITOR_ERROR_NOT_FOUND;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check( c_ast_t const *ast ) {
  assert( ast != NULL );

  if ( !c_ast_check_errors( ast ) )
    return false;

  if ( cdecl_is_initialized )
    c_ast_warn( ast );

  return true;
}

bool c_ast_list_check( c_ast_list_t const *ast_list ) {
  assert( ast_list != NULL );

  if ( slist_empty( ast_list ) )
    return true;                        // LCOV_EXCL_LINE

  c_ast_t const *const first_ast = slist_front( ast_list );
  if ( slist_len( ast_list ) == 1 )
    return c_ast_check( first_ast );

  if ( first_ast->type.btids == TB_auto &&
      !OPT_LANG_IS( auto_TYPE_MULTI_DECL ) ) {
    print_error( &first_ast->loc,
      "\"%s\" with multiple declarators not supported%s\n",
      c_tid_error( TB_auto ),
      C_LANG_WHICH( auto_TYPE_MULTI_DECL )
    );
    return false;
  }

  FOREACH_SLIST_NODE( ast_node, ast_list ) {
    c_ast_t const *const ast = ast_node->data;
    if ( ast->is_param_pack ) {
      print_error( &first_ast->loc,
        "can not use parameter pack in multiple declaration\n"
      );
      return false;
    }
  } // for

  FOREACH_SLIST_NODE( ast_node, ast_list ) {
    c_ast_t const *const ast = ast_node->data;
    //
    // Ensure that a name is not used more than once in the same declaration in
    // C++ or with different types in C.  (In C, more than once with the same
    // type are "tentative definitions" and OK.)
    //
    //      int i, i;                   // OK in C (same type); error in C++
    //      int j, *j;                  // error (different types)
    //
    if ( !c_sname_empty( &ast->sname ) ) {
      FOREACH_SLIST_NODE_UNTIL( prev_ast_node, ast_list, ast_node ) {
        c_ast_t const *const prev_ast = prev_ast_node->data;
        if ( c_sname_empty( &prev_ast->sname ) )
          continue;
        if ( c_sname_cmp( &ast->sname, &prev_ast->sname ) != 0 )
          continue;
        if ( !OPT_LANG_IS( TENTATIVE_DEFS ) ) {
          print_error( &ast->loc,
            "\"%s\": redefinition\n",
            c_sname_gibberish( &ast->sname )
          );
          return false;
        }
        if ( !c_ast_equal( ast, prev_ast ) ) {
          print_error( &ast->loc,
            "\"%s\": redefinition with different type\n",
            c_sname_gibberish( &ast->sname )
          );
          return false;
        }
      } // for
    }

    if ( !c_ast_check( ast ) )
      return false;
  } // for

  return true;
}

bool c_type_ast_check( c_ast_t const *type_ast ) {
  assert( type_ast != NULL );

  if ( !c_ast_check_visitor( type_ast, &c_type_ast_visitor_error ) )
    return false;

  if ( cdecl_is_initialized )
    c_type_ast_warn( type_ast );

  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
