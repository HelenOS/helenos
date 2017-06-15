/*
 * Copyright (c) 2017 Ondrej Hlavaty
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * Memory-mapped register structures of the xHC.
 */

#include <inttypes.h>
#include <usb/debug.h>

#include "debug.h"

#define DUMP_REG_FIELD(ptr, title, size, ...) \
	usb_log_debug2(title "%" PRIu##size, XHCI_REG_RD_FIELD(ptr, size, ##__VA_ARGS__))

#define DUMP_REG_RANGE(ptr, title, size, ...) \
	usb_log_debug2(title "%" PRIu##size, XHCI_REG_RD_RANGE(ptr, size, ##__VA_ARGS__))

#define DUMP_REG_FLAG(ptr, title, size, ...) \
	usb_log_debug2(title "%s", XHCI_REG_RD_FLAG(ptr, size, ##__VA_ARGS__) ? "true" : "false")

#define DUMP_REG_INNER(set, title, field, size, type, ...) \
	DUMP_REG_##type(&(set)->field, title, size, ##__VA_ARGS__)

#define DUMP_REG(set, c) DUMP_REG_INNER(set, "\t" #c ": ", c)

/**
 * Dumps all capability registers.
 */
void xhci_dump_cap_regs(xhci_cap_regs_t *cap)
{
	usb_log_debug2("Capabilities:");

	DUMP_REG(cap, XHCI_CAP_LENGTH);
	DUMP_REG(cap, XHCI_CAP_VERSION);
	DUMP_REG(cap, XHCI_CAP_MAX_SLOTS);
	DUMP_REG(cap, XHCI_CAP_IST);
	DUMP_REG(cap, XHCI_CAP_ERST_MAX);
	DUMP_REG(cap, XHCI_CAP_SPR);
	DUMP_REG(cap, XHCI_CAP_U1EL);
	DUMP_REG(cap, XHCI_CAP_U2EL);
	DUMP_REG(cap, XHCI_CAP_AC64);
	DUMP_REG(cap, XHCI_CAP_BNC);
	DUMP_REG(cap, XHCI_CAP_CSZ);
	DUMP_REG(cap, XHCI_CAP_PPC);
	DUMP_REG(cap, XHCI_CAP_PIND);
	DUMP_REG(cap, XHCI_CAP_C);
	DUMP_REG(cap, XHCI_CAP_LTC);
	DUMP_REG(cap, XHCI_CAP_NSS);
	DUMP_REG(cap, XHCI_CAP_PAE);
	DUMP_REG(cap, XHCI_CAP_SPC);
	DUMP_REG(cap, XHCI_CAP_SEC);
	DUMP_REG(cap, XHCI_CAP_CFC);
	DUMP_REG(cap, XHCI_CAP_MAX_PSA_SIZE);
	DUMP_REG(cap, XHCI_CAP_XECP);
	DUMP_REG(cap, XHCI_CAP_DBOFF);
	DUMP_REG(cap, XHCI_CAP_RTSOFF);
	DUMP_REG(cap, XHCI_CAP_U3C);
	DUMP_REG(cap, XHCI_CAP_CMC);
	DUMP_REG(cap, XHCI_CAP_FSC);
	DUMP_REG(cap, XHCI_CAP_CTC);
	DUMP_REG(cap, XHCI_CAP_LEC);
	DUMP_REG(cap, XHCI_CAP_CIC);
}

/**
 * @}
 */
