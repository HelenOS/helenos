/*
 * Copyright (c) 2010 Vojtech Horky
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

#include <stdio.h>
#include <stdlib.h>
#include <usb/hcd.h>
#include <errno.h>
#include "../tester.h"

#define MAX_ADDRESS 5


const char *test_usbaddrkeep(void)
{
	int rc;
	usb_address_keeping_t addresses;

	TPRINTF("Initializing addresses keeping structure...\n");
	usb_address_keeping_init(&addresses, MAX_ADDRESS);
	
	TPRINTF("Requesting address...\n");
	usb_address_t addr = usb_address_keeping_request(&addresses);
	TPRINTF("Address assigned: %d\n", (int) addr);
	if (addr != 1) {
		return "have not received expected address 1";
	}

	TPRINTF("Releasing not assigned address...\n");
	rc = usb_address_keeping_release(&addresses, 2);
	if (rc != ENOENT) {
		return "have not received expected ENOENT";
	}

	TPRINTF("Releasing acquired address...\n");
	rc = usb_address_keeping_release(&addresses, addr);
	if (rc != EOK) {
		return "have not received expected EOK";
	}

	return NULL;
}
