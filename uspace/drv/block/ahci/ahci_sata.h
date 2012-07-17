/*
 * Copyright (c) 2012 Petr Jerman
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

/** @file
 * Header for AHCI driver (SATA/ATA and related things).
 */

#ifndef __AHCI_SATA_H__
#define __AHCI_SATA_H__

#include <sys/types.h>

/** Standard Command frame. */
typedef struct {
	/** FIS type - always 0x27. */
	uint8_t fis_type;
	/** Indicate that FIS is a Command - always 0x80. */
	uint8_t c;
	/** Command - Identity device - 0xec, Set fetures - 0xef. */
	uint8_t command;
	/** Features - subcommand for set features - set tranfer mode - 0x03. */
	uint8_t features;
	/** 0:23 bits of LBA. */
	uint32_t lba_lower : 24;
	/** Device. */
	uint8_t device;
	/** 24:47 bits of LBA. */
	uint32_t lba_upper : 24;
	/** Features - subcommand for set features - set tranfer mode - 0x03. */
	uint8_t features_upper;
	/** Sector count - transfer mode for set transfer mode operation. */
	uint16_t count;
	/** Reserved. */
	uint8_t reserved1;
	/** Control. */
	uint8_t control;
	/** Reserved. */
	uint32_t reserved2;
} __attribute__((packed)) std_command_frame_t;

/** Command frame for NCQ data operation. */
typedef struct {
	/** FIS type - always 0x27. */
	uint8_t fis_type;
	/** Indicate that FIS is a Command - always 0x80. */
	uint8_t c;
	/** Command - FPDMA Read - 0x60, FPDMA Write - 0x61. */
	uint8_t command;
	/** bits 7:0 of sector count. */
	uint8_t sector_count_low;
	/** bits 7:0 of lba. */
	uint8_t lba0;
	/** bits 15:8 of lba. */
	uint8_t lba1;
	/** bits 23:16 of lba. */
	uint8_t lba2;
	uint8_t fua;
	/** bits 31:24 of lba. */
	uint8_t lba3;
	/** bits 39:32 of lba. */
	uint8_t lba4;
	/** bits 47:40 of lba. */
	uint8_t lba5;
	/** bits 15:8 of sector count. */
	uint8_t sector_count_high;
	/** Tag number of NCQ operation. */
	uint8_t tag;
	/** Reserved. */
	uint8_t reserved1;
	/** Reserved. */
	uint8_t reserved2;
	/** Control. */
	uint8_t control;
	/** Reserved. */
	uint8_t reserved3;
	/** Reserved. */
	uint8_t reserved4;
	/** Reserved. */
	uint8_t reserved5;
	/** Reserved. */
	uint8_t reserved6;
} __attribute__((packed)) ncq_command_frame_t;

/** Data returned from identify device and identify packet device command. */
typedef struct {
	uint16_t gen_conf;
	uint16_t cylinders;
	uint16_t reserved2;
	uint16_t heads;
	uint16_t _vs4;
	uint16_t _vs5;
	uint16_t sectors;
	uint16_t _vs7;
	uint16_t _vs8;
	uint16_t _vs9;
	
	uint16_t serial_number[10];
	uint16_t _vs20;
	uint16_t _vs21;
	uint16_t vs_bytes;
	uint16_t firmware_rev[4];
	uint16_t model_name[20];
	
	uint16_t max_rw_multiple;
	uint16_t reserved48;
	/** Different meaning for packet device. */
	uint16_t caps;
	uint16_t reserved50;
	uint16_t pio_timing;
	uint16_t dma_timing;
	
	uint16_t validity;
	uint16_t cur_cyl;
	uint16_t cur_heads;
	uint16_t cur_sectors;
	uint16_t cur_capacity0;
	uint16_t cur_capacity1;
	uint16_t mss;
	uint16_t total_lba28_0;
	uint16_t total_lba28_1;
	uint16_t sw_dma;
	uint16_t mw_dma;
	uint16_t pio_modes;
	uint16_t min_mw_dma_cycle;
	uint16_t rec_mw_dma_cycle;
	uint16_t min_raw_pio_cycle;
	uint16_t min_iordy_pio_cycle;
	
	uint16_t reserved69;
	uint16_t reserved70;
	uint16_t reserved71;
	uint16_t reserved72;
	uint16_t reserved73;
	uint16_t reserved74;
	
	uint16_t queue_depth;
	/** SATA capatibilities - different meaning for packet device. */
	uint16_t sata_cap;
	/** SATA additional capatibilities - different meaning for packet device. */
	uint16_t sata_cap2;
	uint16_t reserved78[1 + 79 - 78];
	uint16_t version_maj;
	uint16_t version_min;
	uint16_t cmd_set0;
	uint16_t cmd_set1;
	uint16_t csf_sup_ext;
	uint16_t csf_enabled0;
	uint16_t csf_enabled1;
	uint16_t csf_default;
	uint16_t udma;
	
	uint16_t reserved89[1 + 99 - 89];
	
	/* Total number of blocks in LBA-48 addressing. */
	uint16_t total_lba48_0;
	uint16_t total_lba48_1;
	uint16_t total_lba48_2;
	uint16_t total_lba48_3;
	
	/* Note: more fields are defined in ATA/ATAPI-7. */
	uint16_t reserved104[1 + 127 - 104];
	uint16_t _vs128[1 + 159 - 128];
	uint16_t reserved160[1 + 255 - 160];
} __attribute__((packed)) identify_data_t;

/** Capability bits for register device. */
enum ata_regdev_caps {
	rd_cap_iordy = 0x0800,
	rd_cap_iordy_cbd = 0x0400,
	rd_cap_lba = 0x0200,
	rd_cap_dma = 0x0100
};

/** Bits of @c identify_data_t.cmd_set1. */
enum ata_cs1 {
	/** 48-bit address feature set. */
	cs1_addr48 = 0x0400
};

/** SATA capatibilities for not packet device - Serial ATA revision 3_1. */
enum sata_np_cap {
	/** Supports READ LOG DMA EXT. */
	np_cap_log_ext = 0x8000,
	/** Supports Device Automatic Partial to Slumber transitions. */
	np_cap_dev_slm = 0x4000,
	/** Supports Host Automatic Partial to Slumber transitions. */
	np_cap_host_slm = 0x2000,
	/** Supports NCQ priority information. */
	np_cap_ncq_prio = 0x1000,
	/** Supports Unload while NCQ command outstanding. */
	np_cap_unload_ncq = 0x0800,
	/** Supports Phy event counters. */
	np_cap_phy_ctx = 0x0400,
	/** Supports recepits of host-initiated interface power management. */
	np_cap_host_pmngmnt = 0x0200,
	
	/** Supports NCQ. */
	np_cap_ncq = 0x0100,
	
	/** Supports SATA 3. */
	np_cap_sata_3 = 0x0008,
	/** Supports SATA 2. */
	np_cap_sata_2 = 0x0004,
	/** Supports SATA 1. */
	np_cap_sata_1 = 0x0002
};

/** SATA capatibilities for packet device - Serial ATA revision 3_1. */
enum sata_pt_cap {
	/** Supports READ LOG DMA EXT. */
	pt_cap_log_ext = 0x8000,
	/** Supports Device Automatic Partial to Slumber transitions. */
	pt_cap_dev_slm = 0x4000,
	/** Supports Host Automatic Partial to Slumber transitions. */
	pt_cap_host_slm = 0x2000,
	/** Supports Phy event counters. */
	pt_cap_phy_ctx = 0x0400,
	/** Supports recepits of host-initiated interface power management. */
	pt_cap_host_pmngmnt = 0x0200,
	
	/** Supports SATA 3. */
	pt_cap_sat_3 = 0x0008,
	/** Supports SATA 2. */
	pt_cap_sat_2 = 0x0004,
	/** Supports SATA 1. */
	pt_cap_sat_1 = 0x0002
};

#endif
