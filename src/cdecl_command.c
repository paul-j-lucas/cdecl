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

///////////////////////////////////////////////////////////////////////////////

/**
 * Cdecl commands.
 */
static cdecl_command_t const CDECL_COMMANDS[] = {
  { L_CAST,                   CDECL_COMMAND_PROG_NAME,  LANG_ANY          },
  { L_CLASS,                  CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_CONST /* cast */,       CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_DECLARE,                CDECL_COMMAND_PROG_NAME,  LANG_ANY          },
  { L_DEFINE,                 CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_DYNAMIC /* cast */,     CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_ENUM,                   CDECL_COMMAND_FIRST_ARG,  LANG_MIN(C_89)    },
  { L_EXIT,                   CDECL_COMMAND_LANG_ONLY,  LANG_ANY          },
  { L_EXPLAIN,                CDECL_COMMAND_PROG_NAME,  LANG_ANY          },
  { L_HELP,                   CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_INLINE /* namespace */, CDECL_COMMAND_FIRST_ARG,  LANG_CPP_MIN(11)  },
  { L_NAMESPACE,              CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_QUIT,                   CDECL_COMMAND_LANG_ONLY,  LANG_ANY          },
  { L_REINTERPRET /* cast */, CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_SET_COMMAND,            CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_SHOW,                   CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_STATIC /* cast */,      CDECL_COMMAND_FIRST_ARG,  LANG_CPP_ANY      },
  { L_STRUCT,                 CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_TYPEDEF,                CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_UNION,                  CDECL_COMMAND_FIRST_ARG,  LANG_ANY          },
  { L_USING,                  CDECL_COMMAND_FIRST_ARG,  LANG_CPP_MIN(11)  },
  { NULL,                     CDECL_COMMAND_ANYWHERE,   LANG_NONE         },
};

////////// extern functions ///////////////////////////////////////////////////

cdecl_command_t const* cdecl_command_next( cdecl_command_t const *command ) {
  return command == NULL ?
    CDECL_COMMANDS :
    (++command)->literal == NULL ? NULL : command;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
