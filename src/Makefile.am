##
#	cdecl -- C gibberish translator
#	src/Makefile.am
##

bin_PROGRAMS = cdecl

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
AM_YFLAGS = -d
LDADD = $(top_builddir)/lib/libgnu.a

cdecl_SOURCES =	parser.y lexer.l \
		ast.c \
		ast_check.c \
		ast_util.c \
		cdecl.c \
		color.c \
		debug.c \
		diagnostics.c \
		english.c \
		gibberish.c \
		help.c \
		keywords.c \
		lang.c \
		literals.c \
		options.c \
		set.c \
		types.c \
		util.c

if WITH_READLINE
cdecl_SOURCES += autocomplete.c
endif

# vim:set noet sw=8 ts=8:
