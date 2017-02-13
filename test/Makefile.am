##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Paul J. Lucas
##

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

TESTS = tests/cast_pc.test \
	tests/cast_pcc-01.test \
	tests/cast_pcc-02.test \
	tests/declare_fx_rpi.test \
	tests/declare_pc.test \
	tests/explain_cast_pc.test \
	tests/explain_cast_pcc-01.test \
	tests/explain_cast_pcc-02.test \
	tests/explain_cast_pf_ri.test \
	tests/explain_cast_pfpc_ri.test \
	tests/explain_cast_pfpci_ri.test \
	tests/explain_ci.test \
	tests/explain_f_rpi.test \
	tests/explain_fpc_rpi.test \
	tests/explain_fv_rpi.test \
	tests/explain_fx_rpi.test \
	tests/explain_ic.test \
	tests/explain_pc.test \
	tests/help_c.test \
	tests/help_cxx.test \
	tests/set_options.test

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .test 

TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

# vim:set noet sw=8 ts=8:
