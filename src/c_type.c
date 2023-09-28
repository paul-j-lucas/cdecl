/*
**      cdecl -- C gibberish translator
**      src/c_type.c
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
#include <stddef.h>
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

#define C_TID_NAME_CAT(SBUF,TIDS,TIDS_SET,IN_ENGLISH,IS_ERROR,SEP,PSEP) \
  c_tid_name_cat( (SBUF), (TIDS), (TIDS_SET), ARRAY_SIZE(TIDS_SET),     \
                  (IN_ENGLISH), (IS_ERROR), (SEP), (PSEP) )

/// @endcond

/**
 * @addtogroup c-types-group
 * @{
 */

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries for each option.

// extern constants
c_type_t const T_NONE             = { TB_NONE,      TS_NONE,    TA_NONE };
c_type_t const T_ANY              = { TB_ANY,       TS_ANY,     TA_ANY  };
c_type_t const T_ANY_const_CLASS  = { TB_ANY_CLASS, TS_const,   TA_NONE };
c_type_t const T_const_char       = { TB_char,      TS_const,   TA_NONE };
c_type_t const T_TS_typedef       = { TB_NONE,      TS_typedef, TA_NONE };

/// @endcond

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
static char const*  c_type_literal( c_type_info_t const*, bool, bool );

///////////////////////////////////////////////////////////////////////////////

/**
 * Literal for `long long`.
 *
 * @remarks As part of the special case for `long long`, its literal is only
 * `long` because its type, #TB_long_long, is always combined with #TB_long,
 * i.e., two bits are set.  Therefore, when printed, it prints one `long` for
 * #TB_long and another `long` for #TB_long_long (this literal).  That explains
 * why this literal is only one `long`.
 */
static char const L_long_long[] = "long";

/**
 * Literal for `rvalue reference`.

 * @remarks For convenience, this is just a concatenation of `L_rvalue` and
 * `L_reference`.
 */
static char const L_rvalue_reference[] = "rvalue reference";

/**
 * "Literal" for a `typedef` type, e.g., `size_t`.
 *
 * @remarks #TB_typedef exists only so there can be a row/column for it in the
 * \ref OK_TYPE_LANGS table to make things like `signed size_t` illegal.
 * #TB_typedef doesn't have any printable representation (only the name of the
 * type is printed); therefore, its literal is the empty string.
 */
static char const L_typedef_TYPE[] = "";

/**
 * Type mapping for attributes.
 */
static c_type_info_t const C_ATTRIBUTE_INFO[] = {
  { TA_carries_dependency, LANG_carries_dependency, "carries dependency",
    C_LANG_LIT( { LANG_ANY, L_carries_dependency } ) },

  { TA_deprecated, LANG_deprecated, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_deprecated } ) },

  { TA_maybe_unused, LANG_maybe_unused, "maybe unused",
    C_LANG_LIT( { LANG_ANY, L_maybe_unused } ) },

  { TA_nodiscard, LANG_nodiscard, H_non_discardable,
    C_LANG_LIT( { LANG_ANY, L_nodiscard } ) },

  { TA_noreturn, LANG_NONRETURNING_FUNCS, H_non_returning,
    C_LANG_LIT( { LANG_noreturn, L_noreturn  },
                { LANG_ANY,      L__Noreturn } ) },

  { TA_no_unique_address, LANG_no_unique_address, H_non_unique_address,
    C_LANG_LIT( { LANG_ANY, L_no_unique_address } ) },

  { TA_reproducible, LANG_reproducible, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_reproducible } ) },

  { TA_unsequenced, LANG_unsequenced, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_unsequenced } ) },

  // Microsoft extensions
  { TA_MSC___cdecl, LANG_MSC_EXTENSIONS, L_MSC_cdecl,
    C_LANG_LIT( { LANG_ANY, L_MSC___cdecl } ) },

  { TA_MSC___clrcall, LANG_MSC_EXTENSIONS, L_MSC_clrcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___clrcall } ) },

  { TA_MSC___fastcall, LANG_MSC_EXTENSIONS, L_MSC_fastcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___fastcall } ) },

  { TA_MSC___stdcall, LANG_MSC_EXTENSIONS, L_MSC_stdcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___stdcall } ) },

  { TA_MSC___thiscall, LANG_MSC_EXTENSIONS, L_MSC_thiscall,
    C_LANG_LIT( { LANG_ANY, L_MSC___thiscall } ) },

  { TA_MSC___vectorcall, LANG_MSC_EXTENSIONS, L_MSC_vectorcall,
    C_LANG_LIT( { LANG_ANY, L_MSC___vectorcall } ) },
};

/**
 * Type mapping for qualifiers.
 *
 * @remarks Even though `const`, `restrict`, and `volatile` weren't supported
 * until C89, they're allowed in all languages since **cdecl** supports their
 * GNU extension counterparts of `__const`, `__restrict`, and `__volatile` in
 * K&R&nbsp;C.
 *
 * @note This array _must_ have the same size and order as \ref
 * OK_QUALIFIER_LANGS.
 */
static c_type_info_t const C_QUALIFIER_INFO[] = {
  { TS__Atomic, LANG__Atomic, L_atomic,
    C_LANG_LIT( { LANG_ANY, L__Atomic } ) },

  { TS_const, LANG_ANY, L_constant,
    C_LANG_LIT( { ~LANG_const, L_GNU___const },
                { LANG_ANY,    L_const       } ) },

  { TS_NON_EMPTY_ARRAY, LANG_QUALIFIED_ARRAYS, H_non_empty,
    C_LANG_LIT( { LANG_ANY, L_static } ) },

  { TS_REFERENCE, LANG_REF_QUALIFIED_FUNCS, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_reference } ) },

  { TS_RVALUE_REFERENCE, LANG_REF_QUALIFIED_FUNCS, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_rvalue_reference } ) },

  { TS_restrict, LANG_ANY, L_restricted,
    C_LANG_LIT( { ~LANG_restrict, L_GNU___restrict },
                { LANG_ANY,       L_restrict       } ) },

  { TS_volatile, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { ~LANG_volatile, L_GNU___volatile },
                { LANG_ANY,       L_volatile       } ) },

  // Unified Parallel C extensions
  { TS_UPC_relaxed, LANG_C_99, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_relaxed } ) },

  { TS_UPC_shared, LANG_C_99, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_shared } ) },

  { TS_UPC_strict, LANG_C_99, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_UPC_strict } ) },
};

/**
 * Type mapping for storage classes (or storage-class-like).
 *
 * @note This array _must_ have the same size and order as \ref
 * OK_STORAGE_LANGS.
 */
static c_type_info_t const C_STORAGE_INFO[] = {
  // storage classes
  { TS_auto, LANG_auto_STORAGE, L_automatic,
    C_LANG_LIT( { LANG_ANY, L_auto } ) },

  { TS_APPLE___block, LANG_ANY, L_Apple_block,
    C_LANG_LIT( { LANG_ANY, L_Apple___block } ) },

  { TS_extern, LANG_ANY, L_external,
    C_LANG_LIT( { LANG_ANY, L_extern } ) },

  { TS_extern_C, LANG_LINKAGE_DECLS, "external \"C\" linkage",
    C_LANG_LIT( { LANG_ANY, "extern \"C\"" } ) },

  { TS_register, LANG_register, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_register } ) },

  { TS_static, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_static } ) },

  { TS_thread_local, LANG_ANY, "thread local",
    C_LANG_LIT( { ~LANG_THREAD_LOCAL_STORAGE,  L_GNU___thread  },
                { LANG_thread_local,           L_thread_local  },
                { LANG_ANY,                    L__Thread_local } ) },

  { TS_typedef, LANG_ANY, L_type,
    C_LANG_LIT( { LANG_ANY, L_typedef } ) },

  // storage-class-like
  { TS_consteval, LANG_consteval, "constant evaluation",
    C_LANG_LIT( { LANG_ANY, L_consteval } ) },

  { TS_constexpr, LANG_constexpr, "constant expression",
    C_LANG_LIT( { LANG_ANY, L_constexpr } ) },

  { TS_constinit, LANG_constinit, "constant initialization",
    C_LANG_LIT( { LANG_ANY, L_constinit } ) },

  { TS_default, LANG_default_delete_FUNCS, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_default } ) },

  { TS_delete, LANG_default_delete_FUNCS, L_deleted,
    C_LANG_LIT( { LANG_ANY, L_delete } ) },

  { TS_explicit, LANG_explicit, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_explicit } ) },

  { TS_export, LANG_export, L_exported,
    C_LANG_LIT( { LANG_ANY, L_export } ) },

  { TS_final, LANG_final, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_final } ) },

  { TS_friend, LANG_friend, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_friend } ) },

  { TS_inline, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { ~LANG_inline, L_GNU___inline },
                { LANG_ANY,     L_inline       } ) },

  { TS_mutable, LANG_mutable, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_mutable } ) },

  { TS_noexcept, LANG_noexcept, H_no_exception,
    C_LANG_LIT( { LANG_ANY, L_noexcept } ) },

  { TS_override, LANG_override, L_overridden,
    C_LANG_LIT( { LANG_ANY, L_override } ) },

  { TS_this, LANG_EXPLICIT_OBJ_PARAM_DECLS, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_this } ) },

  { TS_throw, LANG_CPP_ANY, H_non_throwing,
    C_LANG_LIT( { LANG_ANY, L_throw } ) },

  { TS_virtual, LANG_virtual, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_virtual } ) },

  { TS_PURE_virtual, LANG_virtual, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_pure } ) },
};

/**
 * Type mapping for simpler types.
 *
 * @note This array _must_ have the same size and order as \ref OK_TYPE_LANGS.
 */
static c_type_info_t const C_TYPE_INFO[] = {
  { TB_void, LANG_void, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_void } ) },

  { TB_auto, LANG_MIN(C_89), L_automatic,
    C_LANG_LIT( { ~LANG_auto_TYPE, L_GNU___auto_type },
                { LANG_ANY,        L_auto            } ) },

  { TB__BitInt, LANG__BitInt, "bit-precise integer",
    C_LANG_LIT( { LANG_ANY, L__BitInt } ) },

  { TB_bool, LANG_BOOLEAN, L_boolean,
    C_LANG_LIT( { LANG_bool, L_bool  },
                { LANG_ANY,  L__Bool } ) },

  { TB_char, LANG_ANY, L_character,
    C_LANG_LIT( { LANG_ANY, L_char } ) },

  { TB_char8_t, LANG_char8_t, "character 8",
    C_LANG_LIT( { LANG_ANY, L_char8_t } ) },

  { TB_char16_t, LANG_char16_32_t, "character 16",
    C_LANG_LIT( { LANG_ANY, L_char16_t } ) },

  { TB_char32_t, LANG_char16_32_t, "character 32",
    C_LANG_LIT( { LANG_ANY, L_char32_t } ) },

  { TB_wchar_t, LANG_wchar_t, "wide character",
    C_LANG_LIT( { LANG_ANY, L_wchar_t } ) },

  { TB_short, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_short } ) },

  { TB_int, LANG_ANY, L_integer,
    C_LANG_LIT( { LANG_ANY, L_int } ) },

  { TB_long, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_long } ) },

  { TB_long_long, LANG_long_long, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_long_long } ) },

  { TB_signed, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { ~LANG_signed, L_GNU___signed },
                { LANG_ANY,     L_signed       } ) },

  { TB_unsigned, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_unsigned } ) },

  { TB_float, LANG_ANY, "floating point",
    C_LANG_LIT( { LANG_ANY, L_float } ) },

  { TB_double, LANG_ANY, "double precision",
    C_LANG_LIT( { LANG_ANY, L_double } ) },

  { TB__Complex, LANG_C_ANY, L_complex,
    C_LANG_LIT( { ~LANG__Complex, L_GNU___complex },
                { LANG_ANY,       L__Complex      } ) },

  { TB__Imaginary, LANG__Imaginary, L_imaginary,
    C_LANG_LIT( { LANG_ANY, L__Imaginary } ) },

  { TB_enum, LANG_enum, L_enumeration,
    C_LANG_LIT( { LANG_ANY, L_enum } ) },

  { TB_struct, LANG_ANY, L_structure,
    C_LANG_LIT( { LANG_ANY, L_struct } ) },

  { TB_union, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_union } ) },

  { TB_class, LANG_class, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_class } ) },

  { TB_typedef, LANG_ANY, .english_lit = NULL,
    C_LANG_LIT( { LANG_ANY, L_typedef_TYPE } ) },

  // Embedded C extensions
  { TB_EMC__Accum, LANG_C_99, L_EMC_accum,
    C_LANG_LIT( { LANG_ANY, L_EMC__Accum } ) },

  { TB_EMC__Fract, LANG_C_99, L_EMC_fract,
    C_LANG_LIT( { LANG_ANY, L_EMC__Fract } ) },

  { TB_EMC__Sat, LANG_C_99, L_EMC_saturated,
    C_LANG_LIT( { LANG_ANY, L_EMC__Sat } ) },
};

/// @cond DOXYGEN_IGNORE

//      shorthand   legal in ...
#define ___         LANG_ANY
#define XXX         LANG_NONE
#define ATO         LANG__Atomic
#define AUS         LANG_auto_STORAGE
#define BIT         LANG__BitInt
#define BOO         LANG_BOOLEAN
#define C08         LANG_char8_t
#define C16         LANG_char16_32_t
#define C32         LANG_char16_32_t
#define CEV         LANG_consteval
#define CEX         LANG_constexpr
#define CEX_VIR     LANG_constexpr_virtual
#define CIN         LANG_constinit
#define CLS         LANG_class
#define COM         LANG__Complex
#define CPP         LANG_CPP_ANY
#define DDF         LANG_default_delete_FUNCS
#define ENC         LANG_enum_class
#define ENU         LANG_enum
#define EXP         LANG_export
#define FIN         LANG_final
#define FRI         LANG_friend
#define IMA         LANG__Imaginary
#define INL         LANG_inline
#define LDO         LANG_long_double
#define LFL         LANG_long_float
#define LLO         LANG_long_long
#define LNK         LANG_LINKAGE_DECLS
#define MUT         LANG_mutable
#define NOE         LANG_noexcept
#define OVR         LANG_override
#define QAR         LANG_QUALIFIED_ARRAYS
#define REF         LANG_REFERENCES
#define REG         LANG_register
#define RVR         LANG_RVALUE_REFERENCES
#define SIG         LANG_signed
#define THI         LANG_EXPLICIT_OBJ_PARAM_DECLS
#define THR         LANG_throw
#define TLS         LANG_THREAD_LOCAL_STORAGE
#define UNC         LANG_unsigned_char
#define UNL         LANG_unsigned_long
#define UNS         LANG_unsigned_short
#define UPC         LANG_C_99
#define VIR         LANG_virtual
#define VOL         LANG_volatile
#define WCH         LANG_wchar_t
#define XPL         LANG_explicit

/// @endcond

// There is no OK_ATTRIBUTE_LANGS because all combinations of attributes are
// legal.

/**
 * Legal combinations of qualifiers in languages.
 *
 * @remarks Even though `const`, `restrict`, and `volatile` weren't supported
 * until C89, they're allowed in all languages since **cdecl** supports their
 * GNU extension counterparts of `__const`, `__restrict`, and `__volatile` in
 * K&R&nbsp;C.
 *
 * @note This array _must_ have the same size and order as \ref
 * C_QUALIFIER_INFO.
 */
static c_lang_id_t const OK_QUALIFIER_LANGS[ ARRAY_SIZE( C_QUALIFIER_INFO ) ][ ARRAY_SIZE( C_QUALIFIER_INFO ) ] = {
// Only the lower triangle is used.
//  ato con nea ref rva res vol   rel sha str
  { ATO,___,___,___,___,___,___,  ___,___,___ }, // atomic
  { ATO,___,___,___,___,___,___,  ___,___,___ }, // const
  { XXX,QAR,QAR,___,___,___,___,  ___,___,___ }, // non-empty (array)
  { XXX,REF,XXX,REF,___,___,___,  ___,___,___ }, // reference
  { XXX,RVR,XXX,XXX,RVR,___,___,  ___,___,___ }, // rvalue reference
  { XXX,___,QAR,REF,RVR,___,___,  ___,___,___ }, // restrict
  { ATO,___,QAR,REF,RVR,___,___,  ___,___,___ }, // volatile

  // Unified Parallel C extensions
  { XXX,UPC,XXX,XXX,XXX,UPC,UPC,  UPC,___,___ }, // relaxed
  { XXX,UPC,XXX,XXX,XXX,UPC,UPC,  UPC,UPC,___ }, // shared
  { XXX,UPC,XXX,XXX,XXX,UPC,UPC,  XXX,UPC,UPC }, // strict
};

/**
 * Legal combinations of storage classes in languages.
 *
 * @note This array _must_ have the same size and order as \ref C_STORAGE_INFO.
 */
static c_lang_id_t const OK_STORAGE_LANGS[ ARRAY_SIZE( C_STORAGE_INFO ) ][ ARRAY_SIZE( C_STORAGE_INFO ) ] = {
// Only the lower triangle is used.
//  auto    block   extern  exter-C registe static  thread  typedef c'eval  c'expr  c'init  default delete  explici export  final   friend  inline  mutable noexcep overrid this    throw   virtual pure
  { ___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// auto
  { ___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// block
  { XXX    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// extern
  { XXX    ,___    ,___    ,LNK    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// extern-C
  { XXX    ,___    ,XXX    ,XXX    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// register
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// static
  { XXX    ,___    ,___    ,TLS    ,XXX    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// thread
  { XXX    ,___    ,XXX    ,LNK    ,XXX    ,XXX    ,XXX    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// typedef

  // storage-class-like
  { XXX    ,CEV    ,CEV    ,CEV    ,XXX    ,CEV    ,XXX    ,XXX    ,CEV    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// c'eval
  { AUS&CEX,CEX    ,XXX    ,XXX    ,CEX&REG,CEX    ,XXX    ,XXX    ,XXX    ,CEX    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// c'expr
  { XXX    ,XXX    ,CIN    ,CIN    ,XXX    ,CIN    ,CIN&TLS,XXX    ,XXX    ,XXX    ,CIN    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// c'init
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEV&DDF,CEX&DDF,XXX    ,DDF    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// default
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEV&DDF,CEX&DDF,XXX    ,XXX    ,DDF    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// delete
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEX    ,XXX    ,DDF    ,DDF    ,XPL    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// explicit
  { XXX    ,XXX    ,EXP    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEX&EXP,CIN    ,XXX    ,XXX    ,XXX    ,EXP    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// export
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEX&FIN,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,FIN    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// final
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEV    ,CEX    ,XXX    ,DDF    ,XXX    ,XXX    ,XXX    ,XXX    ,FRI    ,___    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// friend
  { XXX    ,XXX    ,___    ,LNK    ,XXX    ,___    ,XXX    ,XXX    ,CEV    ,CEX    ,CIN    ,DDF    ,DDF    ,XPL    ,EXP    ,FIN    ,FRI    ,INL    ,___    ,___    ,___    ,___    ,___    ,___    ,___ },// inline
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,MUT    ,___    ,___    ,___    ,___    ,___    ,___ },// mutable
  { XXX    ,XXX    ,NOE    ,NOE    ,XXX    ,NOE    ,XXX    ,NOE    ,CEV&NOE,CEX&NOE,XXX    ,NOE    ,NOE    ,NOE    ,EXP    ,NOE    ,NOE    ,NOE    ,NOE    ,NOE    ,___    ,___    ,___    ,___    ,___ },// noexcept
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEX&OVR,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,FIN&OVR,XXX    ,OVR    ,XXX    ,NOE&OVR,OVR    ,___    ,___    ,___    ,___ },// override
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,THI    ,___    ,___    ,___ },// this
  { XXX    ,XXX    ,CPP    ,LNK    ,XXX    ,CPP    ,XXX    ,CPP    ,CEV    ,CEX    ,XXX    ,DDF    ,DDF    ,XPL    ,XXX    ,FIN    ,XXX    ,CPP    ,THR    ,XXX    ,OVR    ,THI    ,CPP    ,___    ,___ },// throw
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEX_VIR,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,FIN    ,XXX    ,VIR    ,XXX    ,NOE    ,OVR    ,XXX    ,VIR    ,VIR    ,___ },// virtual
  { XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,CEX_VIR,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,XXX    ,VIR    ,XXX    ,NOE    ,OVR    ,XXX    ,VIR    ,VIR    ,VIR },// pure
};

/**
 * Legal combinations of types in languages.
 *
 * @note This array _must_ have the same size and order as \ref C_TYPE_INFO.
 */
static c_lang_id_t const OK_TYPE_LANGS[ ARRAY_SIZE( C_TYPE_INFO ) ][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
// Only the lower triangle is used.
//  voi aut Bit boo cha ch8 c16 c32 wch sho int lon lol sig uns flo dou com ima enu str uni cla typ aca fra sat
  { VOL,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// void
  { XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// auto
  { XXX,XXX,BIT,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// _BitInt
  { XXX,XXX,XXX,BOO,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// bool
  { XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char
  { XXX,XXX,XXX,XXX,XXX,C08,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char8_t
  { XXX,XXX,XXX,XXX,XXX,XXX,C16,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char16_t
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,C32,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// char32_t
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,WCH,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// wchar_t
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// short
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// int
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// long
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,LLO,___,LLO,___,___,___,___,___,___,___,___,___,___,___,___,___,___ },// long long
  { XXX,XXX,BIT,XXX,SIG,XXX,XXX,XXX,XXX,SIG,SIG,SIG,SIG,SIG,___,___,___,___,___,___,___,___,___,___,___,___,___ },// signed
  { XXX,XXX,BIT,XXX,UNC,XXX,XXX,XXX,XXX,UNS,___,UNL,LLO,XXX,___,___,___,___,___,___,___,___,___,___,___,___,___ },// unsigned
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,LFL,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___,___ },// float
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,LDO,XXX,XXX,XXX,XXX,___,___,___,___,___,___,___,___,___,___,___ },// double
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,COM,COM,COM,___,___,___,___,___,___,___,___,___ },// complex
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,IMA,IMA,XXX,IMA,___,___,___,___,___,___,___,___ },// imaginary
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,ENU,___,___,___,___,___,___,___ },// enum
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,ENC,___,___,___,___,___,___,___ },// struct
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___,___,___ },// union
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,ENC,XXX,XXX,CLS,___,___,___,___ },// class
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,___,___,___,___ },// typedef
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,UPC,XXX,UPC,XXX,UPC,UPC,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,UPC,___,___ },// _Accum
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,UPC,XXX,UPC,XXX,UPC,UPC,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,UPC,___ },// _Fract
  { XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,UPC,XXX,UPC,XXX,UPC,UPC,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,XXX,UPC,UPC,UPC },// _Sat
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a tids is some form of `long int` only, and _not_ one of
 * `long float` (K&R&nbsp;C), `long double` (C89), or either `long _Accum` or
 * `long _Fract` (Embedded C).
 *
 * @param tids The \ref c_tid_t to check.
 * @return Returns `true` only if \a tids is some form of `long int`.
 */
NODISCARD
static inline bool c_tid_is_long_int( c_tid_t tids ) {
  return  c_tid_tpid( tids ) == C_TPID_BASE &&
          c_tid_is_except_any( tids, TB_long, TB_ANY_FLOAT | TB_ANY_EMC );
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
 * one_ bit set (not counting the type part ID).
 * @param in_english If `true`, return the pseudo-English literal if one
 * exists.
 * @param is_error If `true`, the name is intended for use in an error message.
 * @return Returns said name.
 */
NODISCARD
static char const* c_tid_name_1( c_tid_t tid, bool in_english, bool is_error ) {
  assert( is_1_bit( c_tid_no_tpid( tid ) ) );

  switch ( c_tid_tpid( tid ) ) {
    case C_TPID_NONE:
      unreachable();

    case C_TPID_BASE:
      FOREACH_ARRAY_ELEMENT( c_type_info_t, ti, C_TYPE_INFO ) {
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english, is_error );
      } // for
      unreachable();

    case C_TPID_STORE:
      FOREACH_ARRAY_ELEMENT( c_type_info_t, ti, C_QUALIFIER_INFO ) {
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english, is_error );
      } // for

      FOREACH_ARRAY_ELEMENT( c_type_info_t, ti, C_STORAGE_INFO ) {
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english, is_error );
      } // for
      unreachable();

    case C_TPID_ATTR:
      FOREACH_ARRAY_ELEMENT( c_type_info_t, ti, C_ATTRIBUTE_INFO ) {
        if ( tid == ti->tid )
          return c_type_literal( ti, in_english, is_error );
      } // for
      unreachable();
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
 * @param is_error If `true`, the name is intended for use in an error message.
 * @param sep The separator character.
 * @param sep_flag A pointer to a variable to keep track of whether \a sep has
 * been concatenated.
 */
static void c_tid_name_cat( strbuf_t *sbuf, c_tid_t tids,
                            c_tid_t const tids_set[const],
                            size_t tids_set_size,
                            bool in_english, bool is_error,
                            char sep, bool *sep_flag ) {
  for ( size_t i = 0; i < tids_set_size; ++i ) {
    if ( !c_tid_is_none( tids & tids_set[i] ) ) {
      char const *const name =
        c_tid_name_1( tids_set[i], in_english, is_error );
      strbuf_sepc_puts( sbuf, sep, sep_flag, name );
    }
  } // for
}

/**
 * Removes #TB_signed from \a btids, if possible.  Specifically:
 *
 * + If \a btids contains #TB_signed but not #TB_char, removes #TB_signed.
 * + If \a btids then becomes #TB_NONE, makes it #TB_int.
 *
 * @param btids The \ref c_tid_t to remove #TB_signed from, if possible.
 * @return Returns a \ref c_tid_t with #TB_signed removed, if possible.
 *
 * @note This function isn't called `c_tid_unsigned` to avoid confusion with
 * the `unsigned` type.
 *
 * @sa c_tid_normalize()
 */
NODISCARD
static c_tid_t c_tid_nosigned( c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );
  if ( c_tid_is_except_any( btids, TB_signed, TB_char ) ) {
    btids &= c_tid_compl( TB_signed );
    if ( btids == TB_NONE )
      btids = TB_int;
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
      return T_NONE;                    // LCOV_EXCL_LINE
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
      return NULL;                      // LCOV_EXCL_LINE
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
 * @param in_english If `true`, return the pseudo-English literal, but only if
 * one exists, \ref opt_english_types is `true`, and \a is_error is `false`.
 * @param is_error If `true`, the name is intended for use in an error message.
 * @return Returns said literal.
 */
NODISCARD
static char const* c_type_literal( c_type_info_t const *ti,
                                   bool in_english, bool is_error ) {
  return  in_english && ti->english_lit != NULL &&
          opt_english_types && !is_error ?
            ti->english_lit : c_lang_literal( ti->lang_lit );
}

/**
 * Gets the name of \a type.
 *
 * @param type The type to get the name for.
 * @param apply_explicit_ecsu If `true`, apply \ref opt_explicit_ecsu_btids.
 * @param in_english If `true`, return the pseudo-English name if possible.
 * @param is_error If `true`, the name is intended for use in an error message.
 * Specifically, c_tid_nosigned() is _not_ called.
 * @return Returns said name.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 *
 * @sa c_tid_nosigned()
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

  if ( c_tid_is_any( atids, TA_noreturn ) && !OPT_LANG_IS( noreturn ) ) {
    //
    // Special case: we store _Noreturn as an attribute, but in C through C17,
    // it's a distinct keyword and printed as such instead being printed
    // between brackets [[like this]].
    //
    static c_tid_t const ATIDS[] = { TA_noreturn };
    C_TID_NAME_CAT(
      sbuf, TA_noreturn, ATIDS, in_english, is_error, ' ', &space
    );
    //
    // Now that we've handled _Noreturn for C, remove its bit and fall through
    // to the regular attribute-printing code.
    //
    atids &= c_tid_compl( TA_noreturn );
  }

  if ( c_tid_is_any( atids, c_tid_compl( TA_ANY_MSC_CALL ) ) ) {
    static c_tid_t const ATIDS[] = {
      TA_carries_dependency,
      TA_deprecated,
      TA_maybe_unused,
      TA_nodiscard,
      TA_noreturn,                      // still here for C++'s [[noreturn]]
      TA_no_unique_address,
      TA_reproducible,
      TA_unsequenced,
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
    C_TID_NAME_CAT( sbuf, atids, ATIDS, in_english, is_error, sep, sep_cat );
    if ( print_brackets )
      strbuf_puts( sbuf, graph_token_c( "]]" ) );
    space = true;
  }

  // Special cases.
  if ( in_english ) {
    if ( c_tid_is_any( btids, TB_ANY_INT_MODIFIER ) &&
        !c_tid_is_any( btids, c_tid_compl( TB_ANY_INT_MODIFIER ) ) ) {
      // In English, be explicit about "int".
      btids |= TB_int;
    }
    if ( c_tid_is_any( stids, TS_final | TS_override ) ) {
      // In English, either "final" or "override" implies "virtual".
      stids |= TS_virtual;
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
        c_tid_no_tpid( stids & (TS_virtual | TS_override | TS_final) )
      )
    );

    if ( is_explicit_int( btids ) ) {
      btids |= TB_int;
    } else if ( c_tid_is_any( btids, TB_ANY_INT_MODIFIER ) ) {
      // In C/C++, explicit "int" isn't needed when at least one int modifier
      // is present.
      btids &= c_tid_compl( TB_int );
    }
  }

  // Types here MUST have a corresponding row AND column in OK_STORAGE_LANGS.
  static c_tid_t const STIDS[] = {

    // These are first so we get names like "deleted constructor".
    TS_default,
    TS_delete,
    TS_extern_C,

    // This is next so "typedef" comes before (almost) everything else.
    TS_typedef,

    // These are next so we get names like "static int".
    TS_auto,
    TS_APPLE___block,
    TS_export,
    TS_extern,
    TS_friend,
    TS_register,
    TS_mutable,
    TS_static,
    TS_this,
    TS_thread_local,

    // These are next so we get names like "static inline".
    TS_explicit,
    TS_inline,

    // These are next so we get names like "static inline final".
    TS_override,
    TS_final,

    // These are next so we get names like "overridden virtual".
    TS_PURE_virtual,
    TS_virtual,
    TS_noexcept,
    TS_throw,

    // These are next so we get names like "static inline constexpr".
    TS_consteval,
    TS_constexpr,
    TS_constinit,
  };
  C_TID_NAME_CAT( sbuf, stids, STIDS, in_english, is_error, ' ', &space );

  c_tid_t east_stids = TS_NONE;
  if ( opt_east_const && !in_english ) {
    east_stids = stids & TS_CV;
    stids &= c_tid_compl( TS_CV );
  }

  // Types here MUST have a corresponding row AND column in OK_QUALIFIER_LANGS.
  static c_tid_t const QUAL_STIDS[] = {
    // These are before "shared" so we get names like "strict shared".
    TS_UPC_relaxed,
    TS_UPC_strict,

    TS_UPC_shared,

    TS_const,
    TS_restrict,
    TS_volatile,
    TS_NON_EMPTY_ARRAY,

    // These are next so we get names like "const reference".
    TS_REFERENCE,
    TS_RVALUE_REFERENCE,

    // This is last so we get names like "const _Atomic".
    TS__Atomic,
  };
  C_TID_NAME_CAT( sbuf, stids, QUAL_STIDS, in_english, is_error, ' ', &space );

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
    btids &= opt_explicit_ecsu_btids;
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
    c_tid_is_any( stids, TS__Atomic ) && !c_tid_is_any( btids, TB_typedef );

  if ( print_parens_for_Atomic ) {
    strbuf_putc( sbuf, '(' );
    space = false;
  }

  // Types here MUST have a corresponding row AND column in OK_TYPE_LANGS.
  static c_tid_t const BTIDS[] = {
    // These are first so we get names like "unsigned int".
    TB_signed,
    TB_unsigned,

    // These are next so we get names like "unsigned long int".
    TB_short,
    TB_long,
    TB_long_long,

    TB_void,
    TB_auto,
    TB__BitInt,
    TB_bool,
    TB_char,
    TB_char8_t,
    TB_char16_t,
    TB_char32_t,
    TB_wchar_t,
    TB_int,
    TB__Complex,
    TB__Imaginary,
    TB_float,
    TB_double,
    TB_enum,
    TB_struct,
    TB_union,
    TB_class,

    // This is next so we get names like "unsigned long _Sat _Fract".
    TB_EMC__Sat,

    TB_EMC__Accum,
    TB_EMC__Fract,
  };
  C_TID_NAME_CAT( sbuf, btids, BTIDS, in_english, is_error, ' ', &space );

  if ( print_parens_for_Atomic )
    strbuf_putc( sbuf, ')' );

  // Microsoft calling conventions must be handled here.
  static c_tid_t const MSC_CALL_ATIDS[] = {
    TA_MSC___cdecl,
    TA_MSC___clrcall,
    TA_MSC___fastcall,
    TA_MSC___stdcall,
    TA_MSC___thiscall,
    TA_MSC___vectorcall,
  };
  C_TID_NAME_CAT(
    sbuf, atids, MSC_CALL_ATIDS, in_english, is_error, ' ', &space
  );

  if ( east_stids != TS_NONE ) {
    C_TID_NAME_CAT(
      sbuf, east_stids, QUAL_STIDS, in_english, is_error, ' ', &space
    );
  }

  // Really special cases.
  if ( c_tid_is_any( btids, TB_namespace ) )
    strbuf_sepc_puts( sbuf, ' ', &space, L_namespace );
  else if ( c_tid_is_any( btids, TB_SCOPE ) )
    strbuf_sepc_puts( sbuf, ' ', &space, L_scope );

  return empty_if_null( sbuf->str );
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
      return 0u;                        // LCOV_EXCL_LINE
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
    new_tids = TB_long_long;
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
      if ( c_tid_is_except_any( tids, TB_short, TB_ANY_EMC ) ||
           c_tid_is_except_any( tids, TB_long, TB_ANY_FLOAT | TB_ANY_EMC ) ||
           c_tid_is_except_any( tids, TB_unsigned, TB_char | TB_ANY_EMC ) ) {
        tids |= TB_int;
      }
      break;
    default:
      /* suppress warning */;
  } // switch
  return tids;
}

unsigned c_tid_scope_order( c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );
  switch ( btids & (TB_ANY_SCOPE | TB_enum) ) {
    case TB_NONE:
    case TB_SCOPE:
      return 0;
    case TB_namespace:
      return 1;
    case TB_struct:
    case TB_union:
    case TB_class:
      return 2;
    case TB_enum:
    case TB_enum | TB_class:
      return 3;
  } // switch

  UNEXPECTED_INT_VALUE( btids );
}

c_tpid_t c_tid_tpid( c_tid_t tids ) {
  //
  // If tids has been complemented, e.g., ~TS_register to denote "all but
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

/** @} */

/* vim:set et sw=2 ts=2: */
