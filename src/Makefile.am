##
#	cdecl -- C gibberish translator
#	src/Makefile.am
##

bin_PROGRAMS = cdecl

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
AM_YFLAGS = -d
LDADD = $(top_builddir)/lib/libgnu.a

cdecl_SOURCES = cdgram.y cdlex.l cdecl.c options.c util.c

# vim:set noet sw=8 ts=8:
