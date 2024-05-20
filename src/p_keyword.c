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
  { PL_define,    Y_PRE_define    },
  { PL_elif,      Y_PRE_elif      },
  { PL_else,      Y_PRE_else      },
  { PL_error,     Y_PRE_error     },
  { PL_if,        Y_PRE_if        },
  { PL_ifdef,     Y_PRE_ifdef     },
  { PL_ifndef,    Y_PRE_ifndef    },
//{ PL_include,   Y_PRE_include   },    // handled within the lexer
  { PL_line,      Y_PRE_line      },
  { PL_undef,     Y_PRE_undef     },

  // C99
  { PL_pragma,    Y_PRE_pragma    },

  // C23
  { PL_elifdef,   Y_PRE_elifdef   },
  { PL_elifndef,  Y_PRE_elifndef  },
  { PL_embed,     Y_PRE_embed     },
  { PL_warning,   Y_PRE_warning   },

  { NULL,         0               }
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
  return bsearch(
    &(p_keyword_t){ .literal = literal }, P_KEYWORDS,
    ARRAY_SIZE( P_KEYWORDS ) - 1/*NULL*/, sizeof( P_KEYWORDS[0] ),
    POINTER_CAST( bsearch_cmp_fn_t, &p_keyword_cmp )
  );
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
