# Process this file with autoconf to produce a configure script.

##
#       cdecl -- C gibberish translator
#       configure.ac
#
#       Copyright (C) 2017-2021  Paul J. Lucas, et al.
#
#       This program is free software: you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation, either version 3 of the License, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

AC_INIT([cdecl],[9.9],[https://github.com/paul-j-lucas/cdecl/issues],[],[https://github.com/paul-j-lucas/cdecl])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_LANG(C)
AC_PROG_CC
gl_EARLY
AX_PROG_FLEX(
  [AC_DEFINE([LEX], [flex], ["flex" found])],
  AS_IF([test ! -f "$srcdir/src/lexer.c"],
        [AC_MSG_ERROR([required program "flex" not found])])
)
AX_PROG_BISON(
  [AC_DEFINE([YACC], [bison], ["bison" found])],
  AS_IF([test ! -f "$srcdir/src/parser.c"],
        [AC_MSG_ERROR([required program "bison" not found])])
)
AC_PROG_INSTALL
AC_PROG_LN_S

# External dependency: readline (enabled by default)
AC_ARG_WITH([readline],
  AS_HELP_STRING([--without-readline], [disable support for readline]),
  [],
  [with_readline=yes]
)
AS_IF([test x$with_readline != xno],
  [
    AC_DEFINE([WITH_READLINE], [1],
      [Define to 1 if readline support is enabled.])
    AS_IF([test x$withval != xyes],
      [
        CPPFLAGS="-I${withval}/include ${CPPFLAGS}"
        LDFLAGS="-L${withval}/lib ${LDFLAGS}"
      ]
    )
  ]
)

# Checks for libraries.

# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([curses.h ncurses.h])
AC_CHECK_HEADERS([getopt.h])
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_HEADERS([sysexits.h])
AC_CHECK_HEADERS([readline/readline.h readline/history.h])
AC_CHECK_HEADERS([term.h], [], [],
[#if defined(HAVE_CURSES_H)
#include <curses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#endif
])
gl_INIT

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_CHECK_MEMBERS([struct passwd.pw_dir],[],[],[[#include <pwd.h>]])
AX_COMPILE_CHECK_SIZEOF(void*)
cdecl_headers='#define CDECL_CONFIGURE
#include "src/c_kind.h"'
AX_COMPILE_CHECK_SIZEOF(c_kind_id_t, $cdecl_headers)
PJL_COMPILE([__typeof__],[],[__typeof__(1) x __attribute((unused)) = 1;])

# Checks for library functions.
AC_FUNC_REALLOC
PJL_COMPILE([rl_completion_func_t],[[#include <stdio.h>
                                     #include <readline/readline.h>]],
  [(void)sizeof(rl_completion_func_t);])
AC_CHECK_DECLS([rl_completion_matches],[],[],
[#include <stdio.h>
#include <readline/readline.h>
])
AC_CHECK_DECLS([RL_PROMPT_START_IGNORE],[],[],
[#include <stdio.h>
#include <readline/readline.h>
])
AC_CHECK_DECLS([rl_gnu_readline_p],[],[],
[#include <stdio.h>
#include <readline/readline.h>
])
AC_CHECK_FUNCS([geteuid getpwuid fmemopen strsep])
AC_SEARCH_LIBS([endwin],[curses ncurses])
AC_SEARCH_LIBS([readline],[readline])
AC_SEARCH_LIBS([add_history],[readline history])

# If readline wasn't disabled by the user, does it actually exist and is it a
# proper readline?
AS_IF([test x$with_readline != xno -a x$ac_cv_search_readline = xno],
  [AC_MSG_ERROR([readline library not found; use --without-readline])]
)

# Program feature: cdecl-debug (enabled by default)
AC_ARG_ENABLE([cdecl-debug],
  AS_HELP_STRING([--disable-cdecl-debug], [disable support for cdecl debugging]),
  [],
  [enable_cdecl_debug=yes]
)
AS_IF([test x$enable_cdecl_debug = xyes],
  [AC_DEFINE([ENABLE_CDECL_DEBUG], [1],
    [Define to 1 if cdecl debugging is enabled.])]
)

# Program feature: term-size (enabled by default)
AC_ARG_ENABLE([term-size],
  AS_HELP_STRING([--disable-term-size], [disable support for terminal size]),
  [],
  [enable_term_size=yes]
)
AS_IF([test x$enable_term_size = xyes -a x$ac_cv_search_endwin = xno],
  [AC_MSG_ERROR([curses library for term-size not found; use --disable-term-size])]
)
AS_IF([test x$enable_term_size = xyes],
  [AC_DEFINE([ENABLE_TERM_SIZE], [1],
    [Define to 1 if term-size is enabled.])]
)

# Program feature: Flex debug (disabled by default)
AC_ARG_ENABLE([flex-debug],
  AS_HELP_STRING([--enable-flex-debug], [enable support for Flex debugging]),
  [],
  [enable_flex_debug=no]
)
AS_IF([test x$enable_flex_debug = xyes],
  [AC_DEFINE([ENABLE_FLEX_DEBUG], [1],
    [Define to 1 if Flex debugging is enabled.])]
)

# Program feature: Bison debug (disabled by default)
AC_ARG_ENABLE([bison-debug],
  AS_HELP_STRING([--enable-bison-debug], [enable support for Bison debugging]),
  [],
  [enable_bison_debug=no]
)
AS_IF([test x$enable_bison_debug = xyes],
  [AC_DEFINE([YYDEBUG], [1],
    [Define to 1 if Bison debugging is enabled.])]
)

# Makefile conditionals.
AM_CONDITIONAL([WITH_READLINE],       [test x$with_readline      != xno])
AM_CONDITIONAL([ENABLE_CDECL_DEBUG],  [test x$enable_cdecl_debug = xyes])
AM_CONDITIONAL([ENABLE_BISON_DEBUG],  [test x$enable_bison_debug = xyes])
AM_CONDITIONAL([ENABLE_FLEX_DEBUG],   [test x$enable_flex_debug  = xyes])

# Miscellaneous.
AX_C___ATTRIBUTE__

# Compiler warnings.
AX_CFLAGS_WARN_ALL
AX_CHECK_COMPILE_FLAG([-Wcast-align], [CFLAGS="$CFLAGS -Wcast-align"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wcomma], [CFLAGS="$CFLAGS -Wcomma"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconditional-type-mismatch], [CFLAGS="$CFLAGS -Wconditional-type-mismatch"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconditional-uninitialized], [CFLAGS="$CFLAGS -Wconditional-uninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconversion], [CFLAGS="$CFLAGS -Wconversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wextra], [CFLAGS="$CFLAGS -Wextra"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wfloat-equal], [CFLAGS="$CFLAGS -Wfloat-equal"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wfor-loop-analysis], [CFLAGS="$CFLAGS -Wfor-loop-analysis"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Widiomatic-parentheses], [CFLAGS="$CFLAGS -Widiomatic-parentheses"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough], [CFLAGS="$CFLAGS -Wimplicit-fallthrough"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wlogical-op-parentheses], [CFLAGS="$CFLAGS -Wlogical-op-parentheses"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wnewline-eof], [CFLAGS="$CFLAGS -Wnewline-eof"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wredundant-decls], [CFLAGS="$CFLAGS -Wredundant-decls"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshadow], [CFLAGS="$CFLAGS -Wshadow"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshift-sign-overflow], [CFLAGS="$CFLAGS -Wshift-sign-overflow"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsign-compare], [CFLAGS="$CFLAGS -Wsign-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsign-conversion], [CFLAGS="$CFLAGS -Wsign-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsometimes-uninitialized], [CFLAGS="$CFLAGS -Wsometimes-uninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wstring-conversion], [CFLAGS="$CFLAGS -Wstring-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wuninitialized], [CFLAGS="$CFLAGS -Wuninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code-break], [CFLAGS="$CFLAGS -Wunreachable-code-break"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code], [CFLAGS="$CFLAGS -Wunreachable-code"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunused], [CFLAGS="$CFLAGS -Wunused"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wwrite-strings], [CFLAGS="$CFLAGS -Wwrite-strings"], [], [-Werror])

# Generate files.
AH_TOP([#ifndef cdecl_config_H
#define cdecl_config_H])
AH_BOTTOM([#endif /* cdecl_config_H */])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  src/Makefile
  test/Makefile
  man/Makefile
  man/man1/Makefile
])
AC_OUTPUT

# vim:set et sw=2 ts=2:
