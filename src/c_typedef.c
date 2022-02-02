/*
**      cdecl -- C gibberish translator
**      src/c_typedef.c
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
 * Defines functions for adding and looking up C/C++ `typedef` or `using`
 * declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_typedef.h"
#include "cdecl.h"
#include "c_ast.h"
#include "c_lang.h"
#include "options.h"
#include "red_black.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

/**
 * Convenience macro for specifying a \ref c_typedef literal having \a SNAME.
 *
 * @param SNAME The sname.
 */
#define C_TYPEDEF_LIT(SNAME) \
  (c_typedef_t){ &(c_ast_t const){ .sname = (SNAME) }, LANG_ANY, true, false }

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct tdef_rb_visit_data {
  c_typedef_visit_fn_t  visit_fn;       ///< Caller's visitor function.
  void                 *v_data;         ///< Caller's optional data.
};
typedef struct tdef_rb_visit_data tdef_rb_visit_data_t;

// local variable definitions
static rb_tree_t    typedefs;           ///< Global set of `typedef`s.
static c_lang_id_t  predefined_lang_ids;///< Languages when predefining types.
static bool         user_defined;       ///< Are new `typedef`s used-defined?

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
static char const *const PREDEFINED_KNR_C[] = {
  "typedef          char *caddr_t",
  "typedef          long  daddr_t",
  "typedef          int   dev_t",
  "typedef struct _iobuf  FILE",
  "typedef unsigned int   ino_t",
  "typedef          int   jmp_buf[37]",
  "typedef          long  off_t",
  "typedef          long  time_t",

  NULL
};

/**
 * Predefined types for C89.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_STD_C_89[] = {
  "typedef          long    clock_t",
  "typedef struct _dirdesc  DIR",
  "struct                   div_t",
  "struct               imaxdiv_t",
  "struct                  ldiv_t",
  "struct                 lldiv_t",
  "typedef          int     errno_t",
  "struct                   fpos_t",
  "struct                   lconv",
  "typedef          long    ptrdiff_t",
  "typedef          int     sig_atomic_t",
  "typedef unsigned long    size_t",
  "typedef          long   ssize_t",
  "typedef          void   *va_list",

  NULL
};

/**
 * Predefined types for C95.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_STD_C_95[] = {
  "struct                 mbstate_t",
  "typedef          int   wctrans_t",
  "typedef unsigned long  wctype_t",
  "typedef          int   wint_t",

  NULL
};

/**
 * Predefined types for C99.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_STD_C_99[] = {
  "typedef   signed char   int8_t",
  "typedef          short  int16_t",
  "typedef          int    int32_t",
  "typedef          long   int64_t",
  "typedef unsigned char  uint8_t",
  "typedef unsigned short uint16_t",
  "typedef unsigned int   uint32_t",
  "typedef unsigned long  uint64_t",

  "typedef          long   intmax_t",
  "typedef          long   intptr_t",
  "typedef unsigned long  uintmax_t",
  "typedef unsigned long  uintptr_t",

  "typedef   signed char   int_fast8_t",
  "typedef          short  int_fast16_t",
  "typedef          int    int_fast32_t",
  "typedef          long   int_fast64_t",
  "typedef unsigned char  uint_fast8_t",
  "typedef unsigned short uint_fast16_t",
  "typedef unsigned int   uint_fast32_t",
  "typedef unsigned long  uint_fast64_t",

  "typedef   signed char   int_least8_t",
  "typedef          short  int_least16_t",
  "typedef          int    int_least32_t",
  "typedef          long   int_least64_t",
  "typedef unsigned char  uint_least8_t",
  "typedef unsigned short uint_least16_t",
  "typedef unsigned int   uint_least32_t",
  "typedef unsigned long  uint_least64_t",

  NULL
};

/**
 * Predefined types for C11.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_STD_C_11[] = {
  "typedef _Atomic          _Bool     atomic_bool",
  "typedef _Atomic          char      atomic_char",
  "typedef _Atomic   signed char      atomic_schar",
  "typedef _Atomic          char8_t   atomic_char8_t",
  "typedef _Atomic          char16_t  atomic_char16_t",
  "typedef _Atomic          char32_t  atomic_char32_t",
  "typedef _Atomic          wchar_t   atomic_wchar_t",
  "typedef _Atomic          short     atomic_short",
  "typedef _Atomic          int       atomic_int",
  "typedef _Atomic          long      atomic_long",
  "typedef _Atomic          long long atomic_llong",
  "typedef _Atomic unsigned char      atomic_uchar",
  "typedef _Atomic unsigned short     atomic_ushort",
  "typedef _Atomic unsigned int       atomic_uint",
  "typedef _Atomic unsigned long      atomic_ulong",
  "typedef _Atomic unsigned long long atomic_ullong",

  "struct                             atomic_flag",
  "typedef _Atomic  ptrdiff_t         atomic_ptrdiff_t",
  "typedef _Atomic  size_t            atomic_size_t",

  "typedef _Atomic  intmax_t          atomic_intmax_t",
  "typedef _Atomic  intptr_t          atomic_intptr_t",
  "typedef _Atomic uintptr_t          atomic_uintptr_t",
  "typedef _Atomic uintmax_t          atomic_uintmax_t",

  "typedef _Atomic  int_fast8_t       atomic_int_fast8_t",
  "typedef _Atomic  int_fast16_t      atomic_int_fast16_t",
  "typedef _Atomic  int_fast32_t      atomic_int_fast32_t",
  "typedef _Atomic  int_fast64_t      atomic_int_fast64_t",
  "typedef _Atomic uint_fast8_t       atomic_uint_fast8_t",
  "typedef _Atomic uint_fast16_t      atomic_uint_fast16_t",
  "typedef _Atomic uint_fast32_t      atomic_uint_fast32_t",
  "typedef _Atomic uint_fast64_t      atomic_uint_fast64_t",

  "typedef _Atomic  int_least8_t      atomic_int_least8_t",
  "typedef _Atomic  int_least16_t     atomic_int_least16_t",
  "typedef _Atomic  int_least32_t     atomic_int_least32_t",
  "typedef _Atomic  int_least64_t     atomic_int_least64_t",
  "typedef _Atomic uint_least8_t      atomic_uint_least8_t",
  "typedef _Atomic uint_least16_t     atomic_uint_least16_t",
  "typedef _Atomic uint_least32_t     atomic_uint_least32_t",
  "typedef _Atomic uint_least64_t     atomic_uint_least64_t",

  "typedef pthread_cond_t             cnd_t",
  "typedef void                     (*constraint_handler_t)(const char *restrict, void *restrict, errno_t)",
  "typedef long     double            max_align_t",
  "typedef enum memory_order          memory_order",
  "typedef pthread_mutex_t            mtx_t",
  "typedef          int               once_flag",
  "typedef unsigned long              rsize_t",
  "typedef pthread_t                  thrd_t",
  "typedef          int             (*thrd_start_t)(void*)",
  "typedef          void             *tss_t",
  "typedef          void            (*tss_dtor_t)(void*)",

  NULL
};

/**
 * Predefined types for Floating-point extensions for C.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_FLOATING_POINT_EXTENSIONS[] = {
  "typedef          float       _Float16",
  "typedef          _Float16    _Float16_t",
  "typedef          float       _Float32",
  "typedef          _Float32    _Float32_t",
  "typedef          _Float32    _Float32x",
  "typedef          double      _Float64",
  "typedef          _Float64    _Float64_t",
  "typedef          _Float64    _Float64x",
  "typedef long     double      _Float128",
  "typedef          _Float128   _Float128_t",
  "typedef          _Float128   _Float128x",

  "typedef          float       _Decimal32",
  "typedef          _Decimal32  _Decimal32_t",
  "typedef          double      _Decimal64",
  "typedef          _Decimal64  _Decimal64x",
  "typedef          _Decimal64  _Decimal64_t",
  "typedef long     double      _Decimal128",
  "typedef          _Decimal128 _Decimal128_t",
  "typedef          _Decimal128 _Decimal128x",

  "typedef          double      double_t",
  "typedef          float       float_t",
  "typedef long     double      long_double_t",

  "struct                       femode_t",
  "struct                       fenv_t",
  "typedef unsigned short       fexcept_t",

  NULL
};

/**
 * Predefined types for `pthread.h`.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_PTHREAD_H[] = {
  "typedef unsigned long  pthread_t",
  "struct                 pthread_barrier_t",
  "struct                 pthread_barrierattr_t",
  "struct                 pthread_cond_t",
  "struct                 pthread_condattr_t",
  "typedef unsigned int   pthread_key_t",
  "struct                 pthread_mutex_t",
  "struct                 pthread_mutexattr_t",
  "typedef          int   pthread_once_t",
  "struct                 pthread_rwlock_t",
  "struct                 pthread_rwlockattr_t",
  "typedef volatile int   pthread_spinlock_t",

  NULL
};

/**
 * Predefined types for C++.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_STD_CPP[] = {
  "namespace std { class                    bad_alloc; }",
  "namespace std { class                    bad_cast; }",
  "namespace std { class                    bad_exception; }",
  "namespace std { class                    bad_type_id; }",
  "namespace std { class                    codecvt_base; }",
  "namespace std { class                    ctype_base; }",
  "namespace std { struct                   div_t; }",
  "namespace std { struct                  ldiv_t; }",
  "namespace std { class                    domain_error; }",
  "namespace std { class                    ios_base; }",
  "namespace std { enum ios_base::          event; }",
  "namespace std { class ios_base   { using event_callback = void (*)(std::ios_base::event, std::ios_base&, int); }; }",
  "namespace std { class                    exception; }",
  "namespace std { class                    filebuf; }",
  "namespace std { class                   wfilebuf; }",
  "namespace std { class ios_base   { using fmtflags = unsigned; }; }",
  "namespace std { class ios_base::         Init; }",
  "namespace std { class                    invalid_argument; }",
  "namespace std { class                    ios; }",
  "namespace std { class                   wios; }",
  "namespace std { class ios_base   { using iostate = unsigned; }; }",
  "namespace std { class                    length_error; }",
  "namespace std { class                    locale; }",
  "namespace std { class                    logic_error; }",
  "namespace std { class ctype_base { using mask = unsigned; }; }",
  "namespace std { class                    messages_base; }",
  "namespace std { class                    money_base; }",
  "namespace std { struct                   nothrow_t; }",
  "namespace std { class ios_base   { using openmode = unsigned; }; }",
  "namespace std { class                    out_of_range; }",
  "namespace std { class                    overflow_error; }",
  "namespace std                    { using ptrdiff_t = long; }",
  "namespace std { class                    range_error; }",
  "namespace std { class                    runtime_error; }",
  "namespace std { class ios_base   { using seekdir = int; }; }",
  "namespace std                    { using sig_atomic_t = int; }",
  "namespace std                    { using size_t = unsigned long; }",
  "namespace std { class                   fstream; }",
  "namespace std { class                  ifstream; }",
  "namespace std { class                  wfstream; }",
  "namespace std { class                 wifstream; }",
  "namespace std { class                  ofstream; }",
  "namespace std { class                 wofstream; }",
  "namespace std { class                   istream; }",
  "namespace std { class                  wistream; }",
  "namespace std { class                  iostream; }",
  "namespace std { class                 wiostream; }",
  "namespace std { class                   ostream; }",
  "namespace std { class                  wostream; }",
  "namespace std { class                    streambuf; }",
  "namespace std { class                   wstreambuf; }",
  "namespace std                    { using streamoff = long long; }",
  "namespace std                    { using streamsize = long; }",
  "namespace std { class                    string; }",
  "namespace std { class                   wstring; }",
  "namespace std { class                    stringbuf; }",
  "namespace std { class                   wstringbuf; }",
  "namespace std { class                    stringstream; }",
  "namespace std { class                   istringstream; }",
  "namespace std { class                   wstringstream; }",
  "namespace std { class                  wistringstream; }",
  "namespace std { class                   ostringstream; }",
  "namespace std { class                  wostringstream; }",
  "namespace std { class                    syncbuf; }",
  "namespace std { class                   wsyncbuf; }",
  "namespace std { class                   osyncstream; }",
  "namespace std { class                  wosyncstream; }",
  "namespace std { class                    time_base; }",
  "namespace std { class                    underflow_error; }",

  NULL
};

/**
 * Predefined types for C++11.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_STD_CPP_11[] = {
  "namespace std           {    struct      adopt_lock_t; }",
  //
  // These atomic_* types are supposed to be typedefs, but they're typedefs to
  // instantiated templates and cdecl doesn't support templates, so we make
  // them structs instead.
  //
  "namespace std          {     struct      atomic_bool; }",
  "namespace std          {     struct      atomic_char8_t; }",
  "namespace std          {     struct      atomic_char16_t; }",
  "namespace std          {     struct      atomic_char32_t; }",
  "namespace std          {     struct      atomic_char; }",
  "namespace std          {     struct      atomic_flag; }",
  "namespace std          {     struct      atomic_int8_t; }",
  "namespace std          {     struct      atomic_int16_t; }",
  "namespace std          {     struct      atomic_int32_t; }",
  "namespace std          {     struct      atomic_int64_t; }",
  "namespace std          {     struct      atomic_int; }",
  "namespace std          {     struct      atomic_int_fast8_t; }",
  "namespace std          {     struct      atomic_int_fast16_t; }",
  "namespace std          {     struct      atomic_int_fast32_t; }",
  "namespace std          {     struct      atomic_int_fast64_t; }",
  "namespace std          {     struct      atomic_int_least8_t; }",
  "namespace std          {     struct      atomic_int_least16_t; }",
  "namespace std          {     struct      atomic_int_least32_t; }",
  "namespace std          {     struct      atomic_int_least64_t; }",
  "namespace std          {     struct      atomic_intmax_t; }",
  "namespace std          {     struct      atomic_intptr_t; }",
  "namespace std          {     struct      atomic_llong; }",
  "namespace std          {     struct      atomic_long; }",
  "namespace std          {     struct      atomic_ptrdiff_t; }",
  "namespace std          {     struct      atomic_schar; }",
  "namespace std          {     struct      atomic_short; }",
  "namespace std          {     struct      atomic_signed_lock_free; }",
  "namespace std          {     struct      atomic_size_t; }",
  "namespace std          {     struct      atomic_uchar; }",
  "namespace std          {     struct      atomic_uint16_t; }",
  "namespace std          {     struct      atomic_uint32_t; }",
  "namespace std          {     struct      atomic_uint64_t; }",
  "namespace std          {     struct      atomic_uint8_t; }",
  "namespace std          {     struct      atomic_uint; }",
  "namespace std          {     struct      atomic_uint_fast8_t; }",
  "namespace std          {     struct      atomic_uint_fast16_t; }",
  "namespace std          {     struct      atomic_uint_fast32_t; }",
  "namespace std          {     struct      atomic_uint_fast64_t; }",
  "namespace std          {     struct      atomic_uint_least8_t; }",
  "namespace std          {     struct      atomic_uint_least16_t; }",
  "namespace std          {     struct      atomic_uint_least32_t; }",
  "namespace std          {     struct      atomic_uint_least64_t; }",
  "namespace std          {     struct      atomic_uintmax_t; }",
  "namespace std          {     struct      atomic_uintptr_t; }",
  "namespace std          {     struct      atomic_ullong; }",
  "namespace std          {     struct      atomic_ulong; }",
  "namespace std          {     struct      atomic_unsigned_lock_free; }",
  "namespace std          {     struct      atomic_ushort; }",
  "namespace std          {     struct      atomic_wchar_t; }",
  "namespace std          {      class      bad_array_new_length; }",
  "namespace std          {      class      bad_function_call; }",
  "namespace std          {      class      bad_weak_ptr; }",
  "namespace std          {      class      condition_variable; }",
  "namespace std          {      class      condition_variable_any; }",
  "namespace std          { enum class      cv_status; }",
  "namespace std          {     struct      defer_lock_t; }",
  "namespace std          {     struct  imaxdiv_t; }",
  "namespace std          {     struct    lldiv_t; }",
  "namespace std          {      class      error_category; }",
  "namespace std          {      class      error_code; }",
  "namespace std          {      class      error_condition; }",
  "namespace std          { class ios_base::failure; }",
  "namespace std          { enum class      future_errc; }",
  "namespace std          {      class      future_error; }",
  "namespace std          { enum class      future_status; }",
  "namespace std::chrono  {      class      high_resolution_clock; }",
  "namespace std          { enum class      launch; }",
  "namespace std::regex_constants { using   match_flag_type = unsigned; }",
  "namespace std          {      using      max_align_t = long double; }",
  "namespace std          {      class      mutex; }",
  "namespace std          {      using      nullptr_t = void*; }",
  "namespace std          {      class      recursive_mutex; }",
  "namespace std          {      class      recursive_timed_mutex; }",
  "namespace std          {      class      regex; }",
  "namespace std          {      class     wregex; }",
  "namespace std          {     struct      regex_error; }",
  "namespace std          {      class      shared_mutex; }",
  "namespace std          {      class      shared_timed_mutex; }",
  "namespace std          {      class   u16string; }",
  "namespace std          {      class   u32string; }",
  "namespace std::chrono  {      class      steady_clock; }",
  "namespace std::regex_constants { using   syntax_option_type = unsigned; }",
  "namespace std::chrono  {      class      system_clock; }",
  "namespace std          {     struct      system_error; }",
  "namespace std          {      class      thread; }",
  "namespace std          {      class      timed_mutex; }",
  "namespace std          {     struct      try_to_lock_t; }",

  NULL
};

/**
 * Predefined types for C++17.
 */
static char const *const PREDEFINED_STD_CPP_17[] = {
  "namespace std             { enum class     align_val_t; }",
  "namespace std             {      class     bad_any_cast; }",
  "namespace std             {      class     bad_optional_access; }",
  "namespace std             {      class     bad_variant_access; }",
  "namespace std             { enum           byte; }",
  "namespace std             { enum class     chars_format; }",
  "namespace std::filesystem { enum class     copy_options; }",
  "namespace std::filesystem {      class     directory_entry; }",
  "namespace std::filesystem {      class     directory_iterator; }",
  "namespace std::filesystem { enum class     directory_options; }",
  "namespace std::filesystem {      class     file_status; }",
  "namespace std::filesystem { enum class     file_type; }",
  "namespace std::filesystem {      class     filesystem_error; }",
  "namespace std::filesystem {      class     path; }",
  "namespace std::filesystem { enum class     perms; }",
  "namespace std::filesystem { enum class     perm_options; }",
  "namespace std::filesystem {      class     recursive_directory_iterator; }",
  "namespace std::filesystem {     struct     space_info; }",
  "namespace std             {      class     string_view; }",
  "namespace std             {      class  u16string_view; }",
  "namespace std             {      class  u32string_view; }",
  "namespace std             {      class    wstring_view; }",

  NULL
};

/**
 * Predefined types for C++20.
 */
static char const *const PREDEFINED_STD_CPP_20[] = {
  "namespace std         {      class   ambiguous_local_time; }",
  "namespace std::chrono { enum class   choose; }",
  "namespace std::chrono {      class   day; }",
  "namespace std         {     struct   destroying_delete_t; }",
  "namespace std::chrono {     struct   file_clock; }",
  "namespace std         {      class   format_error; }",
  "namespace std::chrono {     struct   gps_clock; }",
  "namespace std::chrono {     struct   is_clock; }",
  "namespace std         {      class   jthread; }",
  "namespace std::chrono {     struct   last_spec; }",
  "namespace std::chrono {      class   leap_second; }",
  "namespace std::chrono {     struct   local_info; }",
  "namespace std::chrono {     struct   local_t; }",
  "namespace std::chrono {      class   month; }",
  "namespace std::chrono {      class   month_day; }",
  "namespace std::chrono {      class   month_day_last; }",
  "namespace std::chrono {      class   month_weekday; }",
  "namespace std::chrono {      class   month_weekday_last; }",
  "namespace std::chrono {      class   nonexistent_local_time; }",
  "namespace std         {     struct   nonstopstate_t; }",
  "namespace std         {      class u8string_view; }",
  "namespace std         {      class   stop_source; }",
  "namespace std         {      class   stop_token; }",
  "namespace std         {     struct   strong_equality; }",
  "namespace std::chrono {     struct   sys_info; }",
  "namespace std::chrono {     struct   tai_clock; }",
  "namespace std::chrono {     struct   time_zone; }",
  "namespace std::chrono {      class   time_zone_link; }",
  "namespace std::chrono {     struct   tzdb; }",
  "namespace std::chrono {     struct   tzdb_list; }",
  "namespace std::chrono {     struct   utc_clock; }",
  "namespace std::chrono {      class   weekday; }",
  "namespace std::chrono {      class   weekday_indexed; }",
  "namespace std::chrono {      class   weekday_last; }",
  "namespace std         {     struct   weak_equality; }",
  "namespace std::chrono {      class   year; }",
  "namespace std::chrono {      class   year_month; }",
  "namespace std::chrono {      class   year_month_day; }",
  "namespace std::chrono {      class   year_month_day_last; }",
  "namespace std::chrono {      class   year_month_weekday; }",
  "namespace std::chrono {      class   year_month_weekday_last; }",

  NULL
};


/**
 * Predefined required types for C++20.
 */
static char const *const PREDEFINED_STD_CPP_20_REQUIRED[] = {
  // required for operator <=>
  "namespace std { struct partial_ordering; }",
  "namespace std { struct strong_ordering; }",
  "namespace std { struct weak_ordering; }",

  NULL
};

/**
 * Predefined types for C++23.
 */
static char const *const PREDEFINED_STD_CPP_23[] = {
  "namespace std { class  ispanstream; }",
  "namespace std { class wispanstream; }",
  "namespace std { class  stacktrace_entry; }",

  NULL
};

/**
 * Embedded C types.
 *
 * @sa [Information Technology — Programming languages - C - Extensions to support embedded processors](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1169.pdf)
 */
static char const *const PREDEFINED_EMBEDDED_C[] = {
  "typedef          short _Accum int_hk_t",
  "typedef          short _Fract int_hr_t",
  "typedef                _Accum int_k_t",
  "typedef          long  _Accum int_lk_t",
  "typedef          long  _Fract int_lr_t",
  "typedef                _Fract int_r_t",
  "typedef unsigned short _Accum uint_uhk_t",
  "typedef unsigned short _Fract uint_uhr_t",
  "typedef unsigned       _Accum uint_uk_t",
  "typedef unsigned long  _Accum uint_ulk_t",
  "typedef unsigned long  _Fract uint_ulr_t",
  "typedef unsigned       _Fract uint_ur_t",

  NULL
};

/**
 * Predefined GNU C types.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_GNU_C[] = {
  "typedef _Float128   __float128",
  "typedef _Float16    __fp16",
  "typedef long double __ibm128",
  "typedef _Float64x   __float80",
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
  "typedef long long   __int128",

  "typedef void      (*sighandler_t)(int)",
  NULL
};

/**
 * Predefined miscellaneous standard-ish types.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const PREDEFINED_MISC[] = {
  "typedef  int32_t       blkcnt_t",
  "typedef  int32_t       blksize_t",
  "typedef unsigned       cc_t",
  "struct                 fd_set",
  "typedef unsigned long  fsblkcnt_t",
  "typedef unsigned long  fsfilcnt_t",
  "typedef void          *iconv_t",
  "typedef  int32_t       key_t",
  "struct                 locale_t",
  "typedef  int32_t       mode_t",
  "typedef unsigned long  nfds_t",
  "typedef uint32_t       nlink_t",
  "typedef uint32_t       rlim_t",
  "struct                 siginfo_t",
  "typedef void         (*sig_t)(int)",
  "typedef unsigned long  sigset_t",
  "typedef  void         *timer_t",

  "enum                   clockid_t",
  "typedef  int64_t       suseconds_t",
  "typedef uint32_t       useconds_t",

  "typedef uint32_t       id_t",
  "typedef uint32_t       gid_t",
  "typedef  int32_t       pid_t",
  "typedef uint32_t       uid_t",

  "typedef void          *posix_spawnattr_t",
  "typedef void          *posix_spawn_file_actions_t",

  "struct                 regex_t",
  "struct                 regmatch_t",
  "typedef size_t         regoff_t",

  "typedef uint32_t       in_addr_t",
  "typedef uint16_t       in_port_t",
  "typedef uint32_t       sa_family_t",
  "typedef uint32_t       socklen_t",

  NULL
};

/**
 * Predefined types for Microsoft Windows.
 *
 * @sa [Windows Data Types](https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types)
 */
static char const *const PREDEFINED_WIN32[] = {
  //
  // The comment about GNU C's __int128 type applies to these also.
  //
  "typedef   signed char          __int8",
  "typedef          __int8        _int8",
  "typedef          short         __int16",
  "typedef          __int16       _int16",
  "typedef          int           __int32",
  "typedef          __int32       _int32",
  "typedef          long long     __int64",
  "typedef          __int64       _int64",
  "typedef          wchar_t       __wchar_t",

  "struct                         __m128",
  "struct                         __m128d",
  "struct                         __m128i",
  "struct                         __m64",

  "typedef          int           BOOL",
  "typedef BOOL                 *PBOOL",
  "typedef BOOL                *LPBOOL",
  "typedef          wchar_t       WCHAR",
  "typedef WCHAR                *PWCHAR",
  "typedef unsigned char          BYTE",
  "typedef WCHAR                 TBYTE",
  "typedef BYTE                 *PBYTE",
  "typedef TBYTE               *PTBYTE",
  "typedef BYTE                *LPBYTE",
  "typedef BYTE                   BOOLEAN",
  "typedef BOOLEAN              *PBOOLEAN",
  "typedef          char          CHAR",
  "typedef          char         CCHAR",
  "typedef CHAR                 *PCHAR",
  "typedef CHAR                *LPCHAR",
  "typedef WCHAR                 TCHAR",
  "typedef TCHAR               *PTCHAR",
  "typedef          short         SHORT",
  "typedef SHORT                *PSHORT",
  "typedef          int           INT",
  "typedef INT                  *PINT",
  "typedef          int        *LPINT",
  "typedef          long          LONG",
  "typedef LONG                 *PLONG",
  "typedef          long       *LPLONG",
  "typedef          long long     LONGLONG",
  "typedef LONGLONG             *PLONGLONG",
  "typedef          float         FLOAT",
  "typedef FLOAT                *PFLOAT",
  "typedef          void        *PVOID",
  "typedef          void       *LPVOID",
  "typedef    const void      *LPCVOID",

  "typedef unsigned char          UCHAR",
  "typedef UCHAR                *PUCHAR",
  "typedef unsigned short         USHORT",
  "typedef USHORT               *PUSHORT",
  "typedef unsigned int           UINT",
  "typedef UINT                 *PUINT",
  "typedef unsigned long          ULONG",
  "typedef ULONG                *PULONG",
  "typedef unsigned long long     ULONGLONG",
  "typedef ULONGLONG            *PULONGLONG",

  "typedef unsigned short         WORD",
  "typedef WORD                 *PWORD",
  "typedef WORD                *LPWORD",
  "typedef unsigned long          DWORD",
  "typedef DWORD                *PDWORD",
  "typedef DWORD               *LPDWORD",
  "typedef unsigned long          DWORDLONG",
  "typedef DWORDLONG            *PDWORDLONG",
  "typedef unsigned int           DWORD32",
  "typedef DWORD32              *PDWORD32",
  "typedef unsigned long          DWORD64",
  "typedef DWORD64              *PDWORD64",
  "typedef unsigned long long     QWORD",

  "typedef   signed char          INT8",
  "typedef INT8                 *PINT8",
  "typedef          short         INT16",
  "typedef INT16                *PINT16",
  "typedef          int           INT32",
  "typedef INT32                *PINT32",
  "typedef          long          INT64",
  "typedef INT64                *PINT64",
  "typedef          int           HALF_PTR",
  "typedef HALF_PTR             *PHALF_PTR",
  "typedef        __int64         INT_PTR",
  "typedef INT_PTR              *PINT_PTR",
  "typedef          int           LONG32",
  "typedef LONG32               *PLONG32",
  "typedef        __int64         LONG64",
  "typedef LONG64               *PLONG64",
  "typedef        __int64         LONG_PTR",
  "typedef LONG_PTR             *PLONG_PTR",

  "typedef unsigned char          UINT8",
  "typedef UINT8                *PUINT8",
  "typedef unsigned short         UINT16",
  "typedef UINT16               *PUINT16",
  "typedef unsigned int           UINT32",
  "typedef UINT32               *PUINT32",
  "typedef unsigned long          UINT64",
  "typedef UINT64               *PUINT64",
  "typedef unsigned int           UHALF_PTR",
  "typedef UHALF_PTR            *PUHALF_PTR",
  "typedef unsigned long          UINT_PTR",
  "typedef UINT_PTR             *PUINT_PTR",
  "typedef unsigned int           ULONG32",
  "typedef ULONG32              *PULONG32",
  "typedef unsigned long          ULONG64",
  "typedef ULONG64              *PULONG64",
  "typedef unsigned long          ULONG_PTR",
  "typedef ULONG_PTR            *PULONG_PTR",

  "typedef ULONG_PTR              DWORD_PTR",
  "typedef DWORD_PTR            *PDWORD_PTR",
  "typedef ULONG_PTR              SIZE_T",
  "typedef SIZE_T               *PSIZE_T",
  "typedef LONG_PTR               SSIZE_T",
  "typedef SSIZE_T              *PSSIZE_T",

  "typedef PVOID                  HANDLE",
  "typedef HANDLE               *PHANDLE",
  "typedef HANDLE              *LPHANDLE",
  "typedef HANDLE                 HBITMAP",
  "typedef HANDLE                 HBRUSH",
  "typedef HANDLE                 HCOLORSPACE",
  "typedef HANDLE                 HCONV",
  "typedef HANDLE                 HCONVLIST",
  "typedef HANDLE                 HDC",
  "typedef HANDLE                 HDDEDATA",
  "typedef HANDLE                 HDESK",
  "typedef HANDLE                 HDROP",
  "typedef HANDLE                 HDWP",
  "typedef HANDLE                 HENHMETAFILE",
  "typedef HANDLE                 HFONT",
  "typedef HANDLE                 HGDIOBJ",
  "typedef HANDLE                 HGLOBAL",
  "typedef HANDLE                 HHOOK",
  "typedef HANDLE                 HICON",
  "typedef HICON                  HCURSOR",
  "typedef HANDLE                 HINSTANCE",
  "typedef HANDLE                 HKEY",
  "typedef HKEY                 *PHKEY",
  "typedef HANDLE                 HKL",
  "typedef HANDLE                 HLOCAL",
  "typedef HANDLE                 HMENU",
  "typedef HANDLE                 HMETAFILE",
  "typedef HINSTANCE              HMODULE",
  "typedef HANDLE                 HMONITOR",
  "typedef HANDLE                 HPALETTE",
  "typedef HANDLE                 HPEN",
  "typedef HANDLE                 HRGN",
  "typedef HANDLE                 HRSRC",
  "typedef HANDLE                 HSZ",
  "typedef HANDLE                 HWINSTA",
  "typedef HANDLE                 HWND",

  "typedef          CHAR        *PSTR",
  "typedef   const  CHAR       *PCSTR",
  "typedef          CHAR       *LPSTR",
  "typedef   const  CHAR      *LPCSTR",
  "typedef         WCHAR       *PWSTR",
  "typedef   const WCHAR      *PCWSTR",
  "typedef         WCHAR      *LPWSTR",
  "typedef   const WCHAR     *LPCWSTR",
  "typedef       LPWSTR         PTSTR",
  "typedef       LPWSTR        LPTSTR",
  "typedef      LPCWSTR        PCTSTR",
  "typedef      LPCWSTR       LPCTSTR",

  "typedef WORD                   ATOM",
  "typedef DWORD                  COLORREF",
  "typedef COLORREF            *LPCOLORREF",
  "typedef          int           HFILE",
  "typedef          long          HRESULT",
  "typedef WORD                   LANGID",
  "typedef union _LARGE_INTEGER   LARGE_INTEGER",
  "typedef union _ULARGE_INTEGER ULARGE_INTEGER",
  "typedef DWORD                  LCID",
  "typedef PDWORD                PLCID",
  "typedef DWORD                  LCTYPE",
  "typedef DWORD                  LGRPID",
  "typedef LONG_PTR               LRESULT",
  "typedef HANDLE                 SC_HANDLE",
  "typedef LPVOID                 SC_LOCK",
  "typedef HANDLE                 SERVICE_STATUS_HANDLE",
  "struct                         UNICODE_STRING",
  "typedef LONGLONG               USN",
  "typedef UINT_PTR               WPARAM",

  NULL
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
  rb_tree_cleanup( &typedefs, &free );
}

/**
 * Comparison function for \ref c_typedef data used by the red-black tree.
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the `typedef` name pointed to by \a i_data is less than, equal
 * to, or greater than the `typedef` name pointed to by \a j_data.
 */
PJL_WARN_UNUSED_RESULT
static int c_typedef_cmp( void const *i_data, void const *j_data ) {
  assert( i_data != NULL );
  assert( j_data != NULL );
  c_typedef_t const *const i_tdef = i_data;
  c_typedef_t const *const j_tdef = j_data;
  return c_sname_cmp( &i_tdef->ast->sname, &j_tdef->ast->sname );
}

/**
 * Creates a new \ref c_typedef.
 *
 * @param ast The AST of the type.
 * @return Returns said \ref c_typedef.
 */
PJL_WARN_UNUSED_RESULT
static c_typedef_t* c_typedef_new( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_typedef_t *const tdef = MALLOC( c_typedef_t, 1 );
  tdef->ast = ast;
  //
  // User-defined types are available in the current language and later;
  // predefined types are available only in the language(s) explicitly
  // specified by predefined_lang_ids.
  //
  tdef->lang_ids = user_defined ?
    c_lang_and_newer( opt_lang ) : predefined_lang_ids;
  tdef->user_defined = user_defined;
  tdef->defined_in_english = cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH;
  return tdef;
}

/**
 * Parses an array of predefined type declarations.
 *
 * @param types A pointer to the start of an array of pointers to strings of
 * cdecl commands that define types.  The last element must be NULL.
 */
static void parse_predefined_types( char const *const *types ) {
  assert( types != NULL );
  do {
    if ( unlikely( cdecl_parse_string( *types, strlen( *types ) ) != EX_OK ) )
      INTERNAL_ERR( "\"%s\": failed to parse predefined type\n", *types );
  } while ( *++types != NULL );
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
PJL_WARN_UNUSED_RESULT
static bool rb_visitor( void *node_data, void *v_data ) {
  assert( node_data != NULL );
  assert( v_data != NULL );

  c_typedef_t const *const tdef = node_data;
  tdef_rb_visit_data_t const *const trvd = v_data;

  return (*trvd->visit_fn)( tdef, trvd->v_data );
}

////////// extern functions ///////////////////////////////////////////////////

c_typedef_t const* c_typedef_add( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( !c_sname_empty( &ast->sname ) );

  c_typedef_t *const new_tdef = c_typedef_new( ast );
  rb_node_t const *const old_rb = rb_tree_insert( &typedefs, new_tdef );
  if ( old_rb == NULL )                 // type's name doesn't exist
    return NULL;

  //
  // A typedef having the same name already exists, so we don't need the new
  // c_typedef.
  //
  FREE( new_tdef );

  //
  // In C, multiple typedef declarations having the same name are allowed only
  // if the types are equivalent:
  //
  //      typedef int T;
  //      typedef int T;              // OK
  //      typedef double T;           // error: types aren't equivalent
  //
  c_typedef_t const *const old_tdef = old_rb->data;
  static c_typedef_t const EMPTY_TYPEDEF;
  return c_ast_equal( ast, old_tdef->ast ) ? &EMPTY_TYPEDEF : old_tdef;
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
    rb_tree_find( &typedefs, &C_TYPEDEF_LIT( *sname ) );
  return found_rb != NULL ? found_rb->data : NULL;
}

void c_typedef_init( void ) {
  rb_tree_init( &typedefs, &c_typedef_cmp );
  IF_EXIT( atexit( c_typedef_cleanup ) != 0, EX_OSERR );

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

    predefined_lang_ids = LANG_MIN(C_KNR);
    parse_predefined_types( PREDEFINED_KNR_C );

    predefined_lang_ids = LANG_MIN(C_89);
    parse_predefined_types( PREDEFINED_STD_C_89 );
    parse_predefined_types( PREDEFINED_FLOATING_POINT_EXTENSIONS );
    parse_predefined_types( PREDEFINED_GNU_C );

    predefined_lang_ids = LANG_MIN(C_95);
    parse_predefined_types( PREDEFINED_STD_C_95 );
    parse_predefined_types( PREDEFINED_PTHREAD_H );
    parse_predefined_types( PREDEFINED_WIN32 );

    predefined_lang_ids = LANG_MIN(C_99);
    parse_predefined_types( PREDEFINED_STD_C_99 );

    // However, Embedded C extensions are available only in C99.
    opt_lang = LANG_C_99;
    predefined_lang_ids = LANG_C_99;
    parse_predefined_types( PREDEFINED_EMBEDDED_C );
    opt_lang = LANG_C_NEW;

    // Must be defined after C99.
    predefined_lang_ids = LANG_MIN(C_89);
    parse_predefined_types( PREDEFINED_MISC );

    predefined_lang_ids = LANG_MIN(C_11);
    parse_predefined_types( PREDEFINED_STD_C_11 );
  }

  //
  // Temporarily switch to the latest supported version of C++ so all keywords
  // will be available.
  //
  opt_lang = LANG_CPP_NEW;

  if ( opt_typedefs ) {
    predefined_lang_ids = LANG_MIN(CPP_OLD);
    parse_predefined_types( PREDEFINED_STD_CPP );

    predefined_lang_ids = LANG_MIN(CPP_11);
    parse_predefined_types( PREDEFINED_STD_CPP_11 );

    predefined_lang_ids = LANG_MIN(CPP_17);
    parse_predefined_types( PREDEFINED_STD_CPP_17 );

    predefined_lang_ids = LANG_MIN(CPP_20);
    parse_predefined_types( PREDEFINED_STD_CPP_20 );

    predefined_lang_ids = LANG_MIN(CPP_23);
    parse_predefined_types( PREDEFINED_STD_CPP_23 );
  }

  predefined_lang_ids = LANG_MIN(CPP_20);
  parse_predefined_types( PREDEFINED_STD_CPP_20_REQUIRED );

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

  user_defined = true;
}

c_typedef_t const* c_typedef_visit( c_typedef_visit_fn_t visit_fn,
                                    void *v_data ) {
  assert( visit_fn != NULL );
  tdef_rb_visit_data_t trvd = { visit_fn, v_data };
  rb_node_t const *const rb = rb_tree_visit( &typedefs, &rb_visitor, &trvd );
  return rb != NULL ? rb->data : NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
