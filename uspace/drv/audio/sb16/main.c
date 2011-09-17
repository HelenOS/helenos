/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2011 Vojtech Horky
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
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * Main routines of Creative Labs SoundBlaster 16 driver
 */
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <stdio.h>
#include <errno.h>

//#include <device/hw_res.h>
//#include <str_error.h>


//#include "pci.h"

#define NAME "sb16"

static int sb_add_device(ddf_dev_t *device);
/*----------------------------------------------------------------------------*/
static driver_ops_t sb_driver_ops = {
	.add_device = sb_add_device,
};
/*----------------------------------------------------------------------------*/
static driver_t sb_driver = {
	.name = NAME,
	.driver_ops = &sb_driver_ops
};
//static ddf_dev_ops_t sb_ops = {};
/*----------------------------------------------------------------------------*/
/** Initializes a new ddf driver instance of SB16.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int sb_add_device(ddf_dev_t *device)
{
	assert(device);
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
/** Initializes global driver structures (NONE).
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	printf(NAME": HelenOS SB16 audio driver.\n");
	ddf_log_init(NAME, LVL_DEBUG2);
	return ddf_driver_main(&sb_driver);
}
/**
 * @}
 */

