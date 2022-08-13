/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/sal.h>
#include <arch/types.h>

static sal_ap_wakeup_desc_t *sal_ap_wakeup;

extern uint64_t pal_proc;

uint64_t sal_proc = 0;
uint64_t sal_proc_gp = 0;

void sal_system_table_parse(sal_system_table_header_t *sst)
{
	uint8_t *cur = (uint8_t *) &sst[1];
	uint16_t entry;

	for (entry = 0; entry < sst->entry_count; entry++) {
		switch ((sal_sst_type_t) *cur) {
		case SSTT_ENTRYPOINT_DESC:
			pal_proc = ((sal_entrypoint_desc_t *) cur)->pal_proc;
			sal_proc = ((sal_entrypoint_desc_t *) cur)->sal_proc;
			sal_proc_gp = ((sal_entrypoint_desc_t *) cur)->sal_proc_gp;
			cur += sizeof(sal_entrypoint_desc_t);
			break;
		case SSTT_MEMORY_DESC:
			cur += sizeof(sal_memory_desc_t);
			break;
		case SSTT_PLATFORM_FEATURES_DESC:
			cur += sizeof(sal_platform_features_desc_t);
			break;
		case SSTT_TR_DESC:
			cur += sizeof(sal_tr_desc_t);
			break;
		case SSTT_PTC_COHERENCE_DOMAIN_DESC:
			cur += sizeof(sal_ptc_coherence_domain_desc_t);
			break;
		case SSTT_AP_WAKEUP_DESC:
			sal_ap_wakeup = (sal_ap_wakeup_desc_t *) cur;
			cur += sizeof(sal_ap_wakeup_desc_t);
			break;
		default:
			return;
		}
	}
}

uint64_t sal_base_clock_frequency(void)
{
	uint64_t freq;

	sal_call_1_1(SAL_FREQ_BASE, 0, &freq);

	return freq;
}
