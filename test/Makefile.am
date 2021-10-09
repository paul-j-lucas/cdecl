##
#	cdecl -- C gibberish translator
#	test/Makefile.am
#
#	Copyright (C) 2018-2021  Paul J. Lucas, et al.
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
TESTS =	tests/cdecl--kolor.test \
	tests/cdecl--no-semicolon.test \
	tests/cdecl--no-typedefs.test \
	tests/cdecl-9.test \
	tests/cdecl-Ii.test \
	tests/cdecl-Iiu.test \
	tests/cdecl-Il.test \
	tests/cdecl-Ill.test \
	tests/cdecl-Ilull.test \
	tests/cdecl-Is.test \
	tests/cdecl-Iu.test \
	tests/cdecl-Iui.test \
	tests/cdecl-Iul,ll.test \
	tests/cdecl-Iul.test \
	tests/cdecl-Iull.test \
	tests/cdecl-Iulll.test \
	tests/cdecl-Iulull.test \
	tests/cdecl-Ius.test \
	tests/cdecl-Ix.test \
	tests/cdecl-Sx.test \
	tests/cdecl-h.test \
	tests/cdecl-k.test \
	tests/cdecl-knever.test \
	tests/cdecl-kxxx.test \
	tests/cdecl-s.test \
	tests/cdecl-t.test

if ENABLE_CDECL_DEBUG
TESTS+=	tests/cdecl--debug.test \
	tests/cdecl-d-+i.test \
	tests/cdecl-d-al8i.test \
	tests/cdecl-d-alli.test \
	tests/cdecl-d-cmo_exclam__b.test \
	tests/cdecl-d-eni.test \
	tests/cdecl-d-f2i_pi.test \
	tests/cdecl-d-f_v.test \
	tests/cdecl-d-faia4iaviacias6i_v.test \
	tests/cdecl-d-pmki.test \
	tests/cdecl-d-udc_i.test \
	tests/cdecl-d-z.test
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
TESTS+=	tests/declare_2a4i.test \
	tests/declare_2i.test \
	tests/declare_2pfid_pc.test \
	tests/declare_2pfid_pcc.test \
	tests/declare_2pfid_v.test \
	tests/declare_2pi.test \
	tests/declare_a3a5a7i.test \
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
	tests/declare_ai.test \
	tests/declare_ati-01.test \
	tests/declare_ati-02.test \
	tests/declare_atpi-01.test \
	tests/declare_b.test \
	tests/declare_bii_d.test \
	tests/declare_c.test \
	tests/declare_c_al0.test \
	tests/declare_c_al8.test \
	tests/declare_c_alpv.test \
	tests/declare_cdecl_f.test \
	tests/declare_ci-ec.test \
	tests/declare_ci-knr.test \
	tests/declare_ci.test \
	tests/declare_ck-ec.test \
	tests/declare_ck.test \
	tests/declare_ck2-ec.test \
	tests/declare_ck2.test \
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
	tests/declare_cxd.test \
	tests/declare_cxf.test \
	tests/declare_cz-ec.test \
	tests/declare_cz.test \
	tests/declare_en.test \
	tests/declare_eta2i.test \
	tests/declare_f.test \
	tests/declare_f_pi.test \
	tests/declare_facs3_v.test \
	tests/declare_facv_v.test \
	tests/declare_fas3_v.test \
	tests/declare_fasc3_v.test \
	tests/declare_fav_v.test \
	tests/declare_fccel_v-01.test \
	tests/declare_fccel_v-02.test \
	tests/declare_fccel_v-03.test \
	tests/declare_ff_i_v.test \
	tests/declare_ff_v.test \
	tests/declare_fid_v.test \
	tests/declare_fipfi_v_pfi_v.test \
	tests/declare_fixiy_v.test \
	tests/declare_fkr_pi-c17.test \
	tests/declare_fpc_pi.test \
	tests/declare_fpci_pi.test \
	tests/declare_fpfc_i_d.test \
	tests/declare_frg_v-c89.test \
	tests/declare_frgi_v.test \
	tests/declare_fv_pa3a5i.test \
	tests/declare_fv_pa3a5pi.test \
	tests/declare_fv_pa3i.test \
	tests/declare_fv_pa3pi.test \
	tests/declare_fv_v.test \
	tests/declare_gau.test \
	tests/declare_if_v.test \
	tests/declare_imd.test \
	tests/declare_imf.test \
	tests/declare_k.test \
	tests/declare_ld.test \
	tests/declare_lf.test \
	tests/declare_ll.test \
	tests/declare_n.test \
	tests/declare_nrfi_v-01.test \
	tests/declare_nrfi_v-02.test \
	tests/declare_nrfi_v-03.test \
	tests/declare_p_cdecl_f.test \
	tests/declare_pa3cpa5i.test \
	tests/declare_pa3pa5i.test \
	tests/declare_pa9i.test \
	tests/declare_pati-01.test \
	tests/declare_pbv_pi.test \
	tests/declare_pci-ec.test \
	tests/declare_pcpi.test \
	tests/declare_pfkr_i-c17.test \
	tests/declare_pfv_pi.test \
	tests/declare_pi.test \
	tests/declare_ppi.test \
	tests/declare_rgi.test \
	tests/declare_sa2i.test \
	tests/declare_sc.test \
	tests/declare_sf_v.test \
	tests/declare_si.test \
	tests/declare_sif_v.test \
	tests/declare_sll.test \
	tests/declare_type.test \
	tests/declare_type_pfc_i.test \
	tests/declare_typedef_i.test \
	tests/declare_u.test \
	tests/declare_uc.test \
	tests/declare_ui.test \
	tests/declare_ull.test \
	tests/declare_uw3.test \
	tests/declare_vcpi.test \
	tests/declare_vi-knr.test \
	tests/declare_vpi.test \
	tests/declare_wt.test \
	tests/declare_xvf_i.test \
	tests/declare_z.test \
	tests/declare_zw3.test \
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
	tests/declare_cnmf_v-c.test \
	tests/declare_cri.test \
	tests/declare_en-knr.test \
	tests/declare_enc-c.test \
	tests/declare_etsf_v.test \
	tests/declare_f_ai.test \
	tests/declare_f_f_i.test \
	tests/declare_ff_v-c.test \
	tests/declare_fiiv_v.test \
	tests/declare_fiv_v.test \
	tests/declare_fkr_pi.test \
	tests/declare_fmf_v.test \
	tests/declare_fn.test \
	tests/declare_frg_v.test \
	tests/declare_fsi_v.test \
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
	tests/declare_nri.test \
	tests/declare_pfkr_i.test \
	tests/declare_pk.test \
	tests/declare_prgi.test \
	tests/declare_puw3.test \
	tests/declare_pv-knr.test \
	tests/declare_rgf_v.test \
	tests/declare_rtf_v-c.test \
	tests/declare_rti.test \
	tests/declare_sb.test \
	tests/declare_sc-knr.test \
	tests/declare_sc16.test \
	tests/declare_sc32.test \
	tests/declare_sc8.test \
	tests/declare_si-knr.test \
	tests/declare_sl-knr.test \
	tests/declare_sv.test \
	tests/declare_swt.test \
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
	tests/declare_xxi-c.test

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
	tests/explain_2npi.test \
	tests/explain_a012i.test \
	tests/explain_a0b10i.test \
	tests/explain_a0xAi-01.test \
	tests/explain_a0xai-02.test \
	tests/explain_a3a5a7i.test \
	tests/explain_a3a5i.test \
	tests/explain_a3a5pfv_i.test \
	tests/explain_a3a5pfv_pi.test \
	tests/explain_a3a5pfv_ppa7a9i.test \
	tests/explain_a3a5pfv_ppa7a9pc.test \
	tests/explain_a3a5pfv_ppa7i.test \
	tests/explain_a3a5pfv_ppa7pi.test \
	tests/explain_a3a5pfv_ppi.test \
	tests/explain_a3pfv_i.test \
	tests/explain_a3ppfv_i.test \
	tests/explain_a9cpi.test \
	tests/explain_a9i.test \
	tests/explain_a9pi.test \
	tests/explain_a_mu_k-01.test \
	tests/explain_a_mu_k-02.test \
	tests/explain_a_using_a_mu_i.test \
	tests/explain_ai.test \
	tests/explain_al0_c.test \
	tests/explain_al8_c.test \
	tests/explain_alpv_c.test \
	tests/explain_ati-01.test \
	tests/explain_ati-02.test \
	tests/explain_atpi-01.test \
	tests/explain_atpi-02.test \
	tests/explain_b.test \
	tests/explain_bii_d.test \
	tests/explain_c.test \
	tests/explain_cast_pbv_pi.test \
	tests/explain_cdecl_f.test \
	tests/explain_ci-ec.test \
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
	tests/explain_cxd.test \
	tests/explain_cxf.test \
	tests/explain_en.test \
	tests/explain_eta2i.test \
	tests/explain_f_pi.test \
	tests/explain_f_ppc.test \
	tests/explain_fac_v-1.test \
	tests/explain_fac_v.test \
	tests/explain_facs3_v.test \
	tests/explain_facv_v.test \
	tests/explain_fas3_v.test \
	tests/explain_fasc3_v.test \
	tests/explain_fav_v.test \
	tests/explain_fccel_v.test \
	tests/explain_ff_i_v.test \
	tests/explain_ff_v.test \
	tests/explain_fid_v.test \
	tests/explain_fipfi_v_pfi_v.test \
	tests/explain_fixiy_v.test \
	tests/explain_fkr_i.test \
	tests/explain_fkr_pi-c17.test \
	tests/explain_fpa5d_v.test \
	tests/explain_fpc_pi.test \
	tests/explain_fpci_pi.test \
	tests/explain_fpf_pac_i.test \
	tests/explain_fpfc_i_d.test \
	tests/explain_frg_v-c89.test \
	tests/explain_frgi_v.test \
	tests/explain_fv_pa3a5i.test \
	tests/explain_fv_pa3a5pi.test \
	tests/explain_fv_pa3i.test \
	tests/explain_fv_pa3pi.test \
	tests/explain_fv_pi.test \
	tests/explain_fv_v.test \
	tests/explain_gau.test \
	tests/explain_gif_v.test \
	tests/explain_i-reserved-c.test \
	tests/explain_i_i.test \
	tests/explain_i_pfd_i.test \
	tests/explain_i_pi.test \
	tests/explain_i_typedef.test \
	tests/explain_ic.test \
	tests/explain_if_v.test \
	tests/explain_imd.test \
	tests/explain_imf.test \
	tests/explain_irg.test \
	tests/explain_is.test \
	tests/explain_k-itself.test \
	tests/explain_k.test \
	tests/explain_kc.test \
	tests/explain_keywords.test \
	tests/explain_ld.test \
	tests/explain_lf.test \
	tests/explain_ll.test \
	tests/explain_main_f_i.test \
	tests/explain_main_f_s.test \
	tests/explain_main_fi2apc_i.test \
	tests/explain_main_fi2ppc_i.test \
	tests/explain_main_fiapc_i.test \
	tests/explain_main_fiapcc_i.test \
	tests/explain_main_fippc_i.test \
	tests/explain_main_fippcc_i.test \
	tests/explain_main_fv_i.test \
	tests/explain_mscc_f_v.test \
	tests/explain_mscd_f_v.test \
	tests/explain_msfc_f_v.test \
	tests/explain_mssc_f_v.test \
	tests/explain_mstc_f_v.test \
	tests/explain_msvc_f_v.test \
	tests/explain_mswa_f_v.test \
	tests/explain_n.test \
	tests/explain_na4i.test \
	tests/explain_nfi_c.test \
	tests/explain_ni.test \
	tests/explain_nrfi_v.test \
	tests/explain_oi.test \
	tests/explain_ovo_minus2_i_rk.test \
	tests/explain_ovo_minus_rk_rk.test \
	tests/explain_ovo_plus2_i_rk.test \
	tests/explain_ovo_plus_rk_rk.test \
	tests/explain_p_mscd_f.test \
	tests/explain_p_mssc_f_v.test \
	tests/explain_pa3a5i.test \
	tests/explain_pa3cpa5i.test \
	tests/explain_pa3pa5i.test \
	tests/explain_pa9i.test \
	tests/explain_pati-01.test \
	tests/explain_pati-02.test \
	tests/explain_pbv_pi.test \
	tests/explain_pcpi.test \
	tests/explain_pfv_pi.test \
	tests/explain_pi.test \
	tests/explain_pp_mssc_f_v.test \
	tests/explain_ppi.test \
	tests/explain_rgi.test \
	tests/explain_rgk.test \
	tests/explain_ro_minus2_i_rk.test \
	tests/explain_ro_minus_rk_rk.test \
	tests/explain_ro_plus2_i_rk.test \
	tests/explain_ro_plus_rk_rk.test \
	tests/explain_rro_minus2_i_rk.test \
	tests/explain_rro_minus_rk_rk.test \
	tests/explain_rro_plus2_i_rk.test \
	tests/explain_rro_plus_rk_rk.test \
	tests/explain_rtpi.test \
	tests/explain_sa2i.test \
	tests/explain_sc.test \
	tests/explain_sf_v.test \
	tests/explain_si.test \
	tests/explain_tli.test \
	tests/explain_typedef_i.test \
	tests/explain_typedef_ii32.test \
	tests/explain_typedef_pfc_i.test \
	tests/explain_typedef_ulz.test \
	tests/explain_uc.test \
	tests/explain_ull.test \
	tests/explain_uw3.test \
	tests/explain_vcpi.test \
	tests/explain_vf_v.test \
	tests/explain_vo_minus2_i_rk.test \
	tests/explain_vo_minus_rk_rk.test \
	tests/explain_vo_plus2_i_rk.test \
	tests/explain_vo_plus_rk_rk.test \
	tests/explain_vpi.test \
	tests/explain_wt.test \
	tests/explain_yo_minus2_i_rk.test \
	tests/explain_yo_minus_rk_rk.test \
	tests/explain_yo_plus2_i_rk.test \
	tests/explain_yo_plus_rk_rk.test \
	tests/explain_z.test \
	tests/explain_zw3.test \
	tests/explain_zz.test

#
# C explain-by-default tests
# ==========================
#
TESTS+=	tests/ci.test \
	tests/i.test

#
# C explain error tests
# =====================
# Almost every test here has a corresponding declare_ error test.
#
TESTS+=	tests/explain_+i-c.test \
	tests/explain_0f_v-02.test \
	tests/explain_0f_v-c.test \
	tests/explain_a4f_i.test \
	tests/explain_a4v.test \
	tests/explain_ac9i.test \
	tests/explain_af-c.test \
	tests/explain_al3_c.test \
	tests/explain_al8ty_i.test \
	tests/explain_alf_i.test \
	tests/explain_alrgi.test \
	tests/explain_as9i.test \
	tests/explain_atci.test \
	tests/explain_b-c89.test \
	tests/explain_b_ai.test \
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
	tests/explain_efi-c.test \
	tests/explain_en-knr.test \
	tests/explain_etsf_v.test \
	tests/explain_f_ai.test \
	tests/explain_f_k-knr.test \
	tests/explain_f_typedef_ai.test \
	tests/explain_fasv_v.test \
	tests/explain_fd_i-cknr.test \
	tests/explain_fel_v.test \
	tests/explain_felcc_v.test \
	tests/explain_felel_v.test \
	tests/explain_felv_v.test \
	tests/explain_ff_v-c.test \
	tests/explain_fiiv_v.test \
	tests/explain_fiv_v.test \
	tests/explain_fkr_pi.test \
	tests/explain_fkrel_i.test \
	tests/explain_fn.test \
	tests/explain_frg_v.test \
	tests/explain_fsi_v.test \
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
	tests/explain_main_sf_i-c.test \
	tests/explain_p_cdecl_v.test \
	tests/explain_pmi-c.test \
	tests/explain_puw3.test \
	tests/explain_pv-knr.test \
	tests/explain_rga4i.test \
	tests/explain_rgf_v.test \
	tests/explain_ri-c.test \
	tests/explain_rtf_v-error.test \
	tests/explain_sb.test \
	tests/explain_sc-knr.test \
	tests/explain_sc16.test \
	tests/explain_sc32.test \
	tests/explain_sc8.test \
	tests/explain_si-knr.test \
	tests/explain_sl-knr.test \
	tests/explain_sv.test \
	tests/explain_swt.test \
	tests/explain_tli-c99.test \
	tests/explain_type_i.test \
	tests/explain_typedef_ci32.test \
	tests/explain_ub.test \
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
	tests/explain_xxi-c.test

#
# C misc. error tests
# ===================
#
TESTS+=	tests/k_typedef_i-c.test

#
# C++ declare alt-tokens tests
# ============================
#
TESTS+=	tests/declare_alttokens_dtor.test \
	tests/declare_alttokens_o_amper2_rck_b.test \
	tests/declare_alttokens_o_amper_eq_rki_rk.test \
	tests/declare_alttokens_o_amper_rcki_k.test \
	tests/declare_alttokens_o_circ_eq_i_rk.test \
	tests/declare_alttokens_o_circ_rcki_k.test \
	tests/declare_alttokens_o_exclam_eq_rcki_b.test \
	tests/declare_alttokens_o_exclam_rck_b.test \
	tests/declare_alttokens_o_pipe2_rck_b.test \
	tests/declare_alttokens_o_pipe_2rck_k.test \
	tests/declare_alttokens_o_pipe_eq_i_rk.test \
	tests/declare_alttokens_o_pipe_i_rk.test \
	tests/declare_alttokens_o_tilde_k.test \
	tests/declare_alttokens_rri.test

#
# C++ declare digraph & trigraph tests
# ====================================
#
TESTS+=	tests/declare_digraphs_a9i.test \
	tests/declare_digraphs_nrfi_v-c++.test \
	tests/declare_trigraphs_a9i.test \
	tests/declare_trigraphs_nrfi_v-c++.test \
	tests/declare_trigraphs_nrfi_v-c++14.test \
	tests/declare_trigraphs_o_circ_eq_i_rk.test \
	tests/declare_trigraphs_o_circ_rcki_k.test \
	tests/declare_trigraphs_o_pipe2_2rck_b.test \
	tests/declare_trigraphs_o_pipe_2rck_k.test \
	tests/declare_trigraphs_o_pipe_eq_i_rk.test \
	tests/declare_trigraphs_o_tilde_k.test

#
# C++ explain digraph & trigraph tests
# ====================================
#
TESTS+=	tests/explain_digraphs_a9i-c89.test \
	tests/explain_digraphs_a9i.test \
	tests/explain_digraphs_nrfi_v-c++.test \
	tests/explain_trigraphs_a9i-c++.test \
	tests/explain_trigraphs_a9i.test \
	tests/explain_trigraphs_nrfi_v-c++11.test \
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
	tests/cast_cpi.test \
	tests/cast_cppi.test \
	tests/cast_cvpi.test \
	tests/cast_ll.test \
	tests/cast_pa3cpa5i.test \
	tests/cast_pa3pa5i.test \
	tests/cast_pa9i.test \
	tests/cast_pbv_pi.test \
	tests/cast_pci.test \
	tests/cast_pcpi.test \
	tests/cast_pf_i-01.test \
	tests/cast_pf_i-02.test \
	tests/cast_pfii_i.test \
	tests/cast_pfv_pi.test \
	tests/cast_pfx_i-c17.test \
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
	tests/explain_cast_rtpi.test \
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
	tests/explain_cast_eti.test \
	tests/explain_cast_rgi.test \
	tests/explain_cast_lll.test \
	tests/explain_cast_si.test \
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
TESTS+=	tests/show_all.test \
	tests/show_all-c++.test \
	tests/show_all_predefined-c++.test \
	tests/show_all_predefined.test \
	tests/show_all_predefined_glob-01.test \
	tests/show_all_predefined_glob-02.test \
	tests/show_all_predefined_glob-03.test \
	tests/show_all_predefined_glob-04.test \
	tests/show_all_predefined_typedef-c++.test \
	tests/show_all_typedef.test \
	tests/show_cpf_cpcc_v_using.test \
	tests/show_cpi_using.test \
	tests/show_cpmki_using.test \
	tests/show_int8_t.test \
	tests/show_int8_t_typedef.test \
	tests/show_predefined-c++.test \
	tests/show_predefined.test \
	tests/show_predefined_typedef.test \
	tests/show_predefined_using.test \
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
	tests/typedef_fi_v_show_f.test \
	tests/typedef_i.test \
	tests/typedef_i_declare_i.test \
	tests/typedef_i_explain_i.test \
	tests/typedef_i_int_least32_t.test \
	tests/typedef_i_pi.test \
	tests/typedef_v.test \
	tests/using_fi_v_show_f.test \
	tests/using_i.test \
	tests/using_i_declare_i.test \
	tests/using_i_explain_i.test \
	tests/using_i_int_least32_t.test

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
	tests/typedef_ai.test \
	tests/typedef_c_int_least32_t.test \
	tests/typedef_eti.test \
	tests/typedef_rgi.test \
	tests/typedef_si.test \
	tests/typedef_tni.test \
	tests/typedef_v_explain_v.test \
	tests/using_ai.test \
	tests/using_c_int_least32_t.test \
	tests/using_eti.test \
	tests/using_f-name_v.test \
	tests/using_i-c.test \
	tests/using_rgi.test \
	tests/using_si.test

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
TESTS+=	tests/define_ai.test \
	tests/define_c_int_least32_t.test \
	tests/define_eti.test \
	tests/define_rgi.test \
	tests/define_si.test \
	tests/k_ns_typedef_i.test

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
TESTS+=	tests/define_ns_=c_i.test \
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
TESTS+=	tests/declare_+dtor.test \
	tests/declare_+f_v.test \
	tests/declare_0f_v.test \
	tests/declare_2+f_v.test \
	tests/declare_=+i.test \
	tests/declare_=i.test \
	tests/declare_a.test \
	tests/declare_a3a5pfv_ppa7a9pc.test \
	tests/declare_a_cd_fa_cd_i_i.test \
	tests/declare_a_cd_fi_i.test \
	tests/declare_a_dep_a_mu_i.test \
	tests/declare_a_dep_f_v.test \
	tests/declare_a_mu_f_v-01.test \
	tests/declare_a_mu_f_v-02.test \
	tests/declare_a_mu_i-01.test \
	tests/declare_a_mu_i-02.test \
	tests/declare_a_nd_f_i-01.test \
	tests/declare_a_nd_f_i-02.test \
	tests/declare_af-c++.test \
	tests/declare_b-c++.test \
	tests/declare_c_al0-c++.test \
	tests/declare_c_al8-c++.test \
	tests/declare_cf_v.test \
	tests/declare_cpmi.test \
	tests/declare_crf_v.test \
	tests/declare_crrf_v.test \
	tests/declare_ctor.test \
	tests/declare_ctor_i.test \
	tests/declare_dtor.test \
	tests/declare_ecf_v.test \
	tests/declare_efi.test \
	tests/declare_enc.test \
	tests/declare_enk.test \
	tests/declare_epctor_i.test \
	tests/declare_exf_i.test \
	tests/declare_exi.test \
	tests/declare_f+k_v.test \
	tests/declare_f_pmi.test \
	tests/declare_fa_cd_i_i.test \
	tests/declare_fa_dep_i_v.test \
	tests/declare_fa_mu_i_v-01.test \
	tests/declare_fa_mu_i_v-02.test \
	tests/declare_fa_nd_i_v-01.test \
	tests/declare_fa_nd_i_v-02.test \
	tests/declare_frri_v.test \
	tests/declare_grtpi.test \
	tests/declare_i=+.test \
	tests/declare_ii-c++.test \
	tests/declare_mi.test \
	tests/declare_nrfi_v-02-c++.test \
	tests/declare_nrfi_v-03-c++.test \
	tests/declare_ntf.test \
	tests/declare_nxf.test \
	tests/declare_o_delete_a_pv_v.test \
	tests/declare_o_delete_pv_v.test \
	tests/declare_o_new_a_z_pv.test \
	tests/declare_o_new_z_pv.test \
	tests/declare_ovyf.test \
	tests/declare_pfv_ri.test \
	tests/declare_pmci.test \
	tests/declare_pmf_i.test \
	tests/declare_pmf_pc.test \
	tests/declare_pmf_pk.test \
	tests/declare_pmi.test \
	tests/declare_ra3i.test \
	tests/declare_rci.test \
	tests/declare_rcpi.test \
	tests/declare_ref.test \
	tests/declare_rf_v.test \
	tests/declare_ri.test \
	tests/declare_rpi.test \
	tests/declare_rrf_v.test \
	tests/declare_rtf_v.test \
	tests/declare_sxxi.test \
	tests/declare_vdtor.test \
	tests/declare_vf_v.test \
	tests/declare_vovyf.test \
	tests/declare_xici.test \
	tests/declare_xxi.test \
	tests/declare_yf.test \
	tests/declare_yi.test

#
# C++ declare error tests
# =======================
#
TESTS+=	tests/declare_+ctor-error.test \
	tests/declare_a_cd_i.test \
	tests/declare_a_nua_f.test \
	tests/declare_ai-c++11.test \
	tests/declare_ari.test \
	tests/declare_ati-c++20.test \
	tests/declare_cnmf_v.test \
	tests/declare_epf.test \
	tests/declare_exsi.test \
	tests/declare_fcf.test \
	tests/declare_fco_exclam_b.test \
	tests/declare_nrfi_v-01-c++.test \
	tests/declare_pmri.test \
	tests/declare_pri.test \
	tests/declare_rrgi.test \
	tests/declare_rv.test \
	tests/declare_sctor.test \
	tests/declare_smi.test \
	tests/declare_thf_v.test \
	tests/declare_udl_pcci_i.test \
	tests/declare_v+f_v.test \
	tests/declare_xvi.test

#
# C++ declare member operator tests
# =================================
#
TESTS+=	tests/declare_+o_delete_a_pv_v.test \
	tests/declare_+o_delete_pv_v.test \
	tests/declare_+o_new_a_z_pv.test \
	tests/declare_+o_new_z_pv.test \
	tests/declare_mo_amper2_rck_b.test \
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
	tests/declare_mo_tilde_k.test \
	tests/declare_so_delete_pv_v.test

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
TESTS+=	tests/declare_+o_eq_rck_rk=default.test \
	tests/declare_+o_eq_rck_rk=delete.test \
	tests/declare_o2=_exclam_b.test \
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
TESTS+=	tests/declare_+o_eq_rck_rk=default-c++03.test \
	tests/declare_+o_eq_rck_rk=delete-c++03.test \
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
	tests/declare_o_brackets_2i_i.test \
	tests/declare_o_colon2_rk_rk.test \
	tests/declare_o_dot_k.test \
	tests/declare_o_dot_star_rk_rk.test \
	tests/declare_o_exclam_2i.test \
	tests/declare_o_less_eq_greater_2rck_a-c++17.test \
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
TESTS+=	tests/declare_+udc_i.test \
	tests/declare_ovc_udc_i.test \
	tests/declare_udc_i.test \
	tests/declare_udc_rcc.test

#
# C++ declare user-defined conversion error tests
# ===============================================
#
TESTS+=	tests/declare_udc_a4i.test

#
# C++ declare user-defined literal tests
# ======================================
#
TESTS+=	tests/declare_+udl_c_i.test \
	tests/explain_udl_pccz_ul.test

#
# C++ explain tests
# =================
#
TESTS+=	tests/explain_+compl_dtor.test \
	tests/explain_+ctor.test \
	tests/explain_+ctor_i.test \
	tests/explain_+dtor.test \
	tests/explain_0dtor.test \
	tests/explain_2+dtor.test \
	tests/explain_a.test \
	tests/explain_a_cd_fa_cd_i_i.test \
	tests/explain_a_cd_fi_i.test \
	tests/explain_a_dep_a_mu_i-01.test \
	tests/explain_a_dep_a_mu_i-02.test \
	tests/explain_a_dep_f_v.test \
	tests/explain_a_dep_i.test \
	tests/explain_a_mu_f_v.test \
	tests/explain_a_mu_i.test \
	tests/explain_a_nd_f_v.test \
	tests/explain_a_nua_k-1.test \
	tests/explain_a_nua_k-2.test \
	tests/explain_al0_c-c++.test \
	tests/explain_al8_c-c++.test \
	tests/explain_bitand_i.test \
	tests/explain_cf_v.test \
	tests/explain_compl_dtor.test \
	tests/explain_cpmci.test \
	tests/explain_cpmi.test \
	tests/explain_crf_v.test \
	tests/explain_ctor=default.test \
	tests/explain_ctor_i.test \
	tests/explain_cvf_v.test \
	tests/explain_dtor.test \
	tests/explain_ec_typedef_i.test \
	tests/explain_ec_using_i.test \
	tests/explain_ecf_v-Rstr-01.test \
	tests/explain_ecf_v-Rstr-02.test \
	tests/explain_ecf_v.test \
	tests/explain_efi-itself.test \
	tests/explain_efi.test \
	tests/explain_efui.test \
	tests/explain_efz.test \
	tests/explain_epctor_i.test \
	tests/explain_epictor_i.test \
	tests/explain_epnxctor_i.test \
	tests/explain_exf_i.test \
	tests/explain_exi.test \
	tests/explain_f__pf__v.test \
	tests/explain_f__pv.test \
	tests/explain_f__v.test \
	tests/explain_f_cpmi.test \
	tests/explain_f_pmi.test \
	tests/explain_f_v=delete.test \
	tests/explain_fa_cd_i_i.test \
	tests/explain_fa_dep_i_v.test \
	tests/explain_fa_mu_i_v.test \
	tests/explain_fa_nd_i_v.test \
	tests/explain_fa_v.test \
	tests/explain_fctor_i.test \
	tests/explain_frri_v.test \
	tests/explain_grtpi.test \
	tests/explain_grtri.test \
	tests/explain_grtrri.test \
	tests/explain_i+.test \
	tests/explain_i-reserved-c++.test \
	tests/explain_i2+.test \
	tests/explain_ictor_i.test \
	tests/explain_ii-c++.test \
	tests/explain_inx+ctor.test \
	tests/explain_inx+dtor.test \
	tests/explain_ki+.test \
	tests/explain_main_vf_h.test \
	tests/explain_mi.test \
	tests/explain_nrfi_v-c++.test \
	tests/explain_nsc_typedef.test \
	tests/explain_nxf_v.test \
	tests/explain_nxff_v.test \
	tests/explain_nxtf_v.test \
	tests/explain_ovyf.test \
	tests/explain_pmf_i.test \
	tests/explain_pmf_pc.test \
	tests/explain_pmfii_pk.test \
	tests/explain_pmi.test \
	tests/explain_pmrgi.test \
	tests/explain_ra3i.test \
	tests/explain_rf_v.test \
	tests/explain_rgi-c++11.test \
	tests/explain_ri.test \
	tests/explain_rrf_v.test \
	tests/explain_sf__i.test \
	tests/explain_sxxi.test \
	tests/explain_thf_v-c++17.test \
	tests/explain_using_a3i.test \
	tests/explain_using_pfi_v.test \
	tests/explain_vcompl_dtor.test \
	tests/explain_vdtor.test \
	tests/explain_vovyf.test \
	tests/explain_xxctor.test \
	tests/explain_xxi.test \
	tests/explain_ydtor.test \
	tests/explain_yf.test \
	tests/explain_yi.test

#
# C++ explain error tests
# =======================
#
TESTS+=	tests/explain_+ctor-error.test \
	tests/explain_+dtor-error.test \
	tests/explain_0f_v-03.test \
	tests/explain_a_cd_i.test \
	tests/explain_a_nua_f.test \
	tests/explain_af-c++11.test \
	tests/explain_ai-c++11.test \
	tests/explain_ari.test \
	tests/explain_ati-c++20.test \
	tests/explain_cepctor_i.test \
	tests/explain_cfi.test \
	tests/explain_cri.test \
	tests/explain_ctor_2i=default.test \
	tests/explain_ecf_v-Rstr-03.test \
	tests/explain_efd.test \
	tests/explain_enc.test \
	tests/explain_enk.test \
	tests/explain_exsi.test \
	tests/explain_f+_v.test \
	tests/explain_f__v-c++03.test \
	tests/explain_f_v=default.test \
	tests/explain_f_v__v.test \
	tests/explain_fa_v-c++17.test \
	tests/explain_fac3_v-c++.test \
	tests/explain_fas3_v-c++.test \
	tests/explain_fav_v-c++.test \
	tests/explain_fkr_v.test \
	tests/explain_fpcf_i_v.test \
	tests/explain_frri_v-c++03.test \
	tests/explain_fsf_v.test \
	tests/explain_i+w3.test \
	tests/explain_ktnk.test \
	tests/explain_nrfi_v-c++-error.test \
	tests/explain_p_typedef_ri.test \
	tests/explain_pcf_i.test \
	tests/explain_pmri.test \
	tests/explain_pri.test \
	tests/explain_prri.test \
	tests/explain_rf_i.test \
	tests/explain_rf_v-c++03.test \
	tests/explain_rr.test \
	tests/explain_rrf_v-c++03.test \
	tests/explain_rrgi.test \
	tests/explain_rrr-1.test \
	tests/explain_rrr-2.test \
	tests/explain_rrrr.test \
	tests/explain_rtpi-c++-error.test \
	tests/explain_rv.test \
	tests/explain_scf_v.test \
	tests/explain_smi.test \
	tests/explain_srf_v.test \
	tests/explain_srrf_v.test \
	tests/explain_thf_v.test \
	tests/explain_unknown_attribute.test \
	tests/explain_v+f_v.test \
	tests/explain_xif_i.test \
	tests/explain_xxf_v-c++11.test

#
# C++ explain operator tests
# ==========================
#
TESTS+=	tests/explain_+co_eq2_rck_b=default.test \
	tests/explain_+o_delete_a_pv_v.test \
	tests/explain_+o_delete_pv_v.test \
	tests/explain_+o_eq2_rcki_b.test \
	tests/explain_+o_eq_rck_rk.test \
	tests/explain_+o_eq_rck_rk=default-01.test \
	tests/explain_+o_eq_rck_rk=delete-01.test \
	tests/explain_+o_exclam_eq_rck_rk=delete.test \
	tests/explain_+o_new_a_z_pv.test \
	tests/explain_+o_new_z_pv.test \
	tests/explain_0o_amper_rk_pk.test \
	tests/explain_0o_star_rk_rk.test \
	tests/explain_cmo_less_eq_greater_rck_a=default.test \
	tests/explain_co_amper_rk_pk.test \
	tests/explain_co_eq2_crk_b=default.test \
	tests/explain_co_star_rk_rk.test \
	tests/explain_co_tilde_k.test \
	tests/explain_dtor=delete.test \
	tests/explain_f+o_exclam__b.test \
	tests/explain_fo_eq2_2crk_b=default.test \
	tests/explain_fo_eq2_2k_b=default.test \
	tests/explain_mo_less_eq_greater_rck_a.test \
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
	tests/explain_o_delete_a_pv_v.test \
	tests/explain_o_delete_pv_v.test \
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
	tests/explain_o_less_eq_greater_2rck_a.test \
	tests/explain_o_less_eq_greater_2rck_i.test \
	tests/explain_o_less_eq_greater_2rck_std_partial_ordering.test \
	tests/explain_o_less_eq_greater_2rck_std_strong_ordering.test \
	tests/explain_o_less_eq_greater_2rck_std_weak_ordering.test \
	tests/explain_o_less_eq_rcki_b.test \
	tests/explain_o_less_rcki_b.test \
	tests/explain_o_minus2_rki_rk.test \
	tests/explain_o_minus_2rck_k.test \
	tests/explain_o_minus_eq_i_rk.test \
	tests/explain_o_minus_eq_rki_rk.test \
	tests/explain_o_minus_i_k.test \
	tests/explain_o_minus_rcki_k.test \
	tests/explain_o_new_a_z_pv.test \
	tests/explain_o_new_z_pv.test \
	tests/explain_o_new_zrrk_pv.test \
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
	tests/explain_o_plus2__rk.test \
	tests/explain_o_plus2_rk_rk.test \
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
	tests/explain_ovo_amper_rk_pk.test \
	tests/explain_ovo_star_rk_rk.test \
	tests/explain_ro_amper_rk_pk.test \
	tests/explain_ro_star_rk_rk.test \
	tests/explain_rro_amper_rk_pk.test \
	tests/explain_rro_star_rk_rk.test \
	tests/explain_so_delete_pv_v.test \
	tests/explain_so_new_z_pv.test \
	tests/explain_vo_amper_rk_pk.test \
	tests/explain_vo_star_rk_rk.test \
	tests/explain_yo_amper_rk_pk.test \
	tests/explain_yo_star_rk_rk.test

#
# C++ explain operator error tests
# ================================
#
TESTS+=	tests/explain_+co_eq2_rck_b=default-cpp17.test \
	tests/explain_+o_eq_rck_rk=default-02.test \
	tests/explain_+o_eq_rck_rk=delete-02.test \
	tests/explain_+o_exclam_eq_rck_rk=delete.test \
	tests/explain_cmo_less_eq_greater_rck_i=default.test \
	tests/explain_co_exclam_k_b.test \
	tests/explain_co_less_i_b=default.test \
	tests/explain_co_less_rck_i=default.test \
	tests/explain_fo_eq2_crki_b=default.test \
	tests/explain_fo_eq2_crkk_b=default.test \
	tests/explain_fo_eq2_crkrk_b=default.test \
	tests/explain_fo_eq2_icrk_b=default.test \
	tests/explain_fo_eq2_ik_b=default.test \
	tests/explain_fo_eq2_kcrk_b=default.test \
	tests/explain_fo_eq2_ki_b=default.test \
	tests/explain_fo_eq2_rkcrk_b=default.test \
	tests/explain_fo_exclam__b.test \
	tests/explain_fo_less_eq_greater_2rck_i=default.test \
	tests/explain_fo_plus_2crk_b=default.test \
	tests/explain_o_arrow_i.test \
	tests/explain_o_brackets_2i_i.test \
	tests/explain_o_delete__v.test \
	tests/explain_o_delete_i_v.test \
	tests/explain_o_delete_pv_i.test \
	tests/explain_o_eq_rck_v=default.test \
	tests/explain_o_exclam_2i.test \
	tests/explain_o_exclam__b=delete.test \
	tests/explain_o_less_2rck_b=default.test \
	tests/explain_o_less_rck_b=default.test \
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
	tests/explain_vo_exclam_k_b.test \
	tests/explain_vv_o_new_z.test \
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
	tests/dynamic_cast_pk.test \
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
# C++ explain user-defined conversion tests
# =========================================
#
TESTS+=	tests/explain_+udc_i.test \
	tests/explain_f+udc_i.test \
	tests/explain_ovc_udc_i.test \
	tests/explain_udc_i.test \
	tests/explain_udc_rcc.test

#
# C++ explain user-defined literal tests
# ======================================
#
TESTS+=	tests/explain_+udl_c_i.test \
	tests/explain_udl_c16_i.test \
	tests/explain_udl_c32_i.test \
	tests/explain_udl_c8_i.test \
	tests/explain_udl_c_i-01.test \
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
	tests/explain_udl_pcwtul_i.test \
	tests/explain_udl_pcwtz_i.test \
	tests/explain_udl_ull_i.test \
	tests/explain_udl_wt_i.test

#
# C++ explain user-defined conversion error tests
# ===============================================
#
TESTS+=	tests/explain_fudc_i.test \
	tests/explain_sudc_i.test \
	tests/explain_udc_a4i.test

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
# GNU __attribute__ tests
# =======================
#
TESTS+=	tests/explain_2gat_cpi.test \
	tests/explain_c_gat_pi.test \
	tests/explain_c_gat_vpi.test \
	tests/explain_gat_c_gat_vpi.test \
	tests/explain_gat_cpi.test \
	tests/explain_gat_i-01.test \
	tests/explain_gat_i-02.test \
	tests/explain_gat_i-03.test \
	tests/explain_gat_i-04.test \
	tests/explain_gat_i-05.test \
	tests/explain_gat_i-06.test \
	tests/explain_gat_i-07.test \
	tests/explain_gat_i-08.test \
	tests/explain_gat_i-09.test

#
# Microsoft __declspec tests
# ==========================
#
TESTS+=	tests/explain_i_msds.test \
	tests/explain_k_msds.test \
	tests/explain_msds_i.test \
	tests/explain_msds_k.test \
	tests/explain_p_msds_i.test

###############################################################################

#
# Embedded C tests
# ================
#
TESTS+=	tests/xec_explain_a.test \
	tests/xec_explain_ha.test \
	tests/xec_explain_hr.test \
	tests/xec_explain_la.test \
	tests/xec_explain_lr.test \
	tests/xec_explain_r.test \
	tests/xec_explain_ta.test \
	tests/xec_explain_tr.test \
	tests/xec_explain_tua.test \
	tests/xec_explain_ua.test \
	tests/xec_explain_uhta.test \
	tests/xec_explain_uhtr.test \
	tests/xec_explain_ur.test \
	tests/xec_explain_uta.test

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
TESTS+=	tests/xupc_declare_a2di.test \
	tests/xupc_declare_dpi.test \
	tests/xupc_declare_pdi.test \
	tests/xupc_declare_ptdi.test \
	tests/xupc_declare_rdi.test \
	tests/xupc_declare_rdpi.test \
	tests/xupc_declare_tdi.test \
	tests/xupc_explain_da2i.test \
	tests/xupc_explain_dpi.test \
	tests/xupc_explain_pdi.test \
	tests/xupc_explain_ptdi.test \
	tests/xupc_explain_rdi.test \
	tests/xupc_explain_rdpi.test \
	tests/xupc_explain_tdi.test

#
# Unified Parallel C error tests
# ==============================
#
TESTS+=	tests/xupc_explain_ri.test

###############################################################################

#
# Miscellaneous tests
# ===================
#
TESTS+=	tests/help-c++.test \
	tests/help-c.test \
	tests/help_command-c++.test \
	tests/help_command-c++03.test \
	tests/help_command-c.test \
	tests/help_commands-c++.test \
	tests/help_commands-c.test \
	tests/help_english-c++.test \
	tests/help_english-c++03.test \
	tests/help_english-c.test \
	tests/help_english-c89.test \
	tests/help_english-knr.test \
	tests/help_options.test \
	tests/set.test \
	tests/set_-e-a--st-.test \
	tests/set_alt-tokens.test \
	tests/set_c++.test \
	tests/set_c++03.test \
	tests/set_c++11.test \
	tests/set_c++14.test \
	tests/set_c++17.test \
	tests/set_c++20.test \
	tests/set_c++23.test \
	tests/set_c++98.test \
	tests/set_c.test \
	tests/set_c11.test \
	tests/set_c17.test \
	tests/set_c18.test \
	tests/set_c89.test \
	tests/set_c90.test \
	tests/set_c95.test \
	tests/set_c99.test \
	tests/set_cknr.test \
	tests/set_digraphs-c89.test \
	tests/set_digraphs.test \
	tests/set_digraphs_c89.test \
	tests/set_ea.test \
	tests/set_east-const.test \
	tests/set_east.test \
	tests/set_eastconst.test \
	tests/set_expl.test \
	tests/set_explain-by-default.test \
	tests/set_explain.test \
	tests/set_expli.test \
	tests/set_explicit-ecsu.test \
	tests/set_explicit-ecsu=ecsu.test \
	tests/set_explicit-ecsu=x.test \
	tests/set_explicit-int.test \
	tests/set_explicit-int=i.test \
	tests/set_explicit-int=u.test \
	tests/set_explicit-int=x.test \
	tests/set_graphs.test \
	tests/set_knr.test \
	tests/set_knrc.test \
	tests/set_lang=c++.test \
	tests/set_lang=xxx.test \
	tests/set_nodigraphs.test \
	tests/set_noexplain-by-default.test \
	tests/set_noexplain.test \
	tests/set_noexplicit-int.test \
	tests/set_noexplicit-int=u.test \
	tests/set_nographs.test \
	tests/set_noprompt.test \
	tests/set_nosemicolon.test \
	tests/set_options.test \
	tests/set_prompt.test \
	tests/set_prompt=x.test \
	tests/set_semicolon.test \
	tests/set_trigraphs-c++.test \
	tests/set_trigraphs-cknr.test \
	tests/set_trigraphs.test \
	tests/set_trigraphs_c++17.test \
	tests/set_xxx.test

if ENABLE_CDECL_DEBUG
TESTS+=	tests/set_debug.test \
	tests/set_nodebug.test
endif

if ENABLE_BISON_DEBUG
TESTS+=	tests/set_bison-debug.test \
	tests/set_nobison-debug.test
endif

###############################################################################

BUILD_SRC = $(top_builddir)/src
AM_TESTS_ENVIRONMENT = BUILD_SRC=$(BUILD_SRC); export BUILD_SRC ;
TEST_EXTENSIONS = .test 

TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests data expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

UPDATE_TEST = $(srcdir)/update_test.sh
update:
	@ls $(TESTS) | xargs -n255 -r $(UPDATE_TEST) -s $(BUILD_SRC)

# vim:set noet sw=8 ts=8:
