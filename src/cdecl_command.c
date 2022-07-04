/*
**      cdecl -- C gibberish translator
**      src/cdecl_command.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas, et al.
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
 * Declares data and functions for cdecl commands.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl_command.h"
#include "c_lang.h"
#include "literals.h"

// shorthands
#define ANYWHERE                  CDECL_COMMAND_ANYWHERE
#define FIRST_ARG                 CDECL_COMMAND_FIRST_ARG
#define LANG_ONLY                 CDECL_COMMAND_LANG_ONLY
#define PROG_NAME                 CDECL_COMMAND_PROG_NAME

///////////////////////////////////////////////////////////////////////////////

/**
 * Cdecl commands.
 */
static cdecl_command_t const CDECL_COMMANDS[] = {
  { L_CAST,                   PROG_NAME,  LANG_ANY                },
  { L_CLASS,                  FIRST_ARG,  LANG_CPP_ANY            },
  { L_CONST /* cast */,       FIRST_ARG,  LANG_CPP_ANY            },
  { L_DECLARE,                PROG_NAME,  LANG_ANY                },
  { L_DEFINE,                 FIRST_ARG,  LANG_ANY                },
  { L_DYNAMIC /* cast */,     FIRST_ARG,  LANG_CPP_ANY            },
  { L_ENUM,                   FIRST_ARG,  LANG_ENUM               },
  { L_EXIT,                   LANG_ONLY,  LANG_ANY                },
  { L_EXPLAIN,                PROG_NAME,  LANG_ANY                },
  { L_HELP,                   FIRST_ARG,  LANG_ANY                },
  { L_INCLUDE,                FIRST_ARG,  LANG_ANY                },
  { L_INLINE /* namespace */, FIRST_ARG,  LANG_INLINE_NAMESPACE   },
  { L_NAMESPACE,              FIRST_ARG,  LANG_CPP_ANY            },
  { L_QUIT,                   LANG_ONLY,  LANG_ANY                },
  { L_REINTERPRET /* cast */, FIRST_ARG,  LANG_CPP_ANY            },
  { L_SET_COMMAND,            FIRST_ARG,  LANG_ANY                },
  { L_SHOW,                   FIRST_ARG,  LANG_ANY                },
  { L_STATIC /* cast */,      FIRST_ARG,  LANG_CPP_ANY            },
  { L_STRUCT,                 FIRST_ARG,  LANG_ANY                },
  { L_TYPEDEF,                FIRST_ARG,  LANG_ANY                },
  { L_UNION,                  FIRST_ARG,  LANG_ANY                },
  { L_USING,                  FIRST_ARG,  LANG_USING_DECLARATION  },
  { NULL,                     ANYWHERE,   LANG_NONE               },
};

////////// extern functions ///////////////////////////////////////////////////

cdecl_command_t const* cdecl_command_next( cdecl_command_t const *command ) {
  return command == NULL ?
    CDECL_COMMANDS :
    (++command)->literal == NULL ? NULL : command;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
