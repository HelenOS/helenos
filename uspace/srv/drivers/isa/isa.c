/*
 * Copyright (c) 2010 Lenka Trochtova
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

/**
 * @defgroup isa ISA bus driver.
 * @brief HelenOS ISA bus driver.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <macros.h>

#include <driver.h>
#include <devman.h>
#include <ipc/devman.h>

#define NAME "isa"

static bool isa_add_device(device_t *dev);

/** The isa device driver's standard operations.
 */
static driver_ops_t isa_ops = {
	.add_device = &isa_add_device
};

/** The isa device driver structure. 
 */
static driver_t isa_driver = {
	.name = NAME,
	.driver_ops = &isa_ops
};


static bool isa_add_device(device_t *dev) 
{
	printf(NAME ": isa_add_device, device handle = %d\n", dev->handle);
	
	// TODO add children 
	
	return true;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS root device driver\n");	
	return driver_main(&isa_driver);
}

/**
 * @}
 */
 