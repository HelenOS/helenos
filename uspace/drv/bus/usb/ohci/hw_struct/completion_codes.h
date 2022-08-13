/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#ifndef DRV_OHCI_HW_STRUCT_COMPLETION_CODES_H
#define DRV_OHCI_HW_STRUCT_COMPLETION_CODES_H

#include <errno.h>

enum {
	CC_NOERROR = 0x0,
	CC_CRC = 0x1,
	CC_BITSTUFF = 0x2,
	CC_TOGGLE = 0x3,
	CC_STALL = 0x4,
	CC_NORESPONSE = 0x5,
	CC_PIDFAIL = 0x6,
	CC_PIDUNEXPECTED = 0x7,
	CC_DATAOVERRRUN = 0x8,
	CC_DATAUNDERRRUN = 0x9,
	CC_BUFFEROVERRRUN = 0xc,
	CC_BUFFERUNDERRUN = 0xd,
	CC_NOACCESS1 = 0xe,
	CC_NOACCESS2 = 0xf,
};

inline static errno_t cc_to_rc(unsigned int cc)
{
	switch (cc) {
	case CC_NOERROR:
		return EOK;

	case CC_CRC:
		return EBADCHECKSUM;

	case CC_PIDUNEXPECTED:
	case CC_PIDFAIL:
	case CC_BITSTUFF:
		return EIO;

	case CC_TOGGLE:
	case CC_STALL:
		return ESTALL;

	case CC_NORESPONSE:
		return ETIMEOUT;

	case CC_DATAOVERRRUN:
	case CC_DATAUNDERRRUN:
	case CC_BUFFEROVERRRUN:
	case CC_BUFFERUNDERRUN:
		return EOVERFLOW;

	case CC_NOACCESS1:
	case CC_NOACCESS2:
	default:
		return ENOTSUP;
	}
}

#endif
/**
 * @}
 */
