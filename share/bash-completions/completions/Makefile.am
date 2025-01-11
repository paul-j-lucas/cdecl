##
#	cdecl -- C gibberish translator
#	share/bash-completions/completions/Makefile.am
#
#	Copyright (C) 2024-2025  Paul J. Lucas
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

EXTRA_DIST =	_cdecl

INSTALL_DIR =	$(DESTDIR)$(datarootdir)/bash-completions/completions

install-data-local: $(INSTALL_DIR) _cdecl
	$(INSTALL_SCRIPT) _cdecl $(INSTALL_DIR)

uninstall-local:
	rm -f $(INSTALL_DIR)/_cdecl

$(INSTALL_DIR):
	mkdir -p $@

# vim:set noet sw=8 ts=8:
