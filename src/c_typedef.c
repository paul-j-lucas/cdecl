/*
**      cdecl -- C gibberish translator
**      src/c_typedef.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
 * Defines functions for looking up C/C++ typedef declarations.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast.h"
#include "c_typedef.h"
#include "options.h"
#include "red_black.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct td_rb_visitor_data {
  c_typedef_visitor_t   visitor;        ///< Caller's visitor function.
  void                 *data;           ///< Caller's optional data.
};
typedef struct td_rb_visitor_data td_rb_visitor_data_t;

// local variable definitions
static rb_tree_t *typedefs;             ///< Global set of `typedef`s.
static bool       user_defined;         ///< Are new `typedef`s used-defined?

///////////////////////////////////////////////////////////////////////////////

/**
 * Types from C.
 *
 * @note The underlying types used here are merely typical and do not
 * necessarily match the underlying type on any particular platform.
 */
static char const *const TYPEDEFS_STD_C[] = {
  "typedef long double       max_align_t",// C11
  "typedef          long     ptrdiff_t",
  "typedef int               sig_atomic_t",
  "typedef unsigned long    rsize_t",     // C11
  "typedef          long    ssize_t",
  "typedef unsigned long     size_t",

  "typedef          long     intmax_t",
  "typedef          long     intptr_t",
  "typedef unsigned long    uintmax_t",
  "typedef unsigned long    uintptr_t",

  "typedef struct     div_t     div_t",
  "typedef struct imaxdiv_t imaxdiv_t",
  "typedef struct    ldiv_t    ldiv_t",
  "typedef struct   lldiv_t   lldiv_t",

  "typedef          char     int8_t",
  "typedef          short    int16_t",
  "typedef          int      int32_t",
  "typedef          long     int64_t",
  "typedef unsigned char    uint8_t",
  "typedef unsigned short   uint16_t",
  "typedef unsigned int     uint32_t",
  "typedef unsigned long    uint64_t",

  "typedef          char     int_fast8_t",
  "typedef          short    int_fast16_t",
  "typedef          int      int_fast32_t",
  "typedef          long     int_fast64_t",
  "typedef unsigned char    uint_fast8_t",
  "typedef unsigned short   uint_fast16_t",
  "typedef unsigned int     uint_fast32_t",
  "typedef unsigned long    uint_fast64_t",

  "typedef          char     int_least8_t",
  "typedef          short    int_least16_t",
  "typedef          int      int_least32_t",
  "typedef          long     int_least64_t",
  "typedef unsigned char    uint_least8_t",
  "typedef unsigned short   uint_least16_t",
  "typedef unsigned int     uint_least32_t",
  "typedef unsigned long    uint_least64_t",

  NULL
};

/**
 * Types from `stdatomic.h`.
 */
static char const *const TYPEDEFS_STD_ATOMIC_H[] = {
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

  NULL
};

/**
 * Types from `pthread.h`.
 */
static char const *const TYPEDEFS_PTHREAD_H[] = {
  "typedef unsigned long                pthread_t",
  "typedef struct pthread_barrier_t     pthread_barrier_t",
  "typedef struct pthread_barrierattr_t pthread_barrierattr_t",
  "typedef struct pthread_cond_t        pthread_cond_t",
  "typedef struct pthread_condattr_t    pthread_condattr_t",
  "typedef struct pthread_mutex_t       pthread_mutex_t",
  "typedef struct pthread_mutexattr_t   pthread_mutexattr_t",
  "typedef int                          pthread_once_t",
  "typedef struct pthread_rwlock_t      pthread_rwlock_t",
  "typedef struct pthread_rwlockattr_t  pthread_rwlockattr_t",
  "typedef volatile int                 pthread_spinlock_t",

  NULL
};

/**
 * Types from `threads.h` (C11).
 */
static char const *const TYPEDEFS_THREADS_H[] = {
  "typedef pthread_t        thrd_t",
  "typedef pthread_cond_t   cnd_t",
  "typedef pthread_mutex_t  mtx_t",
  "typedef int              once_flag",
  "typedef int            (*thrd_start_t)(void*)",
  "typedef void           (*tss_dtor_t)(void*)",
  "typedef void*            tss_t",

  NULL
};

/**
 * Types from C++.
 */
static char const *const TYPEDEFS_STD_CPP[] = {
  "namespace std { typedef struct   div_t           div_t;            }",
  "namespace std { typedef struct  ldiv_t          ldiv_t;            }",
  "namespace std { typedef class    exception       exception;        }",
  "namespace std { typedef          long            ptrdiff_t;        }",
  "namespace std { typedef          int             sig_atomic_t;     }",
  "namespace std { typedef unsigned long            size_t;           }",
  "namespace std { typedef struct   streambuf       streambuf;        }",
  "namespace std { typedef struct  wstreambuf      wstreambuf;        }",
  "namespace std { typedef long     long            streamoff;        }",
  "namespace std { typedef          long            streamsize;       }",
  "namespace std { typedef class    string          string;           }",
  "namespace std { typedef class   wstring         wstring;           }",
  // C++11
  "namespace std { typedef struct imaxdiv_t     imaxdiv_t;            }",
  "namespace std { typedef struct   lldiv_t       lldiv_t;            }",
  "namespace std { typedef long     double          max_align_t;      }",
  "namespace std { typedef void                    *nullptr_t;        }",
  "namespace std { typedef class    thread          thread;           }",
  "namespace std { typedef class    u16string       u16string;        }",
  "namespace std { typedef class    u32string       u32string;        }",
  // C++17
  "namespace std { typedef enum     byte            byte;             }",
  // C++20
  "namespace std { typedef class  jthread           jthread;          }",
  "namespace std { typedef struct partial_ordering  partial_ordering; }",
  "namespace std { typedef struct strong_equality   strong_equality;  }",
  "namespace std { typedef struct strong_ordering   strong_ordering;  }",
  "namespace std { typedef struct weak_equality     weak_equality;    }",
  "namespace std { typedef struct weak_ordering     weak_ordering;    }",

  NULL
};

/**
 * Miscellaneous standard types.
 */
static char const *const TYPEDEFS_MISC[] = {
  "typedef  int32_t         blkcnt_t",
  "typedef  int32_t         blksize_t",
  "typedef  int32_t         dev_t",
  "typedef struct __fd_set  fd_set",
  "typedef struct __FILE    FILE",
  "typedef struct __fpos    fpos_t",
  "typedef  int32_t         ino_t",
  "typedef struct __mbstate mbstate_t",
  "typedef  int32_t         mode_t",
  "typedef unsigned long    nfds_t",
  "typedef uint32_t         nlink_t",
  "typedef  int64_t         off_t",

  "typedef  long            clock_t",
  "typedef  long            clockid_t",
  "typedef  int64_t         time_t",
  "typedef  int64_t         suseconds_t",
  "typedef uint32_t         useconds_t",

  "typedef uint32_t         gid_t",
  "typedef  int32_t         pid_t",
  "typedef uint32_t         uid_t",

  "typedef uint32_t         in_addr_t",
  "typedef uint16_t         in_port_t",
  "typedef uint32_t         sa_family_t",
  "typedef uint32_t         socklen_t",

  "typedef  int             errno_t",
  "typedef uint32_t         rlim_t",
  "typedef unsigned long    sigset_t",
  "typedef  int             wint_t",

  NULL
};

/**
 * GNU C types.
 */
static char const *const TYPEDEFS_GNUC[] = {
  "typedef float        _Decimal32",
  "typedef double       _Decimal64",
  "typedef long double  _Decimal128",
  "typedef long double  _Float128",
  "typedef _Float128   __float128",
  "typedef float        _Float16",
  "typedef _Float16    __fp16",
  "typedef long double __ibm128",
  "typedef double       _Float64x",
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
  // to be round-trippable with English.  If it reused T_LONG_LONG, then you'd
  // get:
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

  NULL
};

/**
 * Windows types.
 *
 * @sa https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types
 */
static char const *const TYPEDEFS_WIN32[] = {
  //
  // The comment about GNU C's __int128 type applies to these also.
  //
  "typedef char                   __int8",
  "typedef short                  __int16",
  "typedef int                    __int32",
  "typedef long long              __int64",
  "typedef wchar_t                __wchar_t",

  "typedef int                    BOOL",
  "typedef BOOL                 *PBOOL",
  "typedef BOOL                *LPBOOL",
  "typedef wchar_t                WCHAR",
  "typedef WCHAR                *PWCHAR",
  "typedef unsigned char          BYTE",
  "typedef WCHAR                 TBYTE",
  "typedef BYTE                 *PBYTE",
  "typedef TBYTE               *PTBYTE",
  "typedef BYTE                *LPBYTE",
  "typedef BYTE                   BOOLEAN",
  "typedef BOOLEAN              *PBOOLEAN",
  "typedef char                   CHAR",
  "typedef char                  CCHAR",
  "typedef CHAR                 *PCHAR",
  "typedef CHAR                *LPCHAR",
  "typedef WCHAR                 TCHAR",
  "typedef TCHAR               *PTCHAR",
  "typedef short                  SHORT",
  "typedef SHORT                *PSHORT",
  "typedef int                    INT",
  "typedef INT                  *PINT",
  "typedef int                 *LPINT",
  "typedef long                   LONG",
  "typedef LONG                 *PLONG",
  "typedef long                *LPLONG",
  "typedef long long              LONGLONG",
  "typedef LONGLONG             *PLONGLONG",
  "typedef float                  FLOAT",
  "typedef FLOAT                *PFLOAT",
  "typedef void                 *PVOID",
  "typedef void                *LPVOID",
  "typedef const void         *LPCVOID",

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

  "typedef signed char            INT8",
  "typedef INT8                 *PINT8",
  "typedef short                  INT16",
  "typedef INT16                *PINT16",
  "typedef int                    INT32",
  "typedef INT32                *PINT32",
  "typedef long                   INT64",
  "typedef INT64                *PINT64",
  "typedef int                    HALF_PTR",
  "typedef HALF_PTR             *PHALF_PTR",
  "typedef __int64                INT_PTR",
  "typedef INT_PTR              *PINT_PTR",
  "typedef int                    LONG32",
  "typedef LONG32               *PLONG32",
  "typedef __int64                LONG64",
  "typedef LONG64               *PLONG64",
  "typedef __int64                LONG_PTR",
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

  "typedef CHAR                 *PSTR",
  "typedef const CHAR          *PCSTR",
  "typedef CHAR                *LPSTR",
  "typedef const CHAR         *LPCSTR",
  "typedef WCHAR               *PWSTR",
  "typedef const WCHAR        *PCWSTR",
  "typedef WCHAR              *LPWSTR",
  "typedef const WCHAR       *LPCWSTR",
  "typedef LPWSTR               PTSTR",
  "typedef LPWSTR              LPTSTR",
  "typedef LPCWSTR             PCTSTR",
  "typedef LPCWSTR            LPCTSTR",

  "typedef WORD                   ATOM",
  "typedef DWORD                  COLORREF",
  "typedef COLORREF            *LPCOLORREF",
  "typedef int                    HFILE",
  "typedef long                   HRESULT",
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
  "typedef struct _UNICODE_STRING UNICODE_STRING",
  "typedef LONGLONG               USN",
  "typedef UINT_PTR               WPARAM",

  NULL
};

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets a pointer to a red-black tree node's data cast to `c_typedef_t const*`.
 *
 * @param node A pointer to the node to get the data of.
 * @return Returns said pointer.
 */
C_WARN_UNUSED_RESULT
static inline c_typedef_t const*
c_typedef_node_data_get( rb_node_t const *node ) {
  return REINTERPRET_CAST( c_typedef_t const*, rb_node_data( node ) );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Comparison function for `c_typedef` data used by the red-black tree.
 *
 * @param data_i A pointer to data.
 * @param data_j A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the `typedef` name pointed to by \a data_i is less than, equal
 * to, or greater than the `typedef` name pointed to by \a data_j.
 */
C_WARN_UNUSED_RESULT
static int c_typedef_cmp( void const *data_i, void const *data_j ) {
  c_typedef_t const *const ti = REINTERPRET_CAST( c_typedef_t const*, data_i );
  c_typedef_t const *const tj = REINTERPRET_CAST( c_typedef_t const*, data_j );
  return c_sname_cmp( &ti->ast->sname, &tj->ast->sname );
}

/**
 * Creates a new `c_typedef`.
 *
 * @param ast The `c_ast` of the type.
 * @return Returns said `c_typedef`.
 */
C_WARN_UNUSED_RESULT
static c_typedef_t* c_typedef_new( c_ast_t const *ast ) {
  assert( ast != NULL );
  c_typedef_t *const t = MALLOC( c_typedef_t, 1 );
  t->ast = ast;
  t->user_defined = user_defined;
  return t;
}

/**
 * Parses an array of built-in `typedef` declarations.
 *
 * @param types An array of pointers to `typedef` strings.  The last element
 * must be null.
 */
static void c_typedef_parse_builtins( char const *const types[] ) {
  extern bool parse_string( char const*, size_t );
  for ( char const *const *ptype = types; *ptype != NULL; ++ptype ) {
    bool const ok = parse_string( *ptype, 0 );
    assert( ok );
    (void)ok;
  } // for
}

/**
 * Red-black tree visitor function that forwards to the
 * <code>\ref c_typedef_visitor_t</code> function.
 *
 * @param node_data A pointer to the node's data.
 * @param aux_data Optional data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of `rb_tree_visit()`.
 */
C_WARN_UNUSED_RESULT
static bool rb_visitor( void *node_data, void *aux_data ) {
  c_typedef_t const *const t =
    REINTERPRET_CAST( c_typedef_t const*, node_data );

  td_rb_visitor_data_t const *const vd =
    REINTERPRET_CAST( td_rb_visitor_data_t*, aux_data );

  return (*vd->visitor)( t, vd->data );
}

////////// extern functions ///////////////////////////////////////////////////

c_typedef_add_rv_t c_typedef_add( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( !c_ast_sname_empty( ast ) );

  c_typedef_t *const new_t = c_typedef_new( ast );
  rb_node_t const *const old_rb = rb_tree_insert( typedefs, new_t );
  if ( old_rb == NULL )                 // type's name doesn't exist
    return TD_ADD_ADDED;

  //
  // A typedef having the same name already exists, so we don't need the new
  // c_typedef.
  //
  FREE( new_t );

  //
  // In C, multiple typedef declarations having the same name are allowed only
  // if the types are equivalent:
  //
  //      typedef int T;
  //      typedef int T;              // OK
  //      typedef double T;           // error: types aren't equivalent
  //
  c_typedef_t const *const old_t = c_typedef_node_data_get( old_rb );
  return c_ast_equiv( ast, old_t->ast ) ? TD_ADD_EQUIV : TD_ADD_DIFF;
}

void c_typedef_cleanup( void ) {
  rb_tree_free( typedefs );
  typedefs = NULL;
}

c_typedef_t const* c_typedef_find( c_sname_t const *sname ) {
  assert( sname != NULL );
  //
  // Create a temporary c_typedef with just the name set in order to find it.
  //
  c_ast_t ast;
  ast.sname = *sname;
  c_typedef_t t;
  t.ast = &ast;

  rb_node_t const *const rb_found = rb_tree_find( typedefs, &t );
  return rb_found != NULL ? c_typedef_node_data_get( rb_found ) : NULL;
}

void c_typedef_init( void ) {
  assert( typedefs == NULL );
  typedefs = rb_tree_new( &c_typedef_cmp, NULL );

  if ( opt_typedefs ) {
#ifdef ENABLE_CDECL_DEBUG
    //
    // Temporarily turn off debug output for built-in typedefs.
    //
    bool const prev_debug = opt_debug;
    opt_debug = false;
#endif /* ENABLE_CDECL_DEBUG */
    //
    // Temporarily set the language to the latest C++ version to allow all
    // built-in typedefs.
    //
    c_lang_id_t const prev_lang = opt_lang;
    opt_lang = LANG_CPP_NEW;
#ifdef YYDEBUG
    //
    // Temporarily turn off Bison debug output for built-in typedefs.
    //
    int const prev_yydebug = yydebug;
    yydebug = 0;
#endif /* YYDEBUG */

    c_typedef_parse_builtins( TYPEDEFS_STD_C );
    c_typedef_parse_builtins( TYPEDEFS_STD_ATOMIC_H );
    c_typedef_parse_builtins( TYPEDEFS_PTHREAD_H );
    c_typedef_parse_builtins( TYPEDEFS_THREADS_H );
    c_typedef_parse_builtins( TYPEDEFS_STD_CPP );
    c_typedef_parse_builtins( TYPEDEFS_MISC );
    c_typedef_parse_builtins( TYPEDEFS_GNUC );
    c_typedef_parse_builtins( TYPEDEFS_WIN32 );

#ifdef ENABLE_CDECL_DEBUG
    opt_debug = prev_debug;
#endif /* ENABLE_CDECL_DEBUG */
    opt_lang = prev_lang;
#ifdef YYDEBUG
    yydebug = prev_yydebug;
#endif /* YYDEBUG */
  }

  user_defined = true;
}

c_typedef_t const* c_typedef_visit( c_typedef_visitor_t visitor, void *data ) {
  assert( visitor != NULL );
  td_rb_visitor_data_t vd = { visitor, data };
  rb_node_t const *const rb = rb_tree_visit( typedefs, &rb_visitor, &vd );
  return rb != NULL ? c_typedef_node_data_get( rb ) : NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
