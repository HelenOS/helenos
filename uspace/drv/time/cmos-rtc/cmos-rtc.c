/*
 * Copyright (c) 2012 Maurizio Lombardi
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
 * @defgroup CMOS RTC driver.
 * @brief HelenOS RTC driver.
 * @{
 */

/** @file
 */

#include <errno.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <stdio.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/clock_dev.h>
#include <fibril_synch.h>
#include <device/hw_res.h>
#include <devman.h>

#include "cmos-regs.h"

#define NAME "cmos-rtc"

#define REG_COUNT 2

#define RTC_FROM_FNODE(fnode)  ((rtc_t *) ((fnode)->dev->driver_data))

typedef struct rtc {
	/** DDF device node */
	ddf_dev_t *dev;
	/** DDF function node */
	ddf_fun_t *fun;
	/** The fibril mutex for synchronizing the access to the device */
	fibril_mutex_t mutex;
	/** The base I/O address of the device registers */
	uint32_t io_addr;
	/** The I/O port used to access the CMOS registers */
	ioport8_t *port;
	/** true if a client is connected to the device */
	bool client_connected;
} rtc_t;


static int  rtc_time_get(ddf_fun_t *fun, struct tm *t);
static int  rtc_time_set(ddf_fun_t *fun, struct tm *t);
static int  rtc_dev_add(ddf_dev_t *dev);
static int  rtc_dev_initialize(rtc_t *rtc);
static bool rtc_pio_enable(rtc_t *rtc);
static void rtc_dev_cleanup(rtc_t *rtc);
static int  rtc_open(ddf_fun_t *fun);
static void rtc_close(ddf_fun_t *fun);
static bool rtc_update_in_progress(rtc_t *rtc);
static int  rtc_register_read(rtc_t *rtc, int reg);
static int  bcd2dec(int bcd);


static ddf_dev_ops_t rtc_dev_ops;

/** The RTC device driver's standard operations */
static driver_ops_t rtc_ops = {
	.dev_add = rtc_dev_add,
	.dev_remove = NULL, /* XXX */
};

/** The RTC device driver structure */
static driver_t rtc_driver = {
	.name = NAME,
	.driver_ops = &rtc_ops,
};

/** Clock interface */
static clock_dev_ops_t rtc_clock_dev_ops = {
	.time_get = rtc_time_get,
	.time_set = rtc_time_set,
};

/** Initialize the RTC driver */
static void
rtc_init(void)
{
	ddf_log_init(NAME, LVL_ERROR);

	rtc_dev_ops.open = rtc_open;
	rtc_dev_ops.close = rtc_close;

	rtc_dev_ops.interfaces[CLOCK_DEV_IFACE] = &rtc_clock_dev_ops;
	rtc_dev_ops.default_handler = NULL; /* XXX */
}

/** Clean up the RTC soft state
 *
 * @param rtc  The RTC device
 */
static void
rtc_dev_cleanup(rtc_t *rtc)
{
	if (rtc->dev->parent_sess) {
		async_hangup(rtc->dev->parent_sess);
		rtc->dev->parent_sess = NULL;
	}
}

/** Enable the I/O ports of the device
 *
 * @param rtc  The real time clock device
 *
 * @return  true in case of success, false otherwise
 */
static bool
rtc_pio_enable(rtc_t *rtc)
{
	if (pio_enable((void *)(uintptr_t) rtc->io_addr, REG_COUNT,
	    (void **) &rtc->port)) {

		ddf_msg(LVL_ERROR, "Cannot map the port %#" PRIx32
		    " for device %s", rtc->io_addr, rtc->dev->name);
		return false;
	}

	return true;
}

/** Initialize the RTC device
 *
 * @param rtc  Pointer to the RTC device
 *
 * @return  EOK on success or a negative error code
 */ 
static int
rtc_dev_initialize(rtc_t *rtc)
{
	int rc;
	size_t i;
	hw_resource_t *res;
	bool ioport = false;

	ddf_msg(LVL_DEBUG, "rtc_dev_initialize %s", rtc->dev->name);

	hw_resource_list_t hw_resources;
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));

	/* Connect to the parent's driver */

	rtc->dev->parent_sess = devman_parent_device_connect(EXCHANGE_SERIALIZE,
	    rtc->dev->handle, IPC_FLAG_BLOCKING);
	if (!rtc->dev->parent_sess) {
		ddf_msg(LVL_ERROR, "Failed to connect to parent driver\
		    of device %s.", rtc->dev->name);
		rc = ENOENT;
		goto error;
	}

	/* Get the HW resources */
	rc = hw_res_get_resource_list(rtc->dev->parent_sess, &hw_resources);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to get HW resources\
		    for device %s", rtc->dev->name);
		goto error;
	}

	for (i = 0; i < hw_resources.count; ++i) {
		res = &hw_resources.resources[i];

		if (res->type == IO_RANGE) {
			if (res->res.io_range.size < REG_COUNT) {
				ddf_msg(LVL_ERROR, "I/O range assigned to \
				    device %s is too small", rtc->dev->name);
				rc = ELIMIT;
				goto error;
			}
			rtc->io_addr = res->res.io_range.address;
			ioport = true;
			ddf_msg(LVL_NOTE, "Device %s was assigned I/O address \
			    0x%x", rtc->dev->name, rtc->io_addr);
		}
	}

	if (!ioport) {
		/* No I/O address assigned to this device */
		ddf_msg(LVL_ERROR, "Missing HW resource for device %s",
		    rtc->dev->name);
		rc = ENOENT;
		goto error;
	}

	hw_res_clean_resource_list(&hw_resources);

	return EOK;

error:
	rtc_dev_cleanup(rtc);
	hw_res_clean_resource_list(&hw_resources);

	return rc;
}

/** Read a register from the CMOS memory
 *
 * @param port   The I/O port assigned to the device
 * @param reg    The index of the register to read
 *
 * @return       The value of the register
 */
static int
rtc_register_read(rtc_t *rtc, int reg)
{
	pio_write_8(rtc->port, reg);
	return pio_read_8(rtc->port + 1);
}

/** Check if an update is in progress
 *
 * @param rtc  The rtc device
 *
 * @return  true if an update is in progress, false otherwise
 */
static bool
rtc_update_in_progress(rtc_t *rtc)
{
	return rtc_register_read(rtc, RTC_UPDATE) & RTC_MASK_UPDATE;
}

/** Read the current time from the CMOS
 *
 * @param fun  The RTC function
 * @param t    Pointer to the time variable
 *
 * @return  EOK on success or a negative error code
 */
static int
rtc_time_get(ddf_fun_t *fun, struct tm *t)
{
	bool bcd_mode;
	rtc_t *rtc = RTC_FROM_FNODE(fun);

	fibril_mutex_lock(&rtc->mutex);

	/* now read the registers */
	do {
		/* Suspend until the update process has finished */
		while (rtc_update_in_progress(rtc));

		t->tm_sec  = rtc_register_read(rtc, RTC_SEC);
		t->tm_min  = rtc_register_read(rtc, RTC_MIN);
		t->tm_hour = rtc_register_read(rtc, RTC_HOUR);
		t->tm_mday = rtc_register_read(rtc, RTC_DAY);
		t->tm_mon  = rtc_register_read(rtc, RTC_MON);
		t->tm_year = rtc_register_read(rtc, RTC_YEAR);

		/* Now check if it is stable */
	} while( t->tm_sec  != rtc_register_read(rtc, RTC_SEC) ||
		 t->tm_min  != rtc_register_read(rtc, RTC_MIN) ||
		 t->tm_mday != rtc_register_read(rtc, RTC_DAY) ||
		 t->tm_mon  != rtc_register_read(rtc, RTC_MON) ||
		 t->tm_year != rtc_register_read(rtc, RTC_YEAR));

	/* Check if the RTC is working in BCD mode */
	bcd_mode = rtc_register_read(rtc, RTC_STATUS_B) & RTC_MASK_BCD;

	if (bcd_mode) {
		t->tm_sec  = bcd2dec(t->tm_sec);
		t->tm_min  = bcd2dec(t->tm_min);
		t->tm_hour = bcd2dec(t->tm_hour);
		t->tm_mday = bcd2dec(t->tm_mday);
		t->tm_mon  = bcd2dec(t->tm_mon);
		t->tm_year = bcd2dec(t->tm_year);
	}

	/* Count the months starting from 0, not from 1 */
	t->tm_mon--;

	fibril_mutex_unlock(&rtc->mutex);
	return EOK;
}

/** Set the time in the RTC
 *
 * @param fun  The RTC function
 * @param t    The time value to set
 *
 * @return  EOK or a negative error code
 */
static int
rtc_time_set(ddf_fun_t *fun, struct tm *t)
{
	return EOK;
}

/** The dev_add callback of the rtc driver
 *
 * @param dev  The RTC device
 *
 * @return  EOK on success or a negative error code
 */
static int
rtc_dev_add(ddf_dev_t *dev)
{
	rtc_t *rtc;
	ddf_fun_t *fun = NULL;
	int rc;
	bool need_cleanup = false;

	ddf_msg(LVL_DEBUG, "rtc_dev_add %s (handle = %d)",
	    dev->name, (int) dev->handle);

	rtc = ddf_dev_data_alloc(dev, sizeof(rtc_t));
	if (!rtc)
		return ENOMEM;

	rtc->dev = dev;
	fibril_mutex_initialize(&rtc->mutex);

	rc = rtc_dev_initialize(rtc);
	if (rc != EOK)
		goto error;

	need_cleanup = true;

	if (!rtc_pio_enable(rtc)) {
		rc = EADDRNOTAVAIL;
		goto error;
	}

	fun = ddf_fun_create(dev, fun_exposed, "a");
	if (!fun) {
		ddf_msg(LVL_ERROR, "Failed creating function");
		rc = ENOENT;
		goto error;
	}

	fun->ops = &rtc_dev_ops;
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function");
		goto error;
	}

	rtc->fun = fun;

	ddf_fun_add_to_category(fun, "clock");

	rtc->client_connected = false;

	ddf_msg(LVL_NOTE, "Device %s successfully initialized",
	    dev->name);

	return rc;

error:
	if (fun)
		ddf_fun_destroy(fun);
	if (need_cleanup)
		rtc_dev_cleanup(rtc);
	return rc;
}

/** Open the device
 *
 * @param fun   The function node
 *
 * @return  EOK on success or a negative error code
 */
static int
rtc_open(ddf_fun_t *fun)
{
	int rc;
	rtc_t *rtc = RTC_FROM_FNODE(fun);

	fibril_mutex_lock(&rtc->mutex);

	if (rtc->client_connected)
		rc = EBUSY;
	else {
		rc = EOK;
		rtc->client_connected = true;
	}

	fibril_mutex_unlock(&rtc->mutex);
	return rc;
}

/** Close the device
 *
 * @param fun  The function node
 */
static void
rtc_close(ddf_fun_t *fun)
{
	rtc_t *rtc = RTC_FROM_FNODE(fun);

	fibril_mutex_lock(&rtc->mutex);

	assert(rtc->client_connected);
	rtc->client_connected = false;

	fibril_mutex_unlock(&rtc->mutex);
}

/** Convert from BCD mode to binary mode
 *
 * @param bcd   The number in BCD format to convert
 *
 * @return      The converted value
 */
static int
bcd2dec(int bcd)
{
	return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xf);
}

int
main(int argc, char **argv)
{
	printf(NAME ": HelenOS RTC driver\n");
	rtc_init();
	return ddf_driver_main(&rtc_driver);
}

/**
 * @}
 */
