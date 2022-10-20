/*
**      cdecl -- C gibberish translator
**      src/c_type.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
 * Defines types and functions for C/C++ types.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_TYPE_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_type.h"
#include "c_lang.h"
#include "cdecl.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define C_TYPE_CHECK(LANG_IDS) BLOCK(       \
  c_lang_id_t const lang_ids = (LANG_IDS);  \
  if ( lang_ids != LANG_ANY )               \
    return lang_ids; )

#define C_TID_CHECK_COMBO(TID,TINFO,OK_TYPE_LANGS) C_TYPE_CHECK( \
  c_tid_check_combo( (TID), (TINFO), ARRAY_SIZE(TINFO), (OK_TYPE_LANGS) ) )

#define C_TID_CHECK_LEGAL(TID,TINFO) C_TYPE_CHECK( \
  c_tid_check_legal( (TID), (TINFO), ARRAY_SIZE(TINFO) ) )

#define C_TID_NAME_CAT(SBUF,TIDS,TIDS_SET,IN_ENGLISH,SEP,PSEP)      \
  c_tid_name_cat( (SBUF), (TIDS), (TIDS_SET), ARRAY_SIZE(TIDS_SET), \
                  (IN_ENGLISH), (SEP), (PSEP) )

/// @endcond

// extern constants
c_type_t const T_NONE             = { TB_NONE,      TS_NONE,    TA_NONE };
c_type_t const T_ANY              = { TB_ANY,       TS_ANY,     TA_ANY  };
c_type_t const T_ANY_CONST_CLASS  = { TB_ANY_CLASS, TS_CONST,   TA_NONE };
c_type_t const T_TS_TYPEDEF       = { TB_NONE,      TS_TYPEDEF, TA_NONE };

///////////////////////////////////////////////////////////////////////////////

/**
 * Mapping between C type bits, valid language(s), and literals.
 */
struct c_type_info {
  c_tid_t             tid;              ///< The type.
  c_lang_id_t         lang_ids;         ///< Language(s) OK in.
  char const         *english_lit;      ///< English version (if not NULL).

  /**
   * Array of language(s)/literal pair(s).  The array is terminated by an
   * element that has #LANG_ANY for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_lit;
};
typedef struct c_type_info c_type_info_t;

// local functions
NODISCARD
static char const*  c_type_literal( c_type_info_t const*, bool );

///////////////////////////////////////////////////////////////////////////////

/**
 * Literal for `long long`.
 *
 * @remarks
 * As part of the special case for `long long`, its literal is only `long`
 * because its type, #TB_LONG_LONG, is always combined with #TB_LONG, i.e., two
 * bits are set.  Therefore, when printed, it prints one `long` for #TB_LONG
 * and another `long` for #TB_LONG_LONG (this literal).  That explains why this
 * literal is only one `long`.
 */
static char const L_long_long[] = "long";

/**
 * Literal for `rvalue reference`.

 * @remarks
 * For convenience, this is just a concatenation of `L_rvalue` and
 * `L_reference`.
 */
static char const L_rvalue_reference[] = "rvalue reference";

/**
 * "Literal" for a `typedef` type, e.g., `size_t`.
 *
 * @remarks
 * #TB_TYPEDEF exists only so there can be a row/column for it in the \ref
 * OK_TYPE_LANGS table to make things like `signed size_t` illegal.
 * #TB_TYPEDEF doesn't have any printable representation (only the name of the
 * type is printed); therefore, its literal is the empty string.
 */
static char const L_typedef_TYPE[] = "";

/**
 * Type mapping for attributes.
 */
static c_type_info_t const C_ATTRIBUTE_INFO[] = {
  { TA_CARRIES_DEPENDENCY, LANG_carries_dependency, "carries dependency",
    C_LANG_LIT( { LANG_ANY, L_carries_dependency } ) },

  { TA_DEPRECATED, LANG_deprecated, NULL,
    C_LANG_LIT( { LANG_ANY, L_deprecated } ) },

  { TA_MAYBE_UNUSED, LANG_maybe_unused, "maybe unused",
    C_LANG_LIT( { LANG_ANY, L_maybe_unused } ) },

  { TA_NODISCARD, LANG_nodiscard, H_non_discardable,
    C_LANG_LIT( { LANG_ANY, L_nodiscard } ) },

  { TA_NORETURN, LANG_NONRETURNING_FUNC, H_non_returning,
    C_LANG_LIT( { LANG_noreturn, L_noreturn  },
                { LANG_ANY,      L__Noreturn } ) },

  { TA_NO_UNIQUE_ADDRESS, LANG_no_unique_address, H_non_unique_address,
    C_LANG_LIT( { LANG_ANY, L_no_unique_address } ) },

  // Microsoft extensions
  { TA_MSC_CDECL, LANG_MSC_EXTENSIONS, L_MSC_cdecl,
    C_LANG_LIT( { LANG_ANY, L_MSC___cdecl } ) },

  { TA_MSC_CLRCALL, LANG_MSC_EXTENSIONS, L_MSC_clrcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___clrcall } ) },

  { TA_MSC_FASTCALL, LANG_MSC_EXTENSIONS, L_MSC_fastcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___fastcall } ) },

  { TA_MSC_STDCALL, LANG_MSC_EXTENSIONS, L_MSC_stdcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___stdcall } ) },

  { TA_MSC_THISCALL, LANG_MSC_EXTENSIONS, L_MSC_thiscall,
    C_LANG_LIT( { LANG_ANY, L_MSC___thiscall } ) },

  { TA_MSC_VECTORCALL, LANG_MSC_EXTENSIONS, L_MSC_vectorcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___vectorcall } ) },
};

/**
 * Type mapping for qualifiers.
 *
 * @remarks
 * Even though `const`, `restrict`, and `volatile` weren't supported until C89,
 * they're allowed in all languages since **cdecl** supports their GNU
 * extension counterparts of `__const`, `__restrict`, and `__volatile` in K&R
 * C.
 *
 * @note This array _must_ have the same size and order as OK_QUALIFIER_LANGS.
 */
static c_type_info_t const C_QUALIFIER_INFO[] = {
  { TS_ATOMIC, LANG__Atomic, L_atomic,
    C_LANG_LIT( { LANG_ANY, L__Atomic } ) },

  { TS_CONST, LANG_ANY, L_constant,
    C_LANG_LIT( { ~LANG_const, L_GNU___const },
                { LANG_ANY,    L_const       } ) },

  { TS_REFERENCE, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_reference } ) },

  { TS_RVALUE_REFERENCE, LANG_CPP_MIN(11), NULL,
    C_LANG_LIT( { LANG_ANY, L_rvalue_reference } ) },

  { TS_RESTRICT, LANG_ANY, L_restricted,
    C_LANG_LIT( { ~LANG_restrict, L_GNU___restrict },
                { LANG_ANY,       L_restrict       } ) },

  { TS_VOLATILE, LANG_ANY, NULL,
    C_LANG_LIT( { ~LANG_volatile, L_GNU___volatile },
                { LANG_ANY,       L_volatile       } ) },

  // Unified Parallel C extensions
  { TS_UPC_RELAXED, LANG_C_99, NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_relaxed } ) },

  { TS_UPC_SHARED, LANG_C_99, NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_shared } ) },

  { TS_UPC_STRICT, LANG_C_99, NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_strict } ) },
};

/**
 * Type mapping for storage classes (or storage-class-like).
 *
 * @note
 * This array _must_ have the same size and order as OK_STORAGE_LANGS.
 */
static c_type_info_t const C_STORAGE_INFO[] = {
  // storage classes
  { TS_AUTO, LANG_auto_STORAGE, L_automatic,
    C_LANG_LIT( { LANG_ANY, L_auto } ) },

  { TS_APPLE_BLOCK, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_Apple___block } ) },

  { TS_EXTERN, LANG_ANY, L_external,
    C_LANG_LIT( { LANG_ANY, L_extern } ) },

  { TS_EXTERN_C, LANG_CPP_ANY, "external \"C\" linkage",
    C_LANG_LIT( { LANG_ANY, "extern \"C\"" } ) },

  { TS_REGISTER, LANG_register, NULL,
    C_LANG_LIT( { LANG_ANY, L_register } ) },

  { TS_STATIC, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_static } ) },

  { TS_THREAD_LOCAL, LANG_ANY, "thread local",
    C_LANG_LIT( { ~LANG_THREAD_LOCAL_STORAGE,  L_GNU___thread  },
                { LANG_thread_local,           L_thread_local  },
                { LANG_ANY,                    L__Thread_local } ) },

  { TS_TYPEDEF, LANG_ANY, L_type,
    C_LANG_LIT( { LANG_ANY, L_typedef } ) },

  // storage-class-like
  { TS_CONSTEVAL, LANG_consteval, "constant evaluation",
    C_LANG_LIT( { LANG_ANY, L_consteval } ) },

  { TS_CONSTEXPR, LANG_constexpr, "constant expression",
    C_LANG_LIT( { LANG_ANY, L_constexpr } ) },

  { TS_CONSTINIT, LANG_constinit, "constant initialization",
    C_LANG_LIT( { LANG_ANY, L_constinit } ) },

  { TS_DEFAULT, LANG_default_delete_FUNC, NULL,
    C_LANG_LIT( { LANG_ANY, L_default } ) },

  { TS_DELETE, LANG_default_delete_FUNC, L_deleted,
    C_LANG_LIT( { LANG_ANY, L_delete } ) },

  { TS_EXPLICIT, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_explicit } ) },

  { TS_EXPORT, LANG_export, L_exported,
    C_LANG_LIT( { LANG_ANY, L_export } ) },

  { TS_FINAL, LANG_final, NULL,
    C_LANG_LIT( { LANG_ANY, L_final } ) },

  { TS_FRIEND, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_friend } ) },

  { TS_INLINE, LANG_ANY, NULL,
    C_LANG_LIT( { ~LANG_inline, L_GNU___inline },
                { LANG_ANY,     L_inline       } ) },

  { TS_MUTABLE, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_mutable } ) },

  { TS_NOEXCEPT, LANG_noexcept, H_no_exception,
    C_LANG_LIT( { LANG_ANY, L_noexcept } ) },

  { TS_OVERRIDE, LANG_override, L_overridden,
    C_LANG_LIT( { LANG_ANY, L_override } ) },

  { TS_THIS, LANG_EXPLICIT_OBJ_PARAM_DECL, NULL,
    C_LANG_LIT( { LANG_ANY, L_this } ) },

  { TS_THROW, LANG_CPP_ANY, H_non_throwing,
    C_LANG_LIT( { LANG_ANY, L_throw } ) },

  { TS_VIRTUAL, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_virtual } ) },

  { TS_PURE_VIRTUAL, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_pure } ) },
};

/**
 * Type mapping for simpler types.
 *
 * @note
 * This array _must_ have the same size and order as OK_TYPE_LANGS.
 */
static c_type_info_t const C_TYPE_INFO[] = {
  { TB_VOID, LANG_void, NULL,
    C_LANG_LIT( { LANG_ANY, L_void } ) },

  { TB_AUTO, LANG_MIN(C_89), L_automatic,
    C_LANG_LIT( { ~LANG_auto_TYPE, L_GNU___auto_type },
                { LANG_ANY,        L_auto            } ) },

  { TB_BITINT, LANG__BitInt, NULL,
    C_LANG_LIT( { LANG_ANY, L__BitInt } ) },

  { TB_BOOL, LANG_BOOL_TYPE, NULL,
    C_LANG_LIT( { LANG_bool, L_bool  },
                { LANG_ANY,  L__Bool } ) },

  { TB_CHAR, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_char } ) },

  { TB_CHAR8_T, LANG_char8_t, NULL,
    C_LANG_LIT( { LANG_ANY, L_char8_t } ) },

  { TB_CHAR16_T, LANG_char16_32_t, NULL,
    C_LANG_LIT( { LANG_ANY, L_char16_t } ) },

  { TB_CHAR32_T, LANG_char16_32_t, NULL,
    C_LANG_LIT( { LANG_ANY, L_char32_t } ) },

  { TB_WCHAR_T, LANG_wchar_t, NULL,
    C_LANG_LIT( { LANG_ANY, L_wchar_t } ) },

  { TB_SHORT, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_short } ) },

  { TB_INT, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_int } ) },

  { TB_LONG, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_long } ) },

  { TB_LONG_LONG, LANG_long_long, NULL,
    C_LANG_LIT( { LANG_ANY, L_long_long } ) },

  { TB_SIGNED, LANG_ANY, NULL,
    C_LANG_LIT( { ~LANG_signed, L_GNU___signed },
                { LANG_ANY,     L_signed       } ) },

  { TB_UNSIGNED, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_unsigned } ) },

  { TB_FLOAT, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_float } ) },

  { TB_DOUBLE, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_double } ) },

  { TB_COMPLEX, LANG_C_ANY, L_complex,
    C_LANG_LIT( { ~LANG__Complex, L_GNU___complex },
                { LANG_ANY,       L__Complex      } ) },

  { TB_IMAGINARY, LANG__Imaginary, L_imaginary,
    C_LANG_LIT( { LANG_ANY, L__Imaginary } ) },

  { TB_ENUM, LANG_enum, L_enumeration,
    C_LANG_LIT( { LANG_ANY, L_enum } ) },

  { TB_STRUCT, LANG_ANY, L_structure,
    C_LANG_LIT( { LANG_ANY, L_struct } ) },

  { TB_UNION, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_union } ) },

  { TB_CLASS, LANG_CPP_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_class } ) },

  { TB_TYPEDEF, LANG_ANY, NULL,
    C_LANG_LIT( { LANG_ANY, L_typedef_TYPE } ) },

  // Embedded C extensions
  { TB_EMC_ACCUM, LANG_C_99, L_EMC_accum,
    C_LANG_LIT( { LANG_ANY, L_EMC__Accum } ) },

  { TB_EMC_FRACT, LANG_C_99, L_EMC_fract,
    C_LANG_LIT( { LANG_ANY, L_EMC__Fract } ) },

  { TB_EMC_SAT, LANG_C_99, L_EMC_saturated,
    C_LANG_LIT( { LANG_ANY, L_EMC__Sat } ) },
};

/// @cond DOXYGEN_IGNORE

//      shorthand   legal in ...
#define ___         LANG_ANY
#define XXX         LANG_NONE
#define KNR         LANG_C_KNR
#define C89         LANG_MIN(C_89)
#define C95         LANG_MIN(C_95)
#define c99         LANG_C_99
#define C99         LANG_MIN(C_99)
#define C23         LANG_MIN(C_23)
#define C11         LANG_MIN(C_11)
#define CPP         LANG_CPP_ANY
#define P03         LANG_CPP_MIN(03)
#define P11         LANG_CPP_MIN(11)
#define P20         LANG_CPP_MIN(20)
#define P23         LANG_CPP_MIN(23)
#define E11         LANG_C_CPP_MIN(11,11)
#define E13         LANG_C_CPP_MIN(11,23)
#define E30         LANG_C_CPP_MIN(23,20)
#define E31         LANG_C_CPP_MIN(23,11)

/// @endcond

// There is no OK_ATTRIBUTE_LANGS because all combinations of attributes are
// legal.

/**
 * Legal combinations of qualifiers in languages.
 *
 * @remarks
 * Even though `const`, `restrict`, and `volatile` weren't supported until C89,
 * they're allowed in all languages since **cdecl** supports their GNU
 * extension counterparts of `__const`, `__restrict`, and `__volatile` in K&R
 * C.
 *
 * @note
 * This array _must_ have the same size and order as \ref C_QUALIFIER_INFO.
 */
static c_lang_id_t const OK_QUALIFIER_LANGS[][ ARRAY_SIZE( C_QUALIFIER_INFO ) ] = {
// Only the lower triangle is used.
//  ato con ref rva res vol   rel sha str
  { E13,___,___,___,___,___,  ___,___,___ }, // atomic
  { E11,___,___,___,___,___,  ___,___,___ }, // const
  { XXX,CPP,CPP,___,___,___,  ___,___,___ }, // reference
  { XXX,P11,XXX,P11,___,___,  ___,___,___ }, // rvalue reference
  { XXX,___,CPP,P11,___,___,  ___,___,___ }, // restrict
  { E11,___,CPP,P11,___,___,  ___,___,___ }, // volatile

  // Unified Parallel C extensions
  { XXX,c99,XXX,XXX,c99,c99,  c99,___,___ }, // relaxed
  { XXX,c99,XXX,XXX,c99,c99,  c99,c99,___ }, // shared
  { XXX,c99,XXX,XXX,c99,c99,  XXX,c99,c99 }, // strict
};

/**
 * Legal combinations of storage classes in languages.
 *
 * @note
 * This array _must_ have the same size and order as \ref C_STORAGE_INFO.
 */
static c_lang_id_t const OK_STORAGE_LANGS[][ ARRAY_SIZE( C_STORAGE_INFO ) ] = {
// Only the lower triangle is used.
//  aut blo ext exc reg sta thr typ   cev cex cin def del exp exp fin fri inl mut noe ove thi thr vir pur
  { ___,___,___,___,___,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// auto
  { ___,___,___,___,___,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// block
  { XXX,___,___,___,___,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// extern
  { XXX,___,___,CPP,___,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// extern C
  { XXX,___,XXX,XXX,___,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// register
  { XXX,XXX,XXX,XXX,XXX,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// static
  { XXX,___,___,P11,XXX,___,___,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// thread
  { XXX,___,XXX,CPP,XXX,XXX,XXX,___,  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// typedef

  // storage-class-like
  { P11,P11,P11,P20,XXX,P11,XXX,XXX,  P20,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// c'eval
  { C23,E31,P11,P11,C23,P11,XXX,XXX,  XXX,E31,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// c'expr
  { XXX,XXX,P20,P20,XXX,P20,P20,XXX,  XXX,XXX,P20,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// c'init
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  P11,P11,XXX,P11,___,___,___,___,___,___,___,___,___,___,___,___,___ },// default
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  P11,P11,XXX,XXX,P11,___,___,___,___,___,___,___,___,___,___,___,___ },// delete
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,P11,XXX,P11,P11,CPP,___,___,___,___,___,___,___,___,___,___,___ },// explicit
  { XXX,XXX,P20,XXX,XXX,XXX,XXX,XXX,  XXX,P20,P20,XXX,XXX,XXX,P20,___,___,___,___,___,___,___,___,___,___ },// export
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,P11,XXX,XXX,XXX,XXX,XXX,P11,___,___,___,___,___,___,___,___,___ },// final
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  P20,P11,XXX,P20,XXX,XXX,XXX,XXX,CPP,___,___,___,___,___,___,___,___ },// friend
  { XXX,XXX,___,CPP,XXX,___,XXX,XXX,  P20,P11,P20,P11,P11,CPP,P20,P11,CPP,C99,___,___,___,___,___,___,___ },// inline
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,P03,___,___,___,___,___,___ },// mutable
  { XXX,XXX,P11,CPP,XXX,P11,XXX,P11,  P20,P11,XXX,P11,P11,CPP,P20,P11,P11,P11,XXX,P11,___,___,___,___,___ },// noexcept
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,P11,XXX,XXX,XXX,XXX,XXX,P11,XXX,C11,XXX,C11,P11,___,___,___,___ },// override
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,P23,___,___,___ },// this
  { XXX,XXX,CPP,CPP,XXX,CPP,XXX,CPP,  P20,P11,XXX,P11,P11,CPP,XXX,CPP,XXX,CPP,XXX,XXX,CPP,P23,CPP,___,___ },// throw
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,P20,XXX,XXX,XXX,XXX,XXX,P11,XXX,CPP,XXX,C11,P11,XXX,CPP,CPP,___ },// virtual
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,  XXX,P20,XXX,XXX,XXX,XXX,XXX,XXX,XXX,CPP,XXX,C11,P11,XXX,CPP,CPP,CPP },// pure
};

/**
 * Legal combinations of types in languages.
 *
 * @note
 * This array _must_ have the same size and order as \ref C_TYPE_INFO.
 */
static c_lang_id_t const OK_TYPE_LANGS[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
// Only the lower triangle is used.
//  voi aut Bit boo cha ch8 c16 c32 wch sho int lon lol sig uns flo dou com ima enu str uni cla typ aca fra sat
  { C89,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// void
  { XXX,C89,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// auto
  { XXX,XXX,C23,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// _BitInt
  { XXX,XXX,XXX,C99,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// bool
  { XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char
  { XXX,XXX,XXX,XXX,XXX,E30,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char8_t
  { XXX,XXX,XXX,XXX,XXX,XXX,E11,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char16_t
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,E11,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char32_t
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C95,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// wchar_t
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// short
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// int
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// long
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,___,C99,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// long long
  { XXX,XXX,C23,XXX,C89,XXX,XXX,XXX,XXX,C89,C89,C89,C89,C89,___,___,___,___,___,___,___,___,___,___,___,___,___ },// signed
  { XXX,XXX,C23,XXX,___,XXX,XXX,XXX,XXX,___,___,___,C89,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___ },// unsigned
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,KNR,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___ },// float
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C89,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___ },// double
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,C99,C99,___,___,___,___,___,___,___,___,___ },// complex
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,C99,XXX,C99,___,___,___,___,___,___,___,___ },// imaginary
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C89,___,___,___,___,___,___,___ },// enum
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,P11,___,___,___,___,___,___,___ },// struct
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___ },// union
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,P11,XXX,XXX,CPP,___,___,___,___ },// class
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___ },// typedef
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,XXX,C99,XXX,C99,C99,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,___,___ },// _Accum
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,XXX,C99,XXX,C99,C99,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,___ },// _Fract
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,XXX,C99,XXX,C99,C99,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,C99,C99,C99 },// _Sat
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a tids is some form of `long int` only, and _not_ one of
 * `long float` (K&R), `long double` (C89), or either `long _Accum` or `long
 * _Fract` (Embedded C).
 *
 * @param tids The \ref c_tid_t to check.
 * @return Returns `true` only if \a tids is some form of `long int`.
 */
NODISCARD
static inline bool c_tid_is_long_int( c_tid_t tids ) {
  return  c_tid_tpid( tids ) == C_TPID_BASE &&
          c_tid_is_except_any( tids, TB_LONG, TB_ANY_FLOAT | TB_ANY_EMC );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks that the type combination is legal in the current language.
 *
 * @param tids The \ref c_tid_t to check.
 * @param type_infos The array of \ref c_type_info to check against.
 * @param type_infos_size The size of \a type_infos.
 * @param type_langs The type/languages array to check against.
 * @return Returns the bitwise-or of the language(s) \a tids is legal in.
 */
NODISCARD
static c_lang_id_t
c_tid_check_combo( c_tid_t tids, c_type_info_t const type_infos[const],
                   size_t type_infos_size,
                   c_lang_id_t const type_langs[const][type_infos_size] ) {
  for ( size_t row = 0; row < type_infos_size; ++row ) {
    if ( !c_tid_is_none( tids & type_infos[ row ].tid ) ) {
      for ( size_t col = 0; col <= row; ++col ) {
        c_lang_id_t const lang_ids = type_langs[ row ][ col ];
        if ( !c_tid_is_none( tids & type_infos[ col ].tid ) &&
             !opt_lang_is_any( lang_ids ) ) {
          return lang_ids;
        }
      } // for
    }
  } // for
  return LANG_ANY;
}

/**
 * Checks that \a tids is legal in the current language.
 *
 * @param tids The \ref c_tid_t to check.
 * @param type_infos The array of \ref c_type_info to check against.
 * @param type_infos_size The size of \a type_infos.
 * @return Returns the bitwise-or of the language(s) \a tids is legal in.
 */
NODISCARD
static c_lang_id_t
c_tid_check_legal( c_tid_t tids, c_type_info_t const type_infos[const],
                   size_t type_infos_size ) {
  for ( size_t row = 0; row < type_infos_size; ++row ) {
    c_type_info_t const *const ti = &type_infos[ row ];
    if ( !c_tid_is_none( tids & ti->tid ) && !opt_lang_is_any( ti->lang_ids ) )
      return ti->lang_ids;
  } // for
  return LANG_ANY;
}

/**
 * Gets the name of an individual type.
 *
 * @param tid The \ref c_tid_t to get the name for; \a tid _must_ have _exactly
 * one_ bit set.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @return Returns said name.
 */
NODISCARD
static char const* c_tid_name_1( c_tid_t tid, bool in_english ) {
  assert( is_1_bit( c_tid_no_tpid( tid ) ) );

  switch ( c_tid_tpid( tid ) ) {
    case C_TPID_NONE:
      break;                            // LCOV_EXCL_LINE

    case C_TPID_BASE:
      for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_TYPE_INFO[i];
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english );
      } // for
      break;                            // LCOV_EXCL_LINE

    case C_TPID_STORE:
      for ( size_t i = 0; i < ARRAY_SIZE( C_QUALIFIER_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_QUALIFIER_INFO[i];
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english );
      } // for

      for ( size_t i = 0; i < ARRAY_SIZE( C_STORAGE_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_STORAGE_INFO[i];
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english );
      } // for
      break;                            // LCOV_EXCL_LINE

    case C_TPID_ATTR:
      for ( size_t i = 0; i < ARRAY_SIZE( C_ATTRIBUTE_INFO ); ++i ) {
        c_type_info_t const *const ti = &C_ATTRIBUTE_INFO[i];
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english );
      } // for
      break;                            // LCOV_EXCL_LINE
  } // switch

  UNEXPECTED_INT_VALUE( tid );
}

/**
 * Concatenates the partial type name onto the full type name being made.
 *
 * @param sbuf A pointer to the buffer to concatenate the name to.
 * @param tids The \ref c_tid_t to concatenate the name of.
 * @param tids_set The array of types to use.
 * @param tids_set_size The size of \a tids_set.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @param sep The separator character.
 * @param sep_flag A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
static void c_tid_name_cat( strbuf_t *sbuf, c_tid_t tids,
                            c_tid_t const tids_set[const],
                            size_t tids_set_size, bool in_english,
                            char sep, bool *sep_flag ) {
  for ( size_t i = 0; i < tids_set_size; ++i ) {
    if ( !c_tid_is_none( tids & tids_set[i] ) ) {
      char const *const name = c_tid_name_1( tids_set[i], in_english );
      strbuf_sepc_puts( sbuf, sep, sep_flag, name );
    }
  } // for
}

/**
 * Removes #TB_SIGNED from \a btids, if possible.  Specifically:
 *
 * + If \a btids contains #TB_SIGNED but not #TB_CHAR, removes #TB_SIGNED.
 * + If \a btids then becomes #TB_NONE, makes it #TB_INT.
 *
 * @param btids The \ref c_tid_t to remove #TB_SIGNED from, if possible.
 * @return Returns a \ref c_tid_t with #TB_SIGNED removed, if possible.
 *
 * @note This function isn't called `c_tid_unsigned` to avoid confusion with
 * the `unsigned` type.
 *
 * @sa c_tid_normalize()
 */
NODISCARD
static c_tid_t c_tid_nosigned( c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );
  if ( c_tid_is_except_any( btids, TB_SIGNED, TB_CHAR ) ) {
    btids &= c_tid_compl( TB_SIGNED );
    if ( btids == TB_NONE )
      btids = TB_INT;
  }
  return btids;
}

/**
 * Creates a \ref c_type based on the \ref c_tpid "type part ID" of \a tids.
 *
 * @param tids The \ref c_tid_t to create the \ref c_type from.
 * @return Returns said \ref c_type.
 */
NODISCARD
static c_type_t c_tid_to_type( c_tid_t tids ) {
  switch ( c_tid_tpid( tids ) ) {
    case C_TPID_NONE:
      break;                            // LCOV_EXCL_LINE
    case C_TPID_BASE:
      return C_TYPE_LIT_B( tids );
    case C_TPID_STORE:
      return C_TYPE_LIT_S( tids );
    case C_TPID_ATTR:
      return C_TYPE_LIT_A( tids );
  } // switch

  UNEXPECTED_INT_VALUE( tids );
}

/**
 * Gets a pointer to the \ref c_tid_t of \a type that corresponds to the \ref
 * c_tpid "type part ID" of \a tids.
 *
 * @param type The \ref c_type to get a pointer to the relevant \ref c_tid_t
 * of.
 * @param tids The \ref c_tid_t that specifies the part of \a type to get the
 * pointer to.
 * @return Returns a pointer to the corresponding \ref c_tid_t of \a type for
 * the part of \a tids.
 *
 * @sa c_type_get_tid()
 */
NODISCARD
static c_tid_t* c_type_get_tid_ptr( c_type_t *type, c_tid_t tids ) {
  assert( type != NULL );

  switch ( c_tid_tpid( tids ) ) {
    case C_TPID_NONE:
      break;                            // LCOV_EXCL_LINE
    case C_TPID_BASE:
      return &type->btids;
    case C_TPID_STORE:
      return &type->stids;
    case C_TPID_ATTR:
      return &type->atids;
  } // switch

  UNEXPECTED_INT_VALUE( tids );
}

/**
 * Gets the literal of a given \ref c_type_info, either gibberish or, if
 * appropriate and available, pseudo-English.
 *
 * @param ti The \ref c_type_info to get the literal of.
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @return Returns said literal.
 */
NODISCARD
static char const* c_type_literal( c_type_info_t const *ti, bool in_english ) {
  return in_english && ti->english_lit != NULL ?
    ti->english_lit : c_lang_literal( ti->lang_lit );
}

/**
 * Gets the name of \a type.
 *
 * @param type The type to get the name for.
 * @param apply_explicit_ecsu If `true`, apply \ref opt_explicit_ecsu.
 * @param in_english If `true`, return the pseudo-English name if possible.
 * @param is_error If `true`, the name is intended for use in an error message.
 * Specifically, c_tid_normalize() is _not_ called.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_normalize()
 * @sa c_type_name_c()
 * @sa c_type_name_ecsu()
 * @sa c_type_name_english()
 * @sa c_type_name_error()
 */
NODISCARD
static char const* c_type_name_impl( c_type_t const *type,
                                     bool apply_explicit_ecsu, bool in_english,
                                     bool is_error ) {
  static strbuf_t sbufs[ 2 ];
  static unsigned buf_index;

  strbuf_t *const sbuf = &sbufs[ buf_index++ % ARRAY_SIZE( sbufs ) ];
  strbuf_reset( sbuf );
  bool space = false;

  c_tid_t btids = is_error ? type->btids : c_tid_nosigned( type->btids );
  c_tid_t stids = type->stids;
  c_tid_t atids = type->atids;

  if ( OPT_LANG_IS( C_ANY ) && c_tid_is_any( atids, TA_NORETURN ) ) {
    //
    // Special case: we store _Noreturn as an attribute, but in C, it's a
    // distinct keyword and printed as such instead being printed between
    // brackets [[like this]].
    //
    static c_tid_t const ATIDS[] = { TA_NORETURN };
    C_TID_NAME_CAT( sbuf, TA_NORETURN, ATIDS, in_english, ' ', &space );
    //
    // Now that we've handled _Noreturn for C, remove its bit and fall through
    // to the regular attribute-printing code.
    //
    atids &= c_tid_compl( TA_NORETURN );
  }

  if ( c_tid_is_any( atids, c_tid_compl( TA_ANY_MSC_CALL ) ) ) {
    static c_tid_t const ATIDS[] = {
      TA_CARRIES_DEPENDENCY,
      TA_DEPRECATED,
      TA_MAYBE_UNUSED,
      TA_NODISCARD,
      TA_NORETURN,                      // still here for C++'s [[noreturn]]
      TA_NO_UNIQUE_ADDRESS,
      // Microsoft calling conventions must be handled later -- see below.
    };

    bool const print_brackets =
      OPT_LANG_IS( ATTRIBUTES ) &&
      cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH && !in_english;

    bool comma = false;
    char const sep = print_brackets ? ',' : ' ';
    bool *const sep_cat = print_brackets ? &comma : &space;

    if ( print_brackets )
      strbuf_sepc_puts( sbuf, ' ', &space, graph_token_c( "[[" ) );
    C_TID_NAME_CAT( sbuf, atids, ATIDS, in_english, sep, sep_cat );
    if ( print_brackets )
      strbuf_puts( sbuf, graph_token_c( "]]" ) );
    space = true;
  }

  // Special cases.
  if ( in_english ) {
    if ( c_tid_is_any( btids, TB_ANY_MODIFIER ) &&
        !c_tid_is_any( btids, c_tid_compl( TB_ANY_MODIFIER ) ) ) {
      // In English, be explicit about "int".
      btids |= TB_INT;
    }
    if ( c_tid_is_any( stids, TS_FINAL | TS_OVERRIDE ) ) {
      // In English, either "final" or "overrride" implies "virtual".
      stids |= TS_VIRTUAL;
    }
  }
  else /* !in_english */ {
    //
    // In C++, at most one of "virtual", "override", or "final" should be
    // printed.  The type is massaged in g_print_ast() since "virtual" prints
    // before the function signature and either "override" or "final" prints
    // after.  Hence, by the time we get here, at most one bit should be set.
    //
    assert(
      is_01_bit(
        c_tid_no_tpid( stids & (TS_VIRTUAL | TS_OVERRIDE | TS_FINAL) )
      )
    );

    if ( is_explicit_int( btids ) ) {
      btids |= TB_INT;
    } else if ( c_tid_is_any( btids, TB_ANY_MODIFIER ) ) {
      // In C/C++, explicit "int" isn't needed when at least one int modifier
      // is present.
      btids &= c_tid_compl( TB_INT );
    }
  }

  // Types here MUST have a corresponding row AND column in OK_STORAGE_LANGS.
  static c_tid_t const STIDS[] = {

    // These are first so we get names like "deleted constructor".
    TS_DEFAULT,
    TS_DELETE,
    TS_EXTERN_C,

    // This is next so "typedef" comes before (almost) everything else.
    TS_TYPEDEF,

    // These are next so we get names like "static int".
    TS_AUTO,
    TS_APPLE_BLOCK,
    TS_EXPORT,
    TS_EXTERN,
    TS_FRIEND,
    TS_REGISTER,
    TS_MUTABLE,
    TS_STATIC,
    TS_THIS,
    TS_THREAD_LOCAL,

    // These are next so we get names like "static inline".
    TS_EXPLICIT,
    TS_INLINE,

    // These are next so we get names like "static inline final".
    TS_OVERRIDE,
    TS_FINAL,

    // These are next so we get names like "overridden virtual".
    TS_PURE_VIRTUAL,
    TS_VIRTUAL,
    TS_NOEXCEPT,
    TS_THROW,

    // These are next so we get names like "static inline constexpr".
    TS_CONSTEVAL,
    TS_CONSTEXPR,
    TS_CONSTINIT,
  };
  C_TID_NAME_CAT( sbuf, stids, STIDS, in_english, ' ', &space );

  c_tid_t east_stids = TS_NONE;
  if ( opt_east_const && !in_english ) {
    east_stids = stids & TS_CV;
    stids &= c_tid_compl( TS_CV );
  }

  static c_tid_t const QUAL_STIDS[] = {
    // These are before "shared" so we get names like "strict shared".
    TS_UPC_RELAXED,
    TS_UPC_STRICT,

    TS_UPC_SHARED,

    TS_CONST,
    TS_RESTRICT,
    TS_VOLATILE,

    // These are next so we get names like "const reference".
    TS_REFERENCE,
    TS_RVALUE_REFERENCE,

    // This is last so we get names like "const _Atomic".
    TS_ATOMIC,
  };
  C_TID_NAME_CAT( sbuf, stids, QUAL_STIDS, in_english, ' ', &space );

  if ( OPT_LANG_IS( CPP_ANY ) && apply_explicit_ecsu &&
       !in_english && !is_error && c_tid_is_any( btids, TB_ANY_ECSU ) ) {
    //
    // This isn't right for a declaration of an enum with a fixed type, e.g.:
    //
    //      c++decl> declare x as enum E of type int
    //      enum E : int x;             // `enum` is required
    //
    // Hence, we should _not_ mask-off the explicit enum bit for such types;
    // but the only way we can know not to do that is by checking if the AST's
    // kind is K_ENUM and its "of" type is not NULL -- but we don't have access
    // to the AST here.
    //
    // To fix this, gibberish.c has a special case that calls c_type_name_c()
    // (which sets apply_explicit_ecsu to false) instead of c_type_name_ecsu()
    // when the AST's kind is K_ENUM and its "of" type is not NULL.
    //
    btids &= opt_explicit_ecsu;
  }

  //
  // Special case: C++23 adds an _Atomic(T) macro for compatibility with C11,
  // but while _Atomic can be printed without () in C, they're required in C++:
  //
  //      _Atomic int x;                // C11 only
  //      _Atomic(int) y;               // C11 or C++23
  //
  // Note that this handles printing () only for non-typedef types; for typedef
  // types, see the similar special case for K_TYPEDEF in g_ast_print().
  //
  bool const print_parens_for_Atomic =
    OPT_LANG_IS( CPP_MIN(23) ) && !in_english &&
    c_tid_is_any( stids, TS_ATOMIC ) && !c_tid_is_any( btids, TB_TYPEDEF );

  if ( print_parens_for_Atomic ) {
    strbuf_putc( sbuf, '(' );
    space = false;
  }

  static c_tid_t const BTIDS[] = {
    // These are first so we get names like "unsigned int".
    TB_SIGNED,
    TB_UNSIGNED,

    // These are next so we get names like "unsigned long int".
    TB_SHORT,
    TB_LONG,
    TB_LONG_LONG,

    TB_VOID,
    TB_AUTO,
    TB_BITINT,
    TB_BOOL,
    TB_CHAR,
    TB_CHAR8_T,
    TB_CHAR16_T,
    TB_CHAR32_T,
    TB_WCHAR_T,
    TB_INT,
    TB_COMPLEX,
    TB_IMAGINARY,
    TB_FLOAT,
    TB_DOUBLE,
    TB_ENUM,
    TB_STRUCT,
    TB_UNION,
    TB_CLASS,

    // This is next so we get names like "unsigned long _Sat _Fract".
    TB_EMC_SAT,

    TB_EMC_ACCUM,
    TB_EMC_FRACT,
  };
  C_TID_NAME_CAT( sbuf, btids, BTIDS, in_english, ' ', &space );

  if ( print_parens_for_Atomic )
    strbuf_putc( sbuf, ')' );

  // Microsoft calling conventions must be handled here.
  static c_tid_t const MSC_CALL_ATIDS[] = {
    TA_MSC_CDECL,
    TA_MSC_CLRCALL,
    TA_MSC_FASTCALL,
    TA_MSC_STDCALL,
    TA_MSC_THISCALL,
    TA_MSC_VECTORCALL,
  };
  C_TID_NAME_CAT( sbuf, atids, MSC_CALL_ATIDS, in_english, ' ', &space );

  if ( east_stids != TS_NONE )
    C_TID_NAME_CAT( sbuf, east_stids, QUAL_STIDS, in_english, ' ', &space );

  // Really special cases.
  if ( c_tid_is_any( btids, TB_NAMESPACE ) )
    strbuf_sepc_puts( sbuf, ' ', &space, L_namespace );
  else if ( c_tid_is_any( btids, TB_SCOPE ) )
    strbuf_sepc_puts( sbuf, ' ', &space, L_scope );

  return sbuf->str != NULL ? sbuf->str : "";
}

////////// extern functions ///////////////////////////////////////////////////

bool c_type_add( c_type_t *dst_type, c_type_t const *new_type,
                 c_loc_t const *new_loc ) {
  assert( dst_type != NULL );
  assert( new_type != NULL );

  return  c_tid_add( &dst_type->btids, new_type->btids, new_loc ) &&
          c_tid_add( &dst_type->stids, new_type->stids, new_loc ) &&
          c_tid_add( &dst_type->atids, new_type->atids, new_loc );
}

bool c_type_add_tid( c_type_t *dst_type, c_tid_t new_tids,
                     c_loc_t const *new_loc ) {
  c_tid_t *const dst_tids = c_type_get_tid_ptr( dst_type, new_tids );
  return c_tid_add( dst_tids, new_tids, new_loc );
}

c_type_t c_type_and( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  return C_TYPE_LIT(
    i_type->btids & j_type->btids,
    i_type->stids & j_type->stids,
    i_type->atids & j_type->atids
  );
}

void c_type_and_eq_compl( c_type_t *dst_type, c_type_t const *rm_type ) {
  assert( dst_type != NULL );
  assert( rm_type != NULL );

  dst_type->btids &= c_tid_compl( rm_type->btids );
  dst_type->stids &= c_tid_compl( rm_type->stids );
  dst_type->atids &= c_tid_compl( rm_type->atids );
}

c_lang_id_t c_type_check( c_type_t const *type ) {
  // Check that the attribute(s) are legal in the current language.
  C_TID_CHECK_LEGAL( type->atids, C_ATTRIBUTE_INFO );

  // Check that the storage class is legal in the current language.
  C_TID_CHECK_LEGAL( type->stids, C_STORAGE_INFO );

  // Check that the type is legal in the current language.
  C_TID_CHECK_LEGAL( type->btids, C_TYPE_INFO );

  // Check that the qualifier(s) are legal in the current language.
  C_TID_CHECK_LEGAL( type->stids, C_QUALIFIER_INFO );

  // Check that the storage class combination is legal in the current language.
  C_TID_CHECK_COMBO( type->stids, C_STORAGE_INFO, OK_STORAGE_LANGS );

  // Check that the type combination is legal in the current language.
  C_TID_CHECK_COMBO( type->btids, C_TYPE_INFO, OK_TYPE_LANGS );

  // Check that the qualifier combination is legal in the current language.
  C_TID_CHECK_COMBO( type->stids, C_QUALIFIER_INFO, OK_QUALIFIER_LANGS );

  return LANG_ANY;
}

bool c_type_equiv( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  if ( i_type->stids != j_type->stids || i_type->atids != j_type->atids )
    return false;
  c_tid_t const i_btids = c_tid_normalize( i_type->btids );
  c_tid_t const j_btids = c_tid_normalize( j_type->btids );
  return i_btids == j_btids;
}

c_tid_t c_type_get_tid( c_type_t const *type, c_tid_t tids ) {
  assert( type != NULL );

  switch ( c_tid_tpid( tids ) ) {
    case C_TPID_NONE:
      break;                            // LCOV_EXCL_LINE
    case C_TPID_BASE:
      return type->btids;
    case C_TPID_STORE:
      return type->stids;
    case C_TPID_ATTR:
      return type->atids;
  } // switch

  UNEXPECTED_INT_VALUE( tids );
}

bool c_tid_add( c_tid_t *dst_tids, c_tid_t new_tids, c_loc_t const *new_loc ) {
  assert( dst_tids != NULL );
  assert( new_loc != NULL );
  assert( c_tid_tpid( *dst_tids ) == c_tid_tpid( new_tids ) );

  if ( c_tid_is_long_int( *dst_tids ) && c_tid_is_long_int( new_tids ) ) {
    //
    // Special case: if the existing type is "long" and the new type is "long",
    // turn the new type into "long long".
    //
    new_tids = TB_LONG_LONG;
  }

  if ( !c_tid_is_none( *dst_tids & new_tids ) ) {
    print_error( new_loc,
      "\"%s\" can not be combined with \"%s\"\n",
       c_tid_name_error( new_tids ), c_tid_name_error( *dst_tids )
    );
    return false;
  }

  *dst_tids |= new_tids;
  return true;
}

char const* c_tid_name_c( c_tid_t tids ) {
  c_type_t const type = c_tid_to_type( tids );
  return c_type_name_impl( &type,
    /*apply_explicit_ecsu=*/false,
    /*in_english=*/false,
    /*is_error=*/false
  );
}

char const* c_tid_name_english( c_tid_t tids ) {
  c_type_t const type = c_tid_to_type( tids );
  return c_type_name_impl( &type,
    /*apply_explicit_ecsu=*/false,
    /*in_english=*/true,
    /*is_error=*/false
  );
}


char const* c_tid_name_error( c_tid_t tids ) {
  c_type_t const type = c_tid_to_type( tids );
  // When giving an error message, return the type name in pseudo-English if
  // we're parsing pseudo-English or in C/C++ if we're parsing C/C++.
  return c_type_name_impl( &type,
    /*apply_explicit_ecsu=*/false,
    /*in_english=*/cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH,
    /*is_error=*/true
  );
}

c_tid_t c_tid_normalize( c_tid_t tids ) {
  switch ( c_tid_tpid( tids ) ) {
    case C_TPID_BASE:
      tids = c_tid_nosigned( tids );
      // If the type is only implicitly int, make it explicitly int.
      if ( c_tid_is_except_any( tids, TB_SHORT, TB_ANY_EMC ) ||
           c_tid_is_except_any( tids, TB_LONG, TB_ANY_FLOAT | TB_ANY_EMC ) ||
           c_tid_is_except_any( tids, TB_UNSIGNED, TB_CHAR | TB_ANY_EMC ) ) {
        tids |= TB_INT;
      }
      break;
    default:
      /* suppress warning */;
  } // switch
  return tids;
}

unsigned c_tid_scope_order( c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );
  switch ( btids & (TB_ANY_SCOPE | TB_ENUM) ) {
    case TB_NONE:
    case TB_SCOPE:
      return 0;
    case TB_NAMESPACE:
      return 1;
    case TB_STRUCT:
    case TB_UNION:
    case TB_CLASS:
      return 2;
    case TB_ENUM:
    case TB_ENUM | TB_CLASS:
      return 3;
  } // switch

  UNEXPECTED_INT_VALUE( btids );
}

c_tpid_t c_tid_tpid( c_tid_t tids ) {
  //
  // If tids has been complemented, e.g., ~TS_REGISTER to denote "all but
  // register," then we have to complement tids back first.
  //
  if ( c_tid_is_compl( tids ) )
    tids = ~tids;
  tids &= TX_MASK_TPID;
  assert( tids <= C_TPID_ATTR );
  return STATIC_CAST( c_tpid_t, tids );
}

bool c_type_is_any( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  if ( (j_type->stids != TS_NONE &&
          (i_type->stids & j_type->stids) == TS_NONE) ||
       (j_type->atids != TA_NONE &&
          (i_type->atids & j_type->atids) == TA_NONE) ) {
    return false;
  }

  if ( j_type->btids == TB_NONE )
    return true;
  c_tid_t const i_btids = c_tid_normalize( i_type->btids );
  c_tid_t const j_btids = c_tid_normalize( j_type->btids );
  return (i_btids & j_btids) != TB_NONE;
}

char const* c_type_name_c( c_type_t const *type ) {
  return c_type_name_impl( type,
    /*apply_explicit_ecsu=*/false,
    /*in_english=*/false,
    /*is_error=*/false
  );
}

char const* c_type_name_ecsu( c_type_t const *type ) {
  return c_type_name_impl( type,
    /*apply_explicit_ecsu=*/true,
    /*in_english=*/false,
    /*is_error=*/false
  );
}

char const* c_type_name_english( c_type_t const *type ) {
  return c_type_name_impl( type,
    /*apply_explicit_ecsu=*/false,
    /*in_english=*/true,
    /*is_error=*/false
  );
}

char const* c_type_name_error( c_type_t const *type ) {
  // See comment in c_tid_name_error().
  return c_type_name_impl( type,
    /*apply_explicit_ecsu=*/false,
    /*in_english=*/cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH,
    /*is_error=*/true
  );
}

c_type_t c_type_or( c_type_t const *i_type, c_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );

  return C_TYPE_LIT(
    i_type->btids | j_type->btids,
    i_type->stids | j_type->stids,
    i_type->atids | j_type->atids
  );
}

void c_type_or_eq( c_type_t *dst_type, c_type_t const *add_type ) {
  assert( dst_type != NULL );
  assert( add_type != NULL );

  dst_type->btids |= add_type->btids;
  dst_type->stids |= add_type->stids;
  dst_type->atids |= add_type->atids;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
