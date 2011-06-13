/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup kbd_port
 * @ingroup kbd
 * @{
 */
/** @file
 * @brief ADB keyboard port driver.
 */

#include <ipc/adb.h>
#include <async.h>
#include <async_obsolete.h>
#include <kbd_port.h>
#include <kbd.h>
#include <vfs/vfs.h>
#include <fcntl.h>
#include <errno.h>
#include <devmap.h>
#include <devmap_obsolete.h>

static void kbd_port_events(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static void adb_kbd_reg0_data(uint16_t data);

static int adb_port_init(kbd_dev_t *);
static void adb_port_yield(void);
static void adb_port_reclaim(void);
static void adb_port_write(uint8_t data);

kbd_port_ops_t adb_port = {
	.init = adb_port_init,
	.yield = adb_port_yield,
	.reclaim = adb_port_reclaim,
	.write = adb_port_write
};

static kbd_dev_t *kbd_dev;
static int dev_phone;

static int adb_port_init(kbd_dev_t *kdev)
{
	const char *dev = "adb/kbd";
	devmap_handle_t handle;

	kbd_dev = kdev;
	
	int rc = devmap_device_get_handle(dev, &handle, 0);
	if (rc == EOK) {
		dev_phone = devmap_obsolete_device_connect(handle, 0);
		if (dev_phone < 0) {
			printf("%s: Failed to connect to device\n", NAME);
			return dev_phone;
		}
	} else
		return rc;
	
	/* NB: The callback connection is slotted for removal */
	rc = async_obsolete_connect_to_me(dev_phone, 0, 0, 0, kbd_port_events,
	    NULL);
	if (rc != EOK) {
		printf(NAME ": Failed to create callback from device\n");
		return rc;
	}
	
	return EOK;
}

static void adb_port_yield(void)
{
}

static void adb_port_reclaim(void)
{
}

static void adb_port_write(uint8_t data)
{
	/*async_msg_1(dev_phone, CHAR_WRITE_BYTE, data);*/
}

static void kbd_port_events(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Ignore parameters, the connection is already opened */
	while (true) {

		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		int retval;
		
		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case ADB_REG_NOTIF:
			adb_kbd_reg0_data(IPC_GET_ARG1(call));
			break;
		default:
			retval = ENOENT;
		}
		async_answer_0(callid, retval);
	}
}

static void adb_kbd_reg0_data(uint16_t data)
{
	uint8_t b0, b1;

	b0 = (data >> 8) & 0xff;
	b1 = data & 0xff;

	if (b0 != 0xff)
		kbd_push_scancode(kbd_dev, b0);
	if (b1 != 0xff)
		kbd_push_scancode(kbd_dev, b1);
}

/**
 * @}
 */
