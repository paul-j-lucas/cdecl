##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Copyright (C) 2018-2019  Paul J. Lucas, et al.
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

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

###############################################################################

#
# Command-line option tests
# =========================
#
TESTS =	tests/cdecl--no-semicolon.test \
	tests/cdecl--no-typedefs.test \
	tests/cdecl-s.test \
	tests/cdecl-t.test

if ENABLE_CDECL_DEBUG
TESTS+=	tests/cdecl-d.test \
	tests/cdecl--debug.test
endif

#
# Command-line option error tests
# ===============================
#
TESTS+=	tests/cdecl-23.test \
	tests/cdecl-xx.test

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
	tests/declare_ai-01.test \
	tests/declare_ai-02.test \
	tests/declare_ai.test \
	tests/declare_api-01.test \
	tests/declare_b.test \
	tests/declare_bii_d.test \
	tests/declare_c.test \
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
	tests/declare_cu.test \
	tests/declare_cvpi.test \
	tests/declare_cz.test \
	tests/declare_e.test \
	tests/declare_ea2i.test \
	tests/declare_f.test \
	tests/declare_f_pi.test \
	tests/declare_facs3_v.test \
	tests/declare_facv_v.test \
	tests/declare_fas3_v.test \
	tests/declare_fasc3_v.test \
	tests/declare_fav_v.test \
	tests/declare_fcce_v-01.test \
	tests/declare_fcce_v-02.test \
	tests/declare_fcce_v-03.test \
	tests/declare_ff_v.test \
	tests/declare_fg_v-c89.test \
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
	tests/declare_if_v.test \
	tests/declare_jf_i.test \
	tests/declare_k.test \
	tests/declare_ld.test \
	tests/declare_lf.test \
	tests/declare_ll.test \
	tests/declare_md.test \
	tests/declare_mf.test \
	tests/declare_n.test \
	tests/declare_nfi_v-01.test \
	tests/declare_nfi_v-02.test \
	tests/declare_nfi_v-03.test \
	tests/declare_pa3cpa5i.test \
	tests/declare_pa3pa5i.test \
	tests/declare_pa9i.test \
	tests/declare_pai-01.test \
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
	tests/declare_sif_v.test \
	tests/declare_sll.test \
	tests/declare_ti.test \
	tests/declare_tpfc_i.test \
	tests/declare_type.test \
	tests/declare_u.test \
	tests/declare_uc.test \
	tests/declare_ui.test \
	tests/declare_ull.test \
	tests/declare_vcpi.test \
	tests/declare_vpi.test \
	tests/declare_w.test \
	tests/declare_z.test \
	tests/declare_zd.test \
	tests/declare_zf.test \
	tests/declare_zz.test

#
# C declare error tests
# =====================
# Almost every test here has a corresponding explain_ error test.
#
TESTS+=	tests/declare_0f_v-c.test \
	tests/declare_=i-c.test \
	tests/declare_ac3i.test \
	tests/declare_af.test \
	tests/declare_as3i.test \
	tests/declare_avi.test \
	tests/declare_b-c89.test \
	tests/declare_b_ai.test \
	tests/declare_b_f_i.test \
	tests/declare_c-c.test \
	tests/declare_c16-c++03.test \
	tests/declare_c16-c99.test \
	tests/declare_c32-c++03.test \
	tests/declare_c32-c99.test \
	tests/declare_c8-c++17.test \
	tests/declare_cf_v-c.test \
	tests/declare_ci-knr.test \
	tests/declare_cnmf_v.test \
	tests/declare_cri.test \
	tests/declare_e-knr.test \
	tests/declare_ec-c.test \
	tests/declare_esf_v.test \
	tests/declare_f_ai.test \
	tests/declare_f_f_i.test \
	tests/declare_ff_v-c.test \
	tests/declare_fg_v.test \
	tests/declare_fiiv_v.test \
	tests/declare_fiv_v.test \
	tests/declare_fmf_v.test \
	tests/declare_fn.test \
	tests/declare_fsi_v.test \
	tests/declare_fvx_v.test \
	tests/declare_gf_v.test \
	tests/declare_hb.test \
	tests/declare_hc.test \
	tests/declare_hc16.test \
	tests/declare_hc32.test \
	tests/declare_hc8.test \
	tests/declare_hd.test \
	tests/declare_hf.test \
	tests/declare_hl.test \
	tests/declare_hll.test \
	tests/declare_hv.test \
	tests/declare_hw.test \
	tests/declare_ii.test \
	tests/declare_ji.test \
	tests/declare_lb.test \
	tests/declare_lc.test \
	tests/declare_lc16.test \
	tests/declare_lc32.test \
	tests/declare_lc8.test \
	tests/declare_ld-knr.test \
	tests/declare_lf-c.test \
	tests/declare_ll-c89.test \
	tests/declare_ll-knr.test \
	tests/declare_lll.test \
	tests/declare_lv.test \
	tests/declare_lw.test \
	tests/declare_ni.test \
	tests/declare_pk.test \
	tests/declare_pv-knr.test \
	tests/declare_rf_v-c.test \
	tests/declare_sb.test \
	tests/declare_sc-knr.test \
	tests/declare_sc16.test \
	tests/declare_sc32.test \
	tests/declare_sc8.test \
	tests/declare_si-knr.test \
	tests/declare_sl-knr.test \
	tests/declare_sv.test \
	tests/declare_sw.test \
	tests/declare_t.test \
	tests/declare_ub.test \
	tests/declare_uc16.test \
	tests/declare_uc32.test \
	tests/declare_uc8.test \
	tests/declare_ud.test \
	tests/declare_uf.test \
	tests/declare_uv.test \
	tests/declare_uw.test \
	tests/declare_uz.test \
	tests/declare_v.test \
	tests/declare_vf_v-c.test \
	tests/declare_vi-knr.test \
	tests/declare_vri.test \
	tests/declare_w-c89.test \
	tests/declare_xi-c.test

###############################################################################

#
# C explain tests
# ===============
# Almost every test here has a corresponding cast_ and declare_ test.
#
TESTS+=	tests/explain_0f_v-01.test \
	tests/explain_0o_minus2_i_rk.test \
	tests/explain_0o_minus_rk_rk.test \
	tests/explain_0o_plus2_i_rk.test \
	tests/explain_0o_plus_rk_rk.test \
	tests/explain_a012i.test \
	tests/explain_a0b10i.test \
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
	tests/explain_ai-01.test \
	tests/explain_ai-02.test \
	tests/explain_ai.test \
	tests/explain_api-01.test \
	tests/explain_api-02.test \
	tests/explain_b.test \
	tests/explain_bii_d.test \
	tests/explain_c.test \
	tests/explain_ci.test \
	tests/explain_ck.test \
	tests/explain_co_minus2_i_rk.test \
	tests/explain_co_minus_rk_rk.test \
	tests/explain_co_plus2_i_rk.test \
	tests/explain_co_plus_rk_rk.test \
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
	tests/explain_cu.test \
	tests/explain_cvpi.test \
	tests/explain_e.test \
	tests/explain_ea2i.test \
	tests/explain_f_pi.test \
	tests/explain_fac_v.test \
	tests/explain_facs3_v.test \
	tests/explain_facv_v.test \
	tests/explain_fas3_v.test \
	tests/explain_fasc3_v.test \
	tests/explain_fav_v.test \
	tests/explain_fcce_v.test \
	tests/explain_ff_v.test \
	tests/explain_fg_v-c89.test \
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
	tests/explain_if_v.test \
	tests/explain_ig.test \
	tests/explain_is.test \
	tests/explain_it.test \
	tests/explain_kc.test \
	tests/explain_keywords.test \
	tests/explain_ld.test \
	tests/explain_lf.test \
	tests/explain_ll.test \
	tests/explain_md.test \
	tests/explain_mf.test \
	tests/explain_n.test \
	tests/explain_nfi_c.test \
	tests/explain_nfi_v.test \
	tests/explain_ni.test \
	tests/explain_oi.test \
	tests/explain_oo_minus2_i_rk.test \
	tests/explain_oo_minus_rk_rk.test \
	tests/explain_oo_plus2_i_rk.test \
	tests/explain_oo_plus_rk_rk.test \
	tests/explain_pa3a5i.test \
	tests/explain_pa3cpa5i.test \
	tests/explain_pa3pa5i.test \
	tests/explain_pa9i.test \
	tests/explain_pai-01.test \
	tests/explain_pai-02.test \
	tests/explain_pbv_pi.test \
	tests/explain_pcpi.test \
	tests/explain_pfv_pi.test \
	tests/explain_pi.test \
	tests/explain_ppi.test \
	tests/explain_ro_minus2_i_rk.test \
	tests/explain_ro_minus_rk_rk.test \
	tests/explain_ro_plus2_i_rk.test \
	tests/explain_ro_plus_rk_rk.test \
	tests/explain_rpi.test \
	tests/explain_rro_minus2_i_rk.test \
	tests/explain_rro_minus_rk_rk.test \
	tests/explain_rro_plus2_i_rk.test \
	tests/explain_rro_plus_rk_rk.test \
	tests/explain_sa2i.test \
	tests/explain_sc.test \
	tests/explain_sf_v.test \
	tests/explain_si.test \
	tests/explain_ti.test \
	tests/explain_tpfc_i.test \
	tests/explain_uc.test \
	tests/explain_ull.test \
	tests/explain_vcpi.test \
	tests/explain_vf_v.test \
	tests/explain_vo_minus2_i_rk.test \
	tests/explain_vo_minus_rk_rk.test \
	tests/explain_vo_plus2_i_rk.test \
	tests/explain_vo_plus_rk_rk.test \
	tests/explain_vpi.test \
	tests/explain_w.test \
	tests/explain_yo_minus2_i_rk.test \
	tests/explain_yo_minus_rk_rk.test \
	tests/explain_yo_plus2_i_rk.test \
	tests/explain_yo_plus_rk_rk.test \
	tests/explain_z.test \
	tests/explain_zd.test \
	tests/explain_zf.test \
	tests/explain_zz.test

#
# C explain error tests
# =====================
# Almost every test here has a corresponding declare_ error test.
#
TESTS+=	tests/explain_+i-c.test \
	tests/explain_0f_v-02.test \
	tests/explain_0f_v-c.test \
	tests/explain_ac9i.test \
	tests/explain_af.test \
	tests/explain_as9i.test \
	tests/explain_b-c89.test \
	tests/explain_b_ai.test \
	tests/explain_c-c.test \
	tests/explain_c16-c++03.test \
	tests/explain_c16-c99.test \
	tests/explain_c32-c++03.test \
	tests/explain_c32-c99.test \
	tests/explain_c8-c++17.test \
	tests/explain_cf_v-c.test \
	tests/explain_ci-knr.test \
	tests/explain_e-knr.test \
	tests/explain_esf_v.test \
	tests/explain_f_ai.test \
	tests/explain_fasv_v.test \
	tests/explain_fcce_v.test \
	tests/explain_fe_v.test \
	tests/explain_fecc_v.test \
	tests/explain_fee_v.test \
	tests/explain_fev_v.test \
	tests/explain_ff_v-c.test \
	tests/explain_fg_v.test \
	tests/explain_fiiv_v.test \
	tests/explain_fiv_v.test \
	tests/explain_fn.test \
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
	tests/explain_hc8.test \
	tests/explain_hd.test \
	tests/explain_hf.test \
	tests/explain_hl.test \
	tests/explain_hll.test \
	tests/explain_hv.test \
	tests/explain_hw.test \
	tests/explain_ii.test \
	tests/explain_k.test \
	tests/explain_lb.test \
	tests/explain_lc.test \
	tests/explain_lc16.test \
	tests/explain_lc32.test \
	tests/explain_lc8.test \
	tests/explain_ld-knr.test \
	tests/explain_lf-c.test \
	tests/explain_ll-c89.test \
	tests/explain_ll-knr.test \
	tests/explain_lll.test \
	tests/explain_lv.test \
	tests/explain_lw.test \
	tests/explain_pmi-c.test \
	tests/explain_pv-knr.test \
	tests/explain_rf_v-01.test \
	tests/explain_ri-c.test \
	tests/explain_sb.test \
	tests/explain_sc-knr.test \
	tests/explain_sc16.test \
	tests/explain_sc32.test \
	tests/explain_sc8.test \
	tests/explain_si-knr.test \
	tests/explain_sl-knr.test \
	tests/explain_sv.test \
	tests/explain_sw.test \
	tests/explain_type_i.test \
	tests/explain_ub.test \
	tests/explain_uc16.test \
	tests/explain_uc32.test \
	tests/explain_uc8.test \
	tests/explain_ud.test \
	tests/explain_uf.test \
	tests/explain_using_pfi_v-c.test \
	tests/explain_uv.test \
	tests/explain_uw.test \
	tests/explain_v.test \
	tests/explain_vf_v-c.test \
	tests/explain_vi-knr.test \
	tests/explain_voyf-c.test \
	tests/explain_w-c89.test \
	tests/explain_xi-c.test

#
# C declare digraph & trigraph tests
# ==================================
#
TESTS+=	tests/declare_digraphs_a9i.test \
	tests/declare_digraphs_nfi_v-c++.test \
	tests/declare_trigraphs_a9i.test \
	tests/declare_trigraphs_nfi_v-c++.test \
	tests/declare_trigraphs_o_circ_eq_i_rk.test \
	tests/declare_trigraphs_o_circ_rcki_k.test \
	tests/declare_trigraphs_o_pipe2_2rck_b.test \
	tests/declare_trigraphs_o_pipe_2rck_k.test \
	tests/declare_trigraphs_o_pipe_eq_i_rk.test \
	tests/declare_trigraphs_o_tilde_k.test

#
# C explain digraph & trigraph tests
# ==================================
#
TESTS+=	tests/explain_digraphs_a9i.test \
	tests/explain_digraphs_nfi_v-c++.test \
	tests/explain_trigraphs_a9i.test \
	tests/explain_trigraphs_nfi_v-c++.test \
	tests/explain_trigraphs_o_circ_eq_i_rk.test \
	tests/explain_trigraphs_o_circ_rcki_k.test \
	tests/explain_trigraphs_o_pipe2_2rck_b.test \
	tests/explain_trigraphs_o_pipe_2rck_k.test \
	tests/explain_trigraphs_o_pipe_eq_i_rk.test \
	tests/explain_trigraphs_o_tilde_k.test

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
	tests/cast_type.test \
	tests/const_cast_pi-c.test \
	tests/dynamic_cast_pc-c.test \
	tests/reinterpret_cast_pv-c.test \
	tests/static_cast_i-c.test

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
	tests/explain_cast_ri.test \
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
	tests/explain_cast_ti.test \
	tests/explain_const_cast_pi-c.test \
	tests/explain_dynamic_cast_pc-c.test \
	tests/explain_reinterpret_cast_pv-c.test \
	tests/explain_static_cast_i-c.test

###############################################################################

#
# C show tests
# ============
#
TESTS+=	tests/show_all.test \
	tests/show_all_typedef.test \
	tests/show_int8_t.test \
	tests/show_int8_t_typedef.test \
	tests/show_predefined.test \
	tests/show_predefined_typedef.test \
	tests/show_user.test \
	tests/show_user_typedef.test

#
# C show error tests
# ==================
#
TESTS+=	tests/show_foo.test

###############################################################################

#
# C++ namespace, typedef, & using tests
# =====================================
#
TESTS+=	tests/2c_typedef_i.test \
	tests/namespace_+typedef_i.test \
	tests/namespace_c_typedef_i.test \
	tests/namespace_k_typedef_i.test \
	tests/namespace_n_typedef_i.test \
	tests/namespace_namespace_typedef_i-c++14.test \
	tests/namespace_namespace_typedef_i.test \
	tests/namespace_typedef_i.test \
	tests/namespace_using_i.test \
	tests/typedef_fi_v_show_f.test \
	tests/typedef_i.test \
	tests/typedef_i_declare_i.test \
	tests/typedef_i_explain_i.test \
	tests/typedef_i_int_least32_t.test \
	tests/using_fi_v_show_f.test \
	tests/using_i.test \
	tests/using_i_declare_i.test \
	tests/using_i_explain_i.test \
	tests/using_i_int_least32_t.test

#
# C++ namespace, typedef & using error tests
# ==========================================
#
TESTS+=	tests/k_namespace_typedef_i.test \
	tests/namespace_+typedef_i-c++14.test \
	tests/typedef_ai.test \
	tests/typedef_c_int_least32_t.test \
	tests/typedef_ei.test \
	tests/typedef_gi.test \
	tests/typedef_si.test \
	tests/using_ai.test \
	tests/using_c_int_least32_t.test \
	tests/using_ei.test \
	tests/using_gi.test \
	tests/using_i-c.test \
	tests/using_si.test

#
# C++ typedef & using error tests
# ===============================
#
TESTS+=	tests/typedef_i+.test

###############################################################################

#
# C define tests
# ==============
#
TESTS+=	tests/define_i.test \
	tests/define_i_declare_i.test \
	tests/define_i_explain_i.test \
	tests/define_i_int_least32_t.test \
	tests/define_k_declare_k.test

#
# C define error tests
# ====================
#
TESTS+=	tests/define_ai.test \
	tests/define_c_int_least32_t.test \
	tests/define_ei.test \
	tests/define_gi.test \
	tests/define_si.test

#
# C++ define tests
# ================
#
TESTS+=	tests/define_2=_i.test \
	tests/define_=_i.test \
	tests/define_=c_i.test \
	tests/define_k_define_k_explain_k+k.test \
	tests/define_k_explain_pmki.test

#
# C++ define error tests
# ======================
#
TESTS+=	tests/define_namespace_=c_i.test \
	tests/define_=sc_i.test

###############################################################################

#
# C config file tests
# ===================
#
TESTS+=	tests/declare_config_u.test \
	tests/explain_config_u.test \
	tests/using_config_i.test

###############################################################################

#
# C++ declare tests
# =================
#
TESTS+=	tests/declare_+f_v.test \
	tests/declare_0f_v.test \
	tests/declare_2+f_v.test \
	tests/declare_=i.test \
	tests/declare_a.test \
	tests/declare_af-c++.test \
	tests/declare_cdfcdi_i.test \
	tests/declare_cdfi_i.test \
	tests/declare_cf_v.test \
	tests/declare_cpmi.test \
	tests/declare_crf_v.test \
	tests/declare_crrf_v.test \
	tests/declare_df_v.test \
	tests/declare_dui.test \
	tests/declare_ec.test \
	tests/declare_ek.test \
	tests/declare_f+k_v.test \
	tests/declare_f_pmi.test \
	tests/declare_fcdi_i.test \
	tests/declare_fdi_v.test \
	tests/declare_frri_v.test \
	tests/declare_fti_v-01.test \
	tests/declare_fti_v-02.test \
	tests/declare_fui_v-01.test \
	tests/declare_fui_v-02.test \
	tests/declare_mi.test \
	tests/declare_nfi_v-02-c++.test \
	tests/declare_nfi_v-03-c++.test \
	tests/declare_oyf.test \
	tests/declare_pfv_ri.test \
	tests/declare_pmci.test \
	tests/declare_pmf_i.test \
	tests/declare_pmf_pc.test \
	tests/declare_pmi.test \
	tests/declare_ra3i.test \
	tests/declare_rci.test \
	tests/declare_rcpi.test \
	tests/declare_ref.test \
	tests/declare_rf_v.test \
	tests/declare_ri.test \
	tests/declare_rpi.test \
	tests/declare_rrf_v.test \
	tests/declare_sxi.test \
	tests/declare_tf_i-01.test \
	tests/declare_tf_i-02.test \
	tests/declare_uf_v-01.test \
	tests/declare_uf_v-02.test \
	tests/declare_ui-01.test \
	tests/declare_ui-02.test \
	tests/declare_vf_v.test \
	tests/declare_voyf.test \
	tests/declare_xf.test \
	tests/declare_xi.test \
	tests/declare_yf.test

#
# C++ declare error tests
# =======================
#
TESTS+=	tests/declare_ai-c++11.test \
	tests/declare_cdi.test \
	tests/declare_fcf.test \
	tests/declare_fco_exclam_b.test \
	tests/declare_nfi_v-01.test \
	tests/declare_pmri.test \
	tests/declare_pri.test \
	tests/declare_rf_v-01.test \
	tests/declare_rgi.test \
	tests/declare_rv.test \
	tests/declare_smi.test

#
# C++ declare member operator tests
# =================================
#
TESTS+=	tests/declare_mo_amper2_rck_b.test \
	tests/declare_mo_amper_eq_i_rk.test \
	tests/declare_mo_amper_i_rk.test \
	tests/declare_mo_amper_pk.test \
	tests/declare_mo_arrow_pk.test \
	tests/declare_mo_arrow_star_i_rk.test \
	tests/declare_mo_brackets_i_i.test \
	tests/declare_mo_circ_eq_i_rk.test \
	tests/declare_mo_comma_i_rk.test \
	tests/declare_mo_exclam_b.test \
	tests/declare_mo_exclam_eq_i_b.test \
	tests/declare_mo_greater2_eq_i_rk.test \
	tests/declare_mo_greater2_i_k.test \
	tests/declare_mo_greater_eq_rcki_b.test \
	tests/declare_mo_less2_eq_i_rk.test \
	tests/declare_mo_less2_i_k.test \
	tests/declare_mo_less_eq_rcki_b.test \
	tests/declare_mo_minus_eq_i_rk.test \
	tests/declare_mo_minus_i_k.test \
	tests/declare_mo_percent_eq_i_rk.test \
	tests/declare_mo_percent_i_k.test \
	tests/declare_mo_pipe2_rck_b.test \
	tests/declare_mo_pipe_eq_i_rk.test \
	tests/declare_mo_pipe_i_rk.test \
	tests/declare_mo_plus_eq_i_rk.test \
	tests/declare_mo_plus_i_k.test \
	tests/declare_mo_slash_eq_i_rk.test \
	tests/declare_mo_star_eq_i_rk.test \
	tests/declare_mo_star_i_rk.test \
	tests/declare_mo_star_rk.test \
	tests/declare_mo_tilde_k.test

#
# C++ declare non-member operator tests
# =====================================
#
TESTS+=	tests/declare_nmo_amper2_2rck_b.test \
	tests/declare_nmo_amper_eq_rki_rk.test \
	tests/declare_nmo_amper_rcki_k.test \
	tests/declare_nmo_amper_rk_pk.test \
	tests/declare_nmo_arrow_star_rki_rk.test \
	tests/declare_nmo_circ_rcki_k.test \
	tests/declare_nmo_comma_rki_rk.test \
	tests/declare_nmo_exclam_eq_ki_b.test \
	tests/declare_nmo_exclam_rck_b.test \
	tests/declare_nmo_greater2_rcki_k.test \
	tests/declare_nmo_greater_eq_rcki_b.test \
	tests/declare_nmo_greater_rcki_b.test \
	tests/declare_nmo_less2_eq_rki_rk.test \
	tests/declare_nmo_less2_rcki_k.test \
	tests/declare_nmo_less_eq_rcki_b.test \
	tests/declare_nmo_less_rcki_b.test \
	tests/declare_nmo_minus2_rki_rk.test \
	tests/declare_nmo_minus_eq_rki_rk.test \
	tests/declare_nmo_minus_rcki_k.test \
	tests/declare_nmo_percent_eq_rki_rk.test \
	tests/declare_nmo_percent_rcki_k.test \
	tests/declare_nmo_pipe2_2rck_b.test \
	tests/declare_nmo_pipe_eq_rki_rk.test \
	tests/declare_nmo_plus2_rki_rk.test \
	tests/declare_nmo_plus_eq_rki_rk.test \
	tests/declare_nmo_plus_rcki_k.test \
	tests/declare_nmo_slash_eq_rki_rk.test \
	tests/declare_nmo_slash_rcki_k.test \
	tests/declare_nmo_star_eq_rki_rk.test \
	tests/declare_nmo_star_rck_k.test \
	tests/declare_nmo_star_rcki_k.test \
	tests/declare_nmo_tilde_rck_k.test

#
# C++ declare unspecified operator tests
# ======================================
#
TESTS+=	tests/declare_o2=_exclam_b.test \
	tests/declare_o_amper2_2rck_b.test \
	tests/declare_o_amper2_rck_b.test \
	tests/declare_o_amper_2rck_k.test \
	tests/declare_o_amper_eq_i_rk.test \
	tests/declare_o_amper_eq_rki_rk.test \
	tests/declare_o_amper_i_rk.test \
	tests/declare_o_amper_pk.test \
	tests/declare_o_amper_rcki_k.test \
	tests/declare_o_arrow_star_i_rk.test \
	tests/declare_o_arrow_star_rki_rk.test \
	tests/declare_o_circ_eq_i_rk.test \
	tests/declare_o_comma_rki_rk.test \
	tests/declare_o_eq2_i_b.test \
	tests/declare_o_eq2_ki_b.test \
	tests/declare_o_exclam_b.test \
	tests/declare_o_exclam_eq_i_b.test \
	tests/declare_o_exclam_eq_ki_b.test \
	tests/declare_o_exclam_rck_b.test \
	tests/declare_o_greater2_eq_i_rk.test \
	tests/declare_o_greater2_i_k.test \
	tests/declare_o_greater2_rcki_k.test \
	tests/declare_o_greater_eq_rcki_b.test \
	tests/declare_o_less2_eq_i_rk.test \
	tests/declare_o_less2_i_k.test \
	tests/declare_o_less2_rcki_k.test \
	tests/declare_o_less_eq_greater_2rck_a.test \
	tests/declare_o_less_eq_rcki_b.test \
	tests/declare_o_minus_i_k.test \
	tests/declare_o_parens_2i_v.test \
	tests/declare_o_parens_3i_v.test \
	tests/declare_o_parens_i_v.test \
	tests/declare_o_parens_v.test \
	tests/declare_o_percent_eq_i_rk.test \
	tests/declare_o_percent_eq_rki_rk.test \
	tests/declare_o_percent_i_k.test \
	tests/declare_o_percent_rcki_k.test \
	tests/declare_o_pipe2_2rck_b.test \
	tests/declare_o_pipe2_rck_b.test \
	tests/declare_o_pipe_eq_i_rk.test \
	tests/declare_o_pipe_eq_rki_rk.test \
	tests/declare_o_pipe_i_rk.test \
	tests/declare_o_plus_i_k.test \
	tests/declare_o_slash_eq_i_rk.test \
	tests/declare_o_star_eq_i_rk.test \
	tests/declare_o_star_eq_rki_rk.test \
	tests/declare_o_star_i_rk.test \
	tests/declare_o_star_rck_k.test \
	tests/declare_o_star_rcki_k.test \
	tests/declare_o_star_rk.test \
	tests/declare_o_tilde_k.test \
	tests/declare_o_tilde_rck_k.test

#
# C++ declare operator error tests
# ================================
#
TESTS+=	tests/declare_co_exclam_k_b.test \
	tests/declare_fmo_star_rk_rk.test \
	tests/declare_mo_arrow_i.test \
	tests/declare_nmo_amper_i_i.test \
	tests/declare_nmo_arrow_rk_pk.test \
	tests/declare_nmo_brackets_rcki_v.test \
	tests/declare_nmo_eq_rki_rk.test \
	tests/declare_nmo_parens_rck_v.test \
	tests/declare_o_arrow_i.test \
	tests/declare_o_arrow_rk_pk.test \
	tests/declare_o_brackets_2i_i.test \
	tests/declare_o_colon2_rk_rk.test \
	tests/declare_o_dot_k.test \
	tests/declare_o_dot_star_rk_rk.test \
	tests/declare_o_exclam_2i.test \
	tests/declare_o_less_eq_greater_2rck_a-c++17.test \
	tests/declare_o_minus2_d_rk.test \
	tests/declare_o_minus2_rkd_rk.test \
	tests/declare_o_minus_ie_i.test \
	tests/declare_o_oexclam_k_b.test \
	tests/declare_o_plus2_d_rk.test \
	tests/declare_o_plus2_rkd_rk.test \
	tests/declare_o_plus_ie_i.test \
	tests/declare_o_qmark_colon_k2i_i.test \
	tests/declare_ro_exclam_k_b.test \
	tests/declare_rro_exclam_k_b.test \
	tests/declare_vo_exclam_k_b.test \
	tests/declare_yo_exclam_k_b.test

#
# C++ declare user-defined literal tests
# ======================================
#
TESTS+=	tests/declare_+udl_c_i.test

#
# C++ explain tests
# =================
#
TESTS+=	tests/explain_a.test \
	tests/explain_cdfcdi_i.test \
	tests/explain_cdfi_i.test \
	tests/explain_cf_v.test \
	tests/explain_cpmci.test \
	tests/explain_cpmi.test \
	tests/explain_crf_v.test \
	tests/explain_cvf_v.test \
	tests/explain_df_v.test \
	tests/explain_di.test \
	tests/explain_dui-01.test \
	tests/explain_dui-02.test \
	tests/explain_f__pf__v.test \
	tests/explain_f__pv.test \
	tests/explain_f__v.test \
	tests/explain_f_cpmi.test \
	tests/explain_f_pmi.test \
	tests/explain_fcdi_i.test \
	tests/explain_fdi_v.test \
	tests/explain_frri_v.test \
	tests/explain_fti_v.test \
	tests/explain_fui_v.test \
	tests/explain_i+.test \
	tests/explain_i2+.test \
	tests/explain_ki+.test \
	tests/explain_mi.test \
	tests/explain_nfi_v-c++.test \
	tests/explain_oyf.test \
	tests/explain_pmf_i.test \
	tests/explain_pmf_pc.test \
	tests/explain_pmfii_pk.test \
	tests/explain_pmgi.test \
	tests/explain_pmi.test \
	tests/explain_ra3i.test \
	tests/explain_rf_v.test \
	tests/explain_ri.test \
	tests/explain_rrf_v.test \
	tests/explain_sxi.test \
	tests/explain_tf_v.test \
	tests/explain_uf_v.test \
	tests/explain_ui.test \
	tests/explain_using_a3i.test \
	tests/explain_using_pfi_v.test \
	tests/explain_voyf.test \
	tests/explain_xf_v.test \
	tests/explain_xff_v.test \
	tests/explain_xi.test \
	tests/explain_xtf_v.test \
	tests/explain_yf.test

#
# C++ explain error tests
# =======================
#
TESTS+=	tests/explain_ai-c++11.test \
	tests/explain_cdi.test \
	tests/explain_ec.test \
	tests/explain_ek.test \
	tests/explain_f+_v.test \
	tests/explain_f__v-c++03.test \
	tests/explain_f_v__v.test \
	tests/explain_fa_v.test \
	tests/explain_fac3_v-c++.test \
	tests/explain_fas3_v-c++.test \
	tests/explain_fav_v-c++.test \
	tests/explain_frri_v-c++03.test \
	tests/explain_fsf_v.test \
	tests/explain_fx_v.test \
	tests/explain_nfi_v-c++-error.test \
	tests/explain_pmri.test \
	tests/explain_pri.test \
	tests/explain_prri.test \
	tests/explain_rf_v-c++03.test \
	tests/explain_rgi.test \
	tests/explain_rrf_v-c++03.test \
	tests/explain_rv.test \
	tests/explain_smi.test \
	tests/explain_srf_v.test \
	tests/explain_srrf_v.test \
	tests/explain_unknown_attribute.test

#
# C++ explain operator tests
# ==========================
#
TESTS+=	tests/explain_+o_eq2_rcki_b.test \
	tests/explain_+o_eq_rck_rk.test \
	tests/explain_0o_amper_rk_pk.test \
	tests/explain_0o_star_rk_rk.test \
	tests/explain_co_amper_rk_pk.test \
	tests/explain_co_star_rk_rk.test \
	tests/explain_co_tilde_k.test \
	tests/explain_o_amper2_2rck_b.test \
	tests/explain_o_amper2_rck_b.test \
	tests/explain_o_amper_2rck_k.test \
	tests/explain_o_amper_eq_i_rk.test \
	tests/explain_o_amper_eq_rki_rk.test \
	tests/explain_o_amper_i_i.test \
	tests/explain_o_amper_i_rk.test \
	tests/explain_o_amper_pk.test \
	tests/explain_o_amper_rcki_k.test \
	tests/explain_o_amper_rk_pk.test \
	tests/explain_o_arrow_pk.test \
	tests/explain_o_arrow_star_i_rk.test \
	tests/explain_o_arrow_star_rki_rk.test \
	tests/explain_o_brackets_i_i.test \
	tests/explain_o_circ_eq_i_rk.test \
	tests/explain_o_circ_eq_rki_rk.test \
	tests/explain_o_circ_rcki_k.test \
	tests/explain_o_comma_i_rk.test \
	tests/explain_o_comma_rcki_k.test \
	tests/explain_o_comma_rki_rk.test \
	tests/explain_o_eq2_i_b.test \
	tests/explain_o_eq2_ki_b.test \
	tests/explain_o_exclam_b.test \
	tests/explain_o_exclam_eq_i_b.test \
	tests/explain_o_exclam_eq_ki_b.test \
	tests/explain_o_exclam_rck_b.test \
	tests/explain_o_greater2_eq_i_rk.test \
	tests/explain_o_greater2_eq_rki_rk.test \
	tests/explain_o_greater2_i_k.test \
	tests/explain_o_greater2_i_rk.test \
	tests/explain_o_greater2_rcki_k.test \
	tests/explain_o_greater2_rki_rk.test \
	tests/explain_o_greater_eq_rcki_b.test \
	tests/explain_o_greater_rcki_b.test \
	tests/explain_o_less2_eq_i_rk.test \
	tests/explain_o_less2_eq_rki_rk.test \
	tests/explain_o_less2_i_k.test \
	tests/explain_o_less2_i_rk.test \
	tests/explain_o_less2_rcki_k.test \
	tests/explain_o_less2_rki_rk.test \
	tests/explain_o_less_eq_rcki_b.test \
	tests/explain_o_less_rcki_b.test \
	tests/explain_o_minus2_rki_rk.test \
	tests/explain_o_minus_2rck_k.test \
	tests/explain_o_minus_eq_i_rk.test \
	tests/explain_o_minus_eq_rki_rk.test \
	tests/explain_o_minus_i_k.test \
	tests/explain_o_minus_rcki_k.test \
	tests/explain_o_parens_2i_v.test \
	tests/explain_o_parens_3i_v.test \
	tests/explain_o_parens_i_v.test \
	tests/explain_o_parens_v.test \
	tests/explain_o_percent_2rck_k.test \
	tests/explain_o_percent_eq_i_rk.test \
	tests/explain_o_percent_eq_rki_rk.test \
	tests/explain_o_percent_i_k.test \
	tests/explain_o_percent_rcki_k.test \
	tests/explain_o_pipe2_2rck_b.test \
	tests/explain_o_pipe2_rck_b.test \
	tests/explain_o_pipe_2rck_k.test \
	tests/explain_o_pipe_eq_i_rk.test \
	tests/explain_o_pipe_eq_rki_rk.test \
	tests/explain_o_pipe_i_rk.test \
	tests/explain_o_plus2_rki_rk.test \
	tests/explain_o_plus_2rck_k.test \
	tests/explain_o_plus_eq_i_rk.test \
	tests/explain_o_plus_eq_rki_rk.test \
	tests/explain_o_plus_i_k.test \
	tests/explain_o_plus_rcki_k.test \
	tests/explain_o_slash_2rck_k.test \
	tests/explain_o_slash_eq_i_rk.test \
	tests/explain_o_slash_eq_rki_rk.test \
	tests/explain_o_slash_rcki_k.test \
	tests/explain_o_star_2rck_k.test \
	tests/explain_o_star_eq_i_rk.test \
	tests/explain_o_star_eq_rki_rk.test \
	tests/explain_o_star_i_rk.test \
	tests/explain_o_star_rck_k.test \
	tests/explain_o_star_rcki_k.test \
	tests/explain_o_star_rk.test \
	tests/explain_o_tilde_k.test \
	tests/explain_o_tilde_rck_k.test \
	tests/explain_oo_amper_rk_pk.test \
	tests/explain_oo_star_rk_rk.test \
	tests/explain_ro_amper_rk_pk.test \
	tests/explain_ro_star_rk_rk.test \
	tests/explain_rro_amper_rk_pk.test \
	tests/explain_rro_star_rk_rk.test \
	tests/explain_vo_amper_rk_pk.test \
	tests/explain_vo_star_rk_rk.test \
	tests/explain_yo_amper_rk_pk.test \
	tests/explain_yo_star_rk_rk.test

#
# C++ explain operator error tests
# ================================
#
TESTS+=	tests/explain_co_exclam_k_b.test \
	tests/explain_o_arrow_i.test \
	tests/explain_o_brackets_2i_i.test \
	tests/explain_o_exclam_2i.test \
	tests/explain_o_minus2_d_rk.test \
	tests/explain_o_minus2_rkd_rk.test \
	tests/explain_o_minus_ie_i.test \
	tests/explain_o_oexclam_k_b.test \
	tests/explain_o_plus2_d_rk.test \
	tests/explain_o_plus2_rkd_rk.test \
	tests/explain_o_plus_ie_i.test \
	tests/explain_o_qmark_colon_k2i_i.test \
	tests/explain_ro_exclam_k_b.test \
	tests/explain_rro_exclam_k_b.test \
	tests/explain_vo_exclam_k_b.test \
	tests/explain_yo_exclam_k_b.test

#
# C++ cast tests
# ==============
#
TESTS+=	tests/cast_+i.test \
	tests/cast_pmi.test \
	tests/const_cast_+pi.test \
	tests/const_cast_pi.test \
	tests/dynamic_cast_+pc.test \
	tests/dynamic_cast_pc.test \
	tests/reinterpret_cast_+pv.test \
	tests/reinterpret_cast_pv.test \
	tests/static_cast_+i.test \
	tests/static_cast_i.test

#
# C++ explain cast tests
# ======================
#
TESTS+=	tests/explain_cast_pmi.test \
	tests/explain_cast_=pmi.test \
	tests/explain_cast_pmc=i.test \
	tests/explain_const_cast_+pi.test \
	tests/explain_const_cast_pi.test \
	tests/explain_dynamic_cast_p+c+.test \
	tests/explain_dynamic_cast_pc.test \
	tests/explain_reinterpret_cast_pv.test \
	tests/explain_static_cast_i+.test \
	tests/explain_static_cast_i.test

#
# C++ explain user-defined literal tests
# ======================================
#
TESTS+=	tests/explain_+udl_c_i.test \
	tests/explain_udl_c16_i.test \
	tests/explain_udl_c32_i.test \
	tests/explain_udl_c8_i.test \
	tests/explain_udl_c_i.test \
	tests/explain_udl_ld_i.test \
	tests/explain_udl_pcc16ul_i.test \
	tests/explain_udl_pcc16z_i.test \
	tests/explain_udl_pcc32ul_i.test \
	tests/explain_udl_pcc32z_i.test \
	tests/explain_udl_pcc8ul_i.test \
	tests/explain_udl_pcc8z_i.test \
	tests/explain_udl_pcc_i.test \
	tests/explain_udl_pccul_i.test \
	tests/explain_udl_pccz_i.test \
	tests/explain_udl_pcwul_i.test \
	tests/explain_udl_pcwz_i.test \
	tests/explain_udl_ull_i.test \
	tests/explain_udl_w_i.test

#
# C++ explain user-defined literal error tests
# ============================================
#
TESTS+=	tests/explain_udl__i.test \
	tests/explain_udl_2c_i.test \
	tests/explain_udl_3c_i.test \
	tests/explain_udl_c_i-c.test \
	tests/explain_udl_piul_i.test \
	tests/explain_udl_ul_i.test

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
	tests/set_c++14.test \
	tests/set_c++17.test \
	tests/set_c++20.test \
	tests/set_c++98.test \
	tests/set_c.test \
	tests/set_c11.test \
	tests/set_c18.test \
	tests/set_c89.test \
	tests/set_c95.test \
	tests/set_c99.test \
	tests/set_cknr.test \
	tests/set_digraphs.test \
	tests/set_knr.test \
	tests/set_knrc.test \
	tests/set_nographs.test \
	tests/set_noprompt.test \
	tests/set_nosemicolon.test \
	tests/set_options.test \
	tests/set_prompt.test \
	tests/set_semicolon.test \
	tests/set_trigraphs.test

if ENABLE_CDECL_DEBUG
TESTS+=	tests/set_debug.test \
	tests/set_nodebug.test
endif

if ENABLE_YYDEBUG
TESTS+=	tests/set_yydebug.test \
	tests/set_noyydebug.test
endif

###############################################################################

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .test 

TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests data expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

CDECL = $(top_builddir)/src/cdecl -C
#
# Adding predefined types causes the output of cdecl to change so that the
# expected results for the following tests no longer match and hence "fail."
# This rule updates the expected results of said tests to match the current
# cdecl output so the tests will pass again.
#
update-expected:
	echo 'help' | $(CDECL) > expected/help-c.out
	echo 'help' | $(CDECL) -xc++ > expected/help-c++.out
	echo 'explain int* f(int x, int y)' | $(CDECL) -d > expected/cdecl-d.out
	echo 'explain int* f(int x, int y)' | $(CDECL) --debug > expected/cdecl--debug.out
	echo 'typedef int Int; show all' | $(CDECL) > expected/show_all.out
	echo 'typedef int Int; show all typedef' | $(CDECL) > expected/show_all_typedef.out
	echo 'typedef int Int; show predefined' | $(CDECL) > expected/show_predefined.out
	echo 'typedef int Int; show predefined typedef' | $(CDECL) > expected/show_predefined_typedef.out

# vim:set noet sw=8 ts=8:
