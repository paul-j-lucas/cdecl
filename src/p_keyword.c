/*
**      cdecl -- C gibberish translator
**      src/p_keyword.c
**
**      Copyright (C) 2023-2024  Paul J. Lucas
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
 * Defines functions for looking up C/C++ keyword or C23/C++11 (or later)
 * attribute information.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "p_keyword.h"
#include "literals.h"
#include "util.h"
#include "cdecl_parser.h"               /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <stdlib.h>
#include <string.h>

/// @endcond

/**
 * @addtogroup p-keywords-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Array of all C preprocessor keywords.
 *
 * @note This is not declared `const` because it's sorted once.
 */
static p_keyword_t P_KEYWORDS[] = {
  { L_P_define,   Y_P_define    },
  { L_P_elif,     Y_P_elif      },
  { L_P_else,     Y_P_else      },
  { L_P_error,    Y_P_error     },
  { L_P_if,       Y_P_if        },
  { L_P_ifdef,    Y_P_ifdef     },
  { L_P_ifndef,   Y_P_ifndef    },
//{ L_P_include,  Y_P_include   },      // handled within the lexer
  { L_P_line,     Y_P_line      },
  { L_P_undef,    Y_P_undef     },

  // C99
  { L_P_pragma,   Y_P_pragma    },

  // C23
  { L_P_elifdef,  Y_P_elifdef   },
  { L_P_elifndef, Y_P_elifndef  },
  { L_P_embed,    Y_P_embed     },
  { L_P_warning,  Y_P_warning   },

  { NULL,         0             }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Compares two \ref p_keyword objects.
 *
 * @param i_pk The first \ref p_keyword to compare.
 * @param j_pk The second \ref p_keyword to compare.
 * @return @return Returns a number less than 0, 0, or greater than 0 if \a
 * i_pk is less than, equal to, or greater than \a j_pk, respectively.
 */
NODISCARD
static int p_keyword_cmp( p_keyword_t const *i_pk, p_keyword_t const *j_pk ) {
  return strcmp( i_pk->literal, j_pk->literal );
}

////////// extern functions ///////////////////////////////////////////////////

p_keyword_t const* p_keyword_find( char const *literal ) {
  assert( literal != NULL );

  // the list is small, so linear search is good enough
  for ( p_keyword_t const *pk = P_KEYWORDS; pk->literal != NULL; ++pk ) {
    int const cmp = strcmp( literal, pk->literal );
    if ( cmp > 0 )
      continue;
    if ( cmp < 0 )                      // the array is sorted
      break;
    return pk;
  } // for

  return NULL;
}

void p_keywords_init( void ) {
  ASSERT_RUN_ONCE();
  qsort(                                // don't rely on manual sorting above
    P_KEYWORDS, ARRAY_SIZE( P_KEYWORDS ) - 1/*NULL*/, sizeof( p_keyword_t ),
    POINTER_CAST( qsort_cmp_fn_t, &p_keyword_cmp )
  );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
