/*
**      cdecl -- C gibberish translator
**      src/check.c
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
 * Defines functions for performing semantic checks.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "check.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_type.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "color.h"
#include "did_you_mean.h"
#include "english.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints an error: `can not cast into <kind>`.
 *
 * @param AST The AST.
 * @param HINT The hint.
 */
#define error_kind_not_cast_into(AST,HINT) BLOCK(                     \
  fl_print_error( __FILE__, __LINE__, &(AST)->loc,                    \
    "can not %s %s %s", L_CAST, L_INTO, c_kind_name( (AST)->kind_id ) \
  );                                                                  \
  print_hint( "%s %s %s", L_CAST, L_INTO, (HINT) ); )

/**
 * Prints an error: `<kind> is not supported[ {in|until} <lang>]`.
 *
 * @param AST The AST having the unsupported kind.
 * @param LANG_IDS The bitwise-or of legal language(s).
 */
#define error_kind_not_supported(AST,LANG_IDS)              \
  fl_print_error( __FILE__, __LINE__,                       \
    &(AST)->loc, "%s is not supported%s\n",                 \
    c_kind_name( (AST)->kind_id ), c_lang_which( LANG_IDS ) \
  )

/**
 * Prints an error: `<kind> can not be <type>`.
 *
 * @param AST The AST .
 * @param TID The bad type.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_not_tid(AST,TID,END_STR_LIT)             \
  fl_print_error( __FILE__, __LINE__,                       \
    &(AST)->loc, "%s can not be %s" END_STR_LIT,            \
    c_kind_name( (AST)->kind_id ), c_tid_name_error( TID )  \
  )

/**
 * Prints an error: `<kind> to <kind> is illegal`.
 *
 * @param AST1 The AST having the bad kind.
 * @param AST2 The AST having the other kind.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_to_kind(AST1,AST2,END_STR_LIT)                   \
  fl_print_error( __FILE__, __LINE__,                               \
    &(AST1)->loc, "%s to %s is illegal" END_STR_LIT,                \
    c_kind_name( (AST1)->kind_id ), c_kind_name( (AST2)->kind_id )  \
  )

/**
 * Prints an error: `<kind> to <type> is illegal`.
 *
 * @param AST The AST having the bad kind.
 * @param TID The bad type.
 * @param END_STR_LIT A string literal appended to the end of the error message
 * (either `"\n"` or `""`).
 */
#define error_kind_to_tid(AST,TID,END_STR_LIT)              \
  fl_print_error( __FILE__, __LINE__,                       \
    &(AST)->loc, "%s to %s is illegal" END_STR_LIT,         \
    c_kind_name( (AST)->kind_id ), c_tid_name_error( TID )  \
  )

/**
 * Prints an error: `"<name>": unknown <thing>`.
 *
 * @param AST The AST having the unknown name.
 */
#define error_unknown_name(AST) \
  fl_print_error_unknown_name( __FILE__, __LINE__, &(AST)->loc, &(AST)->sname )

// local constants
static bool const VISITOR_ERROR_FOUND     = true;
static bool const VISITOR_ERROR_NOT_FOUND = false;

/// @endcond

// local functions
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_emc( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_c( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_cpp( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main_char_ptr_param( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_params_knr( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_default( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_params( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_relational_default( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_upc( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_name_equal( c_ast_t const*, char const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_error( c_ast_t*, uint64_t );

PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_type( c_ast_t*, uint64_t );

static void c_ast_warn_name( c_ast_t const* );
static void c_sname_warn( c_sname_t const*, c_loc_t const* );

PJL_WARN_UNUSED_RESULT
static c_lang_id_t is_reserved_name( char const* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Simple wrapper around c_ast_visit().
 *
 * @param ast The AST to check.
 * @param visitor The visitor to use.
 * @param data Optional data passed to c_ast_visit().
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static inline bool c_ast_check_visitor( c_ast_t const *ast,
                                        c_ast_visitor_t visitor,
                                        uint64_t data ) {
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  return c_ast_visit( nonconst_ast, C_VISIT_DOWN, visitor, data ) == NULL;
}

/**
 * Gets whether \a ast has the `register` storage class.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast has the `register` storage class.
 */
PJL_WARN_UNUSED_RESULT
static inline bool c_ast_is_register( c_ast_t const *ast ) {
  return c_type_is_tid_any( &ast->type, TS_REGISTER );
}

/**
 * Returns an "s" or not based on \a n to pluralize a word.
 *
 * @param n A quantity.
 * @return Returns the empty string only if \a n == 1; otherwise returns "s".
 */
PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_alignas( c_ast_t const *ast ) {
  assert( ast != NULL );

  if ( ast->align.kind != C_ALIGNAS_NONE ) {
    if ( c_type_is_tid_any( &ast->type, TS_TYPEDEF ) ) {
      print_error( &ast->loc, "types can not be %s\n", L_ALIGNED );
      return false;
    }

    if ( c_ast_is_register( ast ) ) {
      print_error( &ast->loc,
        "\"%s\" can not be combined with \"%s\"\n", alignas_lang(), L_REGISTER
      );
      return false;
    }

    if ( (ast->kind_id & K_ANY_OBJECT) == K_NONE ) {
      print_error( &ast->loc,
        "%s can not be %s\n", c_kind_name( ast->kind_id ), L_ALIGNED
      );
      return false;
    }

    switch ( ast->align.kind ) {
      case C_ALIGNAS_NONE:
        break;
      case C_ALIGNAS_EXPR: {
        unsigned const alignment = ast->align.as.expr;
        if ( !at_most_one_bit_set( alignment ) ) {
          print_error( &ast->loc,
            "\"%u\": alignment must be a power of 2\n", alignment
          );
          return false;
        }
        break;
      }
      case C_ALIGNAS_TYPE:
        return c_ast_check_declaration( ast->align.as.type_ast );
    } // switch
  }

  return true;
}

/**
 * Checks an array AST for errors.
 *
 * @param ast The array AST to check.
 * @param is_func_param If `true`, \a ast is an AST for a function-like
 * parameter.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_array( c_ast_t const *ast, bool is_func_param ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_ARRAY );

  if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE ) {
    if ( !OPT_LANG_IS(C_MIN(99)) ) {
      print_error( &ast->loc,
        "variable length arrays are not supported%s\n",
        c_lang_which( LANG_C_99 )
      );
      return false;
    }
    if ( !is_func_param ) {
      print_error( &ast->loc,
        "variable length arrays are illegal outside of function parameters\n"
      );
      return false;
    }
  }

  if ( ast->as.array.stid != TS_NONE ) {
    if ( !OPT_LANG_IS(C_MIN(99)) ) {
      print_error( &ast->loc,
        "\"%s\" arrays are not supported%s\n",
        c_tid_name_error( ast->as.array.stid ),
        c_lang_which( LANG_C_99 )
      );
      return false;
    }
    if ( !is_func_param ) {
      print_error( &ast->loc,
        "\"%s\" arrays are illegal outside of function parameters\n",
        c_tid_name_error( ast->as.array.stid )
      );
      return false;
    }
  }

  c_ast_t const *const of_ast = ast->as.array.of_ast;
  switch ( of_ast->kind_id ) {
    case K_BUILTIN:
      if ( c_ast_is_builtin_any( of_ast, TB_VOID ) ) {
        print_error( &ast->loc, "%s of %s", L_ARRAY, L_VOID );
        print_hint( "%s of %s to %s", L_ARRAY, L_POINTER, L_VOID );
        return false;
      }
      if ( c_ast_is_register( of_ast ) ) {
        error_kind_not_tid( ast, TS_REGISTER, "\n" );
        return false;
      }
      break;
    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
      print_error( &ast->loc,
        "%s of %s", L_ARRAY, c_kind_name( of_ast->kind_id )
      );
      print_hint( "%s of %s to %s", L_ARRAY, L_POINTER, L_FUNCTION );
      return false;
    case K_NAME:
      error_unknown_name( of_ast );
      return false;
    default:
      /* suppress warning */;
  } // switch

  return true;
}

/**
 * Checks a built-in type AST for errors.
 *
 * @param ast The built-in AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_builtin( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_BUILTIN );

  if ( (ast->parent_ast == NULL ||
        ast->parent_ast->kind_id != K_USER_DEF_CONVERSION) &&
        ast->type.btid == TB_NONE ) {
    print_error( &ast->loc,
      "implicit \"%s\" is illegal in %s and later\n",
      L_INT, c_lang_name( LANG_C_99 )
    );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TS_INLINE ) && opt_lang < LANG_CPP_17 ) {
    print_error( &ast->loc,
      "%s variables are not supported%s\n",
      L_INLINE, c_lang_which( LANG_CPP_17 )
    );
    return false;
  }

  if ( ast->as.builtin.bit_width > 0 ) {
    if ( c_ast_count_name( ast ) > 1 ) {
      print_error( &ast->loc, "scoped names can not have bit-field widths\n" );
      return false;
    }
    if ( c_type_is_tid_any( &ast->type, TS_ANY ) ) {
      print_error( &ast->loc,
        "%s can not have bit-field widths\n",
        c_tid_name_error( ast->type.stid )
      );
      return false;
    }
  }

  if ( c_ast_is_builtin_any( ast, TB_VOID ) && ast->parent_ast == NULL ) {
    print_error( &ast->loc, "variable of %s", L_VOID );
    print_hint( "%s to %s", L_POINTER, L_VOID );
    return false;
  }

  return c_ast_check_emc( ast ) && c_ast_check_upc( ast );
}

/**
 * Checks a constructor or destructor AST for errors.
 *
 * @param ast The constructor or destructor AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_ctor_dtor( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE );

  bool const is_definition = c_ast_count_name( ast ) > 1;

  if ( is_definition && !c_sname_is_ctor( &ast->sname ) ) {
    print_error( &ast->loc,
      "\"%s\", \"%s\": %s and %s names don't match\n",
      c_ast_name_atr( ast, 1 ), c_ast_local_name( ast ),
      c_type_name_error( c_ast_local_type( ast ) ),
      c_kind_name( ast->kind_id )
    );
    return false;
  }

  bool const is_constructor = ast->kind_id == K_CONSTRUCTOR;

  c_tid_t const ok_stid = is_constructor ?
    (is_definition ? TS_CONSTRUCTOR_DEF : TS_CONSTRUCTOR_DECL) :
    (is_definition ? TS_DESTRUCTOR_DEF  : TS_DESTRUCTOR_DECL ) ;

  c_tid_t const stid = ast->type.stid & c_tid_compl( ok_stid );
  if ( stid != TS_NONE ) {
    print_error( &ast->loc,
      "%s%s can not be %s\n",
      c_kind_name( ast->kind_id ),
      is_definition ? " definitions" : "s",
      c_tid_name_error( stid )
    );
    return false;
  }

  return true;
}

/**
 * Checks an `enum`, `class`, `struct`, or `union` AST for errors.
 *
 * @param ast The `enum`, `class`, `struct`, or union AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_ecsu( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_ENUM_CLASS_STRUCT_UNION );

  c_ast_t const *const of_ast = ast->as.ecsu.of_ast;

  if ( c_type_is_tid_any( &ast->type, TB_ENUM ) ) {
    if ( c_mode == C_GIBBERISH_TO_ENGLISH &&
         c_type_is_tid_any( &ast->type, TB_STRUCT | TB_CLASS ) &&
        !c_type_is_tid_any( &ast->type, TS_TYPEDEF ) ) {
      print_error( &ast->loc,
        "\"%s\": %s classes must just use \"%s\"\n",
        c_type_name_error( &ast->type ), L_ENUM, L_ENUM
      );
      return false;
    }

    if ( of_ast != NULL ) {
      if ( opt_lang < LANG_CPP_11 ) {
        print_error( &of_ast->loc,
          "%s with underlying type is not supported%s\n",
          L_ENUM, c_lang_which( LANG_CPP_11 )
        );
        return false;
      }

      if ( !c_ast_is_builtin_any( of_ast, TB_ANY_INTEGRAL ) ) {
        print_error( &of_ast->loc,
          "%s underlying type must be integral\n",
          L_ENUM
        );
        return false;
      }
    }
  }
  else {                                // class, struct, or union
    if ( of_ast != NULL ) {
      print_error( &of_ast->loc,
        "%s can not specify an underlying type\n",
        c_type_name_error( &ast->type )
      );
      return false;
    }
  }

  return true;
}

/**
 * Checks a built-in Embedded C type AST for errors.
 *
 * @param ast The built-in AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_emc( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_BUILTIN );

  if ( c_type_is_tid_any( &ast->type, TB_EMC_SAT ) &&
      !c_type_is_tid_any( &ast->type, TB_ANY_EMC ) ) {
    print_error( &ast->loc,
      "\"%s\" requires either \"%s\" or \"%s\"\n",
      L_EMC__SAT, L_EMC__ACCUM, L_EMC__FRACT
    );
    return false;
  }

  return true;
}

/**
 * Checks an entire AST for semantic errors.
 *
 * @param ast The AST to check.
 * @param is_func_param If `true`, we're checking a function parameter.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_errors( c_ast_t const *ast, bool is_func_param ) {
  assert( ast != NULL );
  // check in major-to-minor error order
  return  c_ast_check_visitor( ast, c_ast_visitor_error, is_func_param ) &&
          c_ast_check_visitor( ast, c_ast_visitor_type, is_func_param );
}

/**
 * Checks a function-like AST for errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func( c_ast_t const *ast ) {
  assert( ast != NULL );

  if ( ast->kind_id == K_FUNCTION && c_ast_name_equal( ast, "main" ) &&
      ( OPT_LANG_IS(C_ANY) ||
        //
        // Perform extra checks on a function named "main" if either:
        //
        //  + The current language is C; or:
        //
        //  + The current language is C++ and the function does not have any
        //    storage-like type that can't be used with the program's main().
        //    (Otherwise assume it's just a member function named "main".)
        //
        !c_type_is_tid_any( &ast->type, c_tid_compl( TS_MAIN_FUNC ) ) ) &&
      !c_ast_check_func_main( ast ) ) {
    return false;
  }

  return OPT_LANG_IS(C_ANY) ?
    c_ast_check_func_c( ast ) :
    c_ast_check_func_cpp( ast );
}

/**
 * Checks a C function (or block) AST for errors.
 *
 * @param ast The function (or block) AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_c( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_APPLE_BLOCK | K_FUNCTION)) != K_NONE );
  assert( OPT_LANG_IS(C_ANY) );

  c_tid_t const qual_stid = ast->type.stid & TS_MASK_QUALIFIER;
  if ( qual_stid != TS_NONE ) {
    print_error( &ast->loc,
      "\"%s\" %ss is not supported in C\n",
      c_tid_name_error( qual_stid ),
      c_kind_name( ast->kind_id )
    );
    return false;
  }

  return true;
}

/**
 * Checks a C++ function-like AST for errors.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_cpp( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );
  assert( OPT_LANG_IS(CPP_ANY) );

  if ( c_type_is_tid_any( &ast->type, TS_CONSTINIT ) ) {
    error_kind_not_tid( ast, TS_CONSTINIT, "\n" );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TS_ANY_REFERENCE ) ) {
    if ( opt_lang < LANG_CPP_11 ) {
      print_error( &ast->loc,
        "%s qualified %ss is not supported%s\n",
        L_REFERENCE, c_kind_name( ast->kind_id ), c_lang_which( LANG_CPP_11 )
      );
      return false;
    }
    if ( c_type_is_tid_any( &ast->type, TS_EXTERN | TS_STATIC ) ) {
      print_error( &ast->loc,
        "%s qualified %ss can not be %s\n",
        L_REFERENCE, c_kind_name( ast->kind_id ),
        c_tid_name_error( ast->type.stid & (TS_EXTERN | TS_STATIC) )
      );
      return false;
    }
  }

  c_tid_t const member_func_stids = ast->type.stid & TS_MEMBER_FUNC_ONLY;
  c_tid_t const nonmember_func_stids = ast->type.stid & TS_NONMEMBER_FUNC_ONLY;

  if ( member_func_stids != TS_NONE &&
       c_type_is_tid_any( &ast->type, TS_EXTERN | TS_STATIC ) ) {
    print_error( &ast->loc,
      "%s %ss can not be %s\n",
      c_tid_name_error( ast->type.stid & (TS_EXTERN | TS_STATIC) ),
      c_kind_name( ast->kind_id ),
      c_tid_name_error( member_func_stids )
    );
    return false;
  }

  if ( member_func_stids != TS_NONE && nonmember_func_stids != TS_NONE ) {
    print_error( &ast->loc,
      "%ss can not be %s and %s\n",
      c_kind_name( ast->kind_id ),
      c_tid_name_error( member_func_stids ),
      c_tid_name_error( nonmember_func_stids )
    );
    return false;
  }

  unsigned const user_overload_flags = ast->as.func.flags & C_FUNC_MASK_MEMBER;
  switch ( user_overload_flags ) {
    case C_FUNC_MEMBER:
      if ( nonmember_func_stids != TS_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s\n",
          L_MEMBER, c_kind_name( ast->kind_id ),
          c_tid_name_error( nonmember_func_stids )
        );
        return false;
      }
      break;
    case C_FUNC_NON_MEMBER:
      if ( member_func_stids != TS_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s\n",
          H_NON_MEMBER, c_kind_name( ast->kind_id ),
          c_tid_name_error( member_func_stids )
        );
        return false;
      }
      break;
  } // switch

  if ( c_type_is_tid_any( &ast->type, TS_DEFAULT | TS_DELETE ) ) {
    c_ast_t const *param_ast;
    switch ( ast->kind_id ) {
      case K_CONSTRUCTOR: {           // C(C const&)
        if ( c_ast_params_count( ast ) != 1 ) {
          //
          // This isn't correct since copy constructors can have more than one
          // parameter if the additional ones all have default arguments; but
          // cdecl doesn't support default arguments.
          //
          goto only_special;
        }
        param_ast = c_param_ast( c_ast_params( ast ) );
        if ( !c_ast_is_ref_to_tid_any( param_ast, TB_ANY_CLASS ) )
          goto only_special;
        break;
      }

      case K_FUNCTION:
      case K_USER_DEF_CONVERSION:
        if ( c_type_is_tid_any( &ast->type, TS_DEFAULT ) )
          goto only_special;
        break;

      case K_OPERATOR:
        switch ( ast->as.oper.oper_id ) {
          case C_OP_EQ: {               // C& operator=(C const&)
            //
            // For C& operator=(C const&), the parameter and the return type
            // must both be a reference to the same class, struct, or union.
            //
            c_ast_t const *const ret_ast =
              c_ast_is_ref_to_tid_any( ast->as.oper.ret_ast, TB_ANY_CLASS );
            if ( ret_ast == NULL || c_ast_params_count( ast ) != 1 )
              goto only_special;
            param_ast = c_param_ast( c_ast_params( ast ) );
            param_ast = c_ast_is_ref_to_tid_any( param_ast, TB_ANY_CLASS );
            if ( param_ast != ret_ast )
              goto only_special;
            break;
          }

          case C_OP_EQ2:
          case C_OP_EXCLAM_EQ:
          case C_OP_GREATER:
          case C_OP_GREATER_EQ:
          case C_OP_LESS:
          case C_OP_LESS_EQ:
          case C_OP_LESS_EQ_GREATER:
            if ( c_type_is_tid_any( &ast->type, TS_DELETE ) )
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
        goto only_special;
    } // switch
  }

  if ( c_type_is_tid_any( &ast->type, TA_NO_UNIQUE_ADDRESS ) ) {
    error_kind_not_tid( ast, TA_NO_UNIQUE_ADDRESS, "\n" );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TS_VIRTUAL ) ) {
    if ( c_ast_count_name( ast ) > 1 ) {
      print_error( &ast->loc,
        "\"%s\": %s can not be used in file-scoped %ss\n",
        c_ast_full_name( ast ), L_VIRTUAL, c_kind_name( ast->kind_id )
      );
      return false;
    }
  }
  else if ( c_type_is_tid_any( &ast->type, TS_PURE_VIRTUAL ) ) {
    print_error( &ast->loc,
      "non-%s %ss can not be %s\n",
      L_VIRTUAL, c_kind_name( ast->kind_id ), L_PURE
    );
    return false;
  }

  return true;

only_special:
  print_error( &ast->loc,
    "\"%s\" can be used only for special member functions%s\n",
    c_type_name_error( &ast->type ),
    opt_lang >= LANG_CPP_20 && c_type_is_tid_any( &ast->type, TS_DEFAULT ) ?
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_FUNCTION );

  c_ast_t const *const ret_ast = ast->as.func.ret_ast;
  if ( !c_ast_is_builtin_any( ret_ast, TB_INT ) ) {
    print_error( &ret_ast->loc, "main() must return %s\n", L_INT );
    return false;
  }

  size_t const n_params = c_ast_params_count( ast );
  c_ast_t const *param_ast;

  switch ( n_params ) {
    case 0:                             // main()
      break;

    case 1:                             // main(void)
      if ( opt_lang == LANG_C_KNR ) {
        print_error( &ast->loc,
          "main() must have 0, 2, or 3 parameters in %s and later\n",
          c_lang_name( LANG_C_89 )
        );
        return false;
      }

      param_ast = c_param_ast( c_ast_params( ast ) );
      if ( !c_ast_is_builtin_any( param_ast, TB_VOID ) ) {
        print_error( &param_ast->loc,
          "a single parameter for main() must be %s\n", L_VOID
        );
        return false;
      }
      break;

    case 2:                             // main(int, char *argv[])
    case 3:                             // main(int, char *argv[], char *envp[])
      if ( opt_lang > LANG_C_KNR ) {
        c_ast_param_t const *param = c_ast_params( ast );
        param_ast = c_param_ast( param );
        if ( !c_ast_is_builtin_any( param_ast, TB_INT ) ) {
          print_error( &param_ast->loc,
            "main()'s first parameter must be %s\n", L_INT
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
      print_error( &ast->loc, "main() must have 0-3 parameters\n" );
      return false;
  } // switch

  return true;
}

/**
 * Checks that an AST of a main() parameter is either `char*[]` or `char**`
 * optionally including `const`.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is of either type.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main_char_ptr_param( c_ast_t const *ast ) {
  c_ast_t const *const raw_ast = c_ast_untypedef( ast );
  switch ( raw_ast->kind_id ) {
    case K_ARRAY:                       // char *argv[]
    case K_POINTER:                     // char **argv
      if ( !c_ast_is_ptr_to_type( ast->as.parent.of_ast,
              &C_TYPE_LIT_S_ANY( c_tid_compl( TS_CONST ) ),
              &C_TYPE_LIT_B( TB_CHAR ) ) ) {
        print_error( &ast->loc,
          "this parameter of main() must be %s %s %s to [%s] %s\n",
          c_kind_name( ast->kind_id ),
          ast->kind_id == K_ARRAY ? "of" : "to",
          L_POINTER, L_CONST, L_CHAR
        );
        return false;
      }
      break;
    default:                            // ???
      print_error( &ast->loc, "illegal signature for main()\n" );
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_params( c_ast_t const *ast ) {
  if ( opt_lang == LANG_C_KNR )
    return c_ast_check_func_params_knr( ast );

  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );
  assert( opt_lang != LANG_C_KNR );

  c_ast_t const *variadic_ast = NULL, *void_ast = NULL;
  unsigned n_params = 0;

  FOREACH_PARAM( param, ast ) {
    if ( ++n_params > 1 && void_ast != NULL )
      goto only_void;                   // R f(void, T)

    c_ast_t const *const param_ast = c_param_ast( param );

    if ( c_ast_count_name( param_ast ) > 1 ) {
      print_error( &param_ast->loc, "parameter names can not be scoped\n" );
      return false;
    }

    c_tid_t const param_stid =
      TS_MASK_STORAGE & param_ast->type.stid & c_tid_compl( TS_REGISTER );
    if ( param_stid != TS_NONE ) {
      print_error( &param_ast->loc,
        "%s parameters can not be %s\n",
        c_kind_name( ast->kind_id ),
        c_tid_name_error( param_stid )
      );
      return false;
    }

    switch ( param_ast->kind_id ) {
      case K_BUILTIN:
        if ( c_type_is_tid_any( &param_ast->type, TB_AUTO ) &&
             opt_lang < LANG_CPP_20 ) {
          print_error( &param_ast->loc,
            "parameters can not be \"%s\"%s\n", L_AUTO,
            c_lang_which( LANG_CPP_20 )
          );
          return false;
        }
        if ( c_ast_is_builtin_any( param_ast, TB_VOID ) ) {
          //
          // Ordinarily, void parameters are invalid; but a single void
          // function "parameter" is valid (as long as it doesn't have a name).
          //
          if ( !c_ast_empty_name( param_ast ) ) {
            print_error( &param_ast->loc,
              "named parameters can not be %s\n", L_VOID
            );
            return false;
          }
          assert( void_ast == NULL );
          void_ast = param_ast;
          if ( n_params > 1 )
            goto only_void;             // R f(T, void)
          continue;
        }
        PJL_FALLTHROUGH;

      case K_TYPEDEF:
        if ( param_ast->as.tdef.bit_width > 0 ) {
          print_error( &param_ast->loc,
            "parameters can not have bit-field widths\n"
          );
          return false;
        }
        break;

      case K_NAME:
        if ( opt_lang >= LANG_C_2X ) {
          //
          // C2X finally forbids old-style K&R function declarations:
          //
          //      strlen(s)
          //        char *s             // illegal in C2X
          //      {
          //
          print_error( &param_ast->loc,
            "type specifier required by %s\n",
            OPT_LANG_IS(C_ANY) ? c_lang_name( LANG_C_2X ) : "C++"
          );
          return false;
        }
        break;

      case K_PLACEHOLDER:               // should not occur in completed AST
        assert( param_ast->kind_id != K_PLACEHOLDER );
        break;

      case K_VARIADIC:
        if ( ast->kind_id == K_OPERATOR &&
             ast->as.oper.oper_id != C_OP_PARENS ) {
          print_error( &param_ast->loc,
            "%s %s can not have a %s parameter\n",
            L_OPERATOR, c_oper_get( ast->as.oper.oper_id )->name, L_VARIADIC
          );
          return false;
        }
        if ( param->next != NULL ) {
          print_error( &param_ast->loc,
            "%s specifier must be last\n", L_VARIADIC
          );
          return false;
        }
        assert( variadic_ast == NULL );
        variadic_ast = param_ast;
        continue;

      default:
        /* suppress warning */;
    } // switch

    if ( !c_ast_check_errors( param_ast, true ) )
      return false;
  } // for

  if ( variadic_ast != NULL && n_params == 1 ) {
    print_error( &variadic_ast->loc,
      "%s specifier can not be only parameter\n", L_VARIADIC
    );
    return false;
  }

  return true;

only_void:
  print_error( &void_ast->loc,
    "\"%s\" must be only parameter if specified\n", L_VOID
  );
  return false;
}

/**
 * Checks all function parameters for semantic errors in K&R C.
 *
 * @param ast The function-like AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_params_knr( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_APPLE_BLOCK | K_FUNCTION)) != K_NONE );
  assert( opt_lang == LANG_C_KNR );

  FOREACH_PARAM( param, ast ) {
    c_ast_t const *const param_ast = c_param_ast( param );
    switch ( param_ast->kind_id ) {
      case K_NAME:
        break;
      case K_PLACEHOLDER:               // should not occur in completed AST
        assert( param_ast->kind_id != K_PLACEHOLDER );
        break;
      default:
        print_error( &param_ast->loc,
          "%s prototypes are not supported until %s\n",
          L_FUNCTION, c_lang_name( LANG_C_89 )
        );
        return false;
    } // switch
  } // for

  return true;
}

/**
 * Checks an overloaded operator AST for errors.
 *
 * @param ast The overloaded operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );

  if ( (opt_lang & op->lang_ids) == LANG_NONE ) {
    print_error( &ast->loc,
      "overloading %s \"%s\" is not supported%s\n",
      L_OPERATOR, op->name, c_lang_which( op->lang_ids )
    );
    return false;
  }

  unsigned const op_overload_flags = op->flags & C_OP_MASK_OVERLOAD;
  if ( op_overload_flags == C_OP_NOT_OVERLOADABLE ) {
    print_error( &ast->loc,
      "%s %s can not be overloaded\n",
      L_OPERATOR, op->name
    );
    return false;
  }

  unsigned const user_overload_flags = ast->as.oper.flags & C_OP_MASK_OVERLOAD;
  if ( user_overload_flags != C_OP_UNSPECIFIED &&
      (user_overload_flags & op_overload_flags) == 0 ) {
    //
    // The user specified either member or non-member, but the operator can't
    // be that.
    //
    print_error( &ast->loc,
      "%s %s can only be a %s\n",
      L_OPERATOR, op->name,
      op_overload_flags == C_OP_MEMBER ? L_MEMBER : H_NON_MEMBER
    );
    return false;
  }

  switch ( ast->as.oper.oper_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for operators new, new[], delete, and delete[] that can
      // only have specific types.
      //
      if ( c_type_is_tid_any( &ast->type, ~TS_NEW_DELETE_OPER ) ) {
        print_error( &ast->loc,
          "%s %s can not be %s\n",
          L_OPERATOR, op->name, c_type_name_error( &ast->type )
        );
        return false;
      }
      break;
    default:
      /* suppress warning */;
  } // switch

  c_ast_t const *const ret_ast = ast->as.oper.ret_ast;

  switch ( ast->as.oper.oper_id ) {
    case C_OP_ARROW:
      //
      // Special case for operator-> that must return a pointer to a struct,
      // union, or class.
      //
      if ( !c_ast_is_ptr_to_tid_any( ret_ast, TB_ANY_CLASS ) ) {
        print_error( &ret_ast->loc,
          "%s %s must return a %s to %s, %s, or %s\n",
          L_OPERATOR, op->name, L_POINTER, L_STRUCT, L_UNION, L_CLASS
        );
        return false;
      }
      break;

    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for operators delete and delete[] that must return void.
      //
      if ( !c_ast_is_builtin_any( ret_ast, TB_VOID ) ) {
        print_error( &ret_ast->loc,
          "%s %s must return %s\n",
          L_OPERATOR, op->name, L_VOID
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
      if ( !c_ast_is_ptr_to_tid_any( ret_ast, TB_VOID ) ) {
        print_error( &ret_ast->loc,
          "%s %s must return a %s to %s\n",
          L_OPERATOR, op->name, L_POINTER, L_VOID
        );
        return false;
      }
      break;

    default:
      /* suppress warning */;
  } // switch

  if ( c_type_is_tid_any( &ast->type, TS_DEFAULT ) &&
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_default( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );
  assert( c_type_is_tid_any( &ast->type, TS_DEFAULT ) );

  switch ( ast->as.oper.oper_id ) {
    case C_OP_EQ:
      //
      // Detailed checks for defaulted assignment operators are done in
      // c_ast_check_func_cpp().
      //
      break;

    case C_OP_EQ2:
    case C_OP_EXCLAM_EQ:
    case C_OP_GREATER:
    case C_OP_GREATER_EQ:
    case C_OP_LESS:
    case C_OP_LESS_EQ:
    case C_OP_LESS_EQ_GREATER:
      return c_ast_check_oper_relational_default( ast );

    default:
      print_error( &ast->loc,
        "only %s =%s %ss can be %s\n",
        L_OPERATOR, opt_lang >= LANG_CPP_20 ? " and relational" : "",
        L_OPERATOR, L_DEFAULT
      );
      return false;
  } // switch

  return true;
}

/**
 * Checks overloaded operator `delete` and `delete[]` parameters for semantic
 * errors.
 *
 * @param ast The user-defined operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_delete_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );
  assert( ast->as.oper.oper_id == C_OP_DELETE ||
          ast->as.oper.oper_id == C_OP_DELETE_ARRAY );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );

  size_t const n_params = c_ast_params_count( ast );
  if ( n_params == 0 ) {
    print_error( &ast->loc,
      "%s %s must have at least one parameter\n",
      L_OPERATOR, op->name
    );
    return false;
  }

  c_ast_param_t const *const param = c_ast_params( ast );
  c_ast_t const *const param_ast = c_param_ast( param );

  if ( !c_ast_is_ptr_to_tid_any( param_ast, TB_VOID | TB_ANY_CLASS ) ) {
    print_error( &param_ast->loc,
      "invalid parameter type for %s %s; must be a %s to %s, %s, %s, or %s\n",
      L_OPERATOR, op->name,
      L_POINTER, L_VOID, L_CLASS, L_STRUCT, L_UNION
    );
    return false;
  }

  return true;
}

/**
 * Checks overloaded operator `new` and `new[]` parameters for semantic errors.
 *
 * @param ast The user-defined operator `new` AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_new_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );
  assert( ast->as.oper.oper_id == C_OP_NEW ||
          ast->as.oper.oper_id == C_OP_NEW_ARRAY );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );

  size_t const n_params = c_ast_params_count( ast );
  if ( n_params == 0 ) {
    print_error( &ast->loc,
      "%s %s must have at least one parameter\n",
      L_OPERATOR, op->name
    );
    return false;
  }

  c_ast_param_t const *const param = c_ast_params( ast );
  c_ast_t const *const param_ast = c_param_ast( param );

  if ( !c_ast_is_size_t( param_ast ) ) {
    print_error( &param_ast->loc,
      "invalid parameter type for %s %s; must be std::size_t (or equivalent)\n",
      L_OPERATOR, op->name
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );
  unsigned const overload_flags = c_ast_oper_overload( ast );
  char const *const op_monm =           // member or non-member string
    overload_flags == C_OP_MEMBER     ? L_MEMBER     :
    overload_flags == C_OP_NON_MEMBER ? H_NON_MEMBER :
    "";

  //
  // Determine the minimum and maximum number of parameters the operator can
  // have based on whether it's a member, non-member, or unspecified.
  //
  bool const is_ambiguous = c_oper_is_ambiguous( op );
  unsigned req_params_min = 0, req_params_max = 0;
  bool const max_params_is_unlimited = op->params_max == C_OP_PARAMS_UNLIMITED;
  switch ( overload_flags ) {
    case C_OP_NON_MEMBER:
      // Non-member operators must always take at least one parameter (the
      // enum, class, struct, or union for which it's overloaded).
      req_params_min = is_ambiguous || max_params_is_unlimited ?
        1 : op->params_max;
      req_params_max = op->params_max;
      break;
    case C_OP_MEMBER:
      if ( !max_params_is_unlimited ) {
        req_params_min = op->params_min;
        req_params_max = is_ambiguous ? 1 : op->params_min;
        break;
      }
      PJL_FALLTHROUGH;
    case C_OP_UNSPECIFIED:
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
same: print_error( &ast->loc,
        "%s%s%s %s must have exactly %u parameter%s\n",
        SP_AFTER( op_monm ), L_OPERATOR, op->name,
        req_params_min, plural_s( req_params_min )
      );
    else
      print_error( &ast->loc,
        "%s%s%s %s must have at least %u parameter%s\n",
        SP_AFTER( op_monm ), L_OPERATOR, op->name,
        req_params_min, plural_s( req_params_min )
      );
    return false;
  }
  if ( n_params > req_params_max ) {
    if ( op->params_min == req_params_max )
      goto same;
    print_error( &ast->loc,
      "%s%s%s %s can have at most %u parameter%s\n",
      SP_AFTER( op_monm ), L_OPERATOR, op->name,
      op->params_max, plural_s( op->params_max )
    );
    return false;
  }

  //
  // Count the number of enum, class, struct, or union parameters.
  //
  unsigned ecsu_obj_param_count = 0, ecsu_lref_param_count = 0,
           ecsu_rref_param_count = 0;
  FOREACH_PARAM( param, ast ) {
    //
    // Normally we can use c_ast_is_kind_any(), but we need to count objects
    // and lvalue references to objects distinctly to check default relational
    // operators in C++20.
    //
    c_ast_t const *param_ast = c_ast_untypedef( c_param_ast( param ) );
    switch ( param_ast->kind_id ) {
      case K_ENUM_CLASS_STRUCT_UNION:
        ++ecsu_obj_param_count;
        break;
      case K_REFERENCE:
        param_ast = c_ast_unreference( param_ast );
        if ( param_ast->kind_id == K_ENUM_CLASS_STRUCT_UNION )
          ++ecsu_lref_param_count;
        break;
      case K_RVALUE_REFERENCE:
        param_ast = c_ast_unreference( param_ast );
        if ( param_ast->kind_id == K_ENUM_CLASS_STRUCT_UNION )
          ++ecsu_rref_param_count;
        break;
      default:
        /* suppress warning */;
    } // switch
  } // for
  unsigned const ecsu_param_count =
    ecsu_obj_param_count + ecsu_lref_param_count + ecsu_rref_param_count;

  switch ( overload_flags ) {
    case C_OP_NON_MEMBER: {
      //
      // Ensure non-member operators are not const, defaulted, deleted,
      // overridden, final, reference, rvalue reference, nor virtual.
      //
      // Special case: in C++20 and later, relational operators may be
      // defaulted.
      //
      c_tid_t const member_only_stids = ast->type.stid & TS_MEMBER_FUNC_ONLY;
      if ( member_only_stids != TS_NONE ) {
        switch ( ast->as.oper.oper_id ) {
          case C_OP_EQ2:
          case C_OP_EXCLAM_EQ:
          case C_OP_GREATER:
          case C_OP_GREATER_EQ:
          case C_OP_LESS:
          case C_OP_LESS_EQ:
          case C_OP_LESS_EQ_GREATER:
            if ( c_tid_is_any( member_only_stids, TS_DEFAULT ) ) {
              //
              // Detailed checks for defaulted overloaded relational operators
              // are done in c_ast_check_oper_relational_default().
              //
              break;
            }
            PJL_FALLTHROUGH;
          default:
            print_error( &ast->loc,
              "%s %ss can not be %s\n",
              H_NON_MEMBER, L_OPERATOR,
              c_tid_name_error( member_only_stids )
            );
            return false;
        } // switch
      }

      //
      // Ensure non-member operators (except new, new[], delete, and delete[])
      // have at least one enum, class, struct, or union parameter.
      //
      switch ( ast->as.oper.oper_id ) {
        case C_OP_NEW:
        case C_OP_NEW_ARRAY:
        case C_OP_DELETE:
        case C_OP_DELETE_ARRAY:
          break;
        default:
          if ( ecsu_param_count == 0 ) {
            print_error( &ast->loc,
              "at least 1 parameter of a %s %s must be an %s"
              "; or a %s or %s %s thereto\n",
              H_NON_MEMBER, L_OPERATOR,
              c_kind_name( K_ENUM_CLASS_STRUCT_UNION ),
              L_REFERENCE, L_RVALUE, L_REFERENCE
            );
            return false;
          }
      } // switch
      break;
    }

    case C_OP_MEMBER: {
      //
      // Ensure member operators are not friend.
      //
      c_tid_t const non_member_only_stids =
        ast->type.stid & TS_NONMEMBER_FUNC_ONLY;
      if ( non_member_only_stids != TS_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s\n",
          L_MEMBER, L_OPERATOR,
          c_tid_name_error( non_member_only_stids )
        );
        return false;
      }
      break;
    }
  } // switch

  switch ( ast->as.oper.oper_id ) {
    case C_OP_MINUS2:
    case C_OP_PLUS2: {
      //
      // Ensure that the dummy parameter for postfix -- or ++ is type int (or
      // is a typedef of int).
      //
      c_ast_param_t const *param = c_ast_params( ast );
      if ( param == NULL )              // member prefix
        break;
      if ( overload_flags == C_OP_NON_MEMBER ) {
        param = param->next;
        if ( param == NULL )            // non-member prefix
          break;
      }
      // At this point, it's either member or non-member postfix:
      // operator++(int) or operator++(S&,int).
      c_ast_t const *const param_ast = c_param_ast( param );
      if ( !c_ast_is_builtin_any( param_ast, TB_INT ) ) {
        print_error( &param_ast->loc,
          "parameter of postfix %s%s%s %s must be %s\n",
          SP_AFTER( op_monm ), L_OPERATOR, op->name,
          c_tid_name_error( TB_INT )
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_relational_default( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_OPERATOR );
  assert( c_type_is_tid_any( &ast->type, TS_DEFAULT ) );

  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );
  if ( opt_lang < LANG_CPP_20 ) {
    print_error( &ast->loc,
      "%s %s %s is not supported%s\n",
      L_DEFAULT, L_OPERATOR, op->name, c_lang_which( LANG_CPP_MIN(20) )
    );
    return false;
  }

  c_ast_param_t const *const param = c_ast_params( ast );
  c_ast_t const *const param_ast = c_param_ast( param );

  switch ( c_ast_oper_overload( ast ) ) {
    case C_OP_NON_MEMBER: {
      if ( !c_type_is_tid_any( &ast->type, TS_FRIEND ) ) {
        print_error( &ast->loc,
          "%s %s %s %s must also be %s\n",
          L_DEFAULT, H_NON_MEMBER, L_OPERATOR, op->name, L_FRIEND
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
            "%s %s relational %ss must take two "
            "value or reference-to-const parameters of the same %s\n",
            L_DEFAULT, H_NON_MEMBER, L_OPERATOR, L_CLASS
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

    case C_OP_MEMBER: {
      if ( !c_type_is_tid_any( &ast->type, TS_CONST ) ) {
        print_error( &ast->loc,
          "%s %s %s %s must also be %s\n",
          L_DEFAULT, L_MEMBER, L_OPERATOR, op->name, L_CONST
        );
        return false;
      }

      //
      // Default member relational operators must take one class by either
      // value or reference-to-const.
      //
      c_ast_t const *param1_ast = c_ast_is_tid_any( param_ast, TB_ANY_CLASS );
      if ( param1_ast == NULL ) {
        param1_ast = c_ast_is_ref_to_type_any( param_ast, &T_ANY_CONST_CLASS );
        if ( param1_ast == NULL ) {
          print_error( &ast->loc,
            "%s %s relational %ss must take one "
            "value or reference-to-const parameter to a %s\n",
            L_DEFAULT, L_MEMBER, L_OPERATOR, L_CLASS
          );
          return false;
        }
      }
      break;
    }
  } // switch

  c_ast_t const *const ret_ast = ast->as.oper.ret_ast;
  c_ast_t const *const raw_ret_ast = c_ast_untypedef( ret_ast );

  if ( ast->as.oper.oper_id == C_OP_LESS_EQ_GREATER ) {
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
    if ( !c_ast_is_builtin_any( ret_ast, TB_AUTO ) &&
         raw_ret_ast != std_partial_ordering_ast &&
         raw_ret_ast != std_strong_ordering_ast &&
         raw_ret_ast != std_weak_ordering_ast ) {
      print_error( &ret_ast->loc,
        "%s %s must return one of %s, "
        "std::partial_ordering, "
        "std::strong_ordering, or "
        "std::weak_ordering\n",
        L_OPERATOR, op->name, L_AUTO
      );
      return false;
    }
  }
  else if ( !c_ast_is_builtin_any( ret_ast, TB_BOOL ) ) {
    print_error( &ret_ast->loc,
      "%s %s must return %s\n",
      L_OPERATOR, op->name, L_BOOL
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_pointer( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_POINTER) != K_NONE );

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind_id ) {
    case K_FUNCTION:
      if ( ast->kind_id == K_POINTER && !c_type_is_none( &to_ast->type ) ) {
        print_error( &to_ast->loc,
          "%s to %s %s is illegal\n",
          c_kind_name( ast->kind_id ),
          c_type_name_error( &to_ast->type ),
          c_kind_name( to_ast->kind_id )
        );
        return false;
      }
      break;
    case K_NAME:
      error_unknown_name( to_ast );
      return false;
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      error_kind_to_kind( ast, to_ast, "" );
      if ( c_mode == C_ENGLISH_TO_GIBBERISH )
        print_hint( "%s to %s", L_REFERENCE, L_POINTER );
      else
        print_hint( "\"*&\"" );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( c_ast_is_register( to_ast ) ) {
    error_kind_to_tid( ast, TS_REGISTER, "\n" );
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_reference( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_REFERENCE) != K_NONE );

  if ( c_type_is_tid_any( &ast->type, TS_CONST_VOLATILE ) ) {
    c_tid_t const qual_stid = ast->type.stid & TS_MASK_QUALIFIER;
    error_kind_not_tid( ast, qual_stid, "" );
    print_hint( "%s to %s", L_REFERENCE, c_tid_name_error( qual_stid ) );
    return false;
  }

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind_id ) {
    case K_NAME:
      error_unknown_name( to_ast );
      return VISITOR_ERROR_FOUND;
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      error_kind_to_kind( ast, to_ast, "\n" );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( c_ast_is_register( to_ast ) ) {
    error_kind_to_tid( ast, TS_REGISTER, "\n" );
    return false;
  }

  if ( c_ast_is_builtin_any( to_ast, TB_VOID ) ) {
    error_kind_to_tid( ast, TB_VOID, "" );
    print_hint( "%s to %s", L_POINTER, L_VOID );
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_ret_type( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );

  char const *const kind_name = c_kind_name( ast->kind_id );
  c_ast_t const *const ret_ast = ast->as.func.ret_ast;

  switch ( ret_ast->kind_id ) {
    case K_ARRAY:
      print_error( &ret_ast->loc, "%s returning %s", kind_name, L_ARRAY );
      print_hint( "%s returning %s", kind_name, L_POINTER );
      return false;
    case K_BUILTIN:
      if ( c_type_is_tid_any( &ret_ast->type, TB_AUTO ) &&
           opt_lang < LANG_CPP_14 ) {
        print_error( &ret_ast->loc,
          "\"%s\" return type is not supported%s\n",
          L_AUTO, c_lang_which( LANG_CPP_14 )
        );
        return false;
      }
      break;
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      print_error( &ret_ast->loc,
        "%s returning %s",
        kind_name, c_kind_name( ret_ast->kind_id )
      );
      print_hint( "%s returning %s to %s", kind_name, L_POINTER, L_FUNCTION );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( c_type_is_tid_any( &ast->type, TS_EXPLICIT ) ) {
    error_kind_not_tid( ast, TS_EXPLICIT, "\n" );
    return false;
  }

  return true;
}

/**
 * Checks a user-defined conversion operator AST for errors.
 *
 * @param ast The user-defined conversion operator AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_udef_conv( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_USER_DEF_CONVERSION );

  if ( c_type_is_tid_any( &ast->type, ~TS_USER_DEF_CONV) ) {
    print_error( &ast->loc,
      "%s %s %ss can only be: %s\n",
      H_USER_DEFINED, L_CONVERSION, L_OPERATOR,
      c_tid_name_error( TS_USER_DEF_CONV )
    );
    return false;
  }
  if ( c_type_is_tid_any( &ast->type, TS_FRIEND ) && c_ast_empty_name( ast ) ) {
    print_error( &ast->loc,
      "%s %s %s %s must use qualified name\n",
      L_FRIEND, H_USER_DEFINED, L_CONVERSION, L_OPERATOR
    );
    return false;
  }
  c_ast_t const *const conv_ast = ast->as.udef_conv.conv_ast;
  c_ast_t const *const raw_conv_ast = c_ast_untypedef( conv_ast );
  if ( raw_conv_ast->kind_id == K_ARRAY ) {
    print_error( &conv_ast->loc,
      "%s %s %s can not convert to an %s",
      H_USER_DEFINED, L_CONVERSION, L_OPERATOR, L_ARRAY
    );
    print_hint( "%s to %s", L_POINTER, L_ARRAY );
    return false;
  }

  return  c_ast_check_ret_type( ast ) &&
          c_ast_check_func_cpp( ast ) &&
          c_ast_check_func_params( ast );
}

/**
 * Checks all user-defined literal parameters for semantic errors.
 *
 * @param ast The user-defined literal AST to check.
 * @return Returns `true` only if all checks passed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_udef_lit_params( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_USER_DEF_LITERAL );

  size_t const n_params = c_ast_params_count( ast );
  if ( n_params == 0 ) {
    print_error( &ast->loc,
      "%s %s must have a parameter\n", H_USER_DEFINED, L_LITERAL
    );
    return false;
  }

  c_ast_param_t const *param = c_ast_params( ast );
  c_ast_t const *param_ast = c_param_ast( param );
  c_ast_t const *raw_param_ast = c_ast_untypedef( param_ast );
  c_ast_t const *tmp_ast = NULL;

  switch ( n_params ) {
    case 1:
      switch ( raw_param_ast->type.btid ) {
        case TB_CHAR:
        case TB_CHAR8_T:
        case TB_CHAR16_T:
        case TB_CHAR32_T:
        case TB_WCHAR_T:
        case TB_UNSIGNED | TB_LONG | TB_LONG_LONG:
        case TB_UNSIGNED | TB_LONG | TB_LONG_LONG | TB_INT:
        case TB_LONG | TB_DOUBLE:
          break;
        default:                        // check for: char const*
          if ( !c_ast_is_ptr_to_type( param_ast,
                  &T_ANY, &C_TYPE_LIT( TB_CHAR, TS_CONST, TA_NONE ) ) ) {
            print_error( &param_ast->loc,
              "invalid parameter type for %s %s; must be one of: "
              "unsigned long long, long double, "
              "char, const char*, %schar16_t, char32_t, or wchar_t\n",
              H_USER_DEFINED, L_LITERAL,
              opt_lang >= LANG_CPP_20 ? "char8_t, " : ""
            );
            return false;
          }
      } // switch
      break;

    case 2:
      tmp_ast = c_ast_unpointer( raw_param_ast );
      if ( tmp_ast == NULL ||
          !(c_ast_is_tid_any( tmp_ast, TS_CONST ) &&
            c_ast_is_tid_any( tmp_ast, TB_ANY_CHAR )) ) {
        print_error( &param_ast->loc,
          "invalid parameter type for %s %s; must be one of: "
          "const (char|wchar_t|char8_t|char16_t|char32_t)*\n",
          H_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      param_ast = c_param_ast( param->next );
      if ( param_ast == NULL || !c_ast_is_size_t( param_ast ) ) {
        print_error( &param_ast->loc,
          "invalid parameter type for %s %s; "
          "must be std::size_t (or equivalent)\n",
          H_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      break;

    default:
      param_ast = c_param_ast( param->next->next );
      print_error( &param_ast->loc,
        "%s %s may have at most 2 parameters\n", H_USER_DEFINED, L_LITERAL
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
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_upc( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind_id == K_BUILTIN );

  if ( c_type_is_tid_any( &ast->type, TS_UPC_RELAXED | TS_UPC_STRICT ) &&
      !c_type_is_tid_any( &ast->type, TS_UPC_SHARED ) ) {
    print_error( &ast->loc,
      "\"%s\" requires \"%s\"\n",
      c_type_name_error( &ast->type ),
      L_UPC_SHARED
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_name_equal( c_ast_t const *ast, char const *name ) {
  SNAME_VAR_INIT( sname, name );
  return c_sname_cmp( &ast->sname, &sname ) == 0;
}

/**
 * Visitor function that checks an AST for semantic errors.
 *
 * @param ast The AST to check.
 * @param data Cast to `bool`, indicates if \a ast is a function parameter.
 * @return Returns `VISITOR_ERROR_FOUND` if an error was found;
 * `VISITOR_ERROR_NOT_FOUND` if not.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_error( c_ast_t *ast, uint64_t data ) {
  assert( ast != NULL );
  bool const is_func_param = STATIC_CAST( bool, data );

  if ( !c_ast_check_alignas( ast ) )
    return VISITOR_ERROR_FOUND;

  switch ( ast->kind_id ) {
    case K_ARRAY:
      if ( !c_ast_check_array( ast, is_func_param ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_BUILTIN:
      if ( !c_ast_check_builtin( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      if ( !c_ast_check_ecsu( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_OPERATOR:
      if ( !c_ast_check_oper( ast ) )
        return VISITOR_ERROR_FOUND;
      PJL_FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
      if ( !c_ast_check_ret_type( ast ) )
        return VISITOR_ERROR_FOUND;
      PJL_FALLTHROUGH;

    case K_CONSTRUCTOR:
      if ( !(c_ast_check_func( ast ) && c_ast_check_func_params( ast )) )
        return VISITOR_ERROR_FOUND;
      PJL_FALLTHROUGH;

    case K_DESTRUCTOR: {
      if ( (ast->kind_id & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE &&
          !c_ast_check_ctor_dtor( ast ) ) {
        return VISITOR_ERROR_FOUND;
      }

      c_tid_t const func_like_stid =
        ast->type.stid & c_tid_compl( TS_FUNC_LIKE );
      if ( func_like_stid != TS_NONE ) {
        error_kind_not_tid( ast, func_like_stid, "\n" );
        return VISITOR_ERROR_FOUND;
      }

      if ( c_type_is_tid_any( &ast->type, TS_THROW ) &&
           opt_lang >= LANG_CPP_20 ) {
        print_error( &ast->loc,
          "\"%s\" is no longer supported in C++20", L_THROW
        );
        print_hint( "\"%s\"", L_NOEXCEPT );
        return VISITOR_ERROR_FOUND;
      }
      break;
    }

    case K_NAME:
    case K_TYPEDEF:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_NONE:                        // should not occur in completed AST
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:                 // should not occur in completed AST
      assert( ast->kind_id != K_PLACEHOLDER );
      break;

    case K_POINTER_TO_MEMBER:
      if ( OPT_LANG_IS(C_ANY) ) {
        error_kind_not_supported( ast, LANG_CPP_ANY );
        return VISITOR_ERROR_FOUND;
      }
      PJL_FALLTHROUGH;
    case K_POINTER:
      if ( !c_ast_check_pointer( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_lang < LANG_CPP_11 ) {
        error_kind_not_supported( ast, LANG_CPP_MIN(11) );
        return VISITOR_ERROR_FOUND;
      }
      PJL_FALLTHROUGH;
    case K_REFERENCE:
      if ( OPT_LANG_IS(C_ANY) ) {
        error_kind_not_supported( ast, LANG_CPP_ANY );
        return VISITOR_ERROR_FOUND;
      }
      if ( !c_ast_check_reference( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_USER_DEF_CONVERSION:
      if ( !c_ast_check_udef_conv( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_USER_DEF_LITERAL:
      if ( !(c_ast_check_ret_type( ast ) &&
             c_ast_check_func_cpp( ast )  &&
             c_ast_check_udef_lit_params( ast )) ) {
        return VISITOR_ERROR_FOUND;
      }
      break;
  } // switch

  if ( ast->kind_id != K_FUNCTION &&
       c_type_is_tid_any( &ast->type, TS_CONSTEVAL ) ) {
    print_error( &ast->loc, "only functions can be %s\n", L_CONSTEVAL );
    return VISITOR_ERROR_FOUND;
  }

  return VISITOR_ERROR_NOT_FOUND;
}

/**
 * Visitor function that checks an AST for type errors.
 *
 * @param ast The AST to visit.
 * @param data Cast to `bool`, indicates if \a ast is a function parameter.
 * @return Returns `VISITOR_ERROR_FOUND` if an error was found;
 * `VISITOR_ERROR_NOT_FOUND` if not.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_type( c_ast_t *ast, uint64_t data ) {
  assert( ast != NULL );
  bool const is_func_param = STATIC_CAST( bool, data );

  c_lang_id_t const lang_ids = c_type_check( &ast->type );
  if ( lang_ids != LANG_ANY ) {
    print_error( &ast->loc,
      "\"%s\" is illegal for %s%s\n",
      c_type_name_error( &ast->type ), c_kind_name( ast->kind_id ),
      c_lang_which( lang_ids )
    );
    return VISITOR_ERROR_FOUND;
  }

  if ( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE ) {
    if ( opt_lang < LANG_CPP_14 &&
         c_tid_is_any( ast->type.stid, TS_CONSTEXPR ) &&
         c_ast_is_builtin_any( ast->as.func.ret_ast, TB_VOID ) ) {
      print_error( &ast->loc,
        "%s %s is illegal%s\n",
        c_tid_name_error( ast->type.stid ),
        c_tid_name_error( ast->as.func.ret_ast->type.btid ),
        c_lang_which( LANG_CPP_14 )
      );
      return VISITOR_ERROR_FOUND;
    }
  }
  else {
    if ( c_type_is_tid_any( &ast->type, TA_CARRIES_DEPENDENCY ) &&
         !is_func_param ) {
      print_error( &ast->loc,
        "\"%s\" can only appear on functions or function parameters\n",
        c_tid_name_error( TA_CARRIES_DEPENDENCY )
      );
      return VISITOR_ERROR_FOUND;
    }

    if ( c_type_is_tid_any( &ast->type, TA_NORETURN ) ) {
      print_error( &ast->loc,
        "\"%s\" can only appear on functions\n",
        c_tid_name_error( TA_NORETURN )
      );
      return VISITOR_ERROR_FOUND;
    }
  }

  if ( c_type_is_tid_any( &ast->type, TS_RESTRICT ) ) {
    switch ( ast->kind_id ) {
      case K_FUNCTION:
      case K_OPERATOR:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_USER_DEF_CONVERSION:
        //
        // These being declared "restrict" in C is already made an error by
        // checks elsewhere.
        //
      case K_POINTER:
        break;
      default:
        error_kind_not_tid( ast, TS_RESTRICT, "\n" );
        return VISITOR_ERROR_FOUND;
    } // switch
  }

  if ( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE ) {
    FOREACH_PARAM( param, ast ) {
      c_ast_t const *const param_ast = c_param_ast( param );
      if ( !c_ast_check_visitor( param_ast, c_ast_visitor_type, true ) )
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
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_warning( c_ast_t *ast, uint64_t data ) {
  assert( ast != NULL );

  switch ( ast->kind_id ) {
    case K_ARRAY:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
    case K_USER_DEF_CONVERSION:
    case K_VARIADIC:
      // nothing to check
      break;

    case K_USER_DEF_LITERAL:
      if ( c_ast_local_name( ast )[0] != '_' )
        print_warning( &ast->loc,
          "%s %s not starting with '_' are reserved\n",
          H_USER_DEFINED, L_LITERAL
        );
      PJL_FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_FUNCTION:
    case K_OPERATOR: {
      c_ast_t const *const ret_ast = ast->as.func.ret_ast;
      if ( c_type_is_tid_any( &ast->type, TA_NODISCARD ) &&
           c_ast_is_builtin_any( ret_ast, TB_VOID ) ) {
        print_warning( &ast->loc,
          "[[%s]] %ss can not return %s\n",
          L_NODISCARD, c_kind_name( ast->kind_id ), L_VOID
        );
      }
      PJL_FALLTHROUGH;
    }

    case K_CONSTRUCTOR:
      FOREACH_PARAM( param, ast ) {
        c_ast_t const *const param_ast = c_param_ast( param );
        PJL_IGNORE_RV(
          c_ast_check_visitor( param_ast, c_ast_visitor_warning, data )
        );
      } // for
      PJL_FALLTHROUGH;

    case K_DESTRUCTOR:
      if ( c_type_is_tid_any( &ast->type, TS_THROW ) &&
           opt_lang >= LANG_CPP_11 ) {
        print_warning( &ast->loc, "\"%s\" is deprecated in C++11", L_THROW );
        print_hint( "\"%s\"", L_NOEXCEPT );
      }
      break;

    case K_BUILTIN:
      if ( c_ast_is_register( ast ) && opt_lang >= LANG_CPP_11 ) {
        print_warning( &ast->loc,
          "\"%s\" is deprecated in %s\n",
          L_REGISTER, c_lang_name( LANG_CPP_11 )
        );
      }
      break;

    case K_NAME:
      if ( opt_lang > LANG_C_KNR )
        print_warning( &ast->loc, "missing type specifier\n" );
      break;

    case K_NONE:                        // should not occur in completed AST
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:                 // should not occur in completed AST
      assert( ast->kind_id != K_PLACEHOLDER );
      break;
  } // switch

  if ( c_initialized )                  // don't warn for predefined types
    c_ast_warn_name( ast );

  return false;
}

/**
 * Checks an AST's name(s) for warnings.
 *
 * @param ast The AST to check.
 */
static void c_ast_warn_name( c_ast_t const *ast ) {
  assert( ast != NULL );

  c_sname_warn( &ast->sname, &ast->loc );
  switch ( ast->kind_id ) {
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_POINTER_TO_MEMBER:
      c_sname_warn( &ast->as.ecsu.ecsu_sname, &ast->loc );
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

  FOREACH_SCOPE( scope, sname, NULL ) {
    char const *const name = c_scope_data( scope )->name;

    // First, check to see if the name is a keyword in some other language.
    c_keyword_t const *const k = c_keyword_find( name, LANG_ANY, C_KW_CTX_ALL );
    if ( k != NULL ) {
      print_warning( loc,
        "\"%s\" is a keyword in %s\n",
        name, c_lang_oldest_name( k->lang_ids )
      );
      continue;
    }

    // Next, check to see if the name is a reserved name in some language.
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
 * Checks whether \a name is reserved in the current language.  A name is
 * reserved if it matches any of these patterns:
 *
 *      _*          // C: external only; C++: global namespace only.
 *      _[A-Z_]*
 *      *__*        // C++ only.
 *
 * However, we don't check for the first one since cdecl doesn't have either
 * the linkage or the scope of a name.
 *
 * @param name The name to check.
 * @return Returns the bitwise-or of language(s) that \a name is reserved in.
 */
PJL_WARN_UNUSED_RESULT
static c_lang_id_t is_reserved_name( char const *name ) {
  assert( name != NULL );

  if ( name[0] == '_' && (isupper( name[1] ) || name[1] == '_') )
    return LANG_ANY;

  if ( strstr( name, "__" ) != NULL )
    return LANG_CPP_ANY;

  return LANG_NONE;
}

////////// extern functions ///////////////////////////////////////////////////

bool c_ast_check_cast( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );

  c_ast_t const *const storage_ast = c_ast_find_type_any(
    nonconst_ast, C_VISIT_DOWN, &C_TYPE_LIT_S( TS_MASK_STORAGE )
  );

  if ( storage_ast != NULL ) {
    print_error( &ast->loc,
      "can not %s %s %s\n", L_CAST, L_INTO,
      c_tid_name_error( storage_ast->type.stid & TS_MASK_STORAGE )
    );
    return false;
  }

  switch ( ast->kind_id ) {
    case K_ARRAY:
      error_kind_not_cast_into( ast, "pointer" );
      return false;
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
      error_kind_not_cast_into( ast, "pointer to function" );
      return false;
    default:
      /* suppress warning */;
  } // switch

  return c_ast_check_declaration( ast );
}

bool c_ast_check_declaration( c_ast_t const *ast ) {
  assert( ast != NULL );
  if ( !c_ast_check_errors( ast, false ) )
    return false;
  PJL_IGNORE_RV( c_ast_check_visitor( ast, c_ast_visitor_warning, 0 ) );
  return true;
}

bool c_sname_check( c_sname_t const *sname, c_loc_t const *sname_loc ) {
  assert( sname != NULL );
  assert( !c_sname_empty( sname ) );
  assert( sname_loc != NULL );

  c_tid_t prev_btid = TB_NONE;
  unsigned prev_order = 0;

  FOREACH_SCOPE( scope, sname, NULL ) {
    c_type_t *const scope_type = &c_scope_data( scope )->type;
    //
    // Temporarily set scope->next to NULL to chop off any scopes past the
    // given scope to look up a partial sname. For example, given "A::B::C",
    // see if "A::B" exists.  If it does, check that the sname's scope's type
    // matches the previously declared sname's scope's type.
    //
    c_scope_t *const orig_next = scope->next;
    scope->next = NULL;
    c_typedef_t const *const tdef = c_typedef_find_sname( sname );
    if ( tdef != NULL ) {
      c_type_t const *const tdef_type = c_ast_local_type( tdef->ast );
      if ( c_type_is_tid_any( tdef_type, TB_ANY_SCOPE | TB_ENUM ) &&
           !c_type_equal( scope_type, tdef_type ) ) {
        if ( c_type_is_tid_any( scope_type, TB_ANY_SCOPE ) ) {
          //
          // The scope's type is a scope-type and doesn't match a previously
          // declared scope-type, e.g.:
          //
          //      namespace N { class C; }
          //      namespace N::C { class D; }
          //                ^
          //      11: error: "N::C" was previously declared as a class
          //
          print_error( sname_loc,
            "\"%s\" was previously declared as a %s:\n",
            c_sname_full_name( sname ),
            c_type_name_error( tdef_type )
          );
          SGR_START_COLOR( stderr, caret );
          EPUTC( '>' );
          SGR_END_COLOR( stderr );
          EPUTC( ' ' );
          if ( tdef->defined_in_english )
            c_ast_explain_type( tdef->ast, stderr );
          else
            c_typedef_gibberish( tdef, C_GIB_TYPEDEF, stderr );
          scope->next = orig_next;
          return false;
        }

        //
        // Otherwise, copy the previously declared scope's type to the current
        // scope's type.
        //
        *scope_type = *tdef_type;
      }
    }
    scope->next = orig_next;

    unsigned const scope_order = c_tid_scope_order( scope_type->btid );
    if ( scope_order < prev_order ) {
      print_error( sname_loc,
        "%s can not nest inside %s\n",
        c_tid_name_error( scope_type->btid ),
        c_tid_name_error( prev_btid )
      );
      return false;
    }
    prev_btid = scope_type->btid;
    prev_order = scope_order;
  } // for

  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
