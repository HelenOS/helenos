/*
 * Copyright (c) 2014 Jan Kolarik
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

/** @file wmi.h
 *
 * Definitions of the Atheros WMI protocol specified in the
 * Wireless Module Interface (WMI).
 *
 */

#ifndef ATHEROS_WMI_H
#define ATHEROS_WMI_H

#include "htc.h"

/* Macros for creating service identificators. */
#define WMI_SERVICE_GROUP  1

#define CREATE_SERVICE_ID(group, i) \
	(unsigned int) (((unsigned int) group << 8) | (unsigned int) (i))

#define WMI_MGMT_CMD_MASK  0x1000

/** WMI header structure.
 *
 */
typedef struct {
	uint16_t command_id;       /**< Big Endian value! */
	uint16_t sequence_number;  /**< Big Endian value! */
} __attribute__((packed)) wmi_command_header_t;

/** WMI services IDs
 *
 */
typedef enum {
	WMI_CONTROL_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 0),
	WMI_BEACON_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 1),
	WMI_CAB_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 2),
	WMI_UAPSD_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 3),
	WMI_MGMT_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 4),
	WMI_DATA_VOICE_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 5),
	WMI_DATA_VIDEO_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 6),
	WMI_DATA_BE_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 7),
	WMI_DATA_BK_SERVICE = CREATE_SERVICE_ID(WMI_SERVICE_GROUP, 8)
} wmi_services_t;

/** List of WMI commands
 *
 */
typedef enum {
	WMI_ECHO = 0x0001,
	WMI_ACCESS_MEMORY,

	/* Commands used for HOST -> DEVICE communication */
	WMI_GET_FW_VERSION,
	WMI_DISABLE_INTR,
	WMI_ENABLE_INTR,
	WMI_ATH_INIT,
	WMI_ABORT_TXQ,
	WMI_STOP_TX_DMA,
	WMI_ABORT_TX_DMA,
	WMI_DRAIN_TXQ,
	WMI_DRAIN_TXQ_ALL,
	WMI_START_RECV,
	WMI_STOP_RECV,
	WMI_FLUSH_RECV,
	WMI_SET_MODE,
	WMI_NODE_CREATE,
	WMI_NODE_REMOVE,
	WMI_VAP_REMOVE,
	WMI_VAP_CREATE,
	WMI_REG_READ,
	WMI_REG_WRITE,
	WMI_RC_STATE_CHANGE,
	WMI_RC_RATE_UPDATE,
	WMI_TARGET_IC_UPDATE,
	WMI_TX_AGGR_ENABLE,
	WMI_TGT_DETACH,
	WMI_NODE_UPDATE,
	WMI_INT_STATS,
	WMI_TX_STATS,
	WMI_RX_STATS,
	WMI_BITRATE_MASK
} wmi_command_t;

/**Structure used when sending registry buffer
 *
 */
typedef struct {
	uint32_t offset;  /**< Big Endian value! */
	uint32_t value;   /**< Big Endian value! */
} wmi_reg_t;

extern errno_t wmi_reg_read(htc_device_t *, uint32_t, uint32_t *);
extern errno_t wmi_reg_write(htc_device_t *, uint32_t, uint32_t);
extern errno_t wmi_reg_set_clear_bit(htc_device_t *, uint32_t, uint32_t, uint32_t);
extern errno_t wmi_reg_set_bit(htc_device_t *, uint32_t, uint32_t);
extern errno_t wmi_reg_clear_bit(htc_device_t *, uint32_t, uint32_t);
extern errno_t wmi_reg_buffer_write(htc_device_t *, wmi_reg_t *, size_t);
extern errno_t wmi_send_command(htc_device_t *, wmi_command_t, uint8_t *, uint32_t,
    void *);

#endif  /* ATHEROS_WMI_H */
