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
 * Defines functions for performing semantic checks on an AST.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_type.h"
#include "cdecl.h"
#include "did_you_mean.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

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
 * Prints an error: `<kind> not supported in <lang>`.
 *
 * @param AST The AST having the unsupported kind.
 */
#define error_kind_not_supported(AST)             \
  fl_print_error( __FILE__, __LINE__,             \
    &(AST)->loc, "%s not supported in %s\n",      \
    c_kind_name( (AST)->kind_id ), C_LANG_NAME()  \
  )

/**
 * Prints an error: `<kind> can not be <type>`.
 *
 * @param AST The AST .
 * @param TID The bad type.
 * @param NEWLINE If `true`, prints a newline.
 */
#define error_kind_not_tid(AST,TID,NEWLINE)                     \
  fl_print_error( __FILE__, __LINE__,                           \
    &(AST)->loc, "%s can not be %s%s",                          \
    c_kind_name( (AST)->kind_id ), c_type_id_name_error( TID ), \
    (NEWLINE) ? "\n" : ""                                       \
  )

/**
 * Prints an error: `<kind> to <kind>`.
 *
 * @param AST The AST having the bad kind.
 * @param KIND_ID The other kind.
 */
#define error_kind_to_kind(AST,KIND_ID)                   \
  fl_print_error( __FILE__, __LINE__,                     \
    &(AST)->loc, "%s to %s\n",                            \
    c_kind_name( (AST)->kind_id ), c_kind_name( KIND_ID ) \
  )

/**
 * Prints an error: `<kind> to <type>`.
 *
 * @param AST The AST having the bad kind.
 * @param TID The bad type.
 * @param NEWLINE If `true`, prints a newline.
 */
#define error_kind_to_type(AST,TID,NEWLINE)                     \
  fl_print_error( __FILE__, __LINE__,                           \
    &(AST)->loc, "%s to %s%s",                                  \
    c_kind_name( (AST)->kind_id ), c_type_id_name_error( TID ), \
    (NEWLINE) ? "\n" : ""                                       \
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
static bool c_ast_check_oper_params( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_emc( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_c( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_cpp( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main_char_ptr_arg( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_new_params( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_oper_delete_params( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_check_upc( c_ast_t const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_name_eq( c_ast_t const*, char const* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_error( c_ast_t*, void* );

PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_type( c_ast_t*, void* );

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
                                        c_ast_visitor_t visitor, void *data ) {
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
static bool c_ast_check_alignas( c_ast_t *ast ) {
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
            "\"%u\": alignment is not a power of 2\n", alignment
          );
          return false;
        }
        break;
      }
      case C_ALIGNAS_TYPE:
        if ( !c_ast_check_declaration( ast->align.as.type_ast ) )
          return false;
        break;
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
    if ( !C_LANG_IS(C_MIN(99)) ) {
      print_error( &ast->loc,
        "variable length arrays not supported in %s\n",
        C_LANG_NAME()
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

  if ( ast->as.array.store_tid != TS_NONE ) {
    if ( !C_LANG_IS(C_MIN(99)) ) {
      print_error( &ast->loc,
        "\"%s\" arrays not supported in %s\n",
        c_type_id_name_error( ast->as.array.store_tid ),
        C_LANG_NAME()
      );
      return false;
    }
    if ( !is_func_param ) {
      print_error( &ast->loc,
        "\"%s\" arrays are illegal outside of function parameters\n",
        c_type_id_name_error( ast->as.array.store_tid )
      );
      return false;
    }
  }

  c_ast_t const *const of_ast = ast->as.array.of_ast;
  switch ( of_ast->kind_id ) {
    case K_BUILTIN:
      if ( c_type_is_tid_any( &of_ast->type, TB_VOID ) ) {
        print_error( &ast->loc, "%s of %s", L_ARRAY, L_VOID );
        print_hint( "%s of %s to %s", L_ARRAY, L_POINTER, L_VOID );
        return false;
      }
      if ( c_ast_is_register( of_ast ) ) {
        error_kind_not_tid( ast, TS_REGISTER, /*newline=*/true );
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
        ast->type.base_tid == TB_NONE ) {
    print_error( &ast->loc,
      "implicit \"%s\" is illegal in %s\n", L_INT, C_LANG_NAME()
    );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TS_INLINE ) && opt_lang < LANG_CPP_17 ) {
    print_error( &ast->loc,
      "%s variables not supported in %s\n", L_INLINE, C_LANG_NAME()
    );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TS_TYPEDEF ) &&
       ast->as.builtin.bit_width > 0 ) {
    print_error( &ast->loc, "types can not have bit-field widths\n" );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TB_VOID ) && ast->parent_ast == NULL ) {
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
      c_type_name_error( c_ast_local_name_type( ast ) ),
      c_kind_name( ast->kind_id )
    );
    return false;
  }

  bool const is_constructor = ast->kind_id == K_CONSTRUCTOR;

  c_type_id_t const ok_tid = is_constructor ?
    (is_definition ? TS_CONSTRUCTOR_DEF : TS_CONSTRUCTOR_DECL) :
    (is_definition ? TS_DESTRUCTOR_DEF  : TS_DESTRUCTOR_DECL ) ;

  c_type_id_t const tid = ast->type.store_tid & c_type_id_compl( ok_tid );
  if ( tid != TS_NONE ) {
    print_error( &ast->loc,
      "%s%s can not be %s\n",
      c_kind_name( ast->kind_id ),
      is_definition ? " definitions" : "s",
      c_type_id_name_error( tid )
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
    if ( !lexer_is_english() &&
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
          "%s with underlying type not supported in %s\n",
          L_ENUM, C_LANG_NAME()
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
  void *const data = REINTERPRET_CAST( void*, is_func_param );
  return  c_ast_check_visitor( ast, c_ast_visitor_error, data ) &&
          c_ast_check_visitor( ast, c_ast_visitor_type, data );
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

  if ( ast->kind_id == K_FUNCTION && c_ast_name_eq( ast, "main" ) &&
       !c_ast_check_func_main( ast ) ) {
    return false;
  }

  return C_LANG_IS_C() ?
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
  assert( C_LANG_IS_C() );

  c_type_id_t const qual_tid = ast->type.store_tid & TS_MASK_QUALIFIER;
  if ( qual_tid != TS_NONE ) {
    print_error( &ast->loc,
      "\"%s\" %ss not supported in %s\n",
      c_type_id_name_error( qual_tid ),
      c_kind_name( ast->kind_id ),
      C_LANG_NAME()
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
  assert( C_LANG_IS_CPP() );

  if ( c_type_is_tid_any( &ast->type, TS_CONSTINIT ) ) {
    error_kind_not_tid( ast, TS_CONSTINIT, /*newline=*/true );
    return false;
  }

  if ( c_type_is_tid_any( &ast->type, TS_ANY_REFERENCE ) ) {
    if ( opt_lang < LANG_CPP_11 ) {
      print_error( &ast->loc,
        "%s qualified %ss not supported in %s\n",
        L_REFERENCE, c_kind_name( ast->kind_id ), C_LANG_NAME()
      );
      return false;
    }
    if ( c_type_is_tid_any( &ast->type, TS_EXTERN | TS_STATIC ) ) {
      print_error( &ast->loc,
        "%s qualified %ss can not be %s\n",
        L_REFERENCE, c_kind_name( ast->kind_id ),
        c_type_id_name_error(
          ast->type.store_tid & (TS_EXTERN | TS_STATIC)
        )
      );
      return false;
    }
  }

  c_type_id_t const member_func_tids =
    ast->type.store_tid & TS_MEMBER_FUNC_ONLY;
  c_type_id_t const nonmember_func_tids =
    ast->type.store_tid & TS_NONMEMBER_FUNC_ONLY;

  if ( member_func_tids != TS_NONE &&
       c_type_is_tid_any( &ast->type, TS_EXTERN | TS_STATIC ) ) {
    print_error( &ast->loc,
      "%s %ss can not be %s\n",
      c_type_id_name_error( ast->type.store_tid & (TS_EXTERN | TS_STATIC) ),
      c_kind_name( ast->kind_id ),
      c_type_id_name_error( member_func_tids )
    );
    return false;
  }

  if ( member_func_tids != TS_NONE && nonmember_func_tids != TS_NONE ) {
    print_error( &ast->loc,
      "%ss can not be %s and %s\n",
      c_kind_name( ast->kind_id ),
      c_type_id_name_error( member_func_tids ),
      c_type_id_name_error( nonmember_func_tids )
    );
    return false;
  }

  unsigned const user_overload_flags = ast->as.func.flags & C_FUNC_MASK_MEMBER;
  switch ( user_overload_flags ) {
    case C_FUNC_MEMBER:
      if ( nonmember_func_tids != TS_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s\n",
          L_MEMBER, c_kind_name( ast->kind_id ),
          c_type_id_name_error( nonmember_func_tids )
        );
        return false;
      }
      break;
    case C_FUNC_NON_MEMBER:
      if ( member_func_tids != TS_NONE ) {
        print_error( &ast->loc,
          "%s %ss can not be %s\n",
          H_NON_MEMBER, c_kind_name( ast->kind_id ),
          c_type_id_name_error( member_func_tids )
        );
        return false;
      }
      break;
  } // switch

  if ( c_type_is_tid_any( &ast->type, TS_DEFAULT | TS_DELETE ) ) {
    c_ast_t const *ret_ast = NULL;
    switch ( ast->kind_id ) {
      case K_OPERATOR:                // C& operator=(C const&)
        if ( ast->as.oper.oper_id != C_OP_EQ )
          goto only_special;
        ret_ast = ast->as.oper.ret_ast;
        if ( !c_ast_is_ref_to_tid_any( ret_ast, TB_ANY_CLASS ) )
          goto only_special;
        PJL_FALLTHROUGH;
      case K_CONSTRUCTOR: {           // C(C const&)
        if ( c_ast_params_count( ast ) != 1 )
          goto only_special;
        c_ast_t const *param_ast = c_param_ast( c_ast_params( ast ) );
        if ( !c_ast_is_ref_to_tid_any( param_ast, TB_ANY_CLASS ) )
          goto only_special;
        if ( ast->kind_id == K_OPERATOR ) {
          assert( ret_ast != NULL );
          //
          // For C& operator=(C const&), the parameter and the return type must
          // both be a reference to the same class, struct, or union.
          //
          param_ast = c_ast_unreference( param_ast );
          ret_ast = c_ast_unreference( ret_ast );
          if ( param_ast != ret_ast )
            goto only_special;
        }
        break;
      }
      default:
        goto only_special;
    } // switch
  }

  if ( c_type_is_tid_any( &ast->type, TA_NO_UNIQUE_ADDRESS ) ) {
    error_kind_not_tid( ast, TA_NO_UNIQUE_ADDRESS, /*newline=*/true );
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
    "\"%s\" can be used only for special member functions\n",
    c_type_name_error( &ast->type )
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

  if ( !c_type_is_none( &ast->type ) ) {
    print_error( &ast->loc,
      "main() can not be %s\n",
      c_type_name_error( &ast->type )
    );
    return false;
  }

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
          "main() must have 0, 2, or 3 parameters in %s\n", C_LANG_NAME()
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
        if ( !c_ast_check_func_main_char_ptr_arg( param_ast ) )
          return false;

        if ( n_params == 3 ) {          // char *envp[]
          param = param->next;
          param_ast = c_param_ast( param );
          if ( !c_ast_check_func_main_char_ptr_arg( param_ast ) )
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
 * Checks that an AST of a main() argument is either `char*[]` or `char**`
 * optionally including `const`.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is of either type.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_check_func_main_char_ptr_arg( c_ast_t const *ast ) {
  ast = c_ast_untypedef( ast );
  switch ( ast->kind_id ) {
    case K_ARRAY:                       // char *argv[]
    case K_POINTER:                     // char **argv
      if ( !c_ast_is_ptr_to_type( ast->as.parent.of_ast,
              &C_TYPE_LIT( TB_ANY, c_type_id_compl( TS_CONST ), TA_ANY ),
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
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );
  assert( opt_lang != LANG_C_KNR );

  c_ast_t const *variadic_ast = NULL, *void_ast = NULL;
  unsigned n_params = 0;

  FOREACH_PARAM( param, ast ) {
    if ( ++n_params > 1 && void_ast != NULL )
      goto only_void;

    c_ast_t const *const param_ast = c_param_ast( param );

    if ( c_ast_count_name( param_ast ) > 1 ) {
      print_error( &param_ast->loc, "parameter names can not be scoped\n" );
      return false;
    }

    c_type_id_t const param_store_tid =
      TS_MASK_STORAGE &
      param_ast->type.store_tid & c_type_id_compl( TS_REGISTER );
    if ( param_store_tid != TS_NONE ) {
      print_error( &param_ast->loc,
        "%s parameters can not be %s\n",
        c_kind_name( ast->kind_id ),
        c_type_id_name_error( param_store_tid )
      );
      return false;
    }

    switch ( param_ast->kind_id ) {
      case K_BUILTIN:
        if ( c_type_is_tid_any( &param_ast->type, TB_AUTO ) ) {
          print_error( &param_ast->loc, "parameters can not be %s\n", L_AUTO );
          return false;
        }
        if ( c_type_is_tid_any( &param_ast->type, TB_VOID ) ) {
          //
          // Ordinarily, void parameters are invalid; but a single void
          // function "parameter" is valid (as long as it doesn't have a name).
          //
          if ( !c_ast_empty_name( param_ast ) ) {
            print_error( &param_ast->loc,
              "parameters can not be %s\n", L_VOID
            );
            return false;
          }
          void_ast = param_ast;
          if ( n_params > 1 )
            goto only_void;
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
          print_error( &param_ast->loc,
            "%s requires type specifier\n", C_LANG_NAME()
          );
          return false;
        }
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
        variadic_ast = param_ast;
        continue;

      default:
        /* suppress warning */;
    } // switch

    if ( !c_ast_check_errors( param_ast, true ) )
      return false;
  } // for

  if ( ast->kind_id == K_OPERATOR && !c_ast_check_oper_params( ast ) )
    return false;

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
          "%s prototypes not supported in %s\n", L_FUNCTION, C_LANG_NAME()
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
      "overloading %s \"%s\" not supported in %s\n",
      L_OPERATOR, op->name, C_LANG_NAME()
    );
    return false;
  }

  if ( (op->flags & C_OP_MASK_OVERLOAD) == C_OP_NOT_OVERLOADABLE ) {
    print_error( &ast->loc,
      "%s %s can not be overloaded\n",
      L_OPERATOR, op->name
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
      if ( ret_ast->type.base_tid != TB_VOID ) {
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

  unsigned user_overload_flags = ast->as.oper.flags & C_OP_MASK_OVERLOAD;
  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );
  unsigned const op_overload_flags = op->flags & C_OP_MASK_OVERLOAD;
  size_t const n_params = c_ast_params_count( ast );

  char const *const op_type =
    op_overload_flags == C_OP_MEMBER     ? L_MEMBER     :
    op_overload_flags == C_OP_NON_MEMBER ? H_NON_MEMBER :
    "";
  char const *const user_type =
    user_overload_flags == C_OP_MEMBER     ? L_MEMBER     :
    user_overload_flags == C_OP_NON_MEMBER ? H_NON_MEMBER :
    op_type;

  if ( user_overload_flags == C_OP_UNSPECIFIED ) {
    //
    // If the user didn't specify either member or non-member explicitly...
    //
    switch ( op_overload_flags ) {
      //
      // ...and the operator can not be both, then assume the user meant the
      // one the operator can only be.
      //
      case C_OP_MEMBER:
      case C_OP_NON_MEMBER:
        user_overload_flags = op_overload_flags;
        break;
      //
      // ...and the operator can be either one, then infer which one based on
      // the number of parameters given.
      //
      case C_OP_MEMBER | C_OP_NON_MEMBER:
        if ( n_params == op->params_min )
          user_overload_flags = C_OP_MEMBER;
        else if ( n_params == op->params_max )
          user_overload_flags = C_OP_NON_MEMBER;
        break;
    } // switch
  }
  else if ( (user_overload_flags & op_overload_flags) == 0 ) {
    //
    // The user specified either member or non-member, but the operator can't
    // be that.
    //
    print_error( &ast->loc,
      "%s %s can only be a %s\n",
      L_OPERATOR, op->name, op_type
    );
    return false;
  }

  //
  // Determine the minimum and maximum number of parameters the operator can
  // have based on whether it's a member, non-member, or unspecified.
  //
  bool const is_ambiguous = c_oper_is_ambiguous( op );
  unsigned req_params_min = 0, req_params_max = 0;
  switch ( user_overload_flags ) {
    case C_OP_NON_MEMBER:
      // Non-member operators must always take at least one parameter (the
      // enum, class, struct, or union for which it's overloaded).
      req_params_min = is_ambiguous ? 1 : op->params_max;
      req_params_max = op->params_max;
      break;
    case C_OP_MEMBER:
      if ( op->params_max != C_OP_PARAMS_UNLIMITED ) {
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
  if ( n_params < req_params_min ) {
    if ( req_params_min == req_params_max )
same: print_error( &ast->loc,
        "%s%s%s %s must have exactly %u parameter%s\n",
        SP_AFTER( user_type ), L_OPERATOR, op->name,
        req_params_min, plural_s( req_params_min )
      );
    else
      print_error( &ast->loc,
        "%s%s%s %s must have at least %u parameter%s\n",
        SP_AFTER( user_type ), L_OPERATOR, op->name,
        req_params_min, plural_s( req_params_min )
      );
    return false;
  }
  if ( n_params > req_params_max ) {
    if ( op->params_min == req_params_max )
      goto same;
    print_error( &ast->loc,
      "%s%s%s %s can have at most %u parameter%s\n",
      SP_AFTER( user_type ), L_OPERATOR, op->name,
      op->params_max, plural_s( op->params_max )
    );
    return false;
  }

  bool const is_user_non_member = user_overload_flags == C_OP_NON_MEMBER;
  if ( is_user_non_member ) {
    //
    // Ensure non-member operators are not const, defaulted, deleted,
    // overridden, final, reference, rvalue reference, nor virtual.
    //
    c_type_id_t const member_only_tids =
      ast->type.store_tid & TS_MEMBER_FUNC_ONLY;
    if ( member_only_tids != TS_NONE ) {
      print_error( &ast->loc,
        "%s %ss can not be %s\n",
        H_NON_MEMBER, L_OPERATOR,
        c_type_id_name_error( member_only_tids )
      );
      return false;
    }

    //
    // Ensure non-member operators have at least one enum, class, struct, or
    // union parameter.
    //
    bool has_ecsu_param = false;
    FOREACH_PARAM( param, ast ) {
      c_ast_t const *const param_ast = c_param_ast( param );
      if ( c_ast_is_kind_any( param_ast, K_ENUM_CLASS_STRUCT_UNION ) ) {
        has_ecsu_param = true;
        break;
      }
    } // for
    if ( !has_ecsu_param ) {
      print_error( &ast->loc,
        "at least 1 parameter of a %s %s must be an %s"
        "; or a %s or %s %s thereto\n",
        H_NON_MEMBER, L_OPERATOR, c_kind_name( K_ENUM_CLASS_STRUCT_UNION ),
        L_REFERENCE, L_RVALUE, L_REFERENCE
      );
      return false;
    }
  }
  else if ( user_overload_flags == C_OP_MEMBER ) {
    //
    // Ensure member operators are not friend.
    //
    c_type_id_t const non_member_only_tids =
      ast->type.store_tid & TS_NONMEMBER_FUNC_ONLY;
    if ( non_member_only_tids != TS_NONE ) {
      print_error( &ast->loc,
        "%s operators can not be %s\n",
        L_MEMBER,
        c_type_id_name_error( non_member_only_tids )
      );
      return false;
    }
  }

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
      if ( is_user_non_member ) {
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
          SP_AFTER( op_type ), L_OPERATOR, op->name,
          c_type_id_name_error( TB_INT )
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
 * Checks overloaded operator `delete` and `delete[]` parameters for semantic
 * errors.
 *
 * @param ast The user-defined operator `delete` AST to check.
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
  c_ast_t const *const param_ast = c_ast_untypedef( c_param_ast( param ) );

  if ( !c_type_id_is_size_t( param_ast->type.base_tid ) ) {
    print_error( &param_ast->loc,
      "invalid parameter type for %s %s; must be std::size_t (or equivalent)\n",
      L_OPERATOR, op->name
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
    case K_NAME:
      error_unknown_name( to_ast );
      return false;
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      print_error( &ast->loc,
        "%s to %s\n",
        c_kind_name( ast->kind_id ), c_kind_name( to_ast->kind_id )
      );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( c_ast_is_register( to_ast ) ) {
    error_kind_to_type( ast, TS_REGISTER, /*newline=*/true );
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

  if ( c_type_is_tid_any( &ast->type, TS_CONST | TS_VOLATILE ) ) {
    c_type_id_t const qual_tid = ast->type.store_tid & TS_MASK_QUALIFIER;
    error_kind_not_tid( ast, qual_tid, /*newline=*/false );
    print_hint( "%s to %s", L_REFERENCE, c_type_id_name_error( qual_tid ) );
    return false;
  }

  c_ast_t const *const to_ast = ast->as.ptr_ref.to_ast;
  switch ( to_ast->kind_id ) {
    case K_NAME:
      error_unknown_name( to_ast );
      return VISITOR_ERROR_FOUND;
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      error_kind_to_kind( ast, to_ast->kind_id );
      return false;
    default:
      /* suppress warning */;
  } // switch

  if ( c_ast_is_register( to_ast ) ) {
    error_kind_to_type( ast, TS_REGISTER, /*newline=*/true );
    return false;
  }

  if ( c_type_is_tid_any( &to_ast->type, TB_VOID ) ) {
    error_kind_to_type( ast, TB_VOID, /*newline=*/false );
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
      if ( opt_lang < LANG_CPP_14 ) {
        if ( c_type_is_tid_any( &ret_ast->type, TB_AUTO ) ) {
          print_error( &ret_ast->loc,
            "\"%s\" return type not supported in %s\n",
            L_AUTO, C_LANG_NAME()
          );
          return false;
        }
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
    error_kind_not_tid( ast, TS_EXPLICIT, /*newline=*/true );
    return false;
  }

  return true;
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
  c_ast_t const *param_ast = c_ast_untypedef( c_param_ast( param ) );
  c_ast_t const *tmp_ast = NULL;

  switch ( n_params ) {
    case 1:
      switch ( param_ast->type.base_tid ) {
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
      tmp_ast = c_ast_unpointer( param_ast );
      if ( tmp_ast == NULL ||
          !(c_type_is_tid_any( &tmp_ast->type, TS_CONST ) &&
            c_type_is_tid_any( &tmp_ast->type, TB_ANY_CHAR )) ) {
        print_error( &param_ast->loc,
          "invalid parameter type for %s %s; must be one of: "
          "const (char|wchar_t|char8_t|char16_t|char32_t)*\n",
          H_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      param = param->next;
      param_ast = c_ast_untypedef( c_param_ast( param ) );
      if ( param_ast == NULL ||
           !c_type_id_is_size_t( param_ast->type.base_tid ) ) {
        print_error( &param_ast->loc,
          "invalid parameter type for %s %s; "
          "must be std::size_t (or equivalent)\n",
          H_USER_DEFINED, L_LITERAL
        );
        return false;
      }
      break;

    default:
      param = param->next->next;
      param_ast = c_param_ast( param );
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
static bool c_ast_name_eq( c_ast_t const *ast, char const *name ) {
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
static bool c_ast_visitor_error( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  bool const is_func_param = REINTERPRET_CAST( bool, data );

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
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
      if ( !c_ast_check_ret_type( ast ) )
        return VISITOR_ERROR_FOUND;
      PJL_FALLTHROUGH;

    case K_CONSTRUCTOR:
      if ( !c_ast_check_func( ast ) )
        return VISITOR_ERROR_FOUND;
      {
        bool const params_ok =
          ast->kind_id == K_USER_DEF_LITERAL ?
            c_ast_check_udef_lit_params( ast ) :
            opt_lang == LANG_C_KNR ?
              c_ast_check_func_params_knr( ast ) :
              c_ast_check_func_params( ast );
        if ( !params_ok )
          return VISITOR_ERROR_FOUND;
      }
      PJL_FALLTHROUGH;

    case K_DESTRUCTOR: {
      if ( (ast->kind_id & (K_CONSTRUCTOR | K_DESTRUCTOR)) != K_NONE &&
          !c_ast_check_ctor_dtor( ast ) ) {
        return VISITOR_ERROR_FOUND;
      }

      c_type_id_t const func_like_tid =
        ast->type.store_tid & c_type_id_compl( TS_FUNC_LIKE );
      if ( func_like_tid != TS_NONE ) {
        error_kind_not_tid( ast, func_like_tid, /*newline=*/true );
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
      if ( C_LANG_IS_C() ) {
        error_kind_not_supported( ast );
        return VISITOR_ERROR_FOUND;
      }
      PJL_FALLTHROUGH;
    case K_POINTER:
      if ( !c_ast_check_pointer( ast ) )
        return VISITOR_ERROR_FOUND;
      break;

    case K_RVALUE_REFERENCE:
      if ( opt_lang < LANG_CPP_11 ) {
        error_kind_not_supported( ast );
        return VISITOR_ERROR_FOUND;
      }
      PJL_FALLTHROUGH;
    case K_REFERENCE:
      if ( C_LANG_IS_C() ) {
        error_kind_not_supported( ast );
        return VISITOR_ERROR_FOUND;
      }
      if ( !c_ast_check_reference( ast ) )
        return VISITOR_ERROR_FOUND;
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
static bool c_ast_visitor_type( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  bool const is_func_param = REINTERPRET_CAST( bool, data );

  c_lang_id_t const lang_ids = c_type_check( &ast->type );
  if ( lang_ids != LANG_ALL ) {
    if ( lang_ids == LANG_NONE )
      print_error( &ast->loc,
        "\"%s\" is illegal\n", c_type_name_error( &ast->type )
      );
    else
      print_error( &ast->loc,
        "\"%s\" is illegal in %s\n",
        c_type_name_error( &ast->type ), C_LANG_NAME()
      );
    return VISITOR_ERROR_FOUND;
  }

  switch ( ast->kind_id ) {
    case K_USER_DEF_CONVERSION: {
      if ( c_type_is_tid_any( &ast->type, ~TS_USER_DEF_CONV) ) {
        print_error( &ast->loc,
          "%s %s %ss can only be: %s\n",
          H_USER_DEFINED, L_CONVERSION, L_OPERATOR,
          c_type_id_name_error( TS_USER_DEF_CONV )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( c_type_is_tid_any( &ast->type, TS_FRIEND ) &&
           c_ast_empty_name( ast ) ) {
        print_error( &ast->loc,
          "%s %s %s %s must use qualified name\n",
          L_FRIEND, H_USER_DEFINED, L_CONVERSION, L_OPERATOR
        );
        return VISITOR_ERROR_FOUND;
      }
      c_ast_t const *const conv_ast =
        c_ast_untypedef( ast->as.udef_conv.conv_ast );
      if ( conv_ast->kind_id == K_ARRAY ) {
        print_error( &conv_ast->loc,
          "%s %s %s can not convert to an %s",
          H_USER_DEFINED, L_CONVERSION, L_OPERATOR, L_ARRAY
        );
        print_hint( "%s to %s", L_POINTER, L_ARRAY );
        return VISITOR_ERROR_FOUND;
      }
      break;
    }

    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      data = REINTERPRET_CAST( void*, true );
      FOREACH_PARAM( param, ast ) {
        if ( !c_ast_check_visitor( c_param_ast( param ), c_ast_visitor_type,
                                   data ) ) {
          return VISITOR_ERROR_FOUND;
        }
      } // for
      if ( (ast->kind_id & (K_FUNCTION | K_OPERATOR)) != K_NONE )
        break;
      PJL_FALLTHROUGH;

    default:
      if ( !is_func_param &&
           c_type_is_tid_any( &ast->type, TA_CARRIES_DEPENDENCY ) ) {
        print_error( &ast->loc,
          "\"%s\" can only appear on functions or function parameters\n",
          c_type_id_name_error( TA_CARRIES_DEPENDENCY )
        );
        return VISITOR_ERROR_FOUND;
      }
      if ( c_type_is_tid_any( &ast->type, TA_NORETURN ) ) {
        print_error( &ast->loc,
          "\"%s\" can only appear on functions\n",
          c_type_id_name_error( TA_NORETURN )
        );
        return VISITOR_ERROR_FOUND;
      }
  } // switch

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
        error_kind_not_tid( ast, TS_RESTRICT, /*newline=*/true );
        return VISITOR_ERROR_FOUND;
    } // switch
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
static bool c_ast_visitor_warning( c_ast_t *ast, void *data ) {
  assert( ast != NULL );

  switch ( ast->kind_id ) {
    case K_ARRAY:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
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
           c_type_is_tid_any( &ret_ast->type, TB_VOID ) ) {
        print_warning( &ast->loc,
          "[[%s]] %ss can not return %s\n",
          L_NODISCARD, c_kind_name( ast->kind_id ), L_VOID
        );
      }
      if ( c_type_is_tid_any( &ast->type, TS_THROW ) &&
           opt_lang >= LANG_CPP_11 ) {
        print_warning( &ast->loc,
          "\"%s\" is deprecated in %s\n", L_THROW, C_LANG_NAME()
        );
      }

      FOREACH_PARAM( param, ast ) {
        PJL_IGNORE_RV(
          c_ast_check_visitor(
            c_param_ast( param ), c_ast_visitor_warning, data
          )
        );
      } // for
      break;
    }

    case K_BUILTIN:
      if ( c_ast_is_register( ast ) && opt_lang >= LANG_CPP_11 ) {
        print_warning( &ast->loc,
          "\"%s\" is deprecated in %s\n", L_REGISTER, C_LANG_NAME()
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

  FOREACH_SCOPE( scope, c_ast_scope( ast ), NULL ) {
    char const *const name = c_scope_data( scope )->name;
    c_keyword_t const *const k = c_keyword_find( name, LANG_ALL, C_KW_CTX_ALL );
    if ( k != NULL ) {
      print_warning( &ast->loc,
        "\"%s\" is a keyword in %s\n",
        name, c_lang_oldest_name( k->lang_ids )
      );
    }
  } // for

  return false;
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
      c_type_id_name_error( storage_ast->type.store_tid & TS_MASK_STORAGE )
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
  PJL_IGNORE_RV( c_ast_check_visitor( ast, c_ast_visitor_warning, NULL ) );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
