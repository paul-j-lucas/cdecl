##
#	cdecl -- C gibberish translator
#	src/Makefile.am
#
#	Paul J. Lucas
##

bin_PROGRAMS = cdecl

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
AM_YFLAGS = -d
LDADD = $(top_builddir)/lib/libgnu.a

cdecl_SOURCES =	parser.y lexer.l \
		ast.c \
		ast_util.c \
		cdecl.c \
		color.c \
		diagnostics.c \
		english.c \
		errors.c \
		gibberish.c \
		help.c \
		keywords.c \
		lang.c \
		literals.c \
		options.c \
		prompt.c \
		set.c \
		types.c \
		util.c

if WITH_CDECL_DEBUG
cdecl_SOURCES += debug.c
endif

if WITH_READLINE
cdecl_SOURCES += autocomplete.c
endif

# vim:set noet sw=8 ts=8:
