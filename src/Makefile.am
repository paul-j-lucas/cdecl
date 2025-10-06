##
#	cdecl -- C gibberish translator
#	src/Makefile.am
#
#	Copyright (C) 2017-2025  Paul J. Lucas
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

# Suppress printing of "--- target ---" when -j is given to BSD's make.
.MAKE.JOB.PREFIX=

bin_PROGRAMS =	cdecl
check_PROGRAMS=	red_black_test slist_test strbuf_test util_test

AM_CFLAGS =	$(CDECL_CFLAGS)

if ENABLE_ASAN
AM_CFLAGS +=	-fsanitize=address -fno-omit-frame-pointer
endif

if ENABLE_MSAN
AM_CFLAGS +=	-fsanitize=memory -fno-omit-frame-pointer
endif

if ENABLE_UBSAN
AM_CFLAGS +=	-fsanitize=undefined
endif

if ENABLE_COVERAGE
AM_CFLAGS +=	--coverage -g -O0
AM_LDFLAGS =	--coverage
endif

AM_CPPFLAGS =	-I$(top_srcdir)/lib -I$(top_builddir)/lib
if ENABLE_FLEX_DEBUG
AM_LFLAGS =	-d
endif
AM_YFLAGS =	-d -Wno-yacc
LDADD =		$(top_builddir)/lib/libgnu.a

BUILT_SOURCES =	parser.c parser.h lexer.c

cdecl_SOURCES =	parser.y \
		lexer.l lexer.h \
		c_ast.c c_ast.h \
		c_ast_check.c c_ast_check.h \
		c_ast_util.c c_ast_util.h \
		c_ast_warn.c c_ast_warn.h \
		c_keyword.c c_keyword.h \
		c_kind.c c_kind.h \
		c_lang.c c_lang.h \
		c_operator.c c_operator.h \
		c_sglob.c c_sglob.h \
		c_sname.c c_sname.h \
		c_type.c c_type.h \
		c_typedef.c c_typedef.h \
		cdecl.c cdecl.h \
		cdecl_command.c cdecl_command.h \
		cdecl_dym.c cdecl_dym.h \
		cdecl_keyword.c cdecl_keyword.h \
		cdecl_parser.h \
		cdecl_term.c cdecl_term.h \
		cli_options.c cli_options.h \
		color.c color.h \
		config_file.c config_file.h \
		dam_lev.c dam_lev.h \
		did_you_mean.c did_you_mean.h \
		dump.c dump.h \
		english.c english.h \
		gibberish.c gibberish.h \
		help.c help.h \
		literals.c literals.h \
		options.c options.h \
		p_keyword.c p_keyword.h \
		p_kind.c p_kind.h \
		p_macro.c p_macro.h \
		p_predefine.c \
		p_token.c p_token.h \
		parse.c parse.h \
		pjl_config.h config.h \
		print.c print.h \
		prompt.c prompt.h \
		read_line.c read_line.h \
		red_black.c red_black.h \
		set_options.c set_options.h \
		show.c show.h \
		slist.c slist.h \
		strbuf.c strbuf.h \
		type_traits.h \
		types.h \
		util.c util.h

if WITH_READLINE
cdecl_SOURCES += autocomplete.c autocomplete.h
endif

BASE_TEST_SOURCES = \
		cdecl.h \
		pjl_config.h config.h \
		type_traits.h \
		util.c util.h \
		unit_test.c unit_test.h

red_black_test_SOURCES = $(BASE_TEST_SOURCES) \
		red_black.c red_black.h \
		red_black_test.c

slist_test_SOURCES = $(BASE_TEST_SOURCES) \
		slist.c slist.h \
		slist_test.c

strbuf_test_SOURCES  = $(BASE_TEST_SOURCES) \
		strbuf.c strbuf.h \
		strbuf_test.c

util_test_SOURCES = $(BASE_TEST_SOURCES) \
		util_test.c

CDECL =		cdecl$(EXEEXT)
CPPDECL =	c++decl$(EXEEXT)

EXPLAIN =	explain$(EXEEXT)
SYMLINKS =	$(CPPDECL) $(EXPLAIN)

INSTALL_CPPDECL= $(DESTDIR)$(bindir)/$(CPPDECL)
TESTS =		$(check_PROGRAMS)

all-local: $(SYMLINKS)

$(SYMLINKS): $(CDECL)
	rm -f $@
	$(LN_S) $(CDECL) $@

check-local: clean-coverage-local

clean-local:
	rm -f $(BUILT_SOURCES) $(SYMLINKS) parser.output

distclean-local: distclean-coverage-local

clean-coverage-local:
	rm -f *.gcda

distclean-coverage-local: clean-coverage-local
	rm -f *.gcno

parser.output: parser.y
	bison -v --report=all $<
	@rm -f parser.tab.c

install-exec-hook:
	rm -f $(TARGET_CPPDECL)
	$(LN_S) $(CDECL) $(INSTALL_CPPDECL)

uninstall-hook:
	rm -f $(INSTALL_CPPDECL)

# vim:set noet sw=8 ts=8:
