##
#	cdecl -- C gibberish translator
#	src/Makefile.am
#
#	Copyright (C) 2017-2021  Paul J. Lucas, et al.
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

bin_PROGRAMS = cdecl
check_PROGRAMS = red_black_test

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
if ENABLE_FLEX_DEBUG
AM_LFLAGS = -d
endif
AM_YFLAGS = -d
LDADD = $(top_builddir)/lib/libgnu.a

BUILT_SOURCES =	parser.c parser.h lexer.c

cdecl_SOURCES =	parser.y \
		lexer.l lexer.h \
		c_ast.c c_ast.h \
		c_ast_util.c c_ast_util.h \
		c_keyword.c c_keyword.h \
		c_kind.c c_kind.h \
		c_lang.c c_lang.h \
		c_operator.c c_operator.h \
		c_sname.c c_sname.h \
		c_type.c c_type.h \
		c_typedef.c c_typedef.h \
		cdecl.c cdecl.h \
		check.c check.h \
		color.c color.h \
		dam_lev.c dam_lev.h \
		did_you_mean.c did_you_mean.h \
		english.c english.h \
		gibberish.c gibberish.h \
		help.c \
		literals.c literals.h \
		options.c options.h \
		pjl_config.h \
		print.c print.h \
		prompt.c prompt.h \
		red_black.c red_black.h \
		set_options.c set_options.h \
		slist.c slist.h \
		strbuf.c strbuf.h \
		types.h \
		util.c util.h

if ENABLE_CDECL_DEBUG
cdecl_SOURCES += dump.c dump.h
endif

if WITH_READLINE
cdecl_SOURCES += autocomplete.c
endif

red_black_test_SOURCES = \
	pjl_config.h \
	red_black.c red_black.h \
	red_black_test.c \
	slist.c \
	util.c util.h

all-local: c++decl$(EXEEXT)

c++decl$(EXEEXT): cdecl$(EXEEXT)
	rm -f $@
	$(LN_S) $< $@

TESTS = $(check_PROGRAMS)

clean-local:
	rm -f c++decl$(EXEEXT)

parser.output: parser.y
	bison -v --report=all $<
	@rm -f parser.tab.c

install-exec-hook:
	rm -f "$(DESTDIR)$(bindir)/c++decl$(EXEEXT)"
	$(LN_S) cdecl$(EXEEXT) "$(DESTDIR)$(bindir)/c++decl$(EXEEXT)"

uninstall-hook:
	rm -f "$(DESTDIR)$(bindir)/c++decl$(EXEEXT)"

# vim:set noet sw=8 ts=8:
