##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Paul J. Lucas
##

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

# Command-line tests
TESTS =	tests/cdecl-ap.test \
	tests/cdecl-x.test

# C tests
TESTS+= tests/cast_pc.test \
	tests/cast_pcc.test \
	tests/cast_pf_ri-01.test \
	tests/cast_pf_ri-02.test \
	tests/cast_pfpci_ri.test \
	tests/cast_pfx_ri.test \
	tests/declare_ai-10.test \
	tests/declare_api-10.test \
	tests/declare_ci-01.test \
	tests/declare_ci-02.test \
	tests/declare_cpcpi.test \
	tests/declare_cppi.test \
	tests/declare_cvpi.test \
	tests/declare_fii_rv.test \
	tests/declare_fixiy_rv.test \
	tests/declare_fv_rv.test \
	tests/declare_fx_rpi.test \
	tests/declare_ll.test \
	tests/declare_pai-10.test \
	tests/declare_pc.test \
	tests/declare_pcpi.test \
	tests/declare_ppi.test \
	tests/declare_sc.test \
	tests/declare_si.test \
	tests/declare_sll.test \
	tests/declare_ti-01.test \
	tests/declare_ti-02.test \
	tests/declare_u.test \
	tests/declare_uc.test \
	tests/declare_ui.test \
	tests/declare_ull.test \
	tests/explain_ai-012.test \
	tests/explain_ai-0xA-01.test \
	tests/explain_ai-0xa-02.test \
	tests/explain_ai-10.test \
	tests/explain_api-10.test \
	tests/explain_cast_pc.test \
	tests/explain_cast_pcc-01.test \
	tests/explain_cast_pcc-02.test \
	tests/explain_cast_pf_ri.test \
	tests/explain_cast_pfpc_ri.test \
	tests/explain_cast_pfpci_ri.test \
	tests/explain_ci.test \
	tests/explain_cpi.test \
	tests/explain_cvpi.test \
	tests/explain_f_rpi.test \
	tests/explain_fpc_rpi.test \
	tests/explain_fpci_rpi.test \
	tests/explain_fv_rpi.test \
	tests/explain_fx_rpi.test \
	tests/explain_ic.test \
	tests/explain_is.test \
	tests/explain_it.test \
	tests/explain_ll.test \
	tests/explain_pai-10.test \
	tests/explain_pi.test \
	tests/explain_ppc.test \
	tests/explain_rpc.test \
	tests/explain_si.test \
	tests/explain_ti.test \
	tests/explain_vcpi.test \
	tests/explain_vpc.test \
	tests/help_c.test \
	tests/set.test
	tests/set_options.test

# C++ tests
TESTS+=	tests/cast_pmi.test \
	tests/declare_pmi.test \
	tests/declare_rci.test \
	tests/declare_rcpi.test \
	tests/declare_ri-01.test \
	tests/declare_ri-02.test \
	tests/declare_rpi.test \
	tests/explain_cast_pmi.test \
	tests/explain_pmfxx_rps.test \
	tests/explain_pmi.test \
	tests/help_cxx.test

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .test 

TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

# vim:set noet sw=8 ts=8:
