/*
**      cdecl -- C gibberish translator
**      src/c_ast_check.c
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
 * Defines a function for checking an AST for semantic errors.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast_check.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_operator.h"
#include "c_type.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "english.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <string.h>

/// @endcond

/**
 * Storage-class-like types that are _not_ legal with `constexpr` in C only.
 */
#define TS_NOT_CONSTEXPR_C_ONLY   ( TS__Atomic | TS_restrict | TS_volatile )

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints an error: `can not cast into <kind>`.
 *
 * @param AST The AST.
 * @param HINT The hint.
 */
#define error_kind_not_cast_into(AST,HINT) BLOCK(       \
  print_error( &(AST)->loc,                             \
    "can not cast into %s", c_kind_name( (AST)->kind )  \
  );                                                    \
  print_hint( "cast into %s", (HINT) ); )

/**
 * Prints an error: `<kind> not supported[ {in|since|unless|until} <lang>]`.
 *
 * @param AST The AST having the unsupported kind.
 * @param LANG_IDS The bitwise-or of legal language(s).
 */
#define error_kind_not_supported(AST,LANG_IDS)            \
  print_error( &(AST)->loc,                               \
    "%s not supported%s\n",                               \
    c_kind_name( (AST)->kind ), c_lang_which( LANG_IDS )  \
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
    "%s can not be %s%s" END_STR_LIT,                     \
    c_kind_name( (AST)->kind ), c_tid_name_error( TID ),  \
    c_lang_which( LANG_IDS )                              \
  )

/**
 * Prints an error: `<kind> of <kind> is illegal`.
 *
 * @param AST1 The AST having the bad kind.
 * @param AST2 The AST having the other kind.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_of_kind(AST1,AST2,END_STR_LIT)             \
  print_error( &(AST1)->loc,                                  \
    "%s of %s is illegal" END_STR_LIT,                        \
    c_kind_name( (AST1)->kind ), c_kind_name( (AST2)->kind )  \
  )

/**
 * Prints an error: `<kind> to <kind> is illegal`.
 *
 * @param AST1 The AST having the bad kind.
 * @param AST2 The AST having the other kind.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_to_kind(AST1,AST2,END_STR_LIT)             \
  print_error( &(AST1)->loc,                                  \
    "%s to %s is illegal" END_STR_LIT,                        \
    c_kind_name( (AST1)->kind ), c_kind_name( (AST2)->kind )  \
  )

/**
 * Prints an error: `<kind> to <type> is illegal`.
 *
 * @param AST The AST having the bad kind.
 * @param TID The bad type.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_to_tid(AST,TID,END_STR_LIT)          \
  print_error( &(AST)->loc,                             \
    "%s to %s is illegal" END_STR_LIT,                  \
    c_kind_name( (AST)->kind ), c_tid_name_error( TID ) \
  )

/**
 * Prints an error: `<type> <type> is illegal`.
 *
 * @param AST The AST .
 * @param TID1 The first bad type.
 * @param TID2 The second bad type.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (usually `"\n"`).
 */
#define error_tid_not_tid(AST,TID1,TID2,END_STR_LIT)    \
  print_error( &(AST)->loc,                             \
    "\"%s %s\" is illegal" END_STR_LIT,                 \
    c_tid_name_error( TID1 ), c_tid_name_error( TID2 )  \
  )

///////////////////////////////////////////////////////////////////////////////

/**
 * State maintained by c_ast_check_visitor().
 */
struct c_state {
  c_ast_t const  *func_ast;             ///< The current function AST, if any.

  /**
   * Is an AST node the "of" type of a `typedef` AST that is itself a "to" type
   * of a pointer AST?  For example, given:
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
  bool            is_pointee;
};
typedef struct c_state c_state_t;

/**
 * The signature for functions passed to c_ast_check_visitor().
 *
 * @param ast The AST to check.
 * @param data The data to use.
 * @return Returns `true` only if all checks passed.
 */
typedef bool (*c_ast_check_fn_t)( c_ast_t const *ast, user_data_t data );

// local constants

/// Convenience return value for \ref c_ast_visit_fn_t functions.
static bool const VISITOR_ERROR_FOUND     = true;

/// Convenience return value for \ref c_ast_visit_fn_t functions.
static bool const VISITOR_ERROR_NOT_FOUND = false;

// local functions
NODISCARD
static bool         c_ast_check_emc( c_ast_t const* ),
                    c_ast_check_func_main( c_ast_t const* ),
                    c_ast_check_func_main_char_ptr_param( c_ast_t const* ),
                    c_ast_check_func_params_knr( c_ast_t const* ),
                    c_ast_check_func_params_redef( c_ast_t const* ),
                    c_ast_check_lambda_captures( c_ast_t const* ),
                    c_ast_check_lambda_captures_redef( c_ast_t const* ),
                    c_ast_check_oper_default( c_ast_t const* ),
                    c_ast_check_oper_params( c_ast_t const* ),
                    c_ast_check_oper_relational_default( c_ast_t const* ),
                    c_ast_check_upc( c_ast_t const* ),
                    c_ast_name_equal( c_ast_t const*, char const* ),
                    c_ast_visitor_error( c_ast_t const*, user_data_t ),
                    c_ast_visitor_type( c_ast_t const*, user_data_t ),
                    c_ast_visitor_warning( c_ast_t const*, user_data_t );

static void         c_ast_warn_name( c_ast_t const* );
static void         c_sname_warn( c_sname_t const*, c_loc_t const* );

NODISCARD
static c_lang_id_t  is_reserved_name( char const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks an entire AST for semantic errors using \a check_fn.
 *
 * @param ast The AST to check.
 * @param check_fn The check function to use.
 * @param c The c_state to use.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static inline bool c_ast_check_visitor( c_ast_t const *ast,
                                        c_ast_check_fn_t check_fn,
                                        c_state_t const *c ) {
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  c_ast_visit_fn_t const visit_fn = POINTER_CAST( c_ast_visit_fn_t, check_fn );
  user_data_t const data = { .pc = c };
  return c_ast_visit( nonconst_ast, C_VISIT_DOWN, visit_fn, data ) == NULL;
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
 * Gets whether \a ast has the `register` storage class.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast has the `register` storage class.
 */
NODISCARD
static inline bool c_ast_is_register( c_ast_t const *ast ) {
  return c_tid_is_any( ast->type.stids, TS_register );
}

/**
 * Gets whether the membership specification of the C++ operator \a ast matches
 * the overloadability of \a op.
 *
 * @param ast The C++ operator AST to check.
 * @param op The C++ operator to check against.
 * @return Returns `true` only if either:
 *  + The user didn't specify either #C_FUNC_MEMBER or #C_FUNC_NON_MEMBER; or:
 *  + The membership specification of \a ast matches the overloadability of \a
 *    op.
 */
NODISCARD
static inline bool c_ast_oper_mbr_matches( c_ast_t const *ast,
                                           c_operator_t const *op ) {
  return  ast->oper.member == C_FUNC_UNSPECIFIED ||
          (STATIC_CAST( unsigned, ast->oper.member ) &
           STATIC_CAST( unsigned, op->overload  )) != 0;
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
  return &IF_ELSE( param_ast, ast )->loc;
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
 * Checks the `alignas` of an AST for errors.
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
    print_error( &ast->align.loc,
      "%s can not be aligned\n", c_kind_name( ast->kind )
    );
    return false;
  }

  if ( (raw_ast->kind & K_ANY_BIT_FIELD) != 0 &&
        ast->bit_field.bit_width > 0 ) {
    print_error( &ast->align.loc, "bit fields can not be aligned\n" );
    return false;
  }

  if ( (raw_ast->kind & K_CLASS_STRUCT_UNION) != 0 &&
       !OPT_LANG_IS( ALIGNED_CSUS ) ) {
    print_error( &ast->align.loc,
      "%s can not be aligned%s\n",
      c_kind_name( raw_ast->kind ),
      C_LANG_WHICH( ALIGNED_CSUS )
    );
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
    case C_ALIGNAS_TYPE:
      return c_ast_check( ast->align.type_ast );
  } // switch

  return true;
}

/**
 * Checks an array AST for errors.
 *
 * @param ast The array AST to check.
 * @param c The c_state to use.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_array( c_ast_t const *ast, c_state_t const *c ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );
  assert( c != NULL );

  if ( c_tid_is_any( ast->type.stids, TS__Atomic ) ) {
    error_kind_not_tid( ast, TS__Atomic, LANG_NONE, "\n" );
    return false;
  }

  switch ( ast->array.kind ) {
    case C_ARRAY_EMPTY_SIZE:
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
        print_error( &ast->loc, "\"non-empty\" requires an array size\n" );
        return false;
      }
      break;
    case C_ARRAY_INT_SIZE:
      if ( ast->array.size_int == 0 ) {
        print_error( &ast->loc, "array size must be greater than 0\n" );
        return false;
      }
      break;
    case C_ARRAY_NAMED_SIZE: {
      if ( c->func_ast == NULL )
        break;
      c_ast_t const *const size_param_ast =
        c_ast_find_param_named( c->func_ast, ast->array.size_name, ast );
      if ( size_param_ast == NULL )
        break;
      // At this point, we know it's a VLA.
      if ( !c_ast_is_integral( size_param_ast ) ) {
        print_error( &ast->loc,
          "size of array has non-integral type %s\n",
          c_type_name_error( &size_param_ast->type )
        );
        return false;
      }
      FALLTHROUGH;
    }
    case C_ARRAY_VLA_STAR:
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
      c_tid_name_error( ast->type.stids ),
      C_LANG_WHICH( QUALIFIED_ARRAYS )
    );
    return false;
  }

  c_ast_t const *const of_ast = ast->array.of_ast;
  c_ast_t const *const raw_of_ast = c_ast_untypedef( of_ast );
  switch ( raw_of_ast->kind ) {
    case K_ARRAY:
      if ( of_ast->array.kind == C_ARRAY_EMPTY_SIZE ) {
        print_error( &of_ast->loc, "array dimension required\n" );
        return false;
      }
      break;

    case K_BUILTIN:
      if ( c_ast_is_builtin_any( raw_of_ast, TB_void ) ) {
        print_error( &ast->loc, "array of void" );
        print_hint( "array of pointer to void" );
        return false;
      }
      break;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
      error_kind_of_kind( ast, raw_of_ast, "" );
      print_hint( "array of pointer to function" );
      return false;

    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      error_kind_of_kind( ast, raw_of_ast, "" );
      if ( cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH ) {
        print_hint( "%s to array", c_kind_name( raw_of_ast->kind ) );
      } else {
        print_hint( "(%s%s)[]",
          raw_of_ast->kind == K_REFERENCE ? "&" : "&&",
          c_sname_full_name( c_ast_find_name( ast, C_VISIT_DOWN ) )
        );
      }
      return false;

    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
      // nothing to do
      break;

    case K_CAPTURE:                     // impossible
    case K_CAST:                        // impossible
    case K_CONSTRUCTOR:                 // impossible
    case K_DESTRUCTOR:                  // impossible
    case K_LAMBDA:                      // impossible
    case K_NAME:                        // impossible
    case K_OPERATOR:                    // impossible
    case K_TYPEDEF:                     // impossible after c_ast_untypedef()
    case K_UDEF_CONV:                   // impossible
    case K_UDEF_LIT:                    // impossible
    case K_VARIADIC:                    // impossible
      UNEXPECTED_INT_VALUE( raw_of_ast->kind );

    CASE_K_PLACEHOLDER;
  } // switch

  return true;
}

/**
 * Checks a built-in type AST for errors.
 *
 * @param ast The built-in AST to check.
 * @param c The c_state to use.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_builtin( c_ast_t const *ast, c_state_t const *c ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );
  assert( c != NULL );

  if ( ast->type.btids == TB_NONE && !OPT_LANG_IS( IMPLICIT_int ) &&
       !c_ast_parent_is_kind( ast, K_UDEF_CONV ) ) {
    print_error( &ast->loc,
      "implicit \"int\" is illegal%s\n",
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
        c_type_name_error( &ast->type ), min_bits, plural_s( min_bits )
      );
      return false;
    }
    if ( ast->builtin.BitInt.width > C_BITINT_MAXWIDTH ) {
      print_error( &ast->loc,
        "%s can be at most %u bits\n",
        c_type_name_error( &ast->type ), C_BITINT_MAXWIDTH
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
        "[[no_unique_address]] %ss can not have bit-field widths\n",
        c_kind_name( ast->kind )
      );
      return false;
    }
    if ( ast->type.stids != TS_NONE ) {
      print_error( &ast->loc,
        "%s can not have bit-field widths\n",
        c_tid_name_error( ast->type.stids )
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
       !(OPT_LANG_IS( C_ANY ) && c_tid_is_any( ast->type.stids, TS_extern )) &&
       !c->is_pointee ) {
    print_error( &ast->loc, "variable of void" );
    print_hint( "pointer to void" );
    return false;
  }

  return c_ast_check_emc( ast ) && c_ast_check_upc( ast );
}

/**
 * Checks a cast AST for cast-specific errors.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_CAST );

  c_ast_t *const to_ast = ast->cast.to_ast;
  c_ast_t const *const storage_ast = c_ast_find_type_any(
    to_ast, C_VISIT_DOWN, &C_TYPE_LIT_S( TS_ANY_STORAGE )
  );

  if ( storage_ast != NULL ) {
    print_error( &to_ast->loc,
      "can not cast into %s\n",
      c_tid_name_error( storage_ast->type.stids & TS_ANY_STORAGE )
    );
    return false;
  }

  c_ast_t const *const leaf_ast = c_ast_leaf( ast );
  if ( c_ast_is_tid_any( leaf_ast, TB_auto ) ) {
    print_error( &leaf_ast->loc,
      "can not cast into \"%s\"\n",
      c_type_name_error( &leaf_ast->type )
    );
    return false;
  }

  c_ast_t const *const raw_to_ast = c_ast_untypedef( to_ast );

  switch ( raw_to_ast->kind ) {
    case K_ARRAY:
      if ( !c_sname_empty( &ast->sname ) ) {
        print_error( &ast->loc, "arithmetic or pointer type expected\n" );
        return false;
      }
      break;
    case K_FUNCTION:
      error_kind_not_cast_into( to_ast, "pointer to function" );
      return false;
    default:
      /* suppress warning */;
  } // switch

  switch ( ast->cast.kind ) {
    case C_CAST_CONST:
      if ( (raw_to_ast->kind & (K_ANY_POINTER | K_ANY_REFERENCE)) == 0 ) {
        print_error( &to_ast->loc,
          "const_cast must be to a pointer, pointer-to-member, %s\n",
          OPT_LANG_IS( RVALUE_REFERENCES ) ?
            "reference, or rvalue reference" : "or reference"
        );
        return false;
      }
      break;

    case C_CAST_DYNAMIC:
      if ( !c_ast_is_ptr_to_kind_any( raw_to_ast, K_CLASS_STRUCT_UNION ) &&
           !c_ast_is_ref_to_kind_any( raw_to_ast, K_CLASS_STRUCT_UNION ) ) {
        print_error( &to_ast->loc,
          "dynamic_cast must be to a "
          "pointer or reference to a class, struct, or union\n"
        );
        return false;
      }
      break;

    case C_CAST_REINTERPRET:
      if ( c_ast_is_builtin_any( to_ast, TB_void ) ) {
        print_error( &to_ast->loc, "reinterpret_cast can not be to void\n" );
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
 * Checks a constructor or destructor AST for errors.
 *
 * @param ast The constructor or destructor AST to check.
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
      c_type_name_error( c_sname_local_type( &ast->sname ) ),
      c_kind_name( ast->kind )
    );
    return false;
  }

  bool const is_constructor = ast->kind == K_CONSTRUCTOR;

  c_tid_t const ok_stids = is_constructor ?
    (is_definition ? TS_CONSTRUCTOR_DEF : TS_CONSTRUCTOR_DECL) :
    (is_definition ? TS_DESTRUCTOR_DEF  : TS_DESTRUCTOR_DECL ) ;

  c_tid_t const stids = ast->type.stids & c_tid_compl( ok_stids );
  if ( stids != TS_NONE ) {
    print_error( &ast->loc,
      "%s%s can not be %s\n",
      c_kind_name( ast->kind ),
      is_definition ? " definitions" : "s",
      c_tid_name_error( stids )
    );
    return false;
  }

  return true;
}

/**
 * Checks a built-in Embedded C type AST for errors.
 *
 * @param ast The built-in AST to check.
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
      "\"_Sat\" requires either \"_Accum\" or \"_Fract\"\n"
    );
    return false;
  }

  return true;
}

/**
 * Checks an `enum` AST for errors.
 *
 * @param ast The `enum` AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_enum( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_ENUM );

  if ( cdecl_mode == CDECL_GIBBERISH_TO_ENGLISH &&
       c_tid_is_any( ast->type.btids, TB_struct | TB_class ) &&
      !c_tid_is_any( ast->type.stids, TS_typedef ) ) {
    print_error( &ast->loc,
      "\"%s\": enum classes must just use \"enum\"\n",
      c_type_name_error( &ast->type )
    );
    return false;
  }

  if ( ast->enum_.bit_width > 0 && !OPT_LANG_IS( enum_BITFIELDS ) ) {
    print_error( &ast->loc,
      "enum bit-fields not supported%s\n",
      C_LANG_WHICH( enum_BITFIELDS )
    );
    return false;
  }

  c_ast_t const *const of_ast = ast->enum_.of_ast;
  if ( of_ast != NULL ) {
    if ( !OPT_LANG_IS( FIXED_TYPE_enum ) ) {
      print_error( &of_ast->loc,
        "enum with underlying type not supported%s\n",
        C_LANG_WHICH( FIXED_TYPE_enum )
      );
      return false;
    }

    if ( !c_ast_is_builtin_any( of_ast, TB_ANY_INTEGRAL ) ) {
      print_error( &of_ast->loc, "enum underlying type must be integral\n" );
      return false;
    }
  }

  return true;
}

/**
 * Checks an entire AST for semantic errors.
 *
 * @param ast The AST to check.
 * @param c The c_state to use.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_errors( c_ast_t const *ast, c_state_t const *c ) {
  assert( ast != NULL );
  // check in major-to-minor error order
  return  c_ast_check_visitor( ast, c_ast_visitor_error, c ) &&
          c_ast_check_visitor( ast, c_ast_visitor_type, c );
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

  if ( ast->kind == K_FUNCTION && c_ast_name_equal( ast, "main" ) &&
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
      "%s with \"this\" parameter can not be %s\n",
      c_kind_name( ast->kind ),
      c_tid_name_error( ast->type.stids & TS_FUNC_LIKE_NOT_EXPLICIT_OBJ_PARAM )
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
        "reference qualified %ss can not be %s\n",
        c_kind_name( ast->kind ),
        c_tid_name_error( ast->type.stids & TS_ANY_LINKAGE )
      );
      return false;
    }
  }

  c_tid_t const member_func_stids = ast->type.stids & TS_MEMBER_FUNC_ONLY;

  if ( member_func_stids != TS_NONE &&
       c_tid_is_any( ast->type.stids, TS_ANY_LINKAGE ) ) {
    print_error( &ast->loc,
      "%s %ss can not be %s\n",
      c_tid_name_error( ast->type.stids & TS_ANY_LINKAGE ),
      c_kind_name( ast->kind ),
      c_tid_name_error( member_func_stids )
    );
    return false;
  }

  switch ( ast->func.member ) {
    case C_FUNC_MEMBER: {
      //
      // Member functions can't have linkage -- except the new & delete
      // operators may have static explicitly specified.
      //
      c_tid_t linkage_stids = TS_extern | TS_extern_C;
      if ( ast->kind == K_OPERATOR &&
           !c_oper_is_new_delete( ast->oper.operator->oper_id ) ) {
        linkage_stids |= TS_static;
      }
      if ( c_tid_is_any( ast->type.stids, linkage_stids ) ) {
        print_error( &ast->loc,
          "member %ss can not be %s\n",
          c_kind_name( ast->kind ),
          c_tid_name_error( ast->type.stids & linkage_stids )
        );
        return false;
      }
      break;
    }
    case C_FUNC_NON_MEMBER:
      if ( member_func_stids != TS_NONE ) {
        print_error( &ast->loc,
          "non-member %ss can not be %s\n",
          c_kind_name( ast->kind ),
          c_tid_name_error( member_func_stids )
        );
        return false;
      }
      break;
    case C_FUNC_UNSPECIFIED:
      // nothing to do
      break;
  } // switch

  if ( c_tid_is_any( ast->type.stids, TS_default | TS_delete ) ) {
    switch ( ast->kind ) {
      case K_CONSTRUCTOR:
        switch ( c_ast_params_count( ast ) ) {
          case 0:                     // C()
            break;
          case 1:                     // C(C const&)
            if ( c_ast_is_ref_to_class_sname( param_ast, &ast->sname ) )
              break;
            FALLTHROUGH;
          default:
            //
            // This isn't correct since copy constructors can have more than
            // one parameter if the additional ones all have default arguments;
            // but cdecl doesn't support default arguments.
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
        switch ( ast->oper.operator->oper_id ) {
          case C_OP_EQUAL: {            // C& operator=(C const&)
            //
            // For C& operator=(C const&), the parameter and the return type
            // must both be a reference to the same class, struct, or union.
            //
            c_ast_t const *const ret_ast =
              c_ast_is_ref_to_tid_any( ast->oper.ret_ast, TB_ANY_CLASS );
            if ( ret_ast == NULL || c_ast_params_count( ast ) != 1 )
              goto only_special;
            param_ast = c_ast_is_ref_to_tid_any( param_ast, TB_ANY_CLASS );
            if ( param_ast != ret_ast )
              goto only_special;
            break;
          }

          case C_OP_EQUAL2:
          case C_OP_EXCLAM_EQUAL:
          case C_OP_GREATER:
          case C_OP_GREATER_EQUAL:
          case C_OP_LESS:
          case C_OP_LESS_EQUAL:
          case C_OP_LESS_EQUAL_GREATER:
            if ( c_tid_is_any( ast->type.stids, TS_delete ) )
              goto only_special;
            //
            // Detailed checks for defaulted overloaded relational operators
            // are done in c_ast_check_oper_relational_default().
            //
            break;

          default:
            goto only_special;
        } // switch
        break;

      default:
        //
        // The grammar allows only functions, operators, constructors,
        // destructors, and user-defined conversion operators to be either
        // `default` or `delete`.  This function isn't called for destructors
        // and the others have cases above.
        //
        UNEXPECTED_INT_VALUE( ast->kind );
    } // switch
  }

  c_tid_t const not_func_atids = ast->type.atids & c_tid_compl( TA_FUNC );
  if ( not_func_atids != TA_NONE ) {
    error_kind_not_tid( ast, not_func_atids, LANG_NONE, "\n" );
    return false;
  }

  if ( c_tid_is_any( ast->type.stids, TS_virtual ) ) {
    if ( c_sname_count( &ast->sname ) > 1 ) {
      print_error( &ast->loc,
        "\"%s\": virtual can not be used in file-scoped %ss\n",
        c_sname_full_name( &ast->sname ), c_kind_name( ast->kind )
      );
      return false;
    }
  }
  else if ( c_tid_is_any( ast->type.stids, TS_PURE_virtual ) ) {
    print_error( &ast->loc,
      "non-virtual %s can not be pure\n", c_kind_name( ast->kind )
    );
    return false;
  }

  return true;

only_special:
  print_error( &ast->loc,
    "\"%s\" can be used only for special member functions%s\n",
    c_type_name_error( &ast->type ),
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
      "main() can not be %s in C\n",
      c_tid_name_error( ast->type.stids )
    );
    return false;
  }

  c_ast_t const *const ret_ast = ast->func.ret_ast;
  if ( !c_ast_is_builtin_any( ret_ast, TB_int ) ) {
    print_error( &ret_ast->loc, "main() must return int\n" );
    return false;
  }

  size_t const n_params = c_ast_params_count( ast );
  c_param_t const *param = c_ast_params( ast );
  c_ast_t const *param_ast;

  switch ( n_params ) {
    case 0:                             // main()
      break;

    case 1:                             // main(void)
      param_ast = c_param_ast( param );
      if ( opt_lang == LANG_C_KNR ) {
        print_error( &param_ast->loc,
          "main() must have 0, 2, or 3 parameters in %s\n",
          c_lang_name( LANG_C_KNR )
        );
        return false;
      }
      if ( !c_ast_is_builtin_any( param_ast, TB_void ) ) {
        print_error( &param_ast->loc,
          "a single parameter for main() must be void\n"
        );
        return false;
      }
      break;

    case 2:                             // main(int, char *argv[])
    case 3:                             // main(int, char *argv[], char *envp[])
      if ( OPT_LANG_IS( PROTOTYPES ) ) {
        param_ast = c_param_ast( param );
        if ( !c_ast_is_builtin_any( param_ast, TB_int ) ) {
          print_error( &param_ast->loc,
            "main()'s first parameter must be int\n"
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
              &C_TYPE_LIT_S_ANY( c_tid_compl( TS_const ) ),
              &C_TYPE_LIT_B( TB_char ) ) ) {
        print_error( &param_ast->loc,
          "this parameter of main() must be %s %s pointer to [const] char\n",
          c_kind_name( param_ast->kind ),
          param_ast->kind == K_ARRAY ? "of" : "to"
        );
        return false;
      }
      break;
    default:                            // ???
      print_error( &param_ast->loc, "illegal signature for main()\n" );
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

  c_ast_t const *variadic_ast = NULL, *void_ast = NULL;
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
          "%s parameters can not be %s\n",
          c_kind_name( ast->kind ),
          c_tid_name_error( param_stids )
        );
        return false;
      }
    }

    switch ( raw_param_ast->kind ) {
      case K_BUILTIN:
        if ( c_tid_is_any( raw_param_ast->type.btids, TB_auto ) &&
             !OPT_LANG_IS( auto_PARAMETERS ) ) {
          print_error( &param_ast->loc,
            "parameters can not be \"auto\"%s\n",
            C_LANG_WHICH( auto_PARAMETERS )
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
              "void as parameter can not have a name\n"
            );
            return false;
          }
          c_tid_t qual_stids;
          if ( c_ast_is_tid_any_qual( param_ast, TS_CV, &qual_stids ) ) {
            print_error( &param_ast->loc,
              "void as parameter can not be %s\n",
              c_tid_name_error( qual_stids )
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
            "parameters can not have bit-field widths\n"
          );
          return false;
        }
        break;

      case K_NAME:
        if ( !OPT_LANG_IS( KNR_FUNC_DEFS ) ) {
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
             ast->oper.operator->oper_id != C_OP_PARENS ) {
          print_error( &param_ast->loc,
            "operator %s can not have a variadic parameter\n",
            ast->oper.operator->literal
          );
          return false;
        }
        if ( param->next != NULL ) {
          print_error( &param_ast->loc, "variadic specifier must be last\n" );
          return false;
        }
        assert( variadic_ast == NULL );
        variadic_ast = param_ast;
        continue;

      case K_ARRAY:
      case K_CLASS_STRUCT_UNION:
      case K_ENUM:
      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        // nothing to do
        break;

      case K_APPLE_BLOCK:               // impossible
      case K_CAPTURE:                   // impossible
      case K_CAST:                      // impossible
      case K_CONSTRUCTOR:               // impossible
      case K_DESTRUCTOR:                // impossible
      case K_FUNCTION:                  // impossible
      case K_LAMBDA:                    // impossible
      case K_OPERATOR:                  // impossible
      case K_TYPEDEF:                   // impossible after c_ast_untypedef()
      case K_UDEF_CONV:                 // impossible
      case K_UDEF_LIT:                  // impossible
        UNEXPECTED_INT_VALUE( raw_param_ast->kind );

      CASE_K_PLACEHOLDER;
    } // switch

    c_state_t param_c;
    MEM_ZERO( &param_c );
    param_c.func_ast = ast;
    if ( !c_ast_check_errors( param_ast, &param_c ) )
      return false;
  } // for

  if ( !OPT_LANG_IS( VARIADIC_ONLY_PARAMS ) &&
       variadic_ast != NULL && n_params == 1 ) {
    print_error( &variadic_ast->loc,
      "variadic specifier can not be only parameter%s\n",
      C_LANG_WHICH( VARIADIC_ONLY_PARAMS )
    );
    return false;
  }

  return c_ast_check_func_params_redef( ast );

only_void:
  print_error( &void_ast->loc,
    "\"void\" must be only parameter if specified\n"
  );
  return false;
}

/**
 * Checks all function parameters for semantic errors in K&R C.
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
    switch ( param_ast->kind ) {
      case K_NAME:
        break;
      case K_VARIADIC:
        print_error( &param_ast->loc,
          "ellipsis not supported%s\n",
          C_LANG_WHICH( PROTOTYPES )
        );
        return false;
      default:
        print_error( &param_ast->loc,
          "function prototypes not supported%s\n",
          C_LANG_WHICH( PROTOTYPES )
        );
        return false;
      CASE_K_PLACEHOLDER;
    } // switch
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
    FOREACH_AST_FUNC_PARAM_UNTIL( prev_param, ast, param ) {
      c_ast_t const *const prev_param_ast = c_param_ast( prev_param );
      if ( c_sname_empty( &prev_param_ast->sname ) )
        continue;
      if ( c_sname_cmp( &param_ast->sname, &prev_param_ast->sname ) == 0 ) {
        print_error( &param_ast->loc,
          "\"%s\": redefinition of parameter\n",
          c_sname_full_name( &param_ast->sname )
        );
        return false;
      }
    } // for
  } // for

  return true;
}

/**
 * Checks a lambda for semantic errors.
 *
 * @param ast The lambda AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_lambda( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );

  c_tid_t const stids = ast->type.stids & c_tid_compl( TS_LAMBDA );
  if ( stids != TS_NONE ) {
    print_error( &ast->loc,
      "%s can not be %s\n",
      c_kind_name( ast->kind ),
      c_tid_name_error( stids )
    );
    return false;
  }

  return c_ast_check_lambda_captures( ast ) &&
         c_ast_check_lambda_captures_redef( ast );
}

/**
 * Checks lambda captures for semantic errors.
 *
 * @param ast The lambda AST to check.
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
        if ( default_capture_ast != NULL ) {
prev:     print_error( &capture_ast->loc,
            "default capture previously specified\n"
          );
          return false;
        }
        if ( n_captures > 1 ) {
first:    print_error( &capture_ast->loc,
            "default capture must be specified first\n"
          );
          return false;
        }
        assert( default_capture_ast == NULL );
        default_capture_ast = capture_ast;
        break;

      case C_CAPTURE_REFERENCE:
        if ( c_sname_empty( &capture_ast->sname ) ) {
          if ( default_capture_ast != NULL )
            goto prev;
          if ( n_captures > 1 )
            goto first;
          assert( default_capture_ast == NULL );
          default_capture_ast = capture_ast;
        }
        else {
          if ( default_capture_ast != NULL &&
               default_capture_ast->capture.kind == C_CAPTURE_REFERENCE ) {
            print_error( &capture_ast->loc,
              "default capture is already by reference\n"
            );
            return false;
          }
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
 * Checks lambda captures for redefinition (duplicate names, `this`, or
 * `*this`).
 *
 * @param ast The lambda AST to check.
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
          c_sname_full_name( &capture_ast->sname )
        );
        return false;
      }
    } // for
  } // for

  return true;
}

/**
 * Checks an overloaded operator AST for errors.
 *
 * @param ast The overloaded operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  c_operator_t const *const op = ast->oper.operator;

  if ( op->overload == C_OVERLOAD_NONE ) {
    print_error( &ast->loc,
      "operator %s can not be overloaded\n", op->literal
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

  if ( !c_ast_oper_mbr_matches( ast, op ) ) {
    //
    // The user explicitly specified either member or non-member, but the
    // operator can't be that.
    //
    print_error( &ast->loc,
      "operator %s can only be a %s\n",
      op->literal, op->overload == C_OVERLOAD_MEMBER ? L_member : H_non_member
    );
    return false;
  }

  if ( op->overload == C_OVERLOAD_MEMBER &&
       c_tid_is_any( ast->type.stids, TS_static ) ) {
    c_lang_id_t ok_lang_ids = LANG_NONE;
    switch ( op->oper_id ) {
      case C_OP_PARENS:
        if ( OPT_LANG_IS( STATIC_OP_PARENS ) )
          break;
        ok_lang_ids = LANG_STATIC_OP_PARENS;
        FALLTHROUGH;
      default:
        print_error( &ast->loc,
          "operator %s must be non-static%s\n",
          op->literal, c_lang_which( ok_lang_ids )
        );
        return false;
    } // switch
  }

  if ( c_oper_is_new_delete( op->oper_id ) &&
       c_tid_is_any( ast->type.stids, c_tid_compl( TS_NEW_DELETE_OPER ) ) ) {
    //
    // Special case for operators new, new[], delete, and delete[] that can
    // only have specific types.
    //
    print_error( &ast->loc,
      "operator %s can not be %s\n",
      op->literal, c_type_name_error( &ast->type )
    );
    return false;
  }

  c_ast_t const *const ret_ast = ast->oper.ret_ast;

  switch ( op->oper_id ) {
    case C_OP_ARROW:
      //
      // Special case for operator-> that must return a pointer to a struct,
      // union, or class.
      //
      if ( !c_ast_is_ptr_to_kind_any( ret_ast, K_CLASS_STRUCT_UNION ) ) {
        print_error( &ret_ast->loc,
          "operator %s must return a pointer to struct, union, or class\n",
          op->literal
        );
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
          "operator %s must return void\n", op->literal
        );
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
          "operator %s must return a pointer to void\n", op->literal
        );
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
 * Checks overloaded operators that are marked `= default`.
 *
 * @param ast The defaulted operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_default( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( c_tid_is_any( ast->type.stids, TS_default ) );

  switch ( ast->oper.operator->oper_id ) {
    case C_OP_EQUAL:
      //
      // Detailed checks for defaulted assignment operators are done in
      // c_ast_check_func().
      //
      break;

    case C_OP_EQUAL2:
    case C_OP_EXCLAM_EQUAL:
    case C_OP_GREATER:
    case C_OP_GREATER_EQUAL:
    case C_OP_LESS:
    case C_OP_LESS_EQUAL:
    case C_OP_LESS_EQUAL_GREATER:
      return c_ast_check_oper_relational_default( ast );

    default:
      print_error( &ast->loc,
        "only operator =%s operators can be default\n",
        OPT_LANG_IS( default_RELOPS ) ? " and relational" : ""
      );
      return false;
  } // switch

  return true;
}

/**
 * Checks overloaded operator `delete` and `delete[]` parameters for semantic
 * errors.
 *
 * @param ast The overloaded operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_delete_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( ast->oper.operator->oper_id == C_OP_DELETE ||
          ast->oper.operator->oper_id == C_OP_DELETE_ARRAY );

  // minimum number of parameters checked in c_ast_check_oper_params()

  c_param_t const *const param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *const param_ast = c_param_ast( param );

  if ( !c_ast_is_ptr_to_tid_any( param_ast, TB_void | TB_ANY_CLASS ) ) {
    print_error( &param_ast->loc,
      "invalid parameter type for operator %s; "
      "must be a pointer to void, class, struct, or union\n",
      ast->oper.operator->literal
    );
    return false;
  }

  return true;
}

/**
 * Checks overloaded operator `new` and `new[]` parameters for semantic errors.
 *
 * @param ast The overloaded operator `new` AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_new_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( ast->oper.operator->oper_id == C_OP_NEW ||
          ast->oper.operator->oper_id == C_OP_NEW_ARRAY );

  // minimum number of parameters checked in c_ast_check_oper_params()

  c_param_t const *const param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *const param_ast = c_param_ast( param );

  if ( !c_ast_is_size_t( param_ast ) ) {
    print_error( &param_ast->loc,
      "invalid parameter type for operator %s; "
      "must be std::size_t (or equivalent)\n",
      ast->oper.operator->literal
    );
    return false;
  }

  return true;
}

/**
 * Checks all overloaded operator parameters for semantic errors.
 *
 * @param ast The overloaded operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  c_operator_t const *const op = ast->oper.operator;
  c_func_member_t const member = c_ast_oper_overload( ast );
  char const *const member_or_nonmember =
    member == C_FUNC_MEMBER     ? "member "     :
    member == C_FUNC_NON_MEMBER ? "non-member " :
    "";

  //
  // Determine the minimum and maximum number of parameters the operator can
  // have based on whether it's a member, non-member, or unspecified.
  //
  bool const is_ambiguous = c_oper_is_ambiguous( op );
  unsigned req_params_min = 0, req_params_max = 0;
  bool const max_params_unlimited = op->params_max == C_OPER_PARAMS_UNLIMITED;
  switch ( member ) {
    case C_FUNC_NON_MEMBER:
      // Non-member operators must always take at least one parameter (the
      // enum, class, struct, or union for which it's overloaded).
      req_params_min = is_ambiguous || max_params_unlimited ?
        1 : op->params_max;
      req_params_max = op->params_max;
      break;
    case C_FUNC_MEMBER:
      if ( !max_params_unlimited ) {
        req_params_min = op->params_min;
        req_params_max = is_ambiguous ? 1 : op->params_min;
        break;
      }
      FALLTHROUGH;
    case C_FUNC_UNSPECIFIED:
      req_params_min = op->params_min;
      req_params_max = op->params_max;
      break;
  } // switch

  //
  // Ensure the operator has the required number of parameters.
  //
  size_t const n_params = c_ast_params_count( ast );
  if ( n_params < req_params_min ) {
    if ( req_params_min == req_params_max )
same: print_error( c_ast_params_loc( ast ),
        "%soperator %s must have exactly %u parameter%s\n",
        member_or_nonmember, op->literal,
        req_params_min, plural_s( req_params_min )
      );
    else
      print_error( c_ast_params_loc( ast ),
        "%soperator %s must have at least %u parameter%s\n",
        member_or_nonmember, op->literal,
        req_params_min, plural_s( req_params_min )
      );
    return false;
  }
  if ( n_params > req_params_max ) {
    if ( op->params_min == req_params_max )
      goto same;
    print_error( c_ast_params_loc( ast ),
      "%soperator %s can have at most %u parameter%s\n",
      member_or_nonmember, op->literal,
      op->params_max, plural_s( op->params_max )
    );
    return false;
  }

  //
  // Count the number of enum, class, struct, or union parameters, or lvalue or
  // rvalue references thereto.
  //
  unsigned ecsu_param_count = 0;
  FOREACH_AST_FUNC_PARAM( param, ast ) {
    //
    // Normally we could use c_param_ast( ast )->kind directly, but we need to
    // count objects and lvalue references to objects distinctly to check
    // default relational operators in C++20.
    //
    c_ast_t const *param_ast = c_ast_untypedef( c_param_ast( param ) );
    switch ( param_ast->kind ) {
      case K_CLASS_STRUCT_UNION:
      case K_ENUM:
        ++ecsu_param_count;
        break;
      case K_REFERENCE:
        param_ast = c_ast_unreference( param_ast );
        if ( (param_ast->kind & K_ANY_ECSU) != 0 )
          ++ecsu_param_count;
        break;
      case K_RVALUE_REFERENCE:
        param_ast = c_ast_unrvalue_reference( param_ast );
        if ( (param_ast->kind & K_ANY_ECSU) != 0 )
          ++ecsu_param_count;
        break;
      default:
        /* suppress warning */;
    } // switch
  } // for

  switch ( member ) {
    case C_FUNC_NON_MEMBER:
      //
      // Ensure non-member operators (except new, new[], delete, and delete[])
      // have at least one enum, class, struct, or union parameter.
      //
      if ( !c_oper_is_new_delete( op->oper_id ) && ecsu_param_count == 0 ) {
        print_error( c_ast_params_loc( ast ),
          "at least 1 parameter of a non-member operator must be an "
          "enum, class, struct, or union"
          "; or a reference or rvalue reference thereto\n"
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
        print_error( &ast->loc, "member operators can not be friend\n" );
        return false;
      }
      break;

    case C_FUNC_UNSPECIFIED:
      // nothing to do
      break;
  } // switch

  switch ( op->oper_id ) {
    case C_OP_MINUS2:
    case C_OP_PLUS2: {
      //
      // Ensure that the dummy parameter for postfix -- or ++ is type int (or
      // is a typedef of int).
      //
      c_param_t const *param = c_ast_params( ast );
      if ( param == NULL )              // member prefix
        break;
      if ( member == C_FUNC_NON_MEMBER ) {
        param = param->next;
        if ( param == NULL )            // non-member prefix
          break;
      }
      // At this point, it's either member or non-member postfix:
      // operator++(int) or operator++(S&,int).
      c_ast_t const *const param_ast = c_param_ast( param );
      if ( !c_ast_is_builtin_any( param_ast, TB_int ) ) {
        print_error( &param_ast->loc,
          "parameter of postfix %soperator %s must be int\n",
          member_or_nonmember, op->literal
        );
        return false;
      }
      break;
    }

    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      return c_ast_check_oper_delete_params( ast );

    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
      return c_ast_check_oper_new_params( ast );

    default:
      /* suppress warning */;
  } // switch

  return true;
}

/**
 * Checks overloaded relational operators that are marked `= default`.
 *
 * @param ast The defaulted relational operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_oper_relational_default( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );
  assert( c_tid_is_any( ast->type.stids, TS_default ) );

  // number of parameters checked in c_ast_check_oper_params()

  c_operator_t const *const op = ast->oper.operator;

  if ( !OPT_LANG_IS( default_RELOPS ) ) {
    print_error( &ast->loc,
      "default operator %s not supported%s\n",
      op->literal, C_LANG_WHICH( default_RELOPS )
    );
    return false;
  }

  c_param_t const *const param = c_ast_params( ast );
  assert( param != NULL );
  c_ast_t const *const param_ast = c_param_ast( param );

  switch ( c_ast_oper_overload( ast ) ) {
    case C_FUNC_NON_MEMBER: {
      if ( !c_tid_is_any( ast->type.stids, TS_friend ) ) {
        print_error( &ast->loc,
          "default non-member operator %s must also be friend\n", op->literal
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
        param1_ast = c_ast_is_ref_to_type_any( param_ast, &T_ANY_CONST_CLASS );
        if ( param1_ast == NULL ) {
rel_2par: print_error( &ast->loc,
            "default non-member relational operators must take two "
            "value or reference-to-const parameters of the same class\n"
          );
          return false;
        }
        param1_is_ref_to_class = true;
      }

      c_ast_t const *param2_ast = c_param_ast( param->next );
      param2_ast = param1_is_ref_to_class ?
        c_ast_is_ref_to_type_any( param2_ast, &T_ANY_CONST_CLASS ) :
        c_ast_is_tid_any( param2_ast, TB_ANY_CLASS );
      if ( param2_ast == NULL || param1_ast != param2_ast )
        goto rel_2par;
      break;
    }

    case C_FUNC_MEMBER: {
      if ( !c_tid_is_any( ast->type.stids, TS_const ) ) {
        print_error( &ast->loc,
          "default member operator %s must also be const\n", op->literal
        );
        return false;
      }

      //
      // Default member relational operators must take one class parameter by
      // either value or reference-to-const.
      //
      c_ast_t const *param1_ast = c_ast_is_tid_any( param_ast, TB_ANY_CLASS );
      if ( param1_ast == NULL ) {
        param1_ast = c_ast_is_ref_to_type_any( param_ast, &T_ANY_CONST_CLASS );
        if ( param1_ast == NULL ) {
          print_error( c_ast_params_loc( ast ),
            "default member relational operators must take one "
            "value or reference-to-const parameter to a class\n"
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

  c_ast_t const *const ret_ast = ast->oper.ret_ast;
  c_ast_t const *const raw_ret_ast = c_ast_untypedef( ret_ast );

  if ( op->oper_id == C_OP_LESS_EQUAL_GREATER ) {
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
    if ( !c_ast_is_builtin_any( ret_ast, TB_auto ) &&
         raw_ret_ast != std_partial_ordering_ast &&
         raw_ret_ast != std_strong_ordering_ast &&
         raw_ret_ast != std_weak_ordering_ast ) {
      print_error( &ret_ast->loc,
        "operator %s must return one of auto, "
        "std::partial_ordering, "
        "std::strong_ordering, or "
        "std::weak_ordering\n",
        op->literal
      );
      return false;
    }
  }
  else if ( !c_ast_is_builtin_any( ret_ast, TB_bool ) ) {
    print_error( &ret_ast->loc,
      "operator %s must return %s\n",
      op->literal, c_tid_name_error( TB_bool )
    );
    return false;
  }

  return true;
}

/**
 * Checks a pointer or pointer-to-member AST for errors.
 *
 * @param ast The pointer or pointer-to-member AST to check.
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
      error_kind_to_kind( ast, raw_to_ast, "" );
      if ( raw_to_ast == to_ast ) {
        if ( cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH )
          print_hint( "reference to pointer" );
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
          "\"auto\" with pointer declarator not supported%s\n",
          C_LANG_WHICH( auto_POINTER_TYPES )
        );
        return false;
      }
      FALLTHROUGH;
    default:
      if ( c_tid_is_any( ast->type.atids, TA_ANY_MSC_CALL ) ) {
        print_error( &ast->loc,
          "\"%s\": can be used only for functions and pointers to function\n",
          c_tid_name_error( ast->type.atids )
        );
        return false;
      }
  } // switch

  if ( c_ast_is_register( to_ast ) ) {
    error_kind_to_tid( ast, TS_register, "\n" );
    return false;
  }

  return true;
}

/**
 * Checks a reference or rvalue reference AST for errors.
 *
 * @param ast The pointer or pointer-to-member AST to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_reference( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_REFERENCE ) );

  if ( c_tid_is_any( ast->type.stids, TS_CV ) ) {
    c_tid_t const qual_stids = ast->type.stids & TS_ANY_QUALIFIER;
    error_kind_not_tid( ast, qual_stids, LANG_NONE, "" );
    print_hint( "reference to %s", c_tid_name_error( qual_stids ) );
    return false;
  }

  c_ast_t const *const to_ast = ast->ptr_ref.to_ast;

  if ( c_ast_is_builtin_any( to_ast, TB_void ) ) {
    error_kind_to_tid( ast, TB_void, "" );
    print_hint( "pointer to void" );
    return false;
  }

  return true;
}

/**
 * Checks the return type of a function-like AST for errors.
 *
 * @param ast The function-like AST to check the return type of.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool c_ast_check_ret_type( c_ast_t const *ast ) {
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
          "%s returning \"auto\" not supported%s\n",
          kind_name,
          C_LANG_WHICH( auto_RETURN_TYPES )
        );
        return false;
      }
      break;
    case K_CLASS_STRUCT_UNION:
      if ( !OPT_LANG_IS( CSU_RETURN_TYPES ) ) {
        print_error( &ret_ast->loc,
          "%s returning %s not supported%s\n",
          kind_name,
          c_kind_name( raw_ret_ast->kind ),
          C_LANG_WHICH( CSU_RETURN_TYPES )
        );
        return false;
      }
      break;
    case K_FUNCTION:
    case K_OPERATOR:
    case K_UDEF_LIT:
      print_error( &ret_ast->loc,
        "%s returning %s is illegal",
        kind_name, c_kind_name( raw_ret_ast->kind )
      );
      print_hint( "%s returning pointer to function", kind_name );
      return false;
    default:
      /* suppress warning */;
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
 * Checks a user-defined conversion operator AST for errors.
 *
 * @param ast The user-defined conversion operator AST to check.
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
      "user-defined conversion operator can not convert to an array"
    );
    print_hint( "pointer to array" );
    return false;
  }

  return  c_ast_check_ret_type( ast ) &&
          c_ast_check_func( ast ) &&
          c_ast_check_func_params( ast );
}

/**
 * Checks all user-defined literal parameters for semantic errors.
 *
 * @param ast The user-defined literal AST to check.
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

  size_t const n_params = c_ast_params_count( ast );
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
          if ( !c_ast_is_ptr_to_type_any( param_ast,
                  &T_ANY, &C_TYPE_LIT( TB_char, TS_const, TA_NONE ) ) ) {
            print_error( &param_ast->loc,
              "invalid parameter type for user-defined literal; "
              "must be one of: "
              "unsigned long long, "
              "long double, "
              "char, "
              "const char*, "
              "%s"
              "char16_t, "
              "char32_t, "
              "or wchar_t\n",
              OPT_LANG_IS( char8_t ) ? "char8_t, " : ""
            );
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
          "invalid parameter type for user-defined literal; must be one of: "
          "const (char%s|char16_t|char32_t|wchar_t)*\n",
          OPT_LANG_IS( char8_t ) ? "|char8_t" : ""
        );
        return false;
      }
      param_ast = c_param_ast( param->next );
      if ( param_ast == NULL || !c_ast_is_size_t( param_ast ) ) {
        print_error( &param_ast->loc,
          "invalid parameter type for user-defined literal; "
          "must be std::size_t (or equivalent)\n"
        );
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
 * Checks a built-in Unified Parallel C type AST for errors.
 *
 * @param ast The built-in AST to check.
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
      "\"%s\" requires \"shared\"\n", c_type_name_error( &ast->type )
    );
    return false;
  }

  return true;
}

/**
 * Compares the name of \a ast to \a name for equality.
 *
 * @param ast The AST to check.
 * @param name The name to check for.
 * @return Returns `true` only if the name of \a ast is equal to \a name.
 */
