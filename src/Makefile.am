##
#	cdecl -- C gibberish translator
#	src/Makefile.am
##

bin_PROGRAMS = cdecl

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
AM_LFLAGS = --bison-locations
AM_YFLAGS = -d
LDADD = $(top_builddir)/lib/libgnu.a

cdecl_SOURCES =	cdgram.y cdlex.l cdecl.c \
		ast.c color.c keywords.c lang.c literals.c options.c types.c \
		common.c util.c help.c set.c

if WITH_READLINE
cdecl_SOURCES += autocomplete.c
endif

# vim:set noet sw=8 ts=8:
