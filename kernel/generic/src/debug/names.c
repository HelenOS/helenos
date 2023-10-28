/*
 * Copyright (c) 2023 Jiří Zárevúcky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <debug/names.h>
#include <debug/constants.h>
#include <stddef.h>

#define ENTRY(name) [name] = #name

static const char *const dw_access_names[] = {
	ENTRY(DW_ACCESS_public),
	ENTRY(DW_ACCESS_protected),
	ENTRY(DW_ACCESS_private),
};

_Static_assert(DW_ACCESS_MAX == DW_ACCESS_private + 1);

static const char *const dw_at_names[] = {
	ENTRY(DW_AT_sibling),
	ENTRY(DW_AT_location),
	ENTRY(DW_AT_name),
	ENTRY(DW_AT_reserved_0x04),
	ENTRY(DW_AT_fund_type),
	ENTRY(DW_AT_mod_fund_type),
	ENTRY(DW_AT_user_def_type),
	ENTRY(DW_AT_mod_u_d_type),
	ENTRY(DW_AT_ordering),
	ENTRY(DW_AT_subscr_data),
	ENTRY(DW_AT_byte_size),
	ENTRY(DW_AT_bit_offset),
	ENTRY(DW_AT_bit_size),
	ENTRY(DW_AT_reserved_0x0e),
	ENTRY(DW_AT_unused8),
	ENTRY(DW_AT_stmt_list),
	ENTRY(DW_AT_low_pc),
	ENTRY(DW_AT_high_pc),
	ENTRY(DW_AT_language),
	ENTRY(DW_AT_member),
	ENTRY(DW_AT_discr),
	ENTRY(DW_AT_discr_value),
	ENTRY(DW_AT_visibility),
	ENTRY(DW_AT_import),
	ENTRY(DW_AT_string_length),
	ENTRY(DW_AT_common_reference),
	ENTRY(DW_AT_comp_dir),
	ENTRY(DW_AT_const_value),
	ENTRY(DW_AT_containing_type),
	ENTRY(DW_AT_default_value),
	ENTRY(DW_AT_reserved_0x1f),
	ENTRY(DW_AT_inline),
	ENTRY(DW_AT_is_optional),
	ENTRY(DW_AT_lower_bound),
	ENTRY(DW_AT_reserved_0x23),
	ENTRY(DW_AT_reserved_0x24),
	ENTRY(DW_AT_producer),
	ENTRY(DW_AT_reserved_0x26),
	ENTRY(DW_AT_prototyped),
	ENTRY(DW_AT_reserved_0x28),
	ENTRY(DW_AT_reserved_0x29),
	ENTRY(DW_AT_return_addr),
	ENTRY(DW_AT_reserved_0x2b),
	ENTRY(DW_AT_start_scope),
	ENTRY(DW_AT_reserved_0x2d),
	ENTRY(DW_AT_bit_stride),
	ENTRY(DW_AT_upper_bound),
	ENTRY(DW_AT_virtual),
	ENTRY(DW_AT_abstract_origin),
	ENTRY(DW_AT_accessibility),
	ENTRY(DW_AT_address_class),
	ENTRY(DW_AT_artificial),
	ENTRY(DW_AT_base_types),
	ENTRY(DW_AT_calling_convention),
	ENTRY(DW_AT_count),
	ENTRY(DW_AT_data_member_location),
	ENTRY(DW_AT_decl_column),
	ENTRY(DW_AT_decl_file),
	ENTRY(DW_AT_decl_line),
	ENTRY(DW_AT_declaration),
	ENTRY(DW_AT_discr_list),
	ENTRY(DW_AT_encoding),
	ENTRY(DW_AT_external),
	ENTRY(DW_AT_frame_base),
	ENTRY(DW_AT_friend),
	ENTRY(DW_AT_identifier_case),
	ENTRY(DW_AT_macro_info),
	ENTRY(DW_AT_namelist_item),
	ENTRY(DW_AT_priority),
	ENTRY(DW_AT_segment),
	ENTRY(DW_AT_specification),
	ENTRY(DW_AT_static_link),
	ENTRY(DW_AT_type),
	ENTRY(DW_AT_use_location),
	ENTRY(DW_AT_variable_parameter),
	ENTRY(DW_AT_virtuality),
	ENTRY(DW_AT_vtable_elem_location),
	ENTRY(DW_AT_allocated),
	ENTRY(DW_AT_associated),
	ENTRY(DW_AT_data_location),
	ENTRY(DW_AT_byte_stride),
	ENTRY(DW_AT_entry_pc),
	ENTRY(DW_AT_use_UTF8),
	ENTRY(DW_AT_extension),
	ENTRY(DW_AT_ranges),
	ENTRY(DW_AT_trampoline),
	ENTRY(DW_AT_call_column),
	ENTRY(DW_AT_call_file),
	ENTRY(DW_AT_call_line),
	ENTRY(DW_AT_description),
	ENTRY(DW_AT_binary_scale),
	ENTRY(DW_AT_decimal_scale),
	ENTRY(DW_AT_small),
	ENTRY(DW_AT_decimal_sign),
	ENTRY(DW_AT_digit_count),
	ENTRY(DW_AT_picture_string),
	ENTRY(DW_AT_mutable),
	ENTRY(DW_AT_threads_scaled),
	ENTRY(DW_AT_explicit),
	ENTRY(DW_AT_object_pointer),
	ENTRY(DW_AT_endianity),
	ENTRY(DW_AT_elemental),
	ENTRY(DW_AT_pure),
	ENTRY(DW_AT_recursive),
	ENTRY(DW_AT_signature),
	ENTRY(DW_AT_main_subprogram),
	ENTRY(DW_AT_data_bit_offset),
	ENTRY(DW_AT_const_expr),
	ENTRY(DW_AT_enum_class),
	ENTRY(DW_AT_linkage_name),
	ENTRY(DW_AT_string_length_bit_size),
	ENTRY(DW_AT_string_length_byte_size),
	ENTRY(DW_AT_rank),
	ENTRY(DW_AT_str_offsets_base),
	ENTRY(DW_AT_addr_base),
	ENTRY(DW_AT_rnglists_base),
	ENTRY(DW_AT_dwo_id),
	ENTRY(DW_AT_dwo_name),
	ENTRY(DW_AT_reference),
	ENTRY(DW_AT_rvalue_reference),
	ENTRY(DW_AT_macros),
	ENTRY(DW_AT_call_all_calls),
	ENTRY(DW_AT_call_all_source_calls),
	ENTRY(DW_AT_call_all_tail_calls),
	ENTRY(DW_AT_call_return_pc),
	ENTRY(DW_AT_call_value),
	ENTRY(DW_AT_call_origin),
	ENTRY(DW_AT_call_parameter),
	ENTRY(DW_AT_call_pc),
	ENTRY(DW_AT_call_tail_call),
	ENTRY(DW_AT_call_target),
	ENTRY(DW_AT_call_target_clobbered),
	ENTRY(DW_AT_call_data_location),
	ENTRY(DW_AT_call_data_value),
	ENTRY(DW_AT_noreturn),
	ENTRY(DW_AT_alignment),
	ENTRY(DW_AT_export_symbols),
	ENTRY(DW_AT_deleted),
	ENTRY(DW_AT_defaulted),
	ENTRY(DW_AT_loclists_base),
};

_Static_assert(DW_AT_MAX == DW_AT_loclists_base + 1);

static const char *const dw_ate_names[] = {
	ENTRY(DW_ATE_address),
	ENTRY(DW_ATE_boolean),
	ENTRY(DW_ATE_complex_float),
	ENTRY(DW_ATE_float),
	ENTRY(DW_ATE_signed),
	ENTRY(DW_ATE_signed_char),
	ENTRY(DW_ATE_unsigned),
	ENTRY(DW_ATE_unsigned_char),
	ENTRY(DW_ATE_imaginary_float),
	ENTRY(DW_ATE_packed_decimal),
	ENTRY(DW_ATE_numeric_string),
	ENTRY(DW_ATE_edited),
	ENTRY(DW_ATE_signed_fixed),
	ENTRY(DW_ATE_unsigned_fixed),
	ENTRY(DW_ATE_decimal_float),
	ENTRY(DW_ATE_UTF),
	ENTRY(DW_ATE_UCS),
	ENTRY(DW_ATE_ASCII),
};

_Static_assert(DW_ATE_MAX == DW_ATE_ASCII + 1);

static const char *const dw_cc_names[] = {
	ENTRY(DW_CC_normal),
	ENTRY(DW_CC_program),
	ENTRY(DW_CC_nocall),
	ENTRY(DW_CC_pass_by_reference),
	ENTRY(DW_CC_pass_by_value),
};

_Static_assert(DW_CC_MAX == DW_CC_pass_by_value + 1);

static const char *const dw_ut_names[] = {
	ENTRY(DW_UT_compile),
	ENTRY(DW_UT_type),
	ENTRY(DW_UT_partial),
	ENTRY(DW_UT_skeleton),
	ENTRY(DW_UT_split_compile),
	ENTRY(DW_UT_split_type),
};

_Static_assert(DW_UT_MAX == DW_UT_split_type + 1);

static const char *const dw_tag_names[] = {
	ENTRY(DW_TAG_array_type),
	ENTRY(DW_TAG_class_type),
	ENTRY(DW_TAG_entry_point),
	ENTRY(DW_TAG_enumeration_type),
	ENTRY(DW_TAG_formal_parameter),
	ENTRY(DW_TAG_global_subroutine),
	ENTRY(DW_TAG_global_variable),
	ENTRY(DW_TAG_imported_declaration),
	ENTRY(DW_TAG_reserved_0x09),
	ENTRY(DW_TAG_label),
	ENTRY(DW_TAG_lexical_block),
	ENTRY(DW_TAG_local_variable),
	ENTRY(DW_TAG_member),
	ENTRY(DW_TAG_reserved_0x0e),
	ENTRY(DW_TAG_pointer_type),
	ENTRY(DW_TAG_reference_type),
	ENTRY(DW_TAG_compile_unit),
	ENTRY(DW_TAG_string_type),
	ENTRY(DW_TAG_structure_type),
	ENTRY(DW_TAG_subroutine),
	ENTRY(DW_TAG_subroutine_type),
	ENTRY(DW_TAG_typedef),
	ENTRY(DW_TAG_union_type),
	ENTRY(DW_TAG_unspecified_parameters),
	ENTRY(DW_TAG_variant),
	ENTRY(DW_TAG_common_block),
	ENTRY(DW_TAG_common_inclusion),
	ENTRY(DW_TAG_inheritance),
	ENTRY(DW_TAG_inlined_subroutine),
	ENTRY(DW_TAG_module),
	ENTRY(DW_TAG_ptr_to_member_type),
	ENTRY(DW_TAG_set_type),
	ENTRY(DW_TAG_subrange_type),
	ENTRY(DW_TAG_with_stmt),
	ENTRY(DW_TAG_access_declaration),
	ENTRY(DW_TAG_base_type),
	ENTRY(DW_TAG_catch_block),
	ENTRY(DW_TAG_const_type),
	ENTRY(DW_TAG_constant),
	ENTRY(DW_TAG_enumerator),
	ENTRY(DW_TAG_file_type),
	ENTRY(DW_TAG_friend),
	ENTRY(DW_TAG_namelist),
	ENTRY(DW_TAG_namelist_item),
	ENTRY(DW_TAG_packed_type),
	ENTRY(DW_TAG_subprogram),
	ENTRY(DW_TAG_template_type_parameter),
	ENTRY(DW_TAG_template_value_parameter),
	ENTRY(DW_TAG_thrown_type),
	ENTRY(DW_TAG_try_block),
	ENTRY(DW_TAG_variant_part),
	ENTRY(DW_TAG_variable),
	ENTRY(DW_TAG_volatile_type),
	ENTRY(DW_TAG_dwarf_procedure),
	ENTRY(DW_TAG_restrict_type),
	ENTRY(DW_TAG_interface_type),
	ENTRY(DW_TAG_namespace),
	ENTRY(DW_TAG_imported_module),
	ENTRY(DW_TAG_unspecified_type),
	ENTRY(DW_TAG_partial_unit),
	ENTRY(DW_TAG_imported_unit),
	ENTRY(DW_TAG_mutable_type),
	ENTRY(DW_TAG_condition),
	ENTRY(DW_TAG_shared_type),
	ENTRY(DW_TAG_type_unit),
	ENTRY(DW_TAG_rvalue_reference_type),
	ENTRY(DW_TAG_template_alias),
	ENTRY(DW_TAG_coarray_type),
	ENTRY(DW_TAG_generic_subrange),
	ENTRY(DW_TAG_dynamic_type),
	ENTRY(DW_TAG_atomic_type),
	ENTRY(DW_TAG_call_site),
	ENTRY(DW_TAG_call_site_parameter),
	ENTRY(DW_TAG_skeleton_unit),
	ENTRY(DW_TAG_immutable_type),
};

_Static_assert(DW_TAG_MAX == DW_TAG_immutable_type + 1);

static const char *const dw_form_names[] = {
	ENTRY(DW_FORM_addr),
	ENTRY(DW_FORM_reserved_0x02),
	ENTRY(DW_FORM_block2),
	ENTRY(DW_FORM_block4),
	ENTRY(DW_FORM_data2),
	ENTRY(DW_FORM_data4),
	ENTRY(DW_FORM_data8),
	ENTRY(DW_FORM_string),
	ENTRY(DW_FORM_block),
	ENTRY(DW_FORM_block1),
	ENTRY(DW_FORM_data1),
	ENTRY(DW_FORM_flag),
	ENTRY(DW_FORM_sdata),
	ENTRY(DW_FORM_strp),
	ENTRY(DW_FORM_udata),
	ENTRY(DW_FORM_ref_addr),
	ENTRY(DW_FORM_ref1),
	ENTRY(DW_FORM_ref2),
	ENTRY(DW_FORM_ref4),
	ENTRY(DW_FORM_ref8),
	ENTRY(DW_FORM_ref_udata),
	ENTRY(DW_FORM_indirect),
	ENTRY(DW_FORM_sec_offset),
	ENTRY(DW_FORM_exprloc),
	ENTRY(DW_FORM_flag_present),
	ENTRY(DW_FORM_strx),
	ENTRY(DW_FORM_addrx),
	ENTRY(DW_FORM_ref_sup4),
	ENTRY(DW_FORM_strp_sup),
	ENTRY(DW_FORM_data16),
	ENTRY(DW_FORM_line_strp),
	ENTRY(DW_FORM_ref_sig8),
	ENTRY(DW_FORM_implicit_const),
	ENTRY(DW_FORM_loclistx),
	ENTRY(DW_FORM_rnglistx),
	ENTRY(DW_FORM_ref_sup8),
	ENTRY(DW_FORM_strx1),
	ENTRY(DW_FORM_strx2),
	ENTRY(DW_FORM_strx3),
	ENTRY(DW_FORM_strx4),
	ENTRY(DW_FORM_addrx1),
	ENTRY(DW_FORM_addrx2),
	ENTRY(DW_FORM_addrx3),
	ENTRY(DW_FORM_addrx4),
};

_Static_assert(DW_FORM_MAX == DW_FORM_addrx4 + 1);

static const char *const dw_op_names[] = {
	ENTRY(DW_OP_reg),
	ENTRY(DW_OP_basereg),
	ENTRY(DW_OP_addr),
	ENTRY(DW_OP_const),
	ENTRY(DW_OP_deref2),
	ENTRY(DW_OP_deref),
	ENTRY(DW_OP_add),
	ENTRY(DW_OP_const1u),
	ENTRY(DW_OP_const1s),
	ENTRY(DW_OP_const2u),
	ENTRY(DW_OP_const2s),
	ENTRY(DW_OP_const4u),
	ENTRY(DW_OP_const4s),
	ENTRY(DW_OP_const8u),
	ENTRY(DW_OP_const8s),
	ENTRY(DW_OP_constu),
	ENTRY(DW_OP_consts),
	ENTRY(DW_OP_dup),
	ENTRY(DW_OP_drop),
	ENTRY(DW_OP_over),
	ENTRY(DW_OP_pick),
	ENTRY(DW_OP_swap),
	ENTRY(DW_OP_rot),
	ENTRY(DW_OP_xderef),
	ENTRY(DW_OP_abs),
	ENTRY(DW_OP_and),
	ENTRY(DW_OP_div),
	ENTRY(DW_OP_minus),
	ENTRY(DW_OP_mod),
	ENTRY(DW_OP_mul),
	ENTRY(DW_OP_neg),
	ENTRY(DW_OP_not),
	ENTRY(DW_OP_or),
	ENTRY(DW_OP_plus),
	ENTRY(DW_OP_plus_uconst),
	ENTRY(DW_OP_shl),
	ENTRY(DW_OP_shr),
	ENTRY(DW_OP_shra),
	ENTRY(DW_OP_xor),
	ENTRY(DW_OP_bra),
	ENTRY(DW_OP_eq),
	ENTRY(DW_OP_ge),
	ENTRY(DW_OP_gt),
	ENTRY(DW_OP_le),
	ENTRY(DW_OP_lt),
	ENTRY(DW_OP_ne),
	ENTRY(DW_OP_skip),
	ENTRY(DW_OP_lit0),
	ENTRY(DW_OP_lit1),
	ENTRY(DW_OP_lit2),
	ENTRY(DW_OP_lit3),
	ENTRY(DW_OP_lit4),
	ENTRY(DW_OP_lit5),
	ENTRY(DW_OP_lit6),
	ENTRY(DW_OP_lit7),
	ENTRY(DW_OP_lit8),
	ENTRY(DW_OP_lit9),
	ENTRY(DW_OP_lit10),
	ENTRY(DW_OP_lit11),
	ENTRY(DW_OP_lit12),
	ENTRY(DW_OP_lit13),
	ENTRY(DW_OP_lit14),
	ENTRY(DW_OP_lit15),
	ENTRY(DW_OP_lit16),
	ENTRY(DW_OP_lit17),
	ENTRY(DW_OP_lit18),
	ENTRY(DW_OP_lit19),
	ENTRY(DW_OP_lit20),
	ENTRY(DW_OP_lit21),
	ENTRY(DW_OP_lit22),
	ENTRY(DW_OP_lit23),
	ENTRY(DW_OP_lit24),
	ENTRY(DW_OP_lit25),
	ENTRY(DW_OP_lit26),
	ENTRY(DW_OP_lit27),
	ENTRY(DW_OP_lit28),
	ENTRY(DW_OP_lit29),
	ENTRY(DW_OP_lit30),
	ENTRY(DW_OP_lit31),
	ENTRY(DW_OP_reg0),
	ENTRY(DW_OP_reg1),
	ENTRY(DW_OP_reg2),
	ENTRY(DW_OP_reg3),
	ENTRY(DW_OP_reg4),
	ENTRY(DW_OP_reg5),
	ENTRY(DW_OP_reg6),
	ENTRY(DW_OP_reg7),
	ENTRY(DW_OP_reg8),
	ENTRY(DW_OP_reg9),
	ENTRY(DW_OP_reg10),
	ENTRY(DW_OP_reg11),
	ENTRY(DW_OP_reg12),
	ENTRY(DW_OP_reg13),
	ENTRY(DW_OP_reg14),
	ENTRY(DW_OP_reg15),
	ENTRY(DW_OP_reg16),
	ENTRY(DW_OP_reg17),
	ENTRY(DW_OP_reg18),
	ENTRY(DW_OP_reg19),
	ENTRY(DW_OP_reg20),
	ENTRY(DW_OP_reg21),
	ENTRY(DW_OP_reg22),
	ENTRY(DW_OP_reg23),
	ENTRY(DW_OP_reg24),
	ENTRY(DW_OP_reg25),
	ENTRY(DW_OP_reg26),
	ENTRY(DW_OP_reg27),
	ENTRY(DW_OP_reg28),
	ENTRY(DW_OP_reg29),
	ENTRY(DW_OP_reg30),
	ENTRY(DW_OP_reg31),
	ENTRY(DW_OP_breg0),
	ENTRY(DW_OP_breg1),
	ENTRY(DW_OP_breg2),
	ENTRY(DW_OP_breg3),
	ENTRY(DW_OP_breg4),
	ENTRY(DW_OP_breg5),
	ENTRY(DW_OP_breg6),
	ENTRY(DW_OP_breg7),
	ENTRY(DW_OP_breg8),
	ENTRY(DW_OP_breg9),
	ENTRY(DW_OP_breg10),
	ENTRY(DW_OP_breg11),
	ENTRY(DW_OP_breg12),
	ENTRY(DW_OP_breg13),
	ENTRY(DW_OP_breg14),
	ENTRY(DW_OP_breg15),
	ENTRY(DW_OP_breg16),
	ENTRY(DW_OP_breg17),
	ENTRY(DW_OP_breg18),
	ENTRY(DW_OP_breg19),
	ENTRY(DW_OP_breg20),
	ENTRY(DW_OP_breg21),
	ENTRY(DW_OP_breg22),
	ENTRY(DW_OP_breg23),
	ENTRY(DW_OP_breg24),
	ENTRY(DW_OP_breg25),
	ENTRY(DW_OP_breg26),
	ENTRY(DW_OP_breg27),
	ENTRY(DW_OP_breg28),
	ENTRY(DW_OP_breg29),
	ENTRY(DW_OP_breg30),
	ENTRY(DW_OP_breg31),
	ENTRY(DW_OP_regx),
	ENTRY(DW_OP_fbreg),
	ENTRY(DW_OP_bregx),
	ENTRY(DW_OP_piece),
	ENTRY(DW_OP_deref_size),
	ENTRY(DW_OP_xderef_size),
	ENTRY(DW_OP_nop),
	ENTRY(DW_OP_push_object_address),
	ENTRY(DW_OP_call2),
	ENTRY(DW_OP_call4),
	ENTRY(DW_OP_call_ref),
	ENTRY(DW_OP_form_tls_address),
	ENTRY(DW_OP_call_frame_cfa),
	ENTRY(DW_OP_bit_piece),
	ENTRY(DW_OP_implicit_value),
	ENTRY(DW_OP_stack_value),
	ENTRY(DW_OP_implicit_pointer),
	ENTRY(DW_OP_addrx),
	ENTRY(DW_OP_constx),
	ENTRY(DW_OP_entry_value),
	ENTRY(DW_OP_const_type),
	ENTRY(DW_OP_regval_type),
	ENTRY(DW_OP_deref_type),
	ENTRY(DW_OP_xderef_type),
	ENTRY(DW_OP_convert),
	ENTRY(DW_OP_reinterpret),
};

_Static_assert(DW_OP_MAX == DW_OP_reinterpret + 1);

static const char *const dw_lle_names[] = {
	ENTRY(DW_LLE_end_of_list),
	ENTRY(DW_LLE_base_addressx),
	ENTRY(DW_LLE_startx_endx),
	ENTRY(DW_LLE_startx_length),
	ENTRY(DW_LLE_offset_pair),
	ENTRY(DW_LLE_default_location),
	ENTRY(DW_LLE_base_address),
	ENTRY(DW_LLE_start_end),
	ENTRY(DW_LLE_start_length),
};

_Static_assert(DW_LLE_MAX == DW_LLE_start_length + 1);

static const char *const dw_ds_names[] = {
	ENTRY(DW_DS_unsigned),
	ENTRY(DW_DS_leading_overpunch),
	ENTRY(DW_DS_trailing_overpunch),
	ENTRY(DW_DS_leading_separate),
	ENTRY(DW_DS_trailing_separate),
};

_Static_assert(DW_DS_MAX == DW_DS_trailing_separate + 1);

static const char *const dw_end_names[] = {
	ENTRY(DW_END_default),
	ENTRY(DW_END_big),
	ENTRY(DW_END_little),
};

_Static_assert(DW_END_MAX == DW_END_little + 1);

static const char *const dw_vis_names[] = {
	ENTRY(DW_VIS_local),
	ENTRY(DW_VIS_exported),
	ENTRY(DW_VIS_qualified),
};

_Static_assert(DW_VIS_MAX == DW_VIS_qualified + 1);

static const char *const dw_virtuality_names[] = {
	ENTRY(DW_VIRTUALITY_none),
	ENTRY(DW_VIRTUALITY_virtual),
	ENTRY(DW_VIRTUALITY_pure_virtual),
};

_Static_assert(DW_VIRTUALITY_MAX == DW_VIRTUALITY_pure_virtual + 1);

static const char *const dw_lang_names[] = {
	ENTRY(DW_LANG_C89),
	ENTRY(DW_LANG_C),
	ENTRY(DW_LANG_Ada83),
	ENTRY(DW_LANG_C_plus_plus),
	ENTRY(DW_LANG_Cobol74),
	ENTRY(DW_LANG_Cobol85),
	ENTRY(DW_LANG_Fortran77),
	ENTRY(DW_LANG_Fortran90),
	ENTRY(DW_LANG_Pascal83),
	ENTRY(DW_LANG_Modula2),
	ENTRY(DW_LANG_Java),
	ENTRY(DW_LANG_C99),
	ENTRY(DW_LANG_Ada95),
	ENTRY(DW_LANG_Fortran95),
	ENTRY(DW_LANG_PLI),
	ENTRY(DW_LANG_ObjC),
	ENTRY(DW_LANG_ObjC_plus_plus),
	ENTRY(DW_LANG_UPC),
	ENTRY(DW_LANG_D),
	ENTRY(DW_LANG_Python),
	ENTRY(DW_LANG_OpenCL),
	ENTRY(DW_LANG_Go),
	ENTRY(DW_LANG_Modula3),
	ENTRY(DW_LANG_Haskell),
	ENTRY(DW_LANG_C_plus_plus_03),
	ENTRY(DW_LANG_C_plus_plus_11),
	ENTRY(DW_LANG_OCaml),
	ENTRY(DW_LANG_Rust),
	ENTRY(DW_LANG_C11),
	ENTRY(DW_LANG_Swift),
	ENTRY(DW_LANG_Julia),
	ENTRY(DW_LANG_Dylan),
	ENTRY(DW_LANG_C_plus_plus_14),
	ENTRY(DW_LANG_Fortran03),
	ENTRY(DW_LANG_Fortran08),
	ENTRY(DW_LANG_RenderScript),
	ENTRY(DW_LANG_BLISS),
};

_Static_assert(DW_LANG_MAX == DW_LANG_BLISS + 1);

static const char *const dw_id_names[] = {
	ENTRY(DW_ID_case_sensitive),
	ENTRY(DW_ID_up_case),
	ENTRY(DW_ID_down_case),
	ENTRY(DW_ID_case_insensitive),
};

_Static_assert(DW_ID_MAX == DW_ID_case_insensitive + 1);

static const char *const dw_lns_names[] = {
	ENTRY(DW_LNS_copy),
	ENTRY(DW_LNS_advance_pc),
	ENTRY(DW_LNS_advance_line),
	ENTRY(DW_LNS_set_file),
	ENTRY(DW_LNS_set_column),
	ENTRY(DW_LNS_negate_stmt),
	ENTRY(DW_LNS_set_basic_block),
	ENTRY(DW_LNS_const_add_pc),
	ENTRY(DW_LNS_fixed_advance_pc),
	ENTRY(DW_LNS_set_prologue_end),
	ENTRY(DW_LNS_set_epilogue_begin),
	ENTRY(DW_LNS_set_isa),
};

_Static_assert(DW_LNS_MAX == DW_LNS_set_isa + 1);

static const char *const dw_lne_names[] = {
	ENTRY(DW_LNE_end_sequence),
	ENTRY(DW_LNE_set_address),
	ENTRY(DW_LNE_define_file),
	ENTRY(DW_LNE_set_discriminator),
};

_Static_assert(DW_LNE_MAX == DW_LNE_set_discriminator + 1);

static const char *const dw_lnct_names[] = {
	ENTRY(DW_LNCT_path),
	ENTRY(DW_LNCT_directory_index),
	ENTRY(DW_LNCT_timestamp),
	ENTRY(DW_LNCT_size),
	ENTRY(DW_LNCT_MD5),
};

_Static_assert(DW_LNCT_MAX == DW_LNCT_MD5 + 1);

static const char *const dw_inl_names[] = {
	ENTRY(DW_INL_not_inlined),
	ENTRY(DW_INL_inlined),
	ENTRY(DW_INL_declared_not_inlined),
	ENTRY(DW_INL_declared_inlined),
};

_Static_assert(DW_INL_MAX == DW_INL_declared_inlined + 1);

static const char *const dw_ord_names[] = {
	ENTRY(DW_ORD_row_major),
	ENTRY(DW_ORD_col_major),
};

_Static_assert(DW_ORD_MAX == DW_ORD_col_major + 1);

static const char *const dw_dsc_names[] = {
	ENTRY(DW_DSC_label),
	ENTRY(DW_DSC_range),
};

_Static_assert(DW_DSC_MAX == DW_DSC_range + 1);

static const char *const dw_idx_names[] = {
	ENTRY(DW_IDX_compile_unit),
	ENTRY(DW_IDX_type_unit),
	ENTRY(DW_IDX_die_offset),
	ENTRY(DW_IDX_parent),
	ENTRY(DW_IDX_type_hash),
};

_Static_assert(DW_IDX_MAX == DW_IDX_type_hash + 1);

static const char *const dw_defaulted_names[] = {
	ENTRY(DW_DEFAULTED_no),
	ENTRY(DW_DEFAULTED_in_class),
	ENTRY(DW_DEFAULTED_out_of_class),
};

_Static_assert(DW_DEFAULTED_MAX == DW_DEFAULTED_out_of_class + 1);

static const char *const dw_macro_names[] = {
	ENTRY(DW_MACRO_define),
	ENTRY(DW_MACRO_undef),
	ENTRY(DW_MACRO_start_file),
	ENTRY(DW_MACRO_end_file),
	ENTRY(DW_MACRO_define_strp),
	ENTRY(DW_MACRO_undef_strp),
	ENTRY(DW_MACRO_import),
	ENTRY(DW_MACRO_define_sup),
	ENTRY(DW_MACRO_undef_sup),
	ENTRY(DW_MACRO_import_sup),
	ENTRY(DW_MACRO_define_strx),
	ENTRY(DW_MACRO_undef_strx),
};

_Static_assert(DW_MACRO_MAX == DW_MACRO_undef_strx + 1);

static const char *const dw_cfa_names[] = {
	ENTRY(DW_CFA_nop),
	ENTRY(DW_CFA_set_loc),
	ENTRY(DW_CFA_advance_loc1),
	ENTRY(DW_CFA_advance_loc2),
	ENTRY(DW_CFA_advance_loc4),
	ENTRY(DW_CFA_offset_extended),
	ENTRY(DW_CFA_restore_extended),
	ENTRY(DW_CFA_undefined),
	ENTRY(DW_CFA_same_value),
	ENTRY(DW_CFA_register),
	ENTRY(DW_CFA_remember_state),
	ENTRY(DW_CFA_restore_state),
	ENTRY(DW_CFA_def_cfa),
	ENTRY(DW_CFA_def_cfa_register),
	ENTRY(DW_CFA_def_cfa_offset),
	ENTRY(DW_CFA_def_cfa_expression),
	ENTRY(DW_CFA_expression),
	ENTRY(DW_CFA_offset_extended_sf),
	ENTRY(DW_CFA_def_cfa_sf),
	ENTRY(DW_CFA_def_cfa_offset_sf),
	ENTRY(DW_CFA_val_offset),
	ENTRY(DW_CFA_val_offset_sf),
	ENTRY(DW_CFA_val_expression),
};

_Static_assert(DW_CFA_MAX == DW_CFA_val_expression + 1);

static const char *const dw_rle_names[] = {
	ENTRY(DW_RLE_end_of_list),
	ENTRY(DW_RLE_base_addressx),
	ENTRY(DW_RLE_startx_endx),
	ENTRY(DW_RLE_startx_length),
	ENTRY(DW_RLE_offset_pair),
	ENTRY(DW_RLE_base_address),
	ENTRY(DW_RLE_start_end),
	ENTRY(DW_RLE_start_length),
};

_Static_assert(DW_RLE_MAX == DW_RLE_start_length + 1);

#define NAME_FUNC(prefix) \
	const char *prefix##_name(prefix##_t val) \
	{ \
		if (val >= sizeof(prefix##_names) / sizeof(const char *)) return NULL; \
		return prefix##_names[val]; \
	}

NAME_FUNC(dw_access);
NAME_FUNC(dw_at);
NAME_FUNC(dw_ate);
NAME_FUNC(dw_cc);
NAME_FUNC(dw_cfa);
NAME_FUNC(dw_defaulted);
NAME_FUNC(dw_ds);
NAME_FUNC(dw_dsc);
NAME_FUNC(dw_end);
NAME_FUNC(dw_form);
NAME_FUNC(dw_id);
NAME_FUNC(dw_idx);
NAME_FUNC(dw_inl);
NAME_FUNC(dw_lang);
NAME_FUNC(dw_lle);
NAME_FUNC(dw_lnct);
NAME_FUNC(dw_lne);
NAME_FUNC(dw_lns);
NAME_FUNC(dw_macro);
NAME_FUNC(dw_op);
NAME_FUNC(dw_ord);
NAME_FUNC(dw_rle);
NAME_FUNC(dw_tag);
NAME_FUNC(dw_ut);
NAME_FUNC(dw_virtuality);
NAME_FUNC(dw_vis);
