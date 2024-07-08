/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup pci-ide
 * @{
 */
/** @file PC Floppy Disk hardware definitions
 *
 * Based on
 *  - NEC uPD765A datasheet
 *  - Intel 82077AA Floppy Controller Datasheet
 */

#ifndef PC_FLOPPY_HW_H
#define PC_FLOPPY_HW_H

/** Command Codes */
typedef enum {
	/** Read Data */
	fcc_read_data = 0x06,
	/** Read Delete Data */
	fcc_read_ddata = 0x0c,
	/** Write Data */
	fcc_write_data = 0x05,
	/** Write Deleted Data */
	fcc_write_ddata = 0x09,
	/** Read a Track */
	fcc_read_track = 0x02,
	/** Read ID */
	fcc_read_id = 0x0a,
	/** Format a Track */
	fcc_format_track = 0x0d,
	/** Scan Equal */
	fcc_scan_equal = 0x11,
	/** Scan Low or Equal */
	fcc_scan_lequal = 0x19,
	/** Scan High or Equal */
	fcc_scan_hequal = 0x1d,
	/** Recalibrate */
	fcc_recalibrate = 0x07,
	/** Sense Interrupt Status */
	fcc_sense_int_sts = 0x08,
	/** Specify */
	fcc_specify = 0x03,
	/** Sense Drive Status */
	fcc_sense_drv_sts = 0x04,
	/** Seek */
	fcc_seek = 0x0f
} pc_fdc_cmd_code_t;

/** MT|MF|SK flags used in flags_cc byte */
typedef enum {
	/** Multi Track */
	fcf_mt = 0x80,
	/** FM or MFM mode */
	fcf_mf = 0x40,
	/** Skip deleted address mark */
	fcf_sk = 0x20
} pc_fcd_flags_t;

/** Command parameters common for most data commands */
typedef struct {
	/** [MT] | MF | [SK] | command code */
	uint8_t flags_cc;
	/** XXXXX | HD | US1 | US0 */
	uint8_t hd_us;
	/** Cylinder number */
	uint8_t cyl;
	/** Head number */
	uint8_t head;
	/** Record number */
	uint8_t rec;
	/** Number */
	uint8_t number;
	/** End of Track */
	uint8_t eot;
	/** Gap Length */
	uint8_t gpl;
	/** Data Length */
	uint8_t dtl;
} pc_fdc_cmd_data_t;

/** Status data common for most commands */
typedef struct {
	/** Status 0 */
	uint8_t st0;
	/** Status 1 */
	uint8_t st1;
	/** Status 2 */
	uint8_t st2;
	/** Cylinder number */
	uint8_t cyl;
	/** Head number */
	uint8_t head;
	/** Record number */
	uint8_t rec;
	/** Number */
	uint8_t number;
} pc_fdc_cmd_status_t;

/** Command parameters for Read ID command */
typedef struct {
	/** 0 | MF | 0 | command code */
	uint8_t flags_cc;
	/** XXXXX | HD | US1 | US0 */
	uint8_t hd_us;
} pc_fdc_read_id_data_t;

/** Command parameters for Format a Track command */
typedef struct {
	/** 0 | MF | 0 | command code */
	uint8_t flags_cc;
	/** XXXXX | HD | US1 | US0 */
	uint8_t hd_us;
	/** Number */
	uint8_t number;
	/** Sectors per Cylinder */
	uint8_t sec_cyl;
	/** Gap Length */
	uint8_t gpl;
	/** Data Pattern */
	uint8_t dpat;
} pc_fdc_format_track_data_t;

/** Command parameters for Recalibrate command */
typedef struct {
	/** 0 | 0 | 0 | command code */
	uint8_t cc;
	/** XXXXX | 0 | US1 | US0 */
	uint8_t us;
} pc_fdc_recalibrate_data_t;

/** Command parameters for Sense Interrupt Status command */
typedef struct {
	/** 0 | 0 | 0 | command code */
	uint8_t cc;
} pc_fdc_sense_int_sts_data_t;

/** Status data common for Sense Interrupt Status command */
typedef struct {
	/** Status 0 */
	uint8_t st0;
	/** Present Cylinder Number */
	uint8_t pcn;
} pc_fdc_sense_int_sts_status_t;

/** Command parameters for Specify command */
typedef struct {
	/** 0 | 0 | 0 | command code */
	uint8_t cc;
	/** Step Rate Time, Head Unload Time */
	uint8_t srt_hut;
	/** Head Load Time, Non-DMA Mode */
	uint8_t hlt_nd;
} pc_fdc_secify_data_t;

/** Command parameters for Sense Drive Status command */
typedef struct {
	/** 0 | 0 | 0 | command code */
	uint8_t cc;
	/** XXXXX | HD | US1 | US0 */
	uint8_t hd_us;
} pc_fdc_sense_drive_sts_data_t;

/** Command parameters for Seek command */
typedef struct {
	/** 0 | 0 | 0 | command code */
	uint8_t cc;
	/** XXXXX | HD | US1 | US0 */
	uint8_t hd_us;
} pc_fdc_seek_data_t;

/** Bits in Status Register A (SRA) PS/2 Mode */
enum {
	fsra2_int_pending = 0x80,
	fsra2_ndrv2 = 0x40,
	fsra2_step = 0x20,
	fsra2_ntrk0 = 0x10,
	fsra2_hdsel = 0x08,
	fsra2_nindx = 0x04,
	fsra2_nwp = 0x02,
	fsra2_dir = 0x01
};

/** Bits in Status Register A (SRA) Model 30 Mode */
enum {
	fsra3_int_pending = 0x80,
	fsra3_drq = 0x40,
	fsra3_step_ff = 0x20,
	fsra3_trko = 0x10,
	fsra3_nhdsel = 0x08,
	fsra3_index = 0x04,
	fsra3_wp = 0x02,
	fsra3_ndir = 0x01
};

/** Bits in Status Register B (SRB) PS/2 Mode */
enum {
	fsrb_d0sel = 0x20,
	fsrb_wrd_tgl = 0x10,
	fsrb_rdd_tgl = 0x08,
	fsrb_we = 0x04,
	fsrb_me1 = 0x02,
	fsrb_me0 = 0x01
};

/** Bits in Status Register B (SRB) Model 30 Mode */
enum {
	fsrb_ndrv2 = 0x80,
	fsrb_nds1 = 0x40,
	fsrb_nds0 = 0x20,
	fsrb_wrd_ff = 0x10,
	fsrb_rdd_ff = 0x08,
	fsrb_we_ff = 0x04,
	fsrb_nds3 = 0x02,
	fsrb_nds2 = 0x01
};

/** Bits in Digital Output Register (DOR) */
enum {
	fdor_me3 = 0x80,
	fdor_me2 = 0x40,
	fdor_me1 = 0x20,
	fdor_me0 = 0x10,
	fdor_ndmagate = 0x08,
	fdor_nreset = 0x04,
	fdor_ds1 = 0x02,
	fdor_ds0 = 0x01
};

/** Bits in Tape Drive Register (TDR) */
enum {
	ftdr_ts1 = 0x02,
	ftdr_ts0 = 0x01
};

/** Bits in Datarate Select Register (DSR) */
enum {
	fdsr_sw_reset = 0x80,
	fdsr_power_down = 0x40,
	fdsr_precomp2 = 0x10,
	fdsr_precomp1 = 0x08,
	fdsr_precomp0 = 0x04,
	fdsr_drate_sel1 = 0x02,
	fdsr_drate_sel0 = 0x01
};

/** Combined values of DSR.DRATE_SEL1/0 */
enum {
	fdsr_drate_1mbps = 0x03,
	fdsr_drate_500kbps = 0x00,
	fdsr_drate_300kbps = 0x01,
	fdsr_drate_250kbps = 0x02
};

/** Bits in Main Status Register (MSR) */
enum {
	/** Request for Master */
	fmsr_rqm = 0x80,
	/** Data Input/Output */
	fmsr_dio = 0x40,
	/** Execution Mode */
	fmsr_exm = 0x20,
	/** FDC Busy */
	fmsr_cb = 0x10,
	/** FDD 3 Busy */
	fmsr_d3b = 0x08,
	/** FDD 2 Busy */
	fmsr_d2b = 0x04,
	/** FDD 1 Busy */
	fmsr_d1b = 0x02,
	/** FDD 0 Busy */
	fmsr_d0b = 0x01,
};

/** Bits in Digital Input Register, PC-AT Mode */
enum {
	fdira_dsk_chg = 0x80
};

/** Bits in Digital Input Register, PS/2 Mode */
enum {
	fdir2_dsk_chg = 0x80,
	fdir2_drate_sel1 = 0x04,
	fdir2_drate_sel0 = 0x02,
	fdir2_nhigh_dens = 0x01
};

/** Bits in Digital Input Register, Model 30 Mode */
enum {
	fdir3_dsk_chg = 0x80,
	fdir3_ndma_gate = 0x08,
	fdir3_noprec = 0x04,
	fdir3_drate_sel1 = 0x02,
	fdir3_drate_sel0 = 0x01
};

/** Bits in Configuration Control Register (CCR) */
enum {
	fccr_noprec = 0x04,
	fccr_drate_sel1 = 0x02,
	fccr_drate_sel0 = 0x01
};

/** Bits in Status Register 0 (SR0) */
enum {
	fsr0_ic_mask = 0xc0,
	fsr0_ic_normal = 0x00,
	fsr0_ic_abnormal = 0x40,
	fsr0_ic_invcmd = 0x80,
	fsr0_ic_abnormal_poll = 0xc0,
	fsr0_seek_end = 0x20,
	fsr0_equip_check = 0x10,
	fsr0_head_addr = 0x04,
	fsr0_ds1 = 0x02,
	fsr0_ds0 = 0x01
};

/** Bits in Status Register 1 (SR1) */
enum {
	fsr1_end_of_cyl = 0x80,
	fsr1_data_error = 0x20,
	fsr1_overr_underr = 0x10,
	fsr1_no_data = 0x04,
	fsr1_not_writable = 0x02,
	fsr1_missing_am = 0x01
};

/** Bits in Status Register 2 (SR2) */
enum {
	fsr2_control_mark = 0x40,
	fsr1_derr_df = 0x20,
	fsr1_wrong_cyl = 0x10,
	fsr1_bad_cyl = 0x02,
	fsr1_missing_dam = 0x01
};

/** Registers */
typedef union {
	struct { /* read only */
		/** Status Register A */
		uint8_t sra;
		/** Starus Register B */
		uint8_t srb;
		/** Padding */
		uint8_t ro_pad2[2];
		/** Main Status Register */
		uint8_t msr;
		/** Padding */
		uint8_t ro_pad5[2];
		/** Digital Inut Register */
		uint8_t dir;
	};
	struct { /* write only */
		/** Padding */
		uint8_t wo_pad0[4];
		/** Datarate Select Register */
		uint8_t dsr;
		/** Padding */
		uint8_t wo_pad5[2];
		/** Configuration Control Register */
		uint8_t ccr;
	};
	struct { /* read/write */
		/** Padding */
		uint8_t rw_pad0[2];
		/** Digital Output Register */
		uint8_t dor;
		/** Tape Drive Register */
		uint8_t tdr;
		/** Padding */
		uint8_t rw_pad4;
		/** Data (FIFO) */
		uint8_t data;
	};
} pc_fdc_regs_t;

enum {
	/** Max. time we need to wait for MSR status */
	msr_max_wait_usec = 250
};

#endif

/** @}
 */
