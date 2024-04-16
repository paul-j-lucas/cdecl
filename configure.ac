# Process this file with autoconf to produce a configure script.

##
#       cdecl -- C gibberish translator
#       configure.ac
#
#       Copyright (C) 2017-2024  Paul J. Lucas
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

AC_PREREQ([2.69])
AC_INIT([cdecl],[16.3],[https://github.com/paul-j-lucas/cdecl/issues],[],[https://github.com/paul-j-lucas/cdecl])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_EXTRA_RECURSIVE_TARGETS([clean-coverage distclean-coverage])

# Testing feature: code coverage (disabled by default)
AC_ARG_ENABLE([coverage],
  AS_HELP_STRING([--enable-coverage], [enable code coverage]),
  [],
  [enable_coverage=no]
)
AS_IF([test "x$enable_coverage" = xyes],
  #
  # When code coverage is desired, we have to set CFLAGS so it doesn't include
  # -O2 (so we can add -O0 later in src/Makefile.am) before AC_PROG_CC is
  # called as directed by "Autconf 5.10.3: C Compiler Characteristics":
  #
  #     If output variable CFLAGS was not already set, set it to -g -O2 for the
  #     GNU C compiler ....  If your package does not like this default, then
  #     it is acceptable to insert the line ': ${CFLAGS=""}' after AC_INIT and
  #     before AC_PROG_CC to select an empty default instead.
  #
  # There's still more to do after AC_PROG_CC is called and sets $GCC.  This is
  # handed below in part 2.
  #
  [: ${CFLAGS=""}]
)

# Checks for programs.
AC_LANG(C)
AC_PROG_CC
AS_IF([test "x$ac_cv_prog_cc_c11" = xno], [
  AC_MSG_ERROR([a C11 compiler is required to compile $PACKAGE_NAME])
])
gl_EARLY

# Check for flex & bison only if lexer.c & parser.c aren't present. If present,
# it likely means a released version was downloaded so flex & bison shouldn't
# be required.
flex_min_version="2.5.30"
bison_min_version="3.4.2"
AS_IF([test ! -f "$srcdir/src/lexer.c"], [
  #
  # AX_PROG_FLEX calls AM_PROG_LEX that in turn calls AC_PROG_LEX without
  # arguments.  However, AC_PROG_LEX now wants an argument of either yywrap
  # or noyywrap and warns if omitted.  To work around this, call AC_PROG_LEX
  # with an argument ourselves first.
  #
  # Unfortunately, we can't just stop there because AX_PROG_FLEX calls
  # AC_REQUIRE([AM_PROG_LEX]) and would generate an "expanded before it was
  # required" warning.  To work around this, call AM_PROG_LEX ourselves too.
  #
  AC_PROG_LEX([noyywrap])
  AM_PROG_LEX

  AX_PROG_FLEX([FLEX=$LEX],
    [AC_MSG_ERROR([required program "flex" not found])])
  AX_PROG_FLEX_VERSION([$flex_min_version],[],
    [AC_MSG_ERROR([flex version $flex_min_version or later required])])
])
AS_IF([test ! -f "$srcdir/src/parser.c"], [
  AX_PROG_BISON([BISON=$YACC],
    [AC_MSG_ERROR([required program "bison" not found])])
  AX_PROG_BISON_VERSION([$bison_min_version],[],
    [AC_MSG_ERROR([bison version $bison_min_version or later required])])
])

AC_PROG_INSTALL
AC_PROG_LN_S

# External dependency: readline (enabled by default)
AC_ARG_WITH([readline],
  AS_HELP_STRING([--without-readline], [disable readline]),
  [],
  [with_readline=yes]
)
AS_IF([test "x$with_readline" != xno], [
  AC_DEFINE([WITH_READLINE], [1], [Define to 1 if readline support is enabled.])
  AS_IF([test "x$withval" != xyes], [
    CPPFLAGS="-I${withval}/include ${CPPFLAGS}"
    LDFLAGS="-L${withval}/lib ${LDFLAGS}"
  ])
])

# Program feature: Bison debug (disabled by default)
AC_ARG_ENABLE([bison-debug],
  AS_HELP_STRING([--enable-bison-debug], [enable Bison debug output]),
  [],
  [enable_bison_debug=no]
)
AS_IF([test "x$enable_bison_debug" = xyes], [
  AC_DEFINE([ENABLE_BISON_DEBUG], [1], [Define to 1 if Bison debugging is enabled.])
  AC_DEFINE([YYDEBUG], [1], [Define to 1 if Bison debugging is enabled.])
])

# Program feature: Flex debug (disabled by default)
AC_ARG_ENABLE([flex-debug],
  AS_HELP_STRING([--enable-flex-debug], [enable Flex debug output]),
  [],
  [enable_flex_debug=no]
)
AS_IF([test "x$enable_flex_debug" = xyes], [
  AC_DEFINE([ENABLE_FLEX_DEBUG], [1],
    [Define to 1 if Flex debugging is enabled.])
])

# Program feature: term-size (enabled by default)
AC_ARG_ENABLE([term-size],
  AS_HELP_STRING([--disable-term-size], [disable terminal size]),
  [],
  [enable_term_size=yes]
)
AS_IF([test "x$enable_term_size" = xyes], [
  AC_DEFINE([ENABLE_TERM_SIZE], [1], [Define to 1 if term-size is enabled.])
])

# Checks for libraries.

# Checks for header files.
AS_IF([test "x$with_readline" != xno], [
  AC_CHECK_HEADERS([readline/readline.h readline/history.h], [],
    [AC_MSG_ERROR([readline.h header not found; use --without-readline])]
  )
])
AS_IF([test "x$enable_term_size" = xyes], [
  AC_CHECK_HEADERS([curses.h ncurses.h], [],
    [AC_MSG_ERROR([curses library for term-size not found; use --disable-term-size])]
  )
  AC_CHECK_HEADERS([term.h], [], [],
  [#if defined(HAVE_CURSES_H)
  #include <curses.h>
  #elif defined(HAVE_NCURSES_H)
  #include <ncurses.h>
  #endif
  ])
])
AC_CHECK_HEADERS([fnmatch.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_HEADERS([sysexits.h])
AC_HEADER_ASSERT
AC_HEADER_STDBOOL
gl_INIT

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_MEMBERS([struct passwd.pw_dir],[],[],[[#include <pwd.h>]])
PJL_COMPILE([__builtin_expect],[], [(void)__builtin_expect(1,1);])
PJL_COMPILE([__typeof__],[],[__typeof__(1) x __attribute__((unused)) = 1;])

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([geteuid getpwuid fmemopen strsep])
AS_IF([test "x$with_readline" != xno], [
  AC_CHECK_DECLS([rl_completion_matches],[],[],
  [#include <stdio.h>
  #include <readline/readline.h>
  ])
  AC_CHECK_DECLS([rl_ding],[],[],
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
  AC_SEARCH_LIBS([readline],[readline], [],
    [AC_MSG_ERROR([readline library not found; use --without-readline])]
  )
  AC_SEARCH_LIBS([add_history],[readline history])
])
AS_IF([test "x$enable_term_size" = xyes], [
  AC_SEARCH_LIBS([endwin],[curses ncurses], [],
    [AC_MSG_ERROR([curses library for term-size not found; use --disable-term-size])]
  )
  AC_SEARCH_LIBS([tigetnum],[curses ncurses tinfo], [],
    [AC_MSG_ERROR([curses library for term-size not found; use --disable-term-size])]
  )
])

# Testing feature: Address Sanitizer (ASan)
AC_ARG_ENABLE([asan],
  AS_HELP_STRING([--enable-asan],
    [enable Address Sanitizer for debugging]),
  [],
  [enable_asan=no]
)
AS_IF([test "x$enable_asan" = xyes], [
  AC_DEFINE([ENABLE_ASAN], [1], [Define to 1 if Address Sanitizer is enabled.])
])

# Testing feature: Memory Sanitizer (MSan)
AC_ARG_ENABLE([msan],
  AS_HELP_STRING([--enable-msan],
    [enable Memory Sanitizer for debugging]),
  [],
  [enable_msan=no]
)
AS_IF([test "x$enable_msan" = xyes], [
  AC_DEFINE([ENABLE_MSAN], [1],
    [Define to 1 if Memory Sanitizer is enabled.])
])

# Testing feature: Undefined Behavior Sanitizer (UBSan)
AC_ARG_ENABLE([ubsan],
  AS_HELP_STRING([--enable-ubsan],
    [enable Undefined Bahavior Sanitizer for debugging]),
  [],
  [enable_ubsan=no]
)
AS_IF([test "x$enable_ubsan" = xyes], [
  AC_DEFINE([ENABLE_UBSAN], [1],
    [Define to 1 if Undefined Bahavior Sanitizer is enabled.])
])

# Testing feature: code coverage, part 2
AS_IF([test "x$enable_coverage" = xyes], [
  AS_IF([test "x$GCC" != xyes], [
    AC_MSG_ERROR([gcc is required for code coverage])
  ])
  AC_CHECK_TOOL([GCOV], [gcov], [gcov])
  AS_IF([test "x$GCOV" = "x:"], [
    AC_MSG_ERROR([required program "gcov" for code coverage not found])
  ])
  AC_CHECK_PROG([LCOV], [lcov], [lcov])
  AS_IF([test "x$LCOV" = x], [
    AC_MSG_ERROR([required program "lcov" for code coverage not found])
  ])
  AC_CHECK_PROG([GENHTML], [genhtml], [genhtml])
  AS_IF([test "x$GENHTML" = x], [
    AC_MSG_ERROR([required program "genhtml" for code coverage not found])
  ])
  AC_DEFINE([ENABLE_COVERAGE], [1], [Define to 1 if code coverage is enabled.])
])

# Testing program: expect
AS_IF([test "x$with_readline" != xno], [
  AC_CHECK_PROG([EXPECT], [expect], [expect])
  AS_IF([test "x$EXPECT" = x], [
    AC_MSG_WARN([optionl program "expect" for testing autocompletion not found])
  ])
])

# Makefile conditionals.
AM_CONDITIONAL([ENABLE_ASAN],         [test "x$enable_asan"         = xyes])
AM_CONDITIONAL([ENABLE_COVERAGE],     [test "x$enable_coverage"     = xyes])
AM_CONDITIONAL([ENABLE_BISON_DEBUG],  [test "x$enable_bison_debug"  = xyes])
AM_CONDITIONAL([ENABLE_FLEX_DEBUG],   [test "x$enable_flex_debug"   = xyes])
AM_CONDITIONAL([ENABLE_MSAN],         [test "x$enable_msan"         = xyes])
AM_CONDITIONAL([ENABLE_UBSAN],        [test "x$enable_ubsan"        = xyes])
AM_CONDITIONAL([WITH_EXPECT],         [test "x$EXPECT"             != x   ])
AM_CONDITIONAL([WITH_READLINE],       [test "x$with_readline"      != xno ])

# Miscellaneous.
AX_C___ATTRIBUTE__

# Compiler warnings.
AC_SUBST([CDECL_CFLAGS])
CDECL_CFLAGS="-D_ISOC11_SOURCE"
AX_CFLAGS_WARN_ALL([CDECL_CFLAGS])
AX_CHECK_COMPILE_FLAG([-Wcast-align], [CDECL_CFLAGS="$CDECL_CFLAGS -Wcast-align"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wcast-function-type], [CDECL_CFLAGS="$CDECL_CFLAGS -Wcast-function-type"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wcomma], [CDECL_CFLAGS="$CDECL_CFLAGS -Wcomma"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconditional-type-mismatch], [CDECL_CFLAGS="$CDECL_CFLAGS -Wconditional-type-mismatch"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconditional-uninitialized], [CDECL_CFLAGS="$CDECL_CFLAGS -Wconditional-uninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconversion], [CDECL_CFLAGS="$CDECL_CFLAGS -Wconversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wduplicate-enum], [CDECL_CFLAGS="$CDECL_CFLAGS -Wduplicate-enum"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wembedded-directive], [CDECL_CFLAGS="$CDECL_CFLAGS -Wembedded-directive"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wenum-enum-conversion], [CDECL_CFLAGS="$CDECL_CFLAGS -Wenum-enum-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wenum-float-conversion], [CDECL_CFLAGS="$CDECL_CFLAGS -Wenum-float-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wextra], [CDECL_CFLAGS="$CDECL_CFLAGS -Wextra"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wfloat-equal], [CDECL_CFLAGS="$CDECL_CFLAGS -Wfloat-equal"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wfor-loop-analysis], [CDECL_CFLAGS="$CDECL_CFLAGS -Wfor-loop-analysis"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat-nonliteral], [CDECL_CFLAGS="$CDECL_CFLAGS -Wformat-nonliteral"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat-signedness], [CDECL_CFLAGS="$CDECL_CFLAGS -Wformat-signedness"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat-type-confusion], [CDECL_CFLAGS="$CDECL_CFLAGS -Wformat-type-confusion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat=2], [CDECL_CFLAGS="$CDECL_CFLAGS -Wformat=2"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Widiomatic-parentheses], [CDECL_CFLAGS="$CDECL_CFLAGS -Widiomatic-parentheses"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough], [CDECL_CFLAGS="$CDECL_CFLAGS -Wimplicit-fallthrough"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wlogical-op-parentheses], [CDECL_CFLAGS="$CDECL_CFLAGS -Wlogical-op-parentheses"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wmisleading-indentation], [CDECL_CFLAGS="$CDECL_CFLAGS -Wmisleading-indentation"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wnewline-eof], [CDECL_CFLAGS="$CDECL_CFLAGS -Wnewline-eof"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wpedantic], [CDECL_CFLAGS="$CDECL_CFLAGS -Wpedantic"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wredundant-decls], [CDECL_CFLAGS="$CDECL_CFLAGS -Wredundant-decls"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wreserved-identifier], [CDECL_CFLAGS="$CDECL_CFLAGS -Wreserved-identifier"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshadow], [CDECL_CFLAGS="$CDECL_CFLAGS -Wshadow"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshift-sign-overflow], [CDECL_CFLAGS="$CDECL_CFLAGS -Wshift-sign-overflow"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshorten-64-to-32], [CDECL_CFLAGS="$CDECL_CFLAGS -Wshorten-64-to-32"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsign-compare], [CDECL_CFLAGS="$CDECL_CFLAGS -Wsign-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsign-conversion], [CDECL_CFLAGS="$CDECL_CFLAGS -Wsign-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsometimes-uninitialized], [CDECL_CFLAGS="$CDECL_CFLAGS -Wsometimes-uninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wstring-conversion], [CDECL_CFLAGS="$CDECL_CFLAGS -Wstring-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wtautological-compare], [CDECL_CFLAGS="$CDECL_CFLAGS -Wtautological-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wtautological-type-limit-compare], [CDECL_CFLAGS="$CDECL_CFLAGS -Wtautological-type-limit-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wundef], [CDECL_CFLAGS="$CDECL_CFLAGS -Wundef"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wuninitialized], [CDECL_CFLAGS="$CDECL_CFLAGS -Wuninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code-break], [CDECL_CFLAGS="$CDECL_CFLAGS -Wunreachable-code-break"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code-return], [CDECL_CFLAGS="$CDECL_CFLAGS -Wunreachable-code-return"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code], [CDECL_CFLAGS="$CDECL_CFLAGS -Wunreachable-code"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunused], [CDECL_CFLAGS="$CDECL_CFLAGS -Wunused"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wwrite-strings], [CDECL_CFLAGS="$CDECL_CFLAGS -Wwrite-strings"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wzero-as-null-pointer-constant], [CDECL_CFLAGS="$CDECL_CFLAGS -Wzero-as-null-pointer-constant"], [], [-Werror])

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
