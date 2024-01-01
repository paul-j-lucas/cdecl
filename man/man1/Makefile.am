##
#	cdecl -- C gibberish translator
#	man/man1/Makefile.am
#
#	Copyright (C) 2017-2024  Paul J. Lucas
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

dist_man1_MANS = cdecl.1 c++decl.1

GROFF_FLAGS =	-man -Tps

PS2PDF_FLAGS =	-dEmbedAllFonts=true \
		-dHaveTrueTypes=true \
		-dPDFSETTINGS=/prepress \
		-dSubsetFonts=false

pdf-local: cdecl.pdf

cdecl.ps: cdecl.1
	groff $(GROFF_FLAGS) $< > $@

cdecl.pdf: cdecl.ps
	ps2pdf $(PS2PDF_FLAGS) $<

clean-local:
	rm -f cdecl.ps

distclean-local:
	rm -f cdecl.pdf

# vim:set noet sw=8 ts=8:
