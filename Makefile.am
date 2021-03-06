##
#	cdecl -- C gibberish translator
#	Makefile.am
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

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib src man test

EXTRA_DIST =	Doxyfile \
		m4/gnulib-cache.m4 \
		README-2.5.txt \
		README-blocks.txt \
		README.md

.PHONY: doc docs

doc docs:
	doxygen

clean-local:
	rm -rf docs

update-gnulib:
	gnulib-tool --add-import
	rm -f m4/.gitignore

# vim:set noet sw=8 ts=8:
