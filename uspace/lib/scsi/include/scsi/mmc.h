/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup libscsi
 * @{
 */
/**
 * @file SCSI Multi-Media Commands.
 */

#ifndef LIBSCSI_MMC_H_
#define LIBSCSI_MMC_H_

#include <stdint.h>

/** SCSI command codes defined in SCSI-MMC */
enum scsi_cmd_mmc {
	SCSI_CMD_READ_TOC	= 0x43
};

/** SCSI Read TOC/PMA/ATIP command.
 *
 * Note: For SFF 8020 the command must be padded to 12 bytes.
 */
typedef struct {
	/** Operation code (SCSI_CMD_READ_TOC) */
	uint8_t op_code;
	/** Reserved, MSF, Reserved */
	uint8_t msf;
	/** Reserved, Format */
	uint8_t format;
	/** Reserved */
	uint8_t reserved_3;
	/** Reserved */
	uint8_t reserved_4;
	/** Reserved */
	uint8_t reserved_5;
	/** Track/Session Number */
	uint8_t track_sess_no;
	/** Allocation Length */
	uint16_t alloc_len;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_read_toc_t;

/** TOC Track Descriptor */
typedef struct {
	/** Reserved */
	uint8_t reserved0;
	/** ADR, Control */
	uint8_t adr_control;
	/** Track Number */
	uint8_t track_no;
	/** Reserved */
	uint8_t reserved3;
	/** Track Start Address */
	uint32_t start_addr;
} __attribute__((packed)) scsi_toc_track_desc_t;

/** Read TOC response format 00001b: Multi-session Information
 *
 * Returned for Read TOC command with Format 0001b
 */
typedef struct {
	/** TOC Data Length */
	uint16_t toc_len;
	/** First Complete Session Number */
	uint8_t first_sess;
	/** Last Complete Session Number */
	uint8_t last_sess;
	/** TOC Track Descriptor for first track in last complete session */
	scsi_toc_track_desc_t ftrack_lsess;
} __attribute__((packed)) scsi_toc_multisess_data_t;

#endif

/** @}
 */