NODISCARD
static bool c_ast_name_equal( c_ast_t const *ast, char const *name ) {
  assert( ast != NULL );
  assert( name != NULL );

  SNAME_VAR_INIT( sname, name );
  return c_sname_cmp( &ast->sname, &sname ) == 0;
}

/**
 * Visitor function that checks an AST for semantic errors.
 *
 * @param ast The AST to check.
 * @param data The data to use.
 * @return Returns \ref VISITOR_ERROR_FOUND if an error was found;
 * \ref VISITOR_ERROR_NOT_FOUND if not.
 */
NODISCARD
static bool c_ast_visitor_error( c_ast_t const *ast, user_data_t data ) {
  assert( ast != NULL );
  c_state_t const *const c = data.pc;

  if ( !c_ast_check_alignas( ast ) )
    return VISITOR_ERROR_FOUND;

  switch ( ast->kind ) {
    case K_ARRAY:
      if ( !c_ast_check_array( ast, c ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_BUILTIN:
      if ( !c_ast_check_builtin( ast, c ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_CAPTURE:
      // checked in c_ast_check_lambda_captures()
      break;

    case K_CAST:
      if ( !c_ast_check_cast( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_CLASS_STRUCT_UNION:
      // nothing to check
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
      if ( !c_ast_check_ret_type( ast ) )
        return VISITOR_ERROR_FOUND;
      FALLTHROUGH;

    case K_CONSTRUCTOR:
      if ( !(c_ast_check_func( ast ) && c_ast_check_func_params( ast )) )
        return VISITOR_ERROR_FOUND;
      FALLTHROUGH;

    case K_DESTRUCTOR: {
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
    }

    case K_LAMBDA:
      if ( !(c_ast_check_lambda( ast ) &&
             c_ast_check_func_params( ast ) &&
             c_ast_check_ret_type( ast )) ) {
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
      if ( !c_ast_check_reference( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_TYPEDEF: {
      //
      // K_TYPEDEF isn't a "parent" kind since it's not a parent "of" the
      // underlying type, but instead a synonym "for" it.  Hence, we have to
      // recurse into it manually.
      //
      c_ast_t const temp_ast = c_ast_sub_typedef( ast );
      c_state_t temp_c;
      MEM_ZERO( &temp_c );
      temp_c.is_pointee = c_ast_parent_is_kind( ast, K_POINTER );
      data.pc = &temp_c;
      return c_ast_visitor_error( &temp_ast, data );
    }

    case K_UDEF_CONV:
      if ( !c_ast_check_udef_conv( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_UDEF_LIT:
      if ( !(c_ast_check_ret_type( ast ) &&
             c_ast_check_func( ast ) &&
             c_ast_check_udef_lit_params( ast )) ) {
        return VISITOR_ERROR_FOUND;
      }
      break;

    case K_VARIADIC:
      assert( c->func_ast != NULL );
      break;

    CASE_K_PLACEHOLDER;
  } // switch

  if ( ast->kind != K_FUNCTION &&
       c_tid_is_any( ast->type.stids, TS_consteval ) ) {
    print_error( &ast->loc, "only functions can be consteval\n" );
    return VISITOR_ERROR_FOUND;
  }

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for type errors.
 *
 * @param ast The AST to visit.
 * @param data The data to use.
 * @return Returns \ref VISITOR_ERROR_FOUND if an error was found;
 * \ref VISITOR_ERROR_NOT_FOUND if not.
 */
NODISCARD
static bool c_ast_visitor_type( c_ast_t const *ast, user_data_t data ) {
  assert( ast != NULL );
  (void)data;

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
        c_type_name_error( &ast->type ), c_lang_which( ok_lang_ids )
      );
    } else {
      print_error( &ast->loc,
        "\"%s\" is illegal for %s%s\n",
        c_type_name_error( &ast->type ),
        c_kind_name( ast->kind ),
        c_lang_which( ok_lang_ids )
      );
    }
    return VISITOR_ERROR_FOUND;
  }

  if ( (ast->kind & K_ANY_FUNCTION_LIKE) != 0 ) {
    if ( c_tid_is_any( ast->type.stids, TS_constexpr ) &&
         !OPT_LANG_IS( constexpr_RETURN_TYPES ) &&
         c_ast_is_builtin_any( ast->func.ret_ast, TB_void ) ) {
      print_error( &ast->loc,
        "%s %s is illegal%s\n",
        c_tid_name_error( ast->type.stids ),
        c_tid_name_error( ast->func.ret_ast->type.btids ),
        C_LANG_WHICH( constexpr_RETURN_TYPES )
      );
      return VISITOR_ERROR_FOUND;
    }
  }
  else {
    if ( ast->kind != K_ARRAY &&
         c_tid_is_any( ast->type.stids, TS_NON_EMPTY_ARRAY ) ) {
      // Can't use error_kind_not_tid() here because we need to call
      // c_tid_name_english() for TS_NON_EMPTY_ARRAY, not c_tid_name_error().
      print_error( &ast->loc,
        "%s can not be %s\n",
        c_kind_name( ast->kind ), c_tid_name_english( TS_NON_EMPTY_ARRAY )
      );
      return VISITOR_ERROR_FOUND;
    }

    if ( c_tid_is_any( ast->type.stids, TS_constexpr ) &&
         OPT_LANG_IS( C_ANY ) &&
         c_tid_is_any( ast->type.stids, TS_NOT_CONSTEXPR_C_ONLY ) ) {
      error_tid_not_tid( ast,
        TS_constexpr, ast->type.stids & TS_NOT_CONSTEXPR_C_ONLY, " in C\n"
      );
      return VISITOR_ERROR_FOUND;
    }

    c_tid_t const not_object_atids = ast->type.atids & c_tid_compl( TA_OBJECT );
    if ( not_object_atids != TA_NONE ) {
      error_kind_not_tid( ast, not_object_atids, LANG_NONE, "\n" );
      return VISITOR_ERROR_FOUND;
    }
  }

  if ( c_ast_is_tid_any( ast, TS_restrict ) ) {
    c_ast_t const *const raw_ast = c_ast_untypedef( ast );
    switch ( raw_ast->kind ) {
      case K_ARRAY:                     // legal in C; __restrict legal in C++
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
            "pointer to %s can not be %s\n",
            c_kind_name( c_ast_unpointer( ast )->kind ),
            c_tid_name_error( TS_restrict )
          );
          return VISITOR_ERROR_FOUND;
        }
        break;
      default:
        error_kind_not_tid( raw_ast, TS_restrict, LANG_NONE, "\n" );
        return VISITOR_ERROR_FOUND;
    } // switch
  }

  if ( (ast->kind & K_ANY_FUNCTION_LIKE) != 0 ) {
    c_state_t param_c;
    MEM_ZERO( &param_c );
    param_c.func_ast = ast;
    FOREACH_AST_FUNC_PARAM( param, ast ) {
      c_ast_t const *const param_ast = c_param_ast( param );
      if ( !c_ast_check_visitor( param_ast, c_ast_visitor_type, &param_c ) )
        return VISITOR_ERROR_FOUND;
    } // for
  }

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for semantic warnings.
 *
 * @param ast The AST to check.
 * @param data Not used.
 * @return Always returns `false`.
 */
NODISCARD
static bool c_ast_visitor_warning( c_ast_t const *ast, user_data_t data ) {
  assert( ast != NULL );
  (void)data;

  switch ( ast->kind ) {
    case K_ARRAY:
    case K_BUILTIN:
    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
      if ( c_ast_is_register( ast ) && OPT_LANG_IS( MIN(CPP_11) ) ) {
        print_warning( &ast->loc,
          "\"register\" is deprecated%s\n",
          C_LANG_WHICH( MAX(CPP_03) )
        );
      }
      break;

    case K_UDEF_LIT:
      if ( c_sname_local_name( &ast->sname )[0] != '_' )
        print_warning( &ast->loc,
          "user-defined literals not starting with '_' are reserved\n"
        );
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
    case K_LAMBDA:
    case K_OPERATOR: {
      c_ast_t const *const ret_ast = ast->func.ret_ast;
      if ( ret_ast != NULL ) {
        c_tid_t qual_stids;
        PJL_IGNORE_RV( c_ast_untypedef_qual( ret_ast, &qual_stids ) );
        if ( c_tid_is_any( qual_stids, TS_volatile ) &&
            OPT_LANG_IS( CPP_MIN(20) ) ) {
          print_warning( &ret_ast->loc,
            "\"volatile\" return types are deprecated%s\n",
            C_LANG_WHICH( CPP_MAX(17) )
          );
        }
        if ( c_tid_is_any( ast->type.atids, TA_nodiscard ) &&
            c_ast_is_builtin_any( ret_ast, TB_void ) ) {
          print_warning( &ret_ast->loc,
            "[[nodiscard]] %ss can not return void\n",
            c_kind_name( ast->kind )
          );
        }
      }
      FALLTHROUGH;
    }

    case K_CONSTRUCTOR: {
      c_state_t param_c;
      MEM_ZERO( &param_c );
      param_c.func_ast = ast;
      FOREACH_AST_FUNC_PARAM( param, ast ) {
        c_ast_t const *const param_ast = c_param_ast( param );
        PJL_IGNORE_RV(
          c_ast_check_visitor( param_ast, c_ast_visitor_warning, &param_c )
        );
        if ( c_tid_is_any( param_ast->type.stids, TS_volatile ) &&
             OPT_LANG_IS( CPP_MIN(20) ) ) {
          print_warning( &param_ast->loc,
            "\"volatile\" parameter types are deprecated%s\n",
            C_LANG_WHICH( CPP_MAX(17) )
          );
        }
      } // for
      FALLTHROUGH;
    }

    case K_DESTRUCTOR:
      if ( c_tid_is_any( ast->type.stids, TS_throw ) &&
           OPT_LANG_IS( noexcept ) ) {
        print_warning( &ast->loc,
          "\"throw\" is deprecated%s",
          C_LANG_WHICH( CPP_MAX(03) )
        );
        print_hint( "\"noexcept\"" );
      }
      break;

    case K_NAME:
      if ( OPT_LANG_IS( PROTOTYPES ) )
        print_warning( &ast->loc, "missing type specifier; int assumed\n" );
      break;

    case K_CAPTURE:
    case K_CAST:
    case K_UDEF_CONV:
    case K_VARIADIC:
      // nothing to check
      break;

    CASE_K_PLACEHOLDER;
  } // switch

  if ( cdecl_initialized )              // don't warn for predefined types
    c_ast_warn_name( ast );

  return /*stop=*/false;
}

/**
 * Checks an AST's name(s) for warnings.
 *
 * @param ast The AST to check.
 */
static void c_ast_warn_name( c_ast_t const *ast ) {
  assert( ast != NULL );

  c_sname_warn( &ast->sname, &ast->loc );
  switch ( ast->kind ) {
    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_POINTER_TO_MEMBER:
      c_sname_warn( &ast->csu.csu_sname, &ast->loc );
      break;
    default:
      /* suppress warning */;
  } // switch
}

/**
 * Checks a scoped name for warnings.
 *
 * @param sname The scoped name to check.
 * @param loc The location of \a sname.
 */
static void c_sname_warn( c_sname_t const *sname, c_loc_t const *loc ) {
  assert( sname != NULL );

  FOREACH_SNAME_SCOPE( scope, sname ) {
    char const *const name = c_scope_data( scope )->name;

    // First, check to see if the name is a keyword in some other language.
    c_keyword_t const *const ck =
      c_keyword_find( name, LANG_ANY, C_KW_CTX_DEFAULT );
    if ( ck != NULL ) {
      print_warning( loc,
        "\"%s\" is a keyword in %s\n",
        name, c_lang_name( c_lang_oldest( ck->lang_ids ) )
      );
      continue;
    }

    // Next, check to see if the name is reserved in any language.
    c_lang_id_t const reserved_lang_ids = is_reserved_name( name );
    if ( reserved_lang_ids != LANG_NONE ) {
      print_warning( loc, "\"%s\" is a reserved identifier", name );
      char const *const coarse_name = c_lang_coarse_name( reserved_lang_ids );
      if ( coarse_name != NULL )
        EPRINTF( " in %s", coarse_name );
      EPUTC( '\n' );
    }
  } // for
}

/**
 * Checks whether \a name is reserved in any language.  A name is reserved if
 * it matches any of these patterns:
 *
 *      _*          // C: external only; C++: global namespace only.
 *      _[A-Z_]*
 *      *__*        // C++ only.
 *
 * However, we don't check for the first one since **cdecl** doesn't have
 * either the linkage or the scope of a name.
 *
 * @param name The name to check.
 * @return Returns the bitwise-or of language(s) that \a name is reserved in.
 */
NODISCARD
static c_lang_id_t is_reserved_name( char const *name ) {
  assert( name != NULL );

  if ( name[0] == '_' && (isupper( name[1] ) || name[1] == '_') )
    return LANG_ANY;

  if ( strstr( name, "__" ) != NULL )
    return LANG_CPP_ANY;

  return LANG_NONE;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_state_t c;
  MEM_ZERO( &c );
  if ( !c_ast_check_errors( ast, &c ) )
    return false;
  PJL_IGNORE_RV(
    c_ast_check_visitor( ast, c_ast_visitor_warning, &c )
  );
  return true;
}

bool c_ast_list_check( c_ast_list_t const *ast_list ) {
  assert( ast_list != NULL );

  size_t const ast_count = slist_len( ast_list );
  if ( ast_count == 0 )
    return true;

  c_ast_t const *const first_ast = slist_front( ast_list );
  if ( first_ast->type.btids == TB_auto && ast_count > 1 &&
       !OPT_LANG_IS( auto_TYPE_MULTI_DECL ) ) {
    print_error( &first_ast->loc,
      "\"auto\" with multiple declarators is not supported%s\n",
      C_LANG_WHICH( auto_TYPE_MULTI_DECL )
    );
    return false;
  }

  c_ast_t const *prev_ast = NULL;
  FOREACH_SLIST_NODE( node, ast_list ) {
    c_ast_t const *const ast = node->data;
    //
    // Ensure that a name is not used more than once in the same declaration in
    // C++ or with different types in C.  (In C, more than once with the same
    // type are "tentative definitions" and OK.)
    //
    //      int i, i;                   // OK in C (same type); error in C++
    //      int j, *j;                  // error (different types)
    //
    if ( prev_ast != NULL &&
         c_sname_cmp( &ast->sname, &prev_ast->sname ) == 0 ) {
      if ( !OPT_LANG_IS( TENTATIVE_DEFS ) ) {
        print_error( &ast->loc,
          "\"%s\": redefinition\n",
          c_sname_full_name( &ast->sname )
        );
        return false;
      }
      else if ( !c_ast_equal( ast, prev_ast ) ) {
        print_error( &ast->loc,
          "\"%s\": redefinition with different type\n",
          c_sname_full_name( &ast->sname )
        );
        return false;
      }
    }

    if ( !c_ast_check( ast ) )
      return false;
    prev_ast = ast;
  } // for

  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
