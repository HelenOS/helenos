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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <assert.h>
#include <stdio.h>

#include <usb/debug.h>

#include "port_status.h"

struct flag_name
{
	unsigned flag;
	const char *name;
};

static const struct flag_name flags[] =
{
	{ STATUS_SUSPEND, "suspended" },
	{ STATUS_IN_RESET, "in reset" },
	{ STATUS_LOW_SPEED, "low speed device" },
	{ STATUS_ALWAYS_ONE, "always 1 bit" },
	{ STATUS_RESUME, "resume" },
	{ STATUS_LINE_D_MINUS, "line D- value" },
	{ STATUS_LINE_D_PLUS, "line D+ value" },
	{ STATUS_ENABLED_CHANGED, "enabled changed" },
	{ STATUS_ENABLED, "enabled" },
	{ STATUS_CONNECTED_CHANGED, "connected changed" },
	{ STATUS_CONNECTED, "connected" }
};

void print_port_status(port_status_t value)
{
	unsigned i = 0;
	for (;i < sizeof(flags)/sizeof(struct flag_name); ++i) {
		usb_log_debug("\t%s status: %s.\n", flags[i].name,
		  value & flags[i].flag ? "YES" : "NO");
	}
}
/**
 * @}
 */
