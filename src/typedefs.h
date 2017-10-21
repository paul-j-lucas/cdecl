/*
**      cdecl -- C gibberish translator
**      src/typedefs.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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

#ifndef cdecl_typedefs_H
#define cdecl_typedefs_H

/**
 * @file
 * Declares \c enum and \c struct \c typedef definitions in one file.
 *
 * Some headers are bidirectionally dependent, so \c typedef were used
 * originally rather than \c #include.  However, some old C compilers don't
 * like multiple \c typedef definitions even if the types match.  Hence, just
 * put all \c typedef definitions in one file.
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * The source location used by Bison.
 */
struct c_loc {
  //
  // These should be either unsigned or size_t, but Bison generates code that
  // tests these for >= 0 which is always true for unsigned types so it
  // generates warnings; hence these are kept as int to eliminate the warnings.
  //
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

/**
 * Mode of operation.
 */
enum c_mode {
  MODE_ENGLISH_TO_GIBBERISH,
  MODE_GIBBERISH_TO_ENGLISH
};

typedef struct c_ast        c_ast_t;
typedef unsigned            c_ast_id_t;
typedef struct c_ast_list   c_ast_list_t;
typedef struct c_ast_pair   c_ast_pair_t;
typedef struct c_array      c_array_t;
typedef struct c_block      c_block_t;
typedef enum   c_check      c_check_t;
typedef struct c_lang_info  c_lang_info_t;
typedef struct c_loc        c_loc_t;
typedef struct c_builtin    c_builtin_t;
typedef struct c_ecsu       c_ecsu_t;
typedef struct c_func       c_func_t;
typedef struct c_keyword    c_keyword_t;
typedef enum   c_kind       c_kind_t;
typedef enum   c_mode       c_mode_t;
typedef struct c_parent     c_parent_t;
typedef struct c_ptr_mbr    c_ptr_mbr_t;
typedef struct c_ptr_ref    c_ptr_ref_t;
typedef struct c_typedef    c_typedef_t;
typedef enum   v_direction  v_direction_t;

// for Bison
typedef c_loc_t YYLTYPE;
#define YYLTYPE_IS_DECLARED       1
#define YYLTYPE_IS_TRIVIAL        1

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_typedefs_H */
/* vim:set et sw=2 ts=2: */
