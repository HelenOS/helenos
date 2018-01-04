/*
 * Copyright (c) 2011 Jan Vesely
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
