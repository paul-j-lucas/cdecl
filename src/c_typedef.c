/*
**      cdecl -- C gibberish translator
**      src/c_typedef.c
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
 * Defines types, data, and functions for standard types and adding and looking
 * up C/C++ `typedef` or `using` declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_typedef.h"
#include "cdecl.h"
#include "c_ast.h"
#include "c_lang.h"
#include "gibberish.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

/**
 * @addtogroup c-typedef-group
 * @{
 */

/**
 * Convenience macro for specifying a \ref c_typedef literal with an AST having
 * \a SNAME.
 *
 * @param SNAME The sname.
 */
#define C_TYPEDEF_SNAME_LIT(SNAME) \
  C_TYPEDEF_AST_LIT( &(c_ast_t const){ .sname = (SNAME) } )

/**
 * Helper macro for adding a \ref predef_type to an array of them.  It includes
 * the source line number it's defined on.
 *
 * @param S The string literal of the **cdecl** command defining a type.
 */
#define PT(S)                     { S, __LINE__ }

///////////////////////////////////////////////////////////////////////////////

/**
 * Contains a **cdecl** command defining a type and the source line number it's
 * defined on.
 */
struct predef_type {
  char const *str;                      ///< **Cdecl** command defining a type.
  unsigned    line;                     ///< Source line number.
};
typedef struct predef_type predef_type_t;

/**
 * Data passed to our red-black tree visitor function.
 */
struct tdef_rb_visit_data {
  c_typedef_visit_fn_t  visit_fn;       ///< Caller's visitor function.
  void                 *v_data;         ///< Caller's optional data.
};
typedef struct tdef_rb_visit_data tdef_rb_visit_data_t;

// local variables
static c_lang_id_t  predef_lang_ids;    ///< Languages when predefining types.
static rb_tree_t    typedef_set;        ///< Global set of `typedef`s.

///////////////////////////////////////////////////////////////////////////////

/**
 * Predefined types for K&R C.
 * The types here include those shown in:
 *  + The first edition of _The C Programming Language_.
 *  + Various Version 7 Unix manual pages.
 *
 * There likely should be more, but it's hard to find documentation going back
 * that far.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_KNR_C[] = {
  PT( "typedef          char *caddr_t" ),
  PT( "typedef          long  daddr_t" ),
  PT( "typedef          int   dev_t" ),
  PT( "typedef struct _iobuf  FILE" ),
  PT( "typedef unsigned int   ino_t" ),
  PT( "typedef          int   jmp_buf[37]" ),
  PT( "typedef          long  off_t" ),
  PT( "typedef          long  time_t" ),

  PT( NULL )
};

/**
 * Predefined types for C89.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_C_89[] = {
  PT( "typedef          long    clock_t" ),
  PT( "typedef struct _dirdesc  DIR" ),
  PT( "struct                   div_t" ),
  PT( "struct               imaxdiv_t" ),
  PT( "struct                  ldiv_t" ),
  PT( "struct                 lldiv_t" ),
  PT( "typedef          int     errno_t" ),
  PT( "struct                   fpos_t" ),
  PT( "struct                   lconv" ),
  PT( "typedef          long    ptrdiff_t" ),
  PT( "typedef          int     sig_atomic_t" ),
  PT( "typedef unsigned long    size_t" ),
  PT( "typedef          long   ssize_t" ),
  PT( "typedef          void   *va_list" ),

  PT( NULL )
};

/**
 * Predefined types for C95.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_C_95[] = {
  PT( "struct                 mbstate_t" ),
  PT( "typedef          int   wctrans_t" ),
  PT( "typedef unsigned long  wctype_t" ),
  PT( "typedef          int   wint_t" ),

  PT( NULL )
};

/**
 * Predefined types for C99.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_C_99[] = {
  PT( "typedef   signed char   int8_t" ),
  PT( "typedef          short  int16_t" ),
  PT( "typedef          int    int32_t" ),
  PT( "typedef          long   int64_t" ),
  PT( "typedef unsigned char  uint8_t" ),
  PT( "typedef unsigned short uint16_t" ),
  PT( "typedef unsigned int   uint32_t" ),
  PT( "typedef unsigned long  uint64_t" ),

  PT( "typedef          long   intmax_t" ),
  PT( "typedef          long   intptr_t" ),
  PT( "typedef unsigned long  uintmax_t" ),
  PT( "typedef unsigned long  uintptr_t" ),

  PT( "typedef   signed char   int_fast8_t" ),
  PT( "typedef          short  int_fast16_t" ),
  PT( "typedef          int    int_fast32_t" ),
  PT( "typedef          long   int_fast64_t" ),
  PT( "typedef unsigned char  uint_fast8_t" ),
  PT( "typedef unsigned short uint_fast16_t" ),
  PT( "typedef unsigned int   uint_fast32_t" ),
  PT( "typedef unsigned long  uint_fast64_t" ),

  PT( "typedef   signed char   int_least8_t" ),
  PT( "typedef          short  int_least16_t" ),
  PT( "typedef          int    int_least32_t" ),
  PT( "typedef          long   int_least64_t" ),
  PT( "typedef unsigned char  uint_least8_t" ),
  PT( "typedef unsigned short uint_least16_t" ),
  PT( "typedef unsigned int   uint_least32_t" ),
  PT( "typedef unsigned long  uint_least64_t" ),

  PT( NULL )
};

/**
 * Predefined types for C11.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_C_11[] = {
  PT( "typedef _Atomic          _Bool     atomic_bool" ),
  PT( "typedef _Atomic          char      atomic_char" ),
  PT( "typedef _Atomic   signed char      atomic_schar" ),
  PT( "typedef _Atomic          char8_t   atomic_char8_t" ),
  PT( "typedef _Atomic          char16_t  atomic_char16_t" ),
  PT( "typedef _Atomic          char32_t  atomic_char32_t" ),
  PT( "typedef _Atomic          wchar_t   atomic_wchar_t" ),
  PT( "typedef _Atomic          short     atomic_short" ),
  PT( "typedef _Atomic          int       atomic_int" ),
  PT( "typedef _Atomic          long      atomic_long" ),
  PT( "typedef _Atomic          long long atomic_llong" ),
  PT( "typedef _Atomic unsigned char      atomic_uchar" ),
  PT( "typedef _Atomic unsigned short     atomic_ushort" ),
  PT( "typedef _Atomic unsigned int       atomic_uint" ),
  PT( "typedef _Atomic unsigned long      atomic_ulong" ),
  PT( "typedef _Atomic unsigned long long atomic_ullong" ),

  PT( "struct                             atomic_flag" ),
  PT( "typedef _Atomic  ptrdiff_t         atomic_ptrdiff_t" ),
  PT( "typedef _Atomic  size_t            atomic_size_t" ),

  PT( "typedef _Atomic  intmax_t          atomic_intmax_t" ),
  PT( "typedef _Atomic  intptr_t          atomic_intptr_t" ),
  PT( "typedef _Atomic uintptr_t          atomic_uintptr_t" ),
  PT( "typedef _Atomic uintmax_t          atomic_uintmax_t" ),

  PT( "typedef _Atomic  int_fast8_t       atomic_int_fast8_t" ),
  PT( "typedef _Atomic  int_fast16_t      atomic_int_fast16_t" ),
  PT( "typedef _Atomic  int_fast32_t      atomic_int_fast32_t" ),
  PT( "typedef _Atomic  int_fast64_t      atomic_int_fast64_t" ),
  PT( "typedef _Atomic uint_fast8_t       atomic_uint_fast8_t" ),
  PT( "typedef _Atomic uint_fast16_t      atomic_uint_fast16_t" ),
  PT( "typedef _Atomic uint_fast32_t      atomic_uint_fast32_t" ),
  PT( "typedef _Atomic uint_fast64_t      atomic_uint_fast64_t" ),

  PT( "typedef _Atomic  int_least8_t      atomic_int_least8_t" ),
  PT( "typedef _Atomic  int_least16_t     atomic_int_least16_t" ),
  PT( "typedef _Atomic  int_least32_t     atomic_int_least32_t" ),
  PT( "typedef _Atomic  int_least64_t     atomic_int_least64_t" ),
  PT( "typedef _Atomic uint_least8_t      atomic_uint_least8_t" ),
  PT( "typedef _Atomic uint_least16_t     atomic_uint_least16_t" ),
  PT( "typedef _Atomic uint_least32_t     atomic_uint_least32_t" ),
  PT( "typedef _Atomic uint_least64_t     atomic_uint_least64_t" ),

  PT( "typedef pthread_cond_t             cnd_t" ),
  PT( "typedef void                     (*constraint_handler_t)(const char *restrict, void *restrict, errno_t)" ),
  PT( "typedef long     double            max_align_t" ),
  PT( "typedef enum memory_order          memory_order" ),
  PT( "typedef pthread_mutex_t            mtx_t" ),
  PT( "typedef          int               once_flag" ),
  PT( "typedef unsigned long              rsize_t" ),
  PT( "typedef pthread_t                  thrd_t" ),
  PT( "typedef          int             (*thrd_start_t)(void*)" ),
  PT( "typedef          void             *tss_t" ),
  PT( "typedef          void            (*tss_dtor_t)(void*)" ),

  PT( NULL )
};

/**
 * Predefined types for C23.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_C_23[] = {
  PT( "typedef void  *nullptr_t" ),

  PT( NULL )
};

/**
 * Predefined types for Floating-point extensions for C.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_FLOATING_POINT_EXTENSIONS[] = {
  PT( "typedef          float       _Float16" ),
  PT( "typedef          _Float16    _Float16_t" ),
  PT( "typedef          float       _Float32" ),
  PT( "typedef          _Float32    _Float32_t" ),
  PT( "typedef          _Float32    _Float32x" ),
  PT( "typedef          double      _Float64" ),
  PT( "typedef          _Float64    _Float64_t" ),
  PT( "typedef          _Float64    _Float64x" ),
  PT( "typedef long     double      _Float128" ),
  PT( "typedef          _Float128   _Float128_t" ),
  PT( "typedef          _Float128   _Float128x" ),

  PT( "typedef          float       _Decimal32" ),
  PT( "typedef          _Decimal32  _Decimal32_t" ),
  PT( "typedef          double      _Decimal64" ),
  PT( "typedef          _Decimal64  _Decimal64x" ),
  PT( "typedef          _Decimal64  _Decimal64_t" ),
  PT( "typedef long     double      _Decimal128" ),
  PT( "typedef          _Decimal128 _Decimal128_t" ),
  PT( "typedef          _Decimal128 _Decimal128x" ),

  PT( "typedef          double      double_t" ),
  PT( "typedef          float       float_t" ),
  PT( "typedef long     double      long_double_t" ),

  PT( "struct                       femode_t" ),
  PT( "struct                       fenv_t" ),
  PT( "typedef unsigned short       fexcept_t" ),

  PT( NULL )
};

/**
 * Predefined types for `pthread.h`.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_PTHREAD_H[] = {
  PT( "typedef unsigned long  pthread_t" ),
  PT( "struct                 pthread_barrier_t" ),
  PT( "struct                 pthread_barrierattr_t" ),
  PT( "struct                 pthread_cond_t" ),
  PT( "struct                 pthread_condattr_t" ),
  PT( "typedef unsigned int   pthread_key_t" ),
  PT( "struct                 pthread_mutex_t" ),
  PT( "struct                 pthread_mutexattr_t" ),
  PT( "typedef          int   pthread_once_t" ),
  PT( "struct                 pthread_rwlock_t" ),
  PT( "struct                 pthread_rwlockattr_t" ),
  PT( "typedef volatile int   pthread_spinlock_t" ),

  PT( NULL )
};

/**
 * Predefined types for C++.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_CPP[] = {
  PT( "namespace std { class                    bad_alloc; }" ),
  PT( "namespace std { class                    bad_cast; }" ),
  PT( "namespace std { class                    bad_exception; }" ),
  PT( "namespace std { class                    bad_typeid; }" ),
  PT( "namespace std { class                    codecvt_base; }" ),
  PT( "namespace std { class                    ctype_base; }" ),
  PT( "namespace std { struct                   div_t; }" ),
  PT( "namespace std { struct                  ldiv_t; }" ),
  PT( "namespace std { class                    domain_error; }" ),
  PT( "namespace std { class                    ios_base; }" ),
  PT( "namespace std { enum ios_base::          event; }" ),
  PT( "namespace std { class ios_base   { using event_callback = void (*)(std::ios_base::event, std::ios_base&, int); }; }" ),
  PT( "namespace std { class                    exception; }" ),
  PT( "namespace std { class                    filebuf; }" ),
  PT( "namespace std { class                   wfilebuf; }" ),
  PT( "namespace std { class ios_base   { using fmtflags = unsigned; }; }" ),
  PT( "namespace std { class ios_base::         Init; }" ),
  PT( "namespace std { class                    invalid_argument; }" ),
  PT( "namespace std { class                    ios; }" ),
  PT( "namespace std { class                   wios; }" ),
  PT( "namespace std { class ios_base   { using iostate = unsigned; }; }" ),
  PT( "namespace std { class                    length_error; }" ),
  PT( "namespace std { class                    locale; }" ),
  PT( "namespace std { class                    logic_error; }" ),
  PT( "namespace std { class ctype_base { using mask = unsigned; }; }" ),
  PT( "namespace std { class                    messages_base; }" ),
  PT( "namespace std { class                    money_base; }" ),
  PT( "namespace std { struct                   nothrow_t; }" ),
  PT( "namespace std { class ios_base   { using openmode = unsigned; }; }" ),
  PT( "namespace std { class                    out_of_range; }" ),
  PT( "namespace std { class                    overflow_error; }" ),
  PT( "namespace std                    { using ptrdiff_t = long; }" ),
  PT( "namespace std { class                    range_error; }" ),
  PT( "namespace std { class                    runtime_error; }" ),
  PT( "namespace std { class ios_base   { using seekdir = int; }; }" ),
  PT( "namespace std                    { using sig_atomic_t = int; }" ),
  PT( "namespace std                    { using size_t = unsigned long; }" ),
  PT( "namespace std { class                   fstream; }" ),
  PT( "namespace std { class                  ifstream; }" ),
  PT( "namespace std { class                  wfstream; }" ),
  PT( "namespace std { class                 wifstream; }" ),
  PT( "namespace std { class                  ofstream; }" ),
  PT( "namespace std { class                 wofstream; }" ),
  PT( "namespace std { class                   istream; }" ),
  PT( "namespace std { class                  wistream; }" ),
  PT( "namespace std { class                  iostream; }" ),
  PT( "namespace std { class                 wiostream; }" ),
  PT( "namespace std { class                   ostream; }" ),
  PT( "namespace std { class                  wostream; }" ),
  PT( "namespace std { class                    streambuf; }" ),
  PT( "namespace std { class                   wstreambuf; }" ),
  PT( "namespace std                    { using streamoff = long long; }" ),
  PT( "namespace std                    { using streamsize = long; }" ),
  PT( "namespace std { class                    string; }" ),
  PT( "namespace std { class                   wstring; }" ),
  PT( "namespace std { class                    stringbuf; }" ),
  PT( "namespace std { class                   wstringbuf; }" ),
  PT( "namespace std { class                    stringstream; }" ),
  PT( "namespace std { class                   istringstream; }" ),
  PT( "namespace std { class                   wstringstream; }" ),
  PT( "namespace std { class                  wistringstream; }" ),
  PT( "namespace std { class                   ostringstream; }" ),
  PT( "namespace std { class                  wostringstream; }" ),
  PT( "namespace std { class                    syncbuf; }" ),
  PT( "namespace std { class                   wsyncbuf; }" ),
  PT( "namespace std { class                   osyncstream; }" ),
  PT( "namespace std { class                  wosyncstream; }" ),
  PT( "namespace std { class                    time_base; }" ),
  PT( "namespace std { class                    type_info; }" ),
  PT( "namespace std { class                    underflow_error; }" ),

  PT( NULL )
};

/**
 * Predefined types for C++11.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_STD_CPP_11[] = {
  PT( "namespace std           {    struct      adopt_lock_t; }" ),
  //
  // These atomic_* types are supposed to be typedefs, but they're typedefs to
  // instantiated templates and cdecl doesn't support templates, so we make
  // them structs instead.
  //
  PT( "namespace std          {     struct      atomic_bool; }" ),
  PT( "namespace std          {     struct      atomic_char8_t; }" ),
  PT( "namespace std          {     struct      atomic_char16_t; }" ),
  PT( "namespace std          {     struct      atomic_char32_t; }" ),
  PT( "namespace std          {     struct      atomic_char; }" ),
  PT( "namespace std          {     struct      atomic_flag; }" ),
  PT( "namespace std          {     struct      atomic_int8_t; }" ),
  PT( "namespace std          {     struct      atomic_int16_t; }" ),
  PT( "namespace std          {     struct      atomic_int32_t; }" ),
  PT( "namespace std          {     struct      atomic_int64_t; }" ),
  PT( "namespace std          {     struct      atomic_int; }" ),
  PT( "namespace std          {     struct      atomic_int_fast8_t; }" ),
  PT( "namespace std          {     struct      atomic_int_fast16_t; }" ),
  PT( "namespace std          {     struct      atomic_int_fast32_t; }" ),
  PT( "namespace std          {     struct      atomic_int_fast64_t; }" ),
  PT( "namespace std          {     struct      atomic_int_least8_t; }" ),
  PT( "namespace std          {     struct      atomic_int_least16_t; }" ),
  PT( "namespace std          {     struct      atomic_int_least32_t; }" ),
  PT( "namespace std          {     struct      atomic_int_least64_t; }" ),
  PT( "namespace std          {     struct      atomic_intmax_t; }" ),
  PT( "namespace std          {     struct      atomic_intptr_t; }" ),
  PT( "namespace std          {     struct      atomic_llong; }" ),
  PT( "namespace std          {     struct      atomic_long; }" ),
  PT( "namespace std          {     struct      atomic_ptrdiff_t; }" ),
  PT( "namespace std          {     struct      atomic_schar; }" ),
  PT( "namespace std          {     struct      atomic_short; }" ),
  PT( "namespace std          {     struct      atomic_signed_lock_free; }" ),
  PT( "namespace std          {     struct      atomic_size_t; }" ),
  PT( "namespace std          {     struct      atomic_uchar; }" ),
  PT( "namespace std          {     struct      atomic_uint16_t; }" ),
  PT( "namespace std          {     struct      atomic_uint32_t; }" ),
  PT( "namespace std          {     struct      atomic_uint64_t; }" ),
  PT( "namespace std          {     struct      atomic_uint8_t; }" ),
  PT( "namespace std          {     struct      atomic_uint; }" ),
  PT( "namespace std          {     struct      atomic_uint_fast8_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_fast16_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_fast32_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_fast64_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_least8_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_least16_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_least32_t; }" ),
  PT( "namespace std          {     struct      atomic_uint_least64_t; }" ),
  PT( "namespace std          {     struct      atomic_uintmax_t; }" ),
  PT( "namespace std          {     struct      atomic_uintptr_t; }" ),
  PT( "namespace std          {     struct      atomic_ullong; }" ),
  PT( "namespace std          {     struct      atomic_ulong; }" ),
  PT( "namespace std          {     struct      atomic_unsigned_lock_free; }" ),
  PT( "namespace std          {     struct      atomic_ushort; }" ),
  PT( "namespace std          {     struct      atomic_wchar_t; }" ),
  PT( "namespace std          {      class      bad_array_new_length; }" ),
  PT( "namespace std          {      class      bad_function_call; }" ),
  PT( "namespace std          {      class      bad_weak_ptr; }" ),
  PT( "namespace std          {      class      bernoulli_distribution; }" ),
  PT( "namespace std          {      class      condition_variable; }" ),
  PT( "namespace std          {      class      condition_variable_any; }" ),
  PT( "namespace std          { enum class      cv_status; }" ),
  PT( "namespace std          {     struct      defer_lock_t; }" ),
  PT( "namespace std          {     struct  imaxdiv_t; }" ),
  PT( "namespace std          {     struct    lldiv_t; }" ),
  PT( "namespace std          {      class      error_category; }" ),
  PT( "namespace std          {      class      error_code; }" ),
  PT( "namespace std          {      class      error_condition; }" ),
  PT( "namespace std          { class ios_base::failure; }" ),
  PT( "namespace std          { enum class      future_errc; }" ),
  PT( "namespace std          {      class      future_error; }" ),
  PT( "namespace std          { enum class      future_status; }" ),
  PT( "namespace std::chrono  {      class      high_resolution_clock; }" ),
  PT( "namespace std          { enum class      launch; }" ),
  PT( "namespace std::regex_constants { using   match_flag_type = unsigned; }" ),
  PT( "namespace std          {      using      max_align_t = long double; }" ),
  PT( "namespace std          {      class      mutex; }" ),
  PT( "namespace std          {      using      nullptr_t = void*; }" ),
  PT( "namespace std          {      class      random_device; }" ),
  PT( "namespace std          {      class      recursive_mutex; }" ),
  PT( "namespace std          {      class      recursive_timed_mutex; }" ),
  PT( "namespace std          {      class      regex; }" ),
  PT( "namespace std          {      class     wregex; }" ),
  PT( "namespace std          {     struct      regex_error; }" ),
  PT( "namespace std          {      class      shared_mutex; }" ),
  PT( "namespace std          {      class      shared_timed_mutex; }" ),
  PT( "namespace std          {      class   u16string; }" ),
  PT( "namespace std          {      class   u32string; }" ),
  PT( "namespace std::chrono  {      class      steady_clock; }" ),
  PT( "namespace std::regex_constants { using   syntax_option_type = unsigned; }" ),
  PT( "namespace std::chrono  {      class      system_clock; }" ),
  PT( "namespace std          {     struct      system_error; }" ),
  PT( "namespace std          {      class      thread; }" ),
  PT( "namespace std          {      class      timed_mutex; }" ),
  PT( "namespace std          {     struct      try_to_lock_t; }" ),
  PT( "namespace std          {      class      type_index; }" ),

  PT( NULL )
};

/**
 * Predefined types for C++17.
 */
static predef_type_t const PREDEFINED_STD_CPP_17[] = {
  PT( "namespace std             { enum class     align_val_t; }" ),
  PT( "namespace std             {      class     bad_any_cast; }" ),
  PT( "namespace std             {      class     bad_optional_access; }" ),
  PT( "namespace std             {      class     bad_variant_access; }" ),
  PT( "namespace std             { enum           byte; }" ),
  PT( "namespace std             { enum class     chars_format; }" ),
  PT( "namespace std::filesystem { enum class     copy_options; }" ),
  PT( "namespace std::filesystem {      class     directory_entry; }" ),
  PT( "namespace std::filesystem {      class     directory_iterator; }" ),
  PT( "namespace std::filesystem { enum class     directory_options; }" ),
  PT( "namespace std::filesystem {      class     file_status; }" ),
  PT( "namespace std::filesystem { enum class     file_type; }" ),
  PT( "namespace std::filesystem {      class     filesystem_error; }" ),
  PT( "namespace std::filesystem {      class     path; }" ),
  PT( "namespace std::filesystem { enum class     perms; }" ),
  PT( "namespace std::filesystem { enum class     perm_options; }" ),
  PT( "namespace std::filesystem {      class     recursive_directory_iterator; }" ),
  PT( "namespace std::filesystem {     struct     space_info; }" ),
  PT( "namespace std             {      class     string_view; }" ),
  PT( "namespace std             {      class  u16string_view; }" ),
  PT( "namespace std             {      class  u32string_view; }" ),
  PT( "namespace std             {      class    wstring_view; }" ),

  PT( NULL )
};

/**
 * Predefined types for C++20.
 */
static predef_type_t const PREDEFINED_STD_CPP_20[] = {
  PT( "namespace std         {      class   ambiguous_local_time; }" ),
  PT( "namespace std::chrono { enum class   choose; }" ),
  PT( "namespace std::chrono {      class   day; }" ),
  PT( "namespace std         {     struct   destroying_delete_t; }" ),
  PT( "namespace std::chrono {     struct   file_clock; }" ),
  PT( "namespace std         {      class   format_error; }" ),
  PT( "namespace std::chrono {     struct   gps_clock; }" ),
  PT( "namespace std::chrono {     struct   is_clock; }" ),
  PT( "namespace std         {      class   jthread; }" ),
  PT( "namespace std::chrono {     struct   last_spec; }" ),
  PT( "namespace std::chrono {      class   leap_second; }" ),
  PT( "namespace std::chrono {     struct   local_info; }" ),
  PT( "namespace std::chrono {     struct   local_t; }" ),
  PT( "namespace std::chrono {      class   month; }" ),
  PT( "namespace std::chrono {      class   month_day; }" ),
  PT( "namespace std::chrono {      class   month_day_last; }" ),
  PT( "namespace std::chrono {      class   month_weekday; }" ),
  PT( "namespace std::chrono {      class   month_weekday_last; }" ),
  PT( "namespace std::chrono {      class   nonexistent_local_time; }" ),
  PT( "namespace std         {     struct   nonstopstate_t; }" ),
  PT( "namespace std         {     struct   source_location; }" ),
  PT( "namespace std         {      class u8string_view; }" ),
  PT( "namespace std         {      class   stop_source; }" ),
  PT( "namespace std         {      class   stop_token; }" ),
  PT( "namespace std         {     struct   strong_equality; }" ),
  PT( "namespace std::chrono {     struct   sys_info; }" ),
  PT( "namespace std::chrono {     struct   tai_clock; }" ),
  PT( "namespace std::chrono {     struct   time_zone; }" ),
  PT( "namespace std::chrono {      class   time_zone_link; }" ),
  PT( "namespace std::chrono {     struct   tzdb; }" ),
  PT( "namespace std::chrono {     struct   tzdb_list; }" ),
  PT( "namespace std::chrono {     struct   utc_clock; }" ),
  PT( "namespace std::chrono {      class   weekday; }" ),
  PT( "namespace std::chrono {      class   weekday_indexed; }" ),
  PT( "namespace std::chrono {      class   weekday_last; }" ),
  PT( "namespace std         {     struct   weak_equality; }" ),
  PT( "namespace std::chrono {      class   year; }" ),
  PT( "namespace std::chrono {      class   year_month; }" ),
  PT( "namespace std::chrono {      class   year_month_day; }" ),
  PT( "namespace std::chrono {      class   year_month_day_last; }" ),
  PT( "namespace std::chrono {      class   year_month_weekday; }" ),
  PT( "namespace std::chrono {      class   year_month_weekday_last; }" ),

  PT( NULL )
};

/**
 * Predefined required types for C++20.
 */
static predef_type_t const PREDEFINED_STD_CPP_20_REQUIRED[] = {
  // required for operator <=>
  PT( "namespace std { struct partial_ordering; }" ),
  PT( "namespace std { struct strong_ordering; }" ),
  PT( "namespace std { struct weak_ordering; }" ),

  PT( NULL )
};

/**
 * Predefined types for C++23.
 */
static predef_type_t const PREDEFINED_STD_CPP_23[] = {
  PT( "namespace std {   using  float16_t = float; }" ),
  PT( "namespace std {   using bfloat16_t = float; }" ),
  PT( "namespace std {   using  float32_t = float; }" ),
  PT( "namespace std {   using  float64_t = double; }" ),
  PT( "namespace std {   using  float128_t = double[2]; }" ),
  PT( "namespace std { class    spanbuf; }" ),
  PT( "namespace std { class   wspanbuf; }" ),
  PT( "namespace std { class    spanstream; }" ),
  PT( "namespace std { class   ispanstream; }" ),
  PT( "namespace std { class  wispanstream; }" ),
  PT( "namespace std { class   ospanstream; }" ),
  PT( "namespace std { class  wospanstream; }" ),
  PT( "namespace std { class   wspanstream; }" ),
  PT( "namespace std { class    stacktrace; }" ),
  PT( "namespace std { class    stacktrace_entry; }" ),
  PT( "namespace std { struct   unexpect_t; }" ),

  PT( NULL )
};

/**
 * Embedded C types.
 *
 * @sa [Information Technology — Programming languages - C - Extensions to support embedded processors](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1169.pdf)
 */
static predef_type_t const PREDEFINED_EMBEDDED_C[] = {
  PT( "typedef          short _Accum int_hk_t" ),
  PT( "typedef          short _Fract int_hr_t" ),
  PT( "typedef                _Accum int_k_t" ),
  PT( "typedef          long  _Accum int_lk_t" ),
  PT( "typedef          long  _Fract int_lr_t" ),
  PT( "typedef                _Fract int_r_t" ),
  PT( "typedef unsigned short _Accum uint_uhk_t" ),
  PT( "typedef unsigned short _Fract uint_uhr_t" ),
  PT( "typedef unsigned       _Accum uint_uk_t" ),
  PT( "typedef unsigned long  _Accum uint_ulk_t" ),
  PT( "typedef unsigned long  _Fract uint_ulr_t" ),
  PT( "typedef unsigned       _Fract uint_ur_t" ),

  PT( NULL )
};

/**
 * Predefined GNU C types.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_GNU_C[] = {
  PT( "typedef _Float128   __float128" ),
  PT( "typedef _Float16    __fp16" ),
  PT( "typedef long double __ibm128" ),
  PT( "typedef _Float64x   __float80" ),
  //
  // In GNU C, this is a distinct type, not a typedef, which means you can add
  // type modifiers:
  //
  //      unsigned __int128 x;          // legal in GNU C
  //
  // As a typedef, that's illegal in C which means it's also illegal in cdecl.
  //
  // To make it a distinct type in cdecl also, there would need to be a
  // distinct literal, token, and type.  The type has to be distinct in order
  // to be round-trippable with pseudo-English.  If it reused TB_LONG_LONG,
  // then you'd get:
  //
  //      cdecl> declare x as __int128
  //      long long x;                  // should be: __int128
  //      cdecl> explain __int128 x
  //      declare x as long long        // should be: __int128
  //
  // At least with a typedef, you still get the typedef:
  //
  //      cdecl> declare x as __int128
  //      __int128 x;                   // correct
  //      cdecl> explain __int128 x
  //      declare x as __int128         // correct
  //
  // Hence, it's too much work to support this type as distinct and we'll live
  // with not being able to apply type modifiers.
  //
  PT( "typedef long long   __int128" ),

  PT( "typedef void      (*sighandler_t)(int)" ),

  PT( NULL )
};

/**
 * Predefined miscellaneous standard-ish types.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static predef_type_t const PREDEFINED_MISC[] = {
  PT( "typedef  int32_t       blkcnt_t" ),
  PT( "typedef  int32_t       blksize_t" ),
  PT( "typedef unsigned       cc_t" ),
  PT( "struct                 fd_set" ),
  PT( "typedef unsigned long  fsblkcnt_t" ),
  PT( "typedef unsigned long  fsfilcnt_t" ),
  PT( "typedef void          *iconv_t" ),
  PT( "typedef  int32_t       key_t" ),
  PT( "struct                 locale_t" ),
  PT( "typedef  int32_t       mode_t" ),
  PT( "typedef unsigned long  nfds_t" ),
  PT( "typedef uint32_t       nlink_t" ),
  PT( "typedef uint32_t       rlim_t" ),
  PT( "struct                 siginfo_t" ),
  PT( "typedef void         (*sig_t)(int)" ),
  PT( "typedef unsigned long  sigset_t" ),
  PT( "typedef  void         *timer_t" ),

  PT( "enum                   clockid_t" ),
  PT( "typedef  int64_t       suseconds_t" ),
  PT( "typedef uint32_t       useconds_t" ),

  PT( "typedef uint32_t       id_t" ),
  PT( "typedef uint32_t       gid_t" ),
  PT( "typedef  int32_t       pid_t" ),
  PT( "typedef uint32_t       uid_t" ),

  PT( "typedef void          *posix_spawnattr_t" ),
  PT( "typedef void          *posix_spawn_file_actions_t" ),

  PT( "struct                 regex_t" ),
  PT( "struct                 regmatch_t" ),
  PT( "typedef size_t         regoff_t" ),

  PT( "typedef uint32_t       in_addr_t" ),
  PT( "typedef uint16_t       in_port_t" ),
  PT( "typedef uint32_t       sa_family_t" ),
  PT( "typedef uint32_t       socklen_t" ),

  PT( NULL )
};

/**
 * Predefined types for Microsoft Windows.
 *
 * @sa [Windows Data Types](https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types)
 */
static predef_type_t const PREDEFINED_WIN32[] = {
  //
  // The comment about GNU C's __int128 type applies to these also.
  //
  PT( "typedef   signed char          __int8" ),
  PT( "typedef          __int8        _int8" ),
  PT( "typedef          short         __int16" ),
  PT( "typedef          __int16       _int16" ),
  PT( "typedef          int           __int32" ),
  PT( "typedef          __int32       _int32" ),
  PT( "typedef          long long     __int64" ),
  PT( "typedef          __int64       _int64" ),
  PT( "typedef          wchar_t       __wchar_t" ),

  PT( "struct                         __m128" ),
  PT( "struct                         __m128d" ),
  PT( "struct                         __m128i" ),
  PT( "struct                         __m64" ),

  PT( "typedef          int           BOOL" ),
  PT( "typedef BOOL                 *PBOOL" ),
  PT( "typedef BOOL                *LPBOOL" ),
  PT( "typedef          wchar_t       WCHAR" ),
  PT( "typedef WCHAR                *PWCHAR" ),
  PT( "typedef unsigned char          BYTE" ),
  PT( "typedef WCHAR                 TBYTE" ),
  PT( "typedef BYTE                 *PBYTE" ),
  PT( "typedef TBYTE               *PTBYTE" ),
  PT( "typedef BYTE                *LPBYTE" ),
  PT( "typedef BYTE                   BOOLEAN" ),
  PT( "typedef BOOLEAN              *PBOOLEAN" ),
  PT( "typedef          char          CHAR" ),
  PT( "typedef          char         CCHAR" ),
  PT( "typedef CHAR                 *PCHAR" ),
  PT( "typedef CHAR                *LPCHAR" ),
  PT( "typedef WCHAR                 TCHAR" ),
  PT( "typedef TCHAR               *PTCHAR" ),
  PT( "typedef          short         SHORT" ),
  PT( "typedef SHORT                *PSHORT" ),
  PT( "typedef          int           INT" ),
  PT( "typedef INT                  *PINT" ),
  PT( "typedef          int        *LPINT" ),
  PT( "typedef          long          LONG" ),
  PT( "typedef LONG                 *PLONG" ),
  PT( "typedef          long       *LPLONG" ),
  PT( "typedef          long long     LONGLONG" ),
  PT( "typedef LONGLONG             *PLONGLONG" ),
  PT( "typedef          float         FLOAT" ),
  PT( "typedef FLOAT                *PFLOAT" ),
  PT( "typedef          void        *PVOID" ),
  PT( "typedef          void       *LPVOID" ),
  PT( "typedef    const void      *LPCVOID" ),

  PT( "typedef unsigned char          UCHAR" ),
  PT( "typedef UCHAR                *PUCHAR" ),
  PT( "typedef unsigned short         USHORT" ),
  PT( "typedef USHORT               *PUSHORT" ),
  PT( "typedef unsigned int           UINT" ),
  PT( "typedef UINT                 *PUINT" ),
  PT( "typedef unsigned long          ULONG" ),
  PT( "typedef ULONG                *PULONG" ),
  PT( "typedef unsigned long long     ULONGLONG" ),
  PT( "typedef ULONGLONG            *PULONGLONG" ),

  PT( "typedef unsigned short         WORD" ),
  PT( "typedef WORD                 *PWORD" ),
  PT( "typedef WORD                *LPWORD" ),
  PT( "typedef unsigned long          DWORD" ),
  PT( "typedef DWORD                *PDWORD" ),
  PT( "typedef DWORD               *LPDWORD" ),
  PT( "typedef unsigned long          DWORDLONG" ),
  PT( "typedef DWORDLONG            *PDWORDLONG" ),
  PT( "typedef unsigned int           DWORD32" ),
  PT( "typedef DWORD32              *PDWORD32" ),
  PT( "typedef unsigned long          DWORD64" ),
  PT( "typedef DWORD64              *PDWORD64" ),
  PT( "typedef unsigned long long     QWORD" ),

  PT( "typedef   signed char          INT8" ),
  PT( "typedef INT8                 *PINT8" ),
  PT( "typedef          short         INT16" ),
  PT( "typedef INT16                *PINT16" ),
  PT( "typedef          int           INT32" ),
  PT( "typedef INT32                *PINT32" ),
  PT( "typedef          long          INT64" ),
  PT( "typedef INT64                *PINT64" ),
  PT( "typedef          int           HALF_PTR" ),
  PT( "typedef HALF_PTR             *PHALF_PTR" ),
  PT( "typedef        __int64         INT_PTR" ),
  PT( "typedef INT_PTR              *PINT_PTR" ),
  PT( "typedef          int           LONG32" ),
  PT( "typedef LONG32               *PLONG32" ),
  PT( "typedef        __int64         LONG64" ),
  PT( "typedef LONG64               *PLONG64" ),
  PT( "typedef        __int64         LONG_PTR" ),
  PT( "typedef LONG_PTR             *PLONG_PTR" ),

  PT( "typedef unsigned char          UINT8" ),
  PT( "typedef UINT8                *PUINT8" ),
  PT( "typedef unsigned short         UINT16" ),
  PT( "typedef UINT16               *PUINT16" ),
  PT( "typedef unsigned int           UINT32" ),
  PT( "typedef UINT32               *PUINT32" ),
  PT( "typedef unsigned long          UINT64" ),
  PT( "typedef UINT64               *PUINT64" ),
  PT( "typedef unsigned int           UHALF_PTR" ),
  PT( "typedef UHALF_PTR            *PUHALF_PTR" ),
  PT( "typedef unsigned long          UINT_PTR" ),
  PT( "typedef UINT_PTR             *PUINT_PTR" ),
  PT( "typedef unsigned int           ULONG32" ),
  PT( "typedef ULONG32              *PULONG32" ),
  PT( "typedef unsigned long          ULONG64" ),
  PT( "typedef ULONG64              *PULONG64" ),
  PT( "typedef unsigned long          ULONG_PTR" ),
  PT( "typedef ULONG_PTR            *PULONG_PTR" ),

  PT( "typedef ULONG_PTR              DWORD_PTR" ),
  PT( "typedef DWORD_PTR            *PDWORD_PTR" ),
  PT( "typedef ULONG_PTR              SIZE_T" ),
  PT( "typedef SIZE_T               *PSIZE_T" ),
  PT( "typedef LONG_PTR               SSIZE_T" ),
  PT( "typedef SSIZE_T              *PSSIZE_T" ),

  PT( "typedef PVOID                  HANDLE" ),
  PT( "typedef HANDLE               *PHANDLE" ),
  PT( "typedef HANDLE              *LPHANDLE" ),
  PT( "typedef HANDLE                 HBITMAP" ),
  PT( "typedef HANDLE                 HBRUSH" ),
  PT( "typedef HANDLE                 HCOLORSPACE" ),
  PT( "typedef HANDLE                 HCONV" ),
  PT( "typedef HANDLE                 HCONVLIST" ),
  PT( "typedef HANDLE                 HDC" ),
  PT( "typedef HANDLE                 HDDEDATA" ),
  PT( "typedef HANDLE                 HDESK" ),
  PT( "typedef HANDLE                 HDROP" ),
  PT( "typedef HANDLE                 HDWP" ),
  PT( "typedef HANDLE                 HENHMETAFILE" ),
  PT( "typedef HANDLE                 HFONT" ),
  PT( "typedef HANDLE                 HGDIOBJ" ),
  PT( "typedef HANDLE                 HGLOBAL" ),
  PT( "typedef HANDLE                 HHOOK" ),
  PT( "typedef HANDLE                 HICON" ),
  PT( "typedef HICON                  HCURSOR" ),
  PT( "typedef HANDLE                 HINSTANCE" ),
  PT( "typedef HANDLE                 HKEY" ),
  PT( "typedef HKEY                 *PHKEY" ),
  PT( "typedef HANDLE                 HKL" ),
  PT( "typedef HANDLE                 HLOCAL" ),
  PT( "typedef HANDLE                 HMENU" ),
  PT( "typedef HANDLE                 HMETAFILE" ),
  PT( "typedef HINSTANCE              HMODULE" ),
  PT( "typedef HANDLE                 HMONITOR" ),
  PT( "typedef HANDLE                 HPALETTE" ),
  PT( "typedef HANDLE                 HPEN" ),
  PT( "typedef HANDLE                 HRGN" ),
  PT( "typedef HANDLE                 HRSRC" ),
  PT( "typedef HANDLE                 HSZ" ),
  PT( "typedef HANDLE                 HWINSTA" ),
  PT( "typedef HANDLE                 HWND" ),

  PT( "typedef          CHAR        *PSTR" ),
  PT( "typedef   const  CHAR       *PCSTR" ),
  PT( "typedef          CHAR       *LPSTR" ),
  PT( "typedef   const  CHAR      *LPCSTR" ),
  PT( "typedef         WCHAR       *PWSTR" ),
  PT( "typedef   const WCHAR      *PCWSTR" ),
  PT( "typedef         WCHAR      *LPWSTR" ),
  PT( "typedef   const WCHAR     *LPCWSTR" ),
  PT( "typedef       LPWSTR         PTSTR" ),
  PT( "typedef       LPWSTR        LPTSTR" ),
  PT( "typedef      LPCWSTR        PCTSTR" ),
  PT( "typedef      LPCWSTR       LPCTSTR" ),

  PT( "typedef WORD                   ATOM" ),
  PT( "typedef DWORD                  COLORREF" ),
  PT( "typedef COLORREF            *LPCOLORREF" ),
  PT( "typedef          int           HFILE" ),
  PT( "typedef          long          HRESULT" ),
  PT( "typedef WORD                   LANGID" ),
  PT( "typedef union _LARGE_INTEGER   LARGE_INTEGER" ),
  PT( "typedef union _ULARGE_INTEGER ULARGE_INTEGER" ),
  PT( "typedef DWORD                  LCID" ),
  PT( "typedef PDWORD                PLCID" ),
  PT( "typedef DWORD                  LCTYPE" ),
  PT( "typedef DWORD                  LGRPID" ),
  PT( "typedef LONG_PTR               LRESULT" ),
  PT( "typedef HANDLE                 SC_HANDLE" ),
  PT( "typedef LPVOID                 SC_LOCK" ),
  PT( "typedef HANDLE                 SERVICE_STATUS_HANDLE" ),
  PT( "struct                         UNICODE_STRING" ),
  PT( "typedef LONGLONG               USN" ),
  PT( "typedef UINT_PTR               WPARAM" ),

  PT( NULL )
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans up \ref c_typedef data.
 *
 * @sa c_typedef_init()
 */
static void c_typedef_cleanup( void ) {
  // There is no c_typedef_free() function because c_typedef_add() adds only
  // c_typedef_t nodes pointing to pre-existing AST nodes.  The AST nodes are
  // freed independently in parser_cleanup().  Hence, this function frees only
  // the red-black tree, its nodes, and the c_typedef_t data each node points
  // to, but not the AST nodes the c_typedef_t data points to.
  rb_tree_cleanup( &typedef_set, &free );
}

/**
 * Comparison function for two \ref c_typedef.
 *
 * @param i_tdef A pointer to the first \ref c_typedef.
 * @param j_tdef A pointer to the second \ref c_typedef.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the `typedef` name pointed to by \a i_data is less than, equal
 * to, or greater than the `typedef` name pointed to by \a j_data.
 */
NODISCARD
static int c_typedef_cmp( c_typedef_t const *i_tdef,
                          c_typedef_t const *j_tdef ) {
  assert( i_tdef != NULL );
  assert( j_tdef != NULL );
  return c_sname_cmp( &i_tdef->ast->sname, &j_tdef->ast->sname );
}

/**
 * Creates a new \ref c_typedef.
 *
 * @param ast The AST of the type.
 * @param gib_flags The gibberish flags to use; must only be one of
 * #C_GIB_NONE, #C_GIB_TYPEDEF, or #C_GIB_USING.
 * @return Returns said \ref c_typedef.
 */
NODISCARD
static c_typedef_t* c_typedef_new( c_ast_t const *ast, unsigned gib_flags ) {
  assert( ast != NULL );
  assert( is_01_bit_only_in_set( gib_flags, C_GIB_TYPEDEF | C_GIB_USING ) );

  bool const is_predefined = predef_lang_ids != LANG_NONE;

  c_typedef_t *const tdef = MALLOC( c_typedef_t, 1 );
  *tdef = (c_typedef_t){
    .ast = ast,
    .gib_flags = gib_flags,
    .is_predefined = is_predefined,
    //
    // If predef_lang_ids is set, we're predefining a type that's available
    // only in those language(s); otherwise we're defining a user-defined type
    // that's available in the current language and newer.
    //
    .lang_ids = is_predefined ? predef_lang_ids : c_lang_and_newer( opt_lang )
  };

  return tdef;
}

/**
 * Parses an array of predefined type declarations.
 *
 * @param types The array of predefined types to parse.  The last element
 * _must_ have its \ref predef_type::str "str" be NULL.
 */
static void parse_predef_types( predef_type_t const types[static const 2] ) {
  assert( types != NULL );
  for ( predef_type_t const *pt = types; pt->str != NULL; ++pt ) {
    if ( unlikely( cdecl_parse_string( pt->str, strlen( pt->str ) ) != EX_OK ) )
      INTERNAL_ERR( "failed parsing predefined type on line %u\n", pt->line );
  } // for
}

/**
 * Red-black tree visitor function that forwards to the \ref
 * c_typedef_visit_fn_t function.
 *
 * @param node_data A pointer to the node's data.
 * @param v_data Data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
NODISCARD
static bool rb_visitor( void *node_data, void *v_data ) {
  assert( node_data != NULL );
  assert( v_data != NULL );

  c_typedef_t const *const tdef = node_data;
  tdef_rb_visit_data_t const *const trvd = v_data;

  return (*trvd->visit_fn)( tdef, trvd->v_data );
}

////////// extern functions ///////////////////////////////////////////////////

rb_node_t* c_typedef_add( c_ast_t const *ast, unsigned gib_flags ) {
  assert( ast != NULL );
  assert( !c_sname_empty( &ast->sname ) );

  c_typedef_t *const new_tdef = c_typedef_new( ast, gib_flags );
  rb_insert_rv_t const rbi = rb_tree_insert( &typedef_set, new_tdef );
  if ( !rbi.inserted ) {
    //
    // A typedef with the same name exists, so we don't need the new one.
    //
    FREE( new_tdef );
  }
  return rbi.node;
}

c_typedef_t const* c_typedef_find_name( char const *name ) {
  assert( name != NULL );
  c_sname_t sname;
  if ( c_sname_parse( name, &sname ) ) {
    c_typedef_t const *const tdef = c_typedef_find_sname( &sname );
    c_sname_cleanup( &sname );
    return tdef;
  }
  return NULL;
}

c_typedef_t const* c_typedef_find_sname( c_sname_t const *sname ) {
  assert( sname != NULL );
  rb_node_t const *const found_rb =
    rb_tree_find( &typedef_set, &C_TYPEDEF_SNAME_LIT( *sname ) );
  return found_rb != NULL ? found_rb->data : NULL;
}

void c_typedef_init( void ) {
  ASSERT_RUN_ONCE();

  rb_tree_init( &typedef_set, POINTER_CAST( rb_cmp_fn_t, &c_typedef_cmp ) );
  check_atexit( &c_typedef_cleanup );

#ifdef ENABLE_CDECL_DEBUG
  //
  // Temporarily turn off debug output for built-in typedefs.
  //
  bool const orig_cdecl_debug = opt_cdecl_debug;
  opt_cdecl_debug = false;
#endif /* ENABLE_CDECL_DEBUG */
#ifdef ENABLE_FLEX_DEBUG
  //
  // Temporarily turn off Flex debug output for built-in typedefs.
  //
  int const orig_flex_debug = opt_flex_debug;
  opt_flex_debug = false;
#endif /* ENABLE_FLEX_DEBUG */
#ifdef YYDEBUG
  //
  // Temporarily turn off Bison debug output for built-in typedefs.
  //
  int const orig_bison_debug = opt_bison_debug;
  opt_bison_debug = false;
#endif /* YYDEBUG */

  c_lang_id_t const orig_lang = opt_lang;

  if ( opt_typedefs ) {
    //
    // Temporarily switch to the latest supported version of C so all keywords
    // will be available.
    //
    opt_lang = LANG_C_NEW;

    predef_lang_ids = LANG_MIN(C_KNR);
    parse_predef_types( PREDEFINED_KNR_C );

    predef_lang_ids = LANG_MIN(C_89);
    parse_predef_types( PREDEFINED_STD_C_89 );
    parse_predef_types( PREDEFINED_FLOATING_POINT_EXTENSIONS );
    parse_predef_types( PREDEFINED_GNU_C );

    predef_lang_ids = LANG_MIN(C_95);
    parse_predef_types( PREDEFINED_STD_C_95 );
    parse_predef_types( PREDEFINED_PTHREAD_H );
    parse_predef_types( PREDEFINED_WIN32 );

    predef_lang_ids = LANG_MIN(C_99);
    parse_predef_types( PREDEFINED_STD_C_99 );

    // However, Embedded C extensions are available only in C99.
    opt_lang = LANG_C_99;
    predef_lang_ids = LANG_C_99;
    parse_predef_types( PREDEFINED_EMBEDDED_C );
    opt_lang = LANG_C_NEW;

    // Must be defined after C99.
    predef_lang_ids = LANG_MIN(C_89);
    parse_predef_types( PREDEFINED_MISC );

    predef_lang_ids = LANG_MIN(C_11);
    parse_predef_types( PREDEFINED_STD_C_11 );

    predef_lang_ids = LANG_MIN(C_23);
    parse_predef_types( PREDEFINED_STD_C_23 );
  }

  //
  // Temporarily switch to the latest supported version of C++ so all keywords
  // will be available.
  //
  opt_lang = LANG_CPP_NEW;

  if ( opt_typedefs ) {
    predef_lang_ids = LANG_MIN(CPP_OLD);
    parse_predef_types( PREDEFINED_STD_CPP );

    predef_lang_ids = LANG_MIN(CPP_11);
    parse_predef_types( PREDEFINED_STD_CPP_11 );

    predef_lang_ids = LANG_MIN(CPP_17);
    parse_predef_types( PREDEFINED_STD_CPP_17 );

    predef_lang_ids = LANG_MIN(CPP_20);
    parse_predef_types( PREDEFINED_STD_CPP_20 );

    predef_lang_ids = LANG_MIN(CPP_23);
    parse_predef_types( PREDEFINED_STD_CPP_23 );
  }

  predef_lang_ids = LANG_MIN(CPP_20);
  parse_predef_types( PREDEFINED_STD_CPP_20_REQUIRED );

  predef_lang_ids = LANG_NONE;
  opt_lang = orig_lang;

#ifdef ENABLE_CDECL_DEBUG
  opt_cdecl_debug = orig_cdecl_debug;
#endif /* ENABLE_CDECL_DEBUG */
#ifdef ENABLE_FLEX_DEBUG
  opt_flex_debug = orig_flex_debug;
#endif /* ENABLE_FLEX_DEBUG */
#ifdef YYDEBUG
  opt_bison_debug = orig_bison_debug;
#endif /* YYDEBUG */
}

c_typedef_t* c_typedef_remove( rb_node_t *node ) {
  return rb_tree_delete( &typedef_set, node );
}

c_typedef_t const* c_typedef_visit( c_typedef_visit_fn_t visit_fn,
                                    void *v_data ) {
  assert( visit_fn != NULL );
  tdef_rb_visit_data_t trvd = { visit_fn, v_data };
  rb_node_t const *const rb = rb_tree_visit( &typedef_set, &rb_visitor, &trvd );
  return rb != NULL ? rb->data : NULL;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
