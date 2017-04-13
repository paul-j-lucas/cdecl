# Process this file with autoconf to produce a configure script.

##
#       cdecl -- C gibberish translator
#       configure.ac
#
#       Copyright (C) 2017  Paul J. Lucas, et al.
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

AC_INIT([cdecl], [3.0], [paul@lucasmail.org],,[https://github.com/paul-j-lucas/cdecl])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC_C99
gl_EARLY
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([curses.h ncurses.h])
AC_CHECK_HEADERS([getopt.h])
AC_HEADER_STDBOOL
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

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_DECLS([rl_gnu_readline_p],[],[],[#include <readline,readline.h>])
AC_CHECK_FUNCS([fmemopen strsep])
AC_SEARCH_LIBS([readline],[readline])
AC_SEARCH_LIBS([add_history],[readline history])
AC_SEARCH_LIBS([endwin],[curses ncurses])

# Program feature: debug (enabled by default)
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--disable-debug], [disable support for cdecl debugging]),
  [enable_debug=${enableval}],
  [enable_debug="yes"]
)
AS_IF([test x$enable_debug = xyes],
  [AC_DEFINE([WITH_CDECL_DEBUG], [1],
    [Define to 1 if cdecl debugging is enabled.])]
)

# Program feature: yacc debug (disabled by default)
AC_ARG_ENABLE([yydebug],
  AS_HELP_STRING([--enable-yydebug], [enable support for yacc debugging]),
  [enable_yydebug=${enableval}]
  [enable_yydebug="no"]
)
AS_IF([test x$enable_yydebug = xyes],
  [AC_DEFINE([YYDEBUG], [1],
    [Define to 1 if yacc debugging is enabled.])]
)

# Program feature: readline (enabled by default)
AC_ARG_ENABLE([readline],
  AS_HELP_STRING([--disable-readline], [disable support for readline]),
  [enable_debug=${enableval}],
  [enable_debug="yes"]
)
AS_IF([test x$enable_readline != xno -a x$ac_cv_search_readline = xno],
  [AC_MSG_ERROR([readline library not found; use --disable-readline])]
)
AS_IF([test x$enable_readline = xyes],
  [AC_DEFINE([HAVE_READLINE], [1],
    [Define to 1 if readline support is enabled.])]
)

# Program feature: term-columns (enabled by default)
AC_ARG_ENABLE([term-columns],
  AS_HELP_STRING([--disable-term-columns], [disable support for terminal columns]),
  [enable_term_columns=${enableval}],
  [enable_term_columns="yes"]
)
AS_IF([test x$enable_term_columns != xno -a x$ac_cv_search_endwin = xno],
  [AC_MSG_ERROR([curses library for term-columns not found; use --disable-term-columns])]
)
AS_IF([test x$enable_term_columns = xyes],
  [AC_DEFINE([WITH_TERM_COLUMNS], [1],
    [Define to 1 if term-columns is enabled.])]
)

# Makefile conditionals.
AM_CONDITIONAL([WITH_CDECL_DEBUG],  [test x$enable_debug    != xno])
AM_CONDITIONAL([WITH_READLINE],     [test x$enable_readline != xno])
AM_CONDITIONAL([WITH_YYDEBUG],      [test x$enable_yydebug   = xyes])

# Miscellaneous.
AC_LANG(C)
AX_CFLAGS_WARN_ALL
AX_CFLAGS_GCC_OPTION([-Wcast-align])
AX_CFLAGS_GCC_OPTION([-Wextra])
AX_CFLAGS_GCC_OPTION([-Wredundant-decls])
AX_CFLAGS_GCC_OPTION([-Wwrite-strings])

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
