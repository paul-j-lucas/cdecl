##
#	cdecl -- C gibberish translator
#	Makefile.am
#
#	Copyright (C) 2017-2026  Paul J. Lucas
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

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib share src test

EXTRA_DIST =	bootstrap \
		cdecl.css \
		Doxygen.sed \
		Doxyfile \
		lcovrc \
		m4/gnulib-cache.m4 \
		m4/update_m4.sh \
		makedoc.sh \
		README-2.5.txt \
		README-blocks.txt \
		README.md

COVERAGE_INFO =	$(top_builddir)/coverage.info
COVERAGE_DIR =	coverage

DOCS_DIR = 	docs

LCOV_FLAGS =	--capture \
		--config-file "$(top_srcdir)/lcovrc" \
		--directory "$(abs_top_builddir)" \
		--exclude "*/autocomplete.c" \
		--exclude "*/color.c" \
		--exclude "*/color.h" \
		--exclude "*/lexer.c" \
		--exclude "*/parser.c" \
		--output-file $(COVERAGE_INFO)

GENHTML_FLAGS =	--legend \
		--title "$(PACKAGE_NAME)-$(PACKAGE_VERSION) Code Coverage" \
		--output-directory $(top_builddir)/$(COVERAGE_DIR)

.PHONY:	clean-coverage clean-coverage-local \
	distclean-coverage distclean-coverage-local \
	doc docs \
	update-gnulib

doc docs:
	./makedoc.sh $(PACKAGE) $(DOCS_DIR)

check-coverage:
if ENABLE_COVERAGE
	$(MAKE) $(AM_MAKEFLAGS) check
	$(LCOV) $(LCOV_FLAGS)
	$(GENHTML) $(GENHTML_FLAGS) $(COVERAGE_INFO)
	@echo "file://$(abs_builddir)/$(COVERAGE_DIR)/index.html"
else
	@echo "Code coverage not enabled; to enable:"
	@echo
	@echo "    ./configure --enable-coverage"
endif

clean-local: clean-coverage-local
	rm -fr $(DOCS_DIR)

clean-coverage-local:
	rm -f $(COVERAGE_INFO)

distclean-local: distclean-coverage-local

distclean-coverage-local:
	rm -fr $(COVERAGE_DIR)

dist-hook:
	find $(distdir) -name .DS_Store -exec rm -f {} \;

update-gnulib:
	gnulib-tool --add-import --no-vc-files

# vim:set noet sw=8 ts=8:
