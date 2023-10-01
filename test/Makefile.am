##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Copyright (C) 2018-2023  Paul J. Lucas
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
TESTS = tests/cdecl--color.test \
	tests/cdecl--help.test \
	tests/cdecl--no-typedefs.test

#
# Command-line option error tests
# ===============================
#
TESTS+=	tests/bad_cli_options.test

###############################################################################

#
# Declare tests
# =============
#
TESTS+=	tests/declare.test

#
# C declare error tests
# =====================
#
TESTS+=	tests/declare_0f_v-c.test \
	tests/declare_++i-c.test \
	tests/declare_auf.test \
	tests/declare_b-c89.test \
	tests/declare_b_f_i.test \
	tests/declare_bi129.test \
	tests/declare_bl_ai.test \
	tests/declare_c-c.test \
	tests/declare_c16-c++03.test \
	tests/declare_c16-c99.test \
	tests/declare_c32-c++03.test \
	tests/declare_c32-c99.test \
	tests/declare_c8-c++17.test \
	tests/declare_cf_v-c.test \
	tests/declare_cnmf_v-c.test \
	tests/declare_cri.test \
	tests/declare_en-knr.test \
	tests/declare_enc-c.test \
	tests/declare_etstf_v.test \
	tests/declare_f_ai.test \
	tests/declare_f_f_i.test \
	tests/declare_ff_v-c.test \
	tests/declare_fiiv_v.test \
	tests/declare_fiv_v.test \
	tests/declare_fkr_pi.test \
	tests/declare_fmf_v.test \
	tests/declare_fn.test \
	tests/declare_fneai.test \
	tests/declare_frg_v.test \
	tests/declare_fsti_v.test \
	tests/declare_fuw3_v.test \
	tests/declare_fvx_v.test \
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
	tests/declare_hwt.test \
	tests/declare_ii.test \
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
	tests/declare_lwt.test \
	tests/declare_nei.test \
	tests/declare_nri.test \
	tests/declare_pfkr_i.test \
	tests/declare_pk.test \
	tests/declare_prgi.test \
	tests/declare_puw3.test \
	tests/declare_pv-knr.test \
	tests/declare_rgf_v.test \
	tests/declare_rtf_v-c.test \
	tests/declare_rti.test \
	tests/declare_sib.test \
	tests/declare_sic-knr.test \
	tests/declare_sic16.test \
	tests/declare_sic32.test \
	tests/declare_sic8.test \
	tests/declare_sii-knr.test \
	tests/declare_sil-knr.test \
	tests/declare_siv.test \
	tests/declare_siwt.test \
	tests/declare_t.test \
	tests/declare_typedef_i_al8.test \
	tests/declare_ub.test \
	tests/declare_uc16.test \
	tests/declare_uc32.test \
	tests/declare_uc8.test \
	tests/declare_ud.test \
	tests/declare_uf.test \
	tests/declare_uv.test \
	tests/declare_uwt.test \
	tests/declare_uz.test \
	tests/declare_v.test \
	tests/declare_vf_v-c.test \
	tests/declare_vri.test \
	tests/declare_wt-c89.test \
	tests/declare_xvi-c.test \
	tests/declare_xxi-c17.test \
	tests/explain_a_us_i.test \
	tests/k_declare_i.test

#
# C++ declare error tests
# =======================
#
TESTS+=	tests/declare_+ctor-error.test \
	tests/declare_a_nua_f.test \
	tests/declare_ari.test \
	tests/declare_ati-c++20.test \
	tests/declare_aui-c++11.test \
	tests/declare_cnmf_v.test \
	tests/declare_epf.test \
	tests/declare_etmf.test \
	tests/declare_exsti.test \
	tests/declare_fcf.test \
	tests/declare_fco_exclam_b.test \
	tests/declare_nrfi_v-01-c++.test \
	tests/declare_pmri.test \
	tests/declare_pri.test \
	tests/declare_rrgi.test \
	tests/declare_rv.test \
	tests/declare_stctor.test \
	tests/declare_stmi.test \
	tests/declare_v+f_v.test \
	tests/declare_xvi.test

###############################################################################

#
# Explain tests
# =============
#
TESTS+=	tests/explain.test

#
# C explain error tests
# =====================
#
TESTS+=	tests/explain_+i-c.test \
	tests/explain_0f_v-02.test \
	tests/explain_0f_v-c.test \
	tests/explain_2au-c.test \
	tests/explain_a0i.test \
	tests/explain_a4f_i.test \
	tests/explain_a4v.test \
	tests/explain_a5fd_i.test \
	tests/explain_a5fd_pi.test \
	tests/explain_a_nd_i.test \
	tests/explain_a_re_i.test \
	tests/explain_aai.test \
	tests/explain_al3_c.test \
	tests/explain_al4_cw3.test \
	tests/explain_al8ty_i.test \
	tests/explain_alf_i.test \
	tests/explain_alrgi.test \
	tests/explain_atci.test \
	tests/explain_attda2i.test \
	tests/explain_auf-c.test \
	tests/explain_b-c89.test \
	tests/explain_bi0.test \
	tests/explain_bi1.test \
	tests/explain_bl_ai.test \
	tests/explain_c-02.test \
	tests/explain_c-c.test \
	tests/explain_c16-c++03.test \
	tests/explain_c16-c99.test \
	tests/explain_c32-c++03.test \
	tests/explain_c32-c99.test \
	tests/explain_c8-c++17.test \
	tests/explain_c_al3.test \
	tests/explain_cf_v-c.test \
	tests/explain_ci-knr.test \
	tests/explain_dtor.test \
	tests/explain_efi-c17.test \
	tests/explain_en-knr.test \
	tests/explain_enw3-c.test \
	tests/explain_etstf_v.test \
	tests/explain_f2i_v-redef.test \
	tests/explain_f_ai.test \
	tests/explain_f_k-knr.test \
	tests/explain_f_typedef_ai.test \
	tests/explain_fastv_v.test \
	tests/explain_fcv_v.test \
	tests/explain_fd_i-cknr.test \
	tests/explain_fdan_v.test \
	tests/explain_fel_v-c17.test \
	tests/explain_felcc_v.test \
	tests/explain_felel_v.test \
	tests/explain_felv_v.test \
	tests/explain_ff_i_v-02.test \
	tests/explain_ff_v-c.test \
	tests/explain_fiiv_v.test \
	tests/explain_fiv_v.test \
	tests/explain_fkr_pi.test \
	tests/explain_fkrel_i.test \
	tests/explain_fn.test \
	tests/explain_frg_v.test \
	tests/explain_fsti_v.test \
	tests/explain_fuw3_v.test \
	tests/explain_fvel_v.test \
	tests/explain_fvi_v.test \
	tests/explain_fvv_v.test \
	tests/explain_fvx_v.test \
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
	tests/explain_hwt.test \
	tests/explain_i-itself.test \
	tests/explain_ii.test \
	tests/explain_ipi-redef.test \
	tests/explain_k_al64.test \
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
	tests/explain_lwt.test \
	tests/explain_main_f2c_i.test \
	tests/explain_main_f4i_i.test \
	tests/explain_main_f_v.test \
	tests/explain_main_fi_i.test \
	tests/explain_main_fiapc_hi.test \
	tests/explain_main_fiapci_i.test \
	tests/explain_main_fic_i.test \
	tests/explain_main_fipc_i.test \
	tests/explain_main_fippi_i.test \
	tests/explain_main_fkr_i.test \
	tests/explain_main_stf_i-c.test \
	tests/explain_p_cdecl_v.test \
	tests/explain_pau-c.test \
	tests/explain_pmi-c.test \
	tests/explain_puw3.test \
	tests/explain_pv-knr.test \
	tests/explain_rgf_v.test \
	tests/explain_ri-c.test \
	tests/explain_rtf_v-error.test \
	tests/explain_rtpf_v.test \
	tests/explain_rttdi.test \
	tests/explain_sib.test \
	tests/explain_sibi0.test \
	tests/explain_sibi1.test \
	tests/explain_sic-knr.test \
	tests/explain_sic16.test \
	tests/explain_sic32.test \
	tests/explain_sic8.test \
	tests/explain_sii-knr.test \
	tests/explain_sil-knr.test \
	tests/explain_siv.test \
	tests/explain_siwt.test \
	tests/explain_tdenw4.test \
	tests/explain_tli-c99.test \
	tests/explain_type_i.test \
	tests/explain_typedef_ci32.test \
	tests/explain_ub.test \
	tests/explain_ubi0.test \
	tests/explain_uc16.test \
	tests/explain_uc32.test \
	tests/explain_uc8.test \
	tests/explain_ud.test \
	tests/explain_uf.test \
	tests/explain_using_pfi_v-c.test \
	tests/explain_uv.test \
	tests/explain_uwt.test \
	tests/explain_v.test \
	tests/explain_vf_v-c.test \
	tests/explain_vi-knr.test \
	tests/explain_vovyf-c.test \
	tests/explain_wt-c89.test \
	tests/explain_xxi-c17.test \
	tests/explain_xxvi.test \
	tests/k_explain_ai.test \
	tests/k_explain_bl_i.test \
	tests/k_explain_f_i.test \
	tests/k_explain_i.test \
	tests/typedef_cv_explain_ftd_v.test

#
# C misc. error tests
# ===================
#
TESTS+=	tests/k_typedef_i-c.test

#
# C++ explain error tests
# =======================
#
TESTS+=	tests/explain_+ctor-error.test \
	tests/explain_+dtor-error.test \
	tests/explain_0f_v-03.test \
	tests/explain_2i-redef-c++.test \
	tests/explain_a_nua_f.test \
	tests/explain_a_nua_iw4.test \
	tests/explain_ari.test \
	tests/explain_ati-c++20.test \
	tests/explain_auf-c++11.test \
	tests/explain_aui-c++11.test \
	tests/explain_cepctor_i.test \
	tests/explain_cftsrk_v.test \
	tests/explain_cri.test \
	tests/explain_ctor_2i++default.test \
	tests/explain_ecf_v-Rstr-03.test \
	tests/explain_efd.test \
	tests/explain_enc.test \
	tests/explain_enk.test \
	tests/explain_epudc_b-c++03.test \
	tests/explain_etv-c++.test \
	tests/explain_exsti.test \
	tests/explain_f+_v.test \
	tests/explain_f__v-c++03.test \
	tests/explain_f_v++default.test \
	tests/explain_f_v__v.test \
	tests/explain_fac3_v-c++.test \
	tests/explain_fast3_v-c++.test \
	tests/explain_fau_v-c++17.test \
	tests/explain_fav_v-c++.test \
	tests/explain_fithrk_v.test \
	tests/explain_fkr_v.test \
	tests/explain_fpcf_i_v.test \
	tests/explain_frri_v-c++03.test \
	tests/explain_fstf_v.test \
	tests/explain_i+w3.test \
	tests/explain_ktnk.test \
	tests/explain_la_coco.test \
	tests/explain_la_cor.test \
	tests/explain_la_nco.test \
	tests/explain_la_nn-error.test \
	tests/explain_la_nr.test \
	tests/explain_la_rco.test \
	tests/explain_la_rr.test \
	tests/explain_la_rrn.test \
	tests/explain_la_star_ts_star_ts.test \
	tests/explain_la_star_tsts.test \
	tests/explain_la_ts_star_ts.test \
	tests/explain_la_tsts.test \
	tests/explain_nrfi_v-c++-error.test \
	tests/explain_p_typedef_ri.test \
	tests/explain_pcf_i.test \
	tests/explain_pmrgi.test \
	tests/explain_pri.test \
	tests/explain_prri.test \
	tests/explain_rf_v-c++03.test \
	tests/explain_rftsrk_v.test \
	tests/explain_rr.test \
	tests/explain_rrf_v-c++03.test \
	tests/explain_rrgi.test \
	tests/explain_rrr-1.test \
	tests/explain_rrr-2.test \
	tests/explain_rrrr.test \
	tests/explain_rtpi-c++-error.test \
	tests/explain_rv.test \
	tests/explain_stcf_v.test \
	tests/explain_stftsrk_v.test \
	tests/explain_stmi.test \
	tests/explain_strf_v.test \
	tests/explain_strrf_v.test \
	tests/explain_th_f_v.test \
	tests/explain_unknown_attribute.test \
	tests/explain_v+f_v.test \
	tests/explain_vftsrk_v.test \
	tests/explain_xif_i.test \
	tests/explain_xxf_v-c++11.test

###############################################################################

#
# Cast tests
# ==========
#
TESTS+=	tests/cast.test

#
# C cast error tests
# ==================
#
TESTS+=	tests/cast_a3i.test \
	tests/cast_f_i.test \
	tests/cast_pfi_i-cknr.test \
	tests/cast_pfx_i.test \
	tests/cast_rgi.test \
	tests/cast_type_i.test \
	tests/cast_typedef_i.test \
	tests/const_cast_i-c++03.test \
	tests/const_cast_i.test \
	tests/const_cast_pi-c.test \
	tests/dynamic_cast_i.test \
	tests/dynamic_cast_pc-c.test \
	tests/dynamic_cast_pi.test \
	tests/reinterpret_cast_pv-c.test \
	tests/reinterpret_cast_v.test \
	tests/static_cast_i-c.test

#
# C++ cast error tests
# ====================
#
TESTS+=	tests/cast_au.test \
	tests/cast_pau.test

###############################################################################

#
# Explain cast tests
# ==================
#
TESTS+=	tests/explain_cast.test

#
# C explain cast error tests
# ==========================
#
TESTS+=	tests/explain_cast_a3a5a7i.test \
	tests/explain_cast_a3a5i.test \
	tests/explain_cast_a3i.test \
	tests/explain_cast_aui-c17.test \
	tests/explain_cast_eti.test \
	tests/explain_cast_rgi.test \
	tests/explain_cast_lll.test \
	tests/explain_cast_sti.test \
	tests/explain_cast_typedef_i.test \
	tests/explain_const_cast_pi-c.test \
	tests/explain_dynamic_cast_pc-c.test \
	tests/explain_reinterpret_cast_pv-c.test \
	tests/explain_static_cast_i-c.test

###############################################################################

#
# C show tests
# ============
#
TESTS+=	tests/show.test

#
# C show error tests
# ==================
#
TESTS+=	tests/show_foo.test \
	tests/show_using-c++03.test

###############################################################################

#
# C++ namespace, typedef, & using tests
# =====================================
#
TESTS+=	tests/2c_typedef_i.test \
	tests/define_ins_ns_i.test \
	tests/ins_typedef_i.test \
	tests/k_typedef_i_explain_i.test \
	tests/k_typedef_i_typedef_tn_i.test \
	tests/ns_+typedef_i.test \
	tests/ns_c_ns_+e.test \
	tests/ns_c_typedef_i.test \
	tests/ns_ins_ns_typedef_i.test \
	tests/ns_ins_typedef_i.test \
	tests/ns_k_typedef_i.test \
	tests/ns_n_typedef_i.test \
	tests/ns_ns_typedef_i-c++14.test \
	tests/ns_ns_typedef_i.test \
	tests/ns_typedef_i.test \
	tests/ns_using_i.test \
	tests/retypedef-01.test \
	tests/typedef.test \
	tests/using_i.test \
	tests/using_i_declare_i.test \
	tests/using_i_explain_i.test \
	tests/using_i_int_least32_t.test \
	tests/using_pfi_v_show_f.test

#
# C++ namespace, typedef & using error tests
# ==========================================
#
TESTS+=	tests/2k_typedef_i.test \
	tests/c_ns_c.test \
	tests/define_ns_c_ns_c.test \
	tests/en_ns_c.test \
	tests/ins_ns_typedef_i.test \
	tests/ns_+typedef_i-c++14.test \
	tests/ns_c_ns_c.test \
	tests/typedef_au.test \
	tests/typedef_aui-c17.test \
	tests/typedef_c_int_least32_t.test \
	tests/typedef_eti.test \
	tests/typedef_pau.test \
	tests/typedef_rgi.test \
	tests/typedef_sti.test \
	tests/typedef_tni.test \
	tests/typedef_v_explain_v.test \
	tests/using_aui.test \
	tests/using_c_int_least32_t.test \
	tests/using_eti.test \
	tests/using_f-name_v.test \
	tests/using_i-c.test \
	tests/using_rgi.test \
	tests/using_sti.test

#
# C++ typedef & using error tests
# ===============================
#
TESTS+=	tests/typedef_i+.test \
	tests/typedef_iw3.test

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
TESTS+=	tests/define_aui-c17.test \
	tests/define_c_int_least32_t.test \
	tests/define_eti.test \
	tests/define_rgi.test \
	tests/define_sti.test \
	tests/k_ns_typedef_i.test

#
# C++ define tests
# ================
#
TESTS+=	tests/define_2++_i.test \
	tests/define_++_i.test \
	tests/define_++c_i.test \
	tests/define_efi.test \
	tests/define_k_define_k_explain_k+k.test \
	tests/define_k_explain_pmki.test

#
# C++ define error tests
# ======================
#
TESTS+=	tests/define_ns_++c_i.test \
	tests/define_++sc_i.test

###############################################################################

#
# C config file tests
# ===================
#
TESTS+=	tests/declare_config_tdu.test \
	tests/explain_config_tdu.test \
	tests/using_config_i.test

###############################################################################

#
# C++ declare operator tests
# ==========================
#
TESTS+=	tests/declare_operator.test

#
# C++ declare operator error tests
# ================================
#
TESTS+=	tests/declare_+o_eq_rck_rk++default-c++03.test \
	tests/declare_+o_eq_rck_rk++delete-c++03.test \
	tests/declare_co_exclam_k_b.test \
	tests/declare_fmo_star_rk_rk.test \
	tests/declare_mo_arrow_i.test \
	tests/declare_nmo_amper_i_i.test \
	tests/declare_nmo_arrow_rk_pk.test \
	tests/declare_nmo_brackets_rcki_v.test \
	tests/declare_nmo_eq_rki_rk.test \
	tests/declare_nmo_parens_rck_v.test \
	tests/declare_o_arrow_i.test \
	tests/declare_o_arrow_rk_pk.test \
	tests/declare_o_brackets_2i_i-c++20.test \
	tests/declare_o_colon2_rk_rk.test \
	tests/declare_o_dot_k.test \
	tests/declare_o_dot_star_rk_rk.test \
	tests/declare_o_exclam_2i.test \
	tests/declare_o_less_eq_greater_2rck_au-c++17.test \
	tests/declare_o_minus2_d_rk.test \
	tests/declare_o_minus2_rkd_rk.test \
	tests/declare_o_minus_iel_i.test \
	tests/declare_o_oexclam_k_b.test \
	tests/declare_o_plus2_d_rk.test \
	tests/declare_o_plus2_rkd_rk.test \
	tests/declare_o_plus_iel_i.test \
	tests/declare_o_qmark_colon_k2i_i.test \
	tests/declare_ro_exclam_k_b.test \
	tests/declare_rro_exclam_k_b.test \
	tests/declare_vo_exclam_k_b.test \
	tests/declare_yo_exclam_k_b.test

#
# C++ declare user-defined conversion tests
# =========================================
#
TESTS+=	tests/declare_udc.test

#
# C++ declare user-defined conversion error tests
# ===============================================
#
TESTS+=	tests/declare_udc_a4i.test

#
# C++ declare user-defined literal tests
# ======================================
#
TESTS+=	tests/declare_udl.test

#
# C++ declare user-defined literal error tests
# ============================================
#
TESTS+=	tests/declare_udl_pcci_i.test

#
# C++ explain operator tests
# ==========================
#
TESTS+=	tests/explain_operator.test

#
# C++ explain operator error tests
# ================================
#
TESTS+=	tests/explain_+co_eq2_rck_b++default-cpp17.test \
	tests/explain_+o_eq_rck_rk++default-02.test \
	tests/explain_+o_eq_rck_rk++delete-02.test \
	tests/explain_+o_exclam_eq_rck_rk++delete.test \
	tests/explain_cmo_less_eq_greater_rck_i++default.test \
	tests/explain_co_exclam_k_b.test \
	tests/explain_co_less_i_b++default.test \
	tests/explain_co_less_rck_i++default.test \
	tests/explain_fo_eq2_crki_b++default.test \
	tests/explain_fo_eq2_crkk_b++default.test \
	tests/explain_fo_eq2_crkrk_b++default.test \
	tests/explain_fo_eq2_icrk_b++default.test \
	tests/explain_fo_eq2_ik_b++default.test \
	tests/explain_fo_eq2_kcrk_b++default.test \
	tests/explain_fo_eq2_ki_b++default.test \
	tests/explain_fo_eq2_rkcrk_b++default.test \
	tests/explain_fo_exclam__b.test \
	tests/explain_fo_less_eq_greater_2rck_i++default.test \
	tests/explain_fo_plus_2crk_b++default.test \
	tests/explain_o_arrow_i.test \
	tests/explain_o_brackets_2i_i-c++20.test \
	tests/explain_o_delete__v.test \
	tests/explain_o_delete_i_v.test \
	tests/explain_o_delete_pv_i.test \
	tests/explain_o_eq_rck_v++default.test \
	tests/explain_o_exclam_2i.test \
	tests/explain_o_exclam__b++delete.test \
	tests/explain_o_less_2rck_b++default.test \
	tests/explain_o_less_rck_b++default.test \
	tests/explain_o_minus2_d_rk.test \
	tests/explain_o_minus2_rkd_rk.test \
	tests/explain_o_minus_iel_i.test \
	tests/explain_o_new_i_pv.test \
	tests/explain_o_new_z_z.test \
	tests/explain_o_plus2_d_rk.test \
	tests/explain_o_plus2_rkd_rk.test \
	tests/explain_o_plus_iel_i.test \
	tests/explain_o_plus_kel.test \
	tests/explain_o_qmark_colon_k2i_i.test \
	tests/explain_ov_o_exclam_k_b.test \
	tests/explain_ro_exclam_k_b.test \
	tests/explain_rro_exclam_k_b.test \
	tests/explain_sto_eq_i_v.test \
	tests/explain_sto_parens_2i_v-cpp20.test \
	tests/explain_vo_exclam_k_b.test \
	tests/explain_vv_o_new_z.test \
	tests/explain_yo_exclam_k_b.test

#
# C++ explain user-defined conversion tests
# =========================================
#
TESTS+=	tests/explain_udc.test

#
# C++ explain user-defined conversion error tests
# ===============================================
#
TESTS+=	tests/explain_fudc_i.test \
	tests/explain_studc_i.test \
	tests/explain_udc_a4i.test

#
# C++ explain user-defined literal tests
# ======================================
#
TESTS+=	tests/explain_udl.test

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
TESTS+=	tests/cl.test

#
# Command-line error tests
# ========================
#
TESTS+=	tests/cl-exit.test \
	tests/cl-quit.test

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
# include tests
# =============
#
TESTS+= tests/include-ok-nested-01.test \
	tests/include-ok.test

#
# include error tests
# ===================
#
TESTS+=	tests/include-err-01.test \
	tests/include-too_many_files.test \
	tests/include-unquoted.test

###############################################################################

#
# Embedded C tests
# ================
#
TESTS+=	tests/xec.test

#
# Embedded C error tests
# ======================
#
TESTS+=	tests/xec_explain_lla.test \
	tests/xec_explain_llr.test \
	tests/xec_explain_t.test \
	tests/xec_explain_ti.test

#
# Unified Parallel C tests
# ========================
#
TESTS+=	tests/xupc.test

#
# Unified Parallel C error tests
# ==============================
#
TESTS+=	tests/xupc_explain_ri.test

###############################################################################

#
# Auto-completion tests
# =====================
#
if WITH_EXPECT
TESTS+=	tests/ac-cast_into.exp \
	tests/ac-declare.exp \
	tests/ac-explain.exp \
	tests/ac-help.exp \
	tests/ac-set.exp \
	tests/ac-show.exp
endif

###############################################################################

#
# Miscellaneous tests
# ===================
#
TESTS+=	tests/__declspec.test \
	tests/alt-tokens.test \
	tests/declare_synonyms.test \
	tests/digraphs.test \
	tests/east-const.test \
	tests/english-types.test \
	tests/explain-by-default.test \
	tests/explicit-ecsu.test \
	tests/explicit-int.test \
	tests/gnu_attributes.test \
	tests/help.test \
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
	tests/west-pointer.test

if ENABLE_CDECL_DEBUG
TESTS+=	tests/debug.test
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
