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
struct rb_visitor_data {
  c_typedef_visitor_t visitor;          ///< Caller's visitor function.
  void *data;                           ///< Caller's optional data.
};
typedef struct rb_visitor_data rb_visitor_data_t;

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
  "namespace std { typedef class    u16string       u16string;        }",
  "namespace std { typedef class    u32string       u32string;        }",
  // C++17
  "namespace std { typedef enum     byte            byte;             }",
  // C++20
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

  rb_visitor_data_t const *const vd =
    REINTERPRET_CAST( rb_visitor_data_t*, aux_data );

  return vd->visitor( t, vd->data );
}

////////// extern functions ///////////////////////////////////////////////////

td_add_rv_t c_typedef_add( c_ast_t const *ast ) {
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
  rb_tree_free( typedefs, NULL );
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
  typedefs = rb_tree_new( &c_typedef_cmp );

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
    c_typedef_parse_builtins( TYPEDEFS_STD_CPP );
    c_typedef_parse_builtins( TYPEDEFS_MISC );

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
  rb_visitor_data_t vd = { visitor, data };
  rb_node_t const *const rb = rb_tree_visit( typedefs, &rb_visitor, &vd );
  return rb != NULL ? c_typedef_node_data_get( rb ) : NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
