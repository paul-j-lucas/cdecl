##
#	cdecl -- C gibberish translator
#	src/Makefile.am
#
#	Copyright (C) 2017  Paul J. Lucas, et al.
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
		kinds.c \
		lang.c \
		literals.c \
		options.c \
		prompt.c \
		set.c \
		types.c \
		util.c

if ENABLE_CDECL_DEBUG
cdecl_SOURCES += debug.c
endif

if WITH_READLINE
cdecl_SOURCES += autocomplete.c
endif

# vim:set noet sw=8 ts=8:
