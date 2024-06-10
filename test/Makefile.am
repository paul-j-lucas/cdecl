##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Copyright (C) 2018-2024  Paul J. Lucas
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

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

###############################################################################

# Command-line tests
TESTS=	tests/cl.test \
	tests/cl-exit.test \
	tests/cl-quit.test

# Command-line option tests
TESTS+= tests/cdecl--color.test \
	tests/cdecl--commands.test \
	tests/cdecl--help.test \
	tests/cdecl--no-typedefs.test \
	tests/cdecl--options.test

# Command-line option error tests
TESTS+=	tests/cli_options-error.test

# Cast tests
TESTS+=	tests/cast.test \
	tests/cast-error.test

# Concept using tests
TESTS+=	tests/concept.test \
	tests/concept-error.test

# Declare tests
TESTS+=	tests/declare.test \
	tests/declare-error.test

# Declare operator tests
TESTS+=	tests/declare_operator.test \
	tests/declare_operator-error.test

# Declare user-defined conversion tests
TESTS+=	tests/declare_udc.test \
	tests/declare_udc-error.test

# Declare user-defined literal tests
TESTS+=	tests/declare_udl.test \
	tests/declare_udl-error.test

# Define tests
TESTS+=	tests/define.test \
	tests/define-error.test

# Explain tests
TESTS+=	tests/explain.test \
	tests/explain-error.test

# Explain cast tests
TESTS+=	tests/explain_cast.test \
	tests/explain_cast-error.test

# Explain operator tests
TESTS+=	tests/explain_operator.test \
	tests/explain_operator-error.test

# Explain user-defined conversion tests
TESTS+=	tests/explain_udc.test \
	tests/explain_udc-error.test

# Explain user-defined literal tests
TESTS+=	tests/explain_udl.test \
	tests/explain_udl-error.test

# Namespace, typedef, & using tests
TESTS+=	tests/ntu.test \
	tests/ntu-error.test

# Show tests
TESTS+=	tests/show.test \
	tests/show-error.test

# Config file tests
TESTS+=	tests/declare_config_tdu.test \
	tests/explain_config_tdu.test \
	tests/using_config_i.test

# File tests
TESTS+=	tests/file-cast_i.test \
	tests/file-declare_i.test \
	tests/file-explain_i.test

# File error tests
TESTS+=	tests/file-cast_x.test \
	tests/file-declare_x.test \
	tests/file-explain_x.test

# include tests
TESTS+= tests/include-ok-nested-01.test \
	tests/include-ok.test

# include error tests
TESTS+=	tests/include-err-01.test \
	tests/include-too_many_files.test \
	tests/include-unquoted.test

# Embedded C tests
TESTS+=	tests/xec.test \
	tests/xec-error.test

# Unified Parallel C tests
TESTS+=	tests/xupc.test \
	tests/xupc-error.test

# Miscellaneous tests
TESTS+=	tests/__declspec.test \
	tests/alt-tokens.test \
	tests/debug.test \
	tests/declare_synonyms.test \
	tests/digraphs.test \
	tests/east-const.test \
	tests/english-types.test \
	tests/explicit-ecsu.test \
	tests/explicit-int.test \
	tests/gnu_attributes.test \
	tests/help.test \
	tests/help-error.test \
	tests/infer-command.test \
	tests/integers.test \
	tests/lang.test \
	tests/prompt.test \
	tests/semicolon.test \
	tests/set.test \
	tests/set_expl.test \
	tests/set_expli.test \
	tests/set_explicit-ecsu_x.test \
	tests/set_explicit-int.test \
	tests/set_explicit-int_x.test \
	tests/set_graphs.test \
	tests/set_lang_xxx.test \
	tests/set_nodigraphs.test \
	tests/set_noexplicit-int_u.test \
	tests/set_options.test \
	tests/set_prompt_x.test \
	tests/set_xxx.test \
	tests/trigraphs.test \
	tests/using.test \
	tests/west-decl.test

# C Preprocessor tests
TESTS+=	tests/expand.test \
	tests/expand-error.test \
	tests/expand-C23_std-6.10.4.1-ex-2.test \
	tests/expand-C23_std-6.10.4.3-ex-1.test \
	tests/expand-C23_std-6.10.4.5-ex-4.test \
	tests/expand-C23_std-6.10.4.5-ex-5.test \
	tests/expand-C23_std-6.10.4.5-ex-7.test \
	tests/expand-robertelder.test \
	tests/preprocessor-error.test

# Auto-completion tests
if WITH_EXPECT
TESTS+=	tests/ac-cast_into.exp \
	tests/ac-declare.exp \
	tests/ac-explain.exp \
	tests/ac-help.exp \
	tests/ac-set.exp \
	tests/ac-show.exp
endif

###############################################################################

BUILD_SRC = $(abs_top_builddir)/src
AM_TESTS_ENVIRONMENT = BUILD_SRC=$(BUILD_SRC) ; export BUILD_SRC ;
TEST_EXTENSIONS = .exp .test

EXP_LOG_DRIVER  = $(srcdir)/run_test.sh
TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests data expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

UPDATE_TEST = $(srcdir)/update_test.sh
update:
	@ls $(TESTS) | xargs -n255 -r $(UPDATE_TEST) -s $(BUILD_SRC)

# vim:set noet sw=8 ts=8:
