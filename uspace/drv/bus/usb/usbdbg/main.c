/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbdbg
 * @{
 */
/**
 * @file
 * Main routines of USB debug device driver.
 */
#include <errno.h>
#include <usb/debug.h>
#include <usb/dev/driver.h>

#define NAME "usbdbg"

static int usbdbg_device_add(usb_device_t *dev)
{
	usb_log_info("usbdbg_device_add");
	return EOK;
}

static int usbdbg_device_remove(usb_device_t *dev)
{
	usb_log_info("usbdbg_device_remove");
	return EOK;
}

static int usbdbg_device_gone(usb_device_t *dev)
{
	usb_log_info("usbdbg_device_gone");
	return EOK;
}

static int usbdbg_function_online(ddf_fun_t *fun)
{
	/* TODO: What if this is the control function? */
	return ddf_fun_online(fun);
}

static int usbdbg_function_offline(ddf_fun_t *fun)
{
	/* TODO: What if this is the control function? */
	return ddf_fun_offline(fun);
}

/** USB debug driver ops. */
static const usb_driver_ops_t dbg_driver_ops = {
	.device_add = usbdbg_device_add,
	.device_rem = usbdbg_device_remove,
	.device_gone = usbdbg_device_gone,
	.function_online = usbdbg_function_online,
	.function_offline = usbdbg_function_offline
};

/** USB debug driver. */
static const usb_driver_t dbg_driver = {
	.name = NAME,
	.ops = &dbg_driver_ops,
	.endpoints = NULL
};

int main(int argc, char *argv[])
{
	printf(NAME ": USB debug device driver.\n");

	log_init(NAME);

	return usb_driver_main(&dbg_driver);
}

/**
 * @}
 */
