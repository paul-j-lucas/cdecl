##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Paul J. Lucas
##

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

###############################################################################

#
# Command-line option error tests
# ===============================
#
TESTS =	tests/cdecl-ap.test \
	tests/cdecl-x.test

###############################################################################

#
# C declare tests
# ===============
# Almost every test here has a corresponding cast_ and explain_ test.
#
TESTS+=	tests/declare_a3a5a7i.test \
	tests/declare_a3a5i.test \
	tests/declare_a3a5pfv_i.test \
	tests/declare_a3a5pfv_pi.test \
	tests/declare_a3a5pfv_ppa7a9i.test \
	tests/declare_a3a5pfv_ppa7i.test \
	tests/declare_a3a5pfv_ppa7pi.test \
	tests/declare_a3a5pfv_ppi.test \
	tests/declare_a3pfv_i.test \
	tests/declare_a3ppfv_i.test \
	tests/declare_a9cpi.test \
	tests/declare_a9i.test \
	tests/declare_a9pi.test \
	tests/declare_bii_d.test \
	tests/declare_ci.test \
	tests/declare_constant.test \
	tests/declare_cpa3cpa5i.test \
	tests/declare_cpa3pa5i.test \
	tests/declare_cpa9ci.test \
	tests/declare_cpa9i.test \
	tests/declare_cpbv_pi.test \
	tests/declare_cpcpi.test \
	tests/declare_cpfv_pi.test \
	tests/declare_cpi.test \
	tests/declare_cppi.test \
	tests/declare_cvpi.test \
	tests/declare_ea2i.test \
	tests/declare_f_pi.test \
	tests/declare_fcce_v-01.test \
	tests/declare_fcce_v-02.test \
	tests/declare_fcce_v-03.test \
	tests/declare_fg_v.test \
	tests/declare_fgi_v.test \
	tests/declare_fid_v.test \
	tests/declare_fipfi_v_pfi_v.test \
	tests/declare_fixiy_v.test \
	tests/declare_fpc_pi.test \
	tests/declare_fpci_pi.test \
	tests/declare_fpfc_i_d.test \
	tests/declare_fv_pa3a5i.test \
	tests/declare_fv_pa3a5pi.test \
	tests/declare_fv_pa3i.test \
	tests/declare_fv_pa3pi.test \
	tests/declare_fv_v.test \
	tests/declare_fx_pi.test \
	tests/declare_gi.test \
	tests/declare_lf.test \
	tests/declare_ll.test \
	tests/declare_pa3cpa5i.test \
	tests/declare_pa3pa5i.test \
	tests/declare_pa9i.test \
	tests/declare_pbv_pi.test \
	tests/declare_pcpi.test \
	tests/declare_pfv_pi.test \
	tests/declare_pfx_i.test \
	tests/declare_pi.test \
	tests/declare_ppi.test \
	tests/declare_sa2i.test \
	tests/declare_sc.test \
	tests/declare_sf_v.test \
	tests/declare_si.test \
	tests/declare_sll.test \
	tests/declare_ti.test \
	tests/declare_type.test \
	tests/declare_u.test \
	tests/declare_uc.test \
	tests/declare_ui.test \
	tests/declare_ull.test \
	tests/declare_vcpi.test \
	tests/declare_vpi.test

#
# C declare error tests
# =====================
# Almost every test here has a corresponding explain_ error test.
#
TESTS+=	tests/declare_b_ai.test \
	tests/declare_b_f_i.test \
	tests/declare_f_ai.test \
	tests/declare_f_f_i.test \
	tests/declare_fiiv_v.test \
	tests/declare_fiv_v.test \
	tests/declare_fsi_v.test \
	tests/declare_fvx_v.test \
	tests/declare_gf_v.test \
	tests/declare_hb.test \
	tests/declare_hc.test \
	tests/declare_hc16.test \
	tests/declare_hc32.test \
	tests/declare_hv.test \
	tests/declare_hw.test \
	tests/declare_lb.test \
	tests/declare_lc.test \
	tests/declare_lc16.test \
	tests/declare_lc32.test \
	tests/declare_lf-c.test \
	tests/declare_lll.test \
	tests/declare_lv.test \
	tests/declare_lw.test \
	tests/declare_sb.test \
	tests/declare_sc16.test \
	tests/declare_sc32.test \
	tests/declare_sv.test \
	tests/declare_sw.test \
	tests/declare_ub.test \
	tests/declare_uc16.test \
	tests/declare_uc32.test \
	tests/declare_uv.test \
	tests/declare_uw.test \
	tests/declare_v.test \
	tests/declare_vf_v-c.test

###############################################################################

#
# C explain tests
# ===============
# Almost every test here has a corresponding cast_ and declare_ test.
#
TESTS+=	tests/explain_a012i.test \
	tests/explain_a0xAi-01.test \
	tests/explain_a0xai-02.test \
	tests/explain_a3a5a7i.test \
	tests/explain_a3a5i.test \
	tests/explain_a3a5pfv_i.test \
	tests/explain_a3a5pfv_pi.test \
	tests/explain_a3a5pfv_ppa7a9i.test \
	tests/explain_a3a5pfv_ppa7i.test \
	tests/explain_a3a5pfv_ppa7pi.test \
	tests/explain_a3a5pfv_ppi.test \
	tests/explain_a3pfv_i.test \
	tests/explain_a3ppfv_i.test \
	tests/explain_a9cpi.test \
	tests/explain_a9i.test \
	tests/explain_a9pi.test \
	tests/explain_bii_d.test \
	tests/explain_ci.test \
	tests/explain_ck.test \
	tests/explain_cpa3cpa5i.test \
	tests/explain_cpa3pa5i.test \
	tests/explain_cpa9ci.test \
	tests/explain_cpa9i.test \
	tests/explain_cpbv_pi.test \
	tests/explain_cpcpi.test \
	tests/explain_cpfv_pi.test \
	tests/explain_cpi.test \
	tests/explain_cppi.test \
	tests/explain_cs.test \
	tests/explain_cvpi.test \
	tests/explain_ea2i.test \
	tests/explain_f_pi.test \
	tests/explain_fcce_v.test \
	tests/explain_fg_v.test \
	tests/explain_fgi_v.test \
	tests/explain_fid_v.test \
	tests/explain_fipfi_v_pfi_v.test \
	tests/explain_fixiy_v.test \
	tests/explain_fpc_pi.test \
	tests/explain_fpci_pi.test \
	tests/explain_fpfc_i_d.test \
	tests/explain_fv_pa3a5i.test \
	tests/explain_fv_pa3a5pi.test \
	tests/explain_fv_pa3i.test \
	tests/explain_fv_pa3pi.test \
	tests/explain_fv_pi.test \
	tests/explain_fv_v.test \
	tests/explain_fx_pi.test \
	tests/explain_gi.test \
	tests/explain_ic.test \
	tests/explain_ig.test \
	tests/explain_is.test \
	tests/explain_it.test \
	tests/explain_kc.test \
	tests/explain_keywords.test \
	tests/explain_lf.test \
	tests/explain_ll.test \
	tests/explain_pa3a5i.test \
	tests/explain_pa3cpa5i.test \
	tests/explain_pa3pa5i.test \
	tests/explain_pa9i.test \
	tests/explain_pbv_pi.test \
	tests/explain_pcpi.test \
	tests/explain_pfv_pi.test \
	tests/explain_pi.test \
	tests/explain_ppi.test \
	tests/explain_rpi.test \
	tests/explain_sa2i.test \
	tests/explain_sc.test \
	tests/explain_sf_v.test \
	tests/explain_si.test \
	tests/explain_ti.test \
	tests/explain_uc.test \
	tests/explain_ull.test \
	tests/explain_vcpi.test \
	tests/explain_vf_v.test \
	tests/explain_vpi.test

#
# C explain error tests
# =====================
# Almost every test here has a corresponding declare_ error test.
#
TESTS+=	tests/explain_b_ai.test \
	tests/explain_f_ai.test \
	tests/explain_fcce_v.test \
	tests/explain_fe_v.test \
	tests/explain_fecc_v.test \
	tests/explain_fee_v.test \
	tests/explain_fev_v.test \
	tests/explain_fiiv_v.test \
	tests/explain_fiv_v.test \
	tests/explain_fsi_v.test \
	tests/explain_fve_v.test \
	tests/explain_fvi_v.test \
	tests/explain_fvv_v.test \
	tests/explain_fvx_v.test \
	tests/explain_gf_v.test \
	tests/explain_gk.test \
	tests/explain_hb.test \
	tests/explain_hc.test \
	tests/explain_hc16.test \
	tests/explain_hc32.test \
	tests/explain_hv.test \
	tests/explain_hw.test \
	tests/explain_lb.test \
	tests/explain_lc.test \
	tests/explain_lc16.test \
	tests/explain_lc32.test \
	tests/explain_lf-c.test \
	tests/explain_lll.test \
	tests/explain_lv.test \
	tests/explain_lw.test \
	tests/explain_pmi-c.test \
	tests/explain_ri-c.test \
	tests/explain_sb.test \
	tests/explain_sc16.test \
	tests/explain_sc32.test \
	tests/explain_sv.test \
	tests/explain_sw.test \
	tests/explain_ub.test \
	tests/explain_uc16.test \
	tests/explain_uc32.test \
	tests/explain_uv.test \
	tests/explain_uw.test \
	tests/explain_v.test \
	tests/explain_vf_v-c.test

###############################################################################

#
# C cast tests
# ============
# Almost every test here has a corresponding declare_ and explain_ test.
#
TESTS+=	tests/cast_bii_d.test \
	tests/cast_ci.test \
	tests/cast_constant.test \
	tests/cast_cpa3cpa5i.test \
	tests/cast_cpa3pa5i.test \
	tests/cast_cpa9ci.test \
	tests/cast_cpa9i.test \
	tests/cast_cpbv_pi.test \
	tests/cast_cpcpi.test \
	tests/cast_cpfv_pi.test \
	tests/cast_cppi.test \
	tests/cast_cvpi.test \
	tests/cast_ll.test \
	tests/cast_pa3cpa5i.test \
	tests/cast_pa3pa5i.test \
	tests/cast_pa9i.test \
	tests/cast_pci.test \
	tests/cast_pcpi.test \
	tests/cast_pf_i-01.test \
	tests/cast_pf_i-02.test \
	tests/cast_pfii_i.test \
	tests/cast_pfv_pi.test \
	tests/cast_pfx_i.test \
	tests/cast_pi.test \
	tests/cast_ppi.test \
	tests/cast_sc.test \
	tests/cast_si.test \
	tests/cast_sll.test \
	tests/cast_u.test \
	tests/cast_uc.test \
	tests/cast_ui.test \
	tests/cast_ull.test \
	tests/cast_vcpi.test \
	tests/cast_vpi.test

#
# C cast error tests
# ==================
#
TESTS+=	tests/cast_a3i.test \
	tests/cast_f_i.test \
	tests/cast_ti.test \
	tests/cast_type.test

###############################################################################

#
# C explain cast tests
# ====================
# Almost every test here has a corresponding cast_ test.
#
TESTS+=	tests/explain_cast_ci.test \
	tests/explain_cast_cpa3cpa5i.test \
	tests/explain_cast_cpa3pa5i.test \
	tests/explain_cast_cpa9ci.test \
	tests/explain_cast_cpa9i.test \
	tests/explain_cast_cpbv_pi.test \
	tests/explain_cast_cpcpi.test \
	tests/explain_cast_cpfv_pi.test \
	tests/explain_cast_cpi.test \
	tests/explain_cast_cppi.test \
	tests/explain_cast_cvpi.test \
	tests/explain_cast_ic.test \
	tests/explain_cast_ll.test \
	tests/explain_cast_pa3a5i.test \
	tests/explain_cast_pa3cpa5i.test \
	tests/explain_cast_pa3pa5i.test \
	tests/explain_cast_pa9i.test \
	tests/explain_cast_pci.test \
	tests/explain_cast_pcpi.test \
	tests/explain_cast_pf_i.test \
	tests/explain_cast_pfpci_i.test \
	tests/explain_cast_pfpi_i.test \
	tests/explain_cast_pfv_pi.test \
	tests/explain_cast_pi.test \
	tests/explain_cast_pic.test \
	tests/explain_cast_ppi.test \
	tests/explain_cast_rpi.test \
	tests/explain_cast_ull.test \
	tests/explain_cast_vcpi.test \
	tests/explain_cast_vpi.test

#
# C explain cast error tests
# ==========================
#
TESTS+=	tests/explain_cast_a3a5a7i.test \
	tests/explain_cast_a3a5i.test \
	tests/explain_cast_a3i.test \
	tests/explain_cast_ai.test \
	tests/explain_cast_ei.test \
	tests/explain_cast_gi.test \
	tests/explain_cast_lll.test \
	tests/explain_cast_si.test \
	tests/explain_cast_ti.test

###############################################################################

#
# C++ declare tests
# =================
#
TESTS+=	tests/declare_cpmi.test \
	tests/declare_frri_v.test \
	tests/declare_pfv_ri.test \
	tests/declare_pmci.test \
	tests/declare_pmf_i.test \
	tests/declare_pmf_pc.test \
	tests/declare_pmi.test \
	tests/declare_rci.test \
	tests/declare_rcpi.test \
	tests/declare_ref.test \
	tests/declare_ri.test \
	tests/declare_rpi.test \
	tests/declare_vf_v.test

#
# C++ declare error tests
# =======================
#
TESTS+=	tests/declare_pmri.test \
	tests/declare_pri.test \
	tests/declare_rgi.test \
	tests/declare_rv.test

#
# C++ explain tests
# =================
#
TESTS+=	tests/explain_frri_v.test \
	tests/explain_pmf_i.test \
	tests/explain_pmf_pc.test \
	tests/explain_pmfii_pk.test \
	tests/explain_pmgi.test \
	tests/explain_pmi.test \
	tests/explain_ri.test

#
# C++ explain error tests
# =======================
#
TESTS+=	tests/explain_frri_v-c++03.test \
	tests/explain_fx_v.test \
	tests/explain_pmri.test \
	tests/explain_pri.test \
	tests/explain_prri.test \
	tests/explain_rgi.test \
	tests/explain_rv.test

#
# C++ cast tests
# ==============
#
TESTS+=	tests/cast_pmi.test

#
# C++ explain cast tests
# ======================
#
TESTS+=	tests/explain_cast_pmi.test

###############################################################################

#
# Command-line tests
# ==================
#
TESTS+=	tests/cl-cast_i.test \
	tests/cl-declare_i.test \
	tests/cl-explain_i.test

###############################################################################

#
# File tests
# ==========
#
TESTS+=	tests/file-cast_i.test \
	tests/file-declare_i.test \
	tests/file-explain_i.test

#
# File error tests
# ================
#
TESTS+=	tests/file-cast_x.test \
	tests/file-declare_x.test \
	tests/file-explain_x.test

###############################################################################

#
# Miscellaneous tests
# ===================
#
TESTS+=	tests/help-c.test \
	tests/help-c++.test \
	tests/set.test \
	tests/set_c++.test \
	tests/set_c++03.test \
	tests/set_c++11.test \
	tests/set_c++98.test \
	tests/set_c.test \
	tests/set_c11.test \
	tests/set_c89.test \
	tests/set_c99.test \
	tests/set_cknr.test \
	tests/set_knr.test \
	tests/set_knrc.test \
	tests/set_noprompt.test \
	tests/set_nosemicolon.test \
	tests/set_options.test \
	tests/set_prompt.test \
	tests/set_semicolon.test

if WITH_CDECL_DEBUG
TESTS+=	tests/set_debug.test \
	tests/set_nodebug.test
endif

###############################################################################

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .test 

TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

# vim:set noet sw=8 ts=8:
