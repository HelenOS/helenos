/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ia64_SAL_H_
#define BOOT_ia64_SAL_H_

#include <arch/types.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Essential SAL procedures' IDs
 */
#define SAL_FREQ_BASE	0x1000012

typedef struct {
	uint8_t signature[4];
	uint32_t total_length;
	uint16_t sal_revision;
	uint16_t entry_count;
	uint8_t checksum;
	uint8_t reserved1[7];
	uint16_t sal_a_version;
	uint16_t sal_b_version;
	uint8_t oem_id[32];
	uint8_t product_id[32];
	uint8_t reserved2[8];
} sal_system_table_header_t;

typedef enum {
	SSTT_ENTRYPOINT_DESC,
	SSTT_MEMORY_DESC,
	SSTT_PLATFORM_FEATURES_DESC,
	SSTT_TR_DESC,
	SSTT_PTC_COHERENCE_DOMAIN_DESC,
	SSTT_AP_WAKEUP_DESC
} sal_sst_type_t;

typedef struct {
	uint8_t type;
	uint8_t reserved1[7];
	uint64_t pal_proc;
	uint64_t sal_proc;
	uint64_t sal_proc_gp;
	uint8_t reserved2[16];
} sal_entrypoint_desc_t;

/* This descriptor is unused on Itanium systems. */
typedef struct {
	uint8_t type;
	uint8_t unused[31];
} sal_memory_desc_t;

typedef struct {
	uint8_t type;
	uint8_t features;
	uint8_t reserved[14];
} sal_platform_features_desc_t;

typedef struct {
	uint8_t type;
	uint8_t tr_type;
	uint8_t tr_number;
	uint8_t reserved1[5];
	uint64_t va;
	uint64_t psc;
	uint8_t reserved2[8];
} sal_tr_desc_t;

typedef struct {
	uint8_t type;
	uint8_t reserved[3];
	uint32_t coherence_domains;
	uint64_t coherence_domain_info;
} sal_ptc_coherence_domain_desc_t;

typedef struct {
	uint8_t type;
	uint8_t mechanism;
	uint8_t reserved[6];
	uint64_t vector;
} sal_ap_wakeup_desc_t;

extern void sal_system_table_parse(sal_system_table_header_t *);

extern uint64_t sal_base_clock_frequency(void);

#define sal_call_1_1(id, arg1, ret1) \
	sal_call((id), (arg1), 0, 0, 0, 0, 0, 0, (ret1), NULL, NULL)

extern uint64_t sal_call(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t, uint64_t *, uint64_t *, uint64_t *);

#endif
