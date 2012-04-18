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
#include <ipc/clock_ctl.h>

#include "cmos-regs.h"

#define NAME "cmos-rtc"

#define REG_COUNT 2

#define RTC_FROM_FNODE(fnode)  ((rtc_t *) ((fnode)->dev->driver_data))
#define RTC_FROM_DEV(devnode)  ((rtc_t *) ((devnode)->driver_data))

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
	/** true if device is removed */
	bool removed;
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
static unsigned bcd2bin(unsigned bcd);
static unsigned bin2bcd(unsigned binary);
static void rtc_default_handler(ddf_fun_t *fun,
    ipc_callid_t callid, ipc_call_t *call);
static int rtc_dev_remove(ddf_dev_t *dev);
static int rtc_tm_sanity_check(struct tm *t, int epoch);
static void rtc_register_write(rtc_t *rtc, int reg, int data);
static bool is_leap_year(int year);

static int days_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static ddf_dev_ops_t rtc_dev_ops;

/** The RTC device driver's standard operations */
static driver_ops_t rtc_ops = {
	.dev_add = rtc_dev_add,
	.dev_remove = rtc_dev_remove,
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
	rtc_dev_ops.default_handler = &rtc_default_handler;
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
 * @param rtc    The rtc device
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

/** Write a register to the CMOS memory
 *
 * @param rtc    The rtc device
 * @param reg    The index of the register to write
 * @param data   The data to write
 */
static void
rtc_register_write(rtc_t *rtc, int reg, int data)
{
	pio_write_8(rtc->port, reg);
	pio_write_8(rtc->port + 1, data);
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
	return rtc_register_read(rtc, RTC_STATUS_A) & RTC_MASK_UPDATE;
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
	bool pm_mode = false;
	int  epoch = 1900;
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

	/* Check if the RTC is working in 12h mode */
	bool _12h_mode = !(rtc_register_read(rtc, RTC_STATUS_B) &
	    RTC_MASK_24H);

	if (_12h_mode) {
		/* The RTC is working in 12h mode, check if it is AM or PM */
		if (t->tm_hour & 0x80) {
			/* PM flag is active, it must be cleared */
			t->tm_hour &= ~0x80;
			pm_mode = true;
		}
	}

	/* Check if the RTC is working in BCD mode */
	bcd_mode = !(rtc_register_read(rtc, RTC_STATUS_B) & RTC_MASK_BCD);

	if (bcd_mode) {
		t->tm_sec  = bcd2bin(t->tm_sec);
		t->tm_min  = bcd2bin(t->tm_min);
		t->tm_hour = bcd2bin(t->tm_hour);
		t->tm_mday = bcd2bin(t->tm_mday);
		t->tm_mon  = bcd2bin(t->tm_mon);
		t->tm_year = bcd2bin(t->tm_year);
	}

	if (_12h_mode) {
		/* Convert to 24h mode */
		if (pm_mode) {
			if (t->tm_hour < 12)
				t->tm_hour += 12;
		} else if (t->tm_hour == 12)
			t->tm_hour = 0;
	}

	/* Count the months starting from 0, not from 1 */
	t->tm_mon--;

	if (t->tm_year < 100) {
		/* tm_year is the number of years since 1900 but the
		 * RTC epoch is 2000.
		 */
		epoch = 2000;
		t->tm_year += 100;
	}

	fibril_mutex_unlock(&rtc->mutex);

	return rtc_tm_sanity_check(t, epoch);
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
	int  rc;
	bool bcd_mode;
	int  reg_b;
	int  reg_a;
	int  epoch;
	rtc_t *rtc = RTC_FROM_FNODE(fun);

	fibril_mutex_lock(&rtc->mutex);

	/* Detect the RTC epoch */
	if (rtc_register_read(rtc, RTC_YEAR) < 100)
		epoch = 2000;
	else
		epoch = 1900;

	rc = rtc_tm_sanity_check(t, epoch);
	if (rc != EOK) {
		fibril_mutex_unlock(&rtc->mutex);
		return rc;
	}

	t->tm_mon++; /* counts from 1, not from 0 */

	reg_b = rtc_register_read(rtc, RTC_STATUS_B);

	if (!(reg_b & RTC_MASK_24H)) {
		/* Force 24h mode of operation */
		reg_b |= RTC_MASK_24H;
		rtc_register_write(rtc, RTC_STATUS_B, reg_b);
	}

	if (epoch == 2000) {
		/* The RTC epoch is year 2000  but the tm_year
		 * field counts years since 1900.
		 */
		t->tm_year -= 100;
	}

	/* Check if the rtc is working in bcd mode */
	bcd_mode = !(reg_b & RTC_MASK_BCD);
	if (bcd_mode) {
		/* Convert the tm struct fields in BCD mode */
		t->tm_sec  = bin2bcd(t->tm_sec);
		t->tm_min  = bin2bcd(t->tm_min);
		t->tm_hour = bin2bcd(t->tm_hour);
		t->tm_mday = bin2bcd(t->tm_mday);
		t->tm_mon  = bin2bcd(t->tm_mon);
		t->tm_year = bin2bcd(t->tm_year);
	}

	/* Inhibit updates */
	rtc_register_write(rtc, RTC_STATUS_B, reg_b | RTC_MASK_INH);

	/* Write current time to RTC */
	rtc_register_write(rtc, RTC_SEC, t->tm_sec);
	rtc_register_write(rtc, RTC_MIN, t->tm_min);
	rtc_register_write(rtc, RTC_HOUR, t->tm_hour);
	rtc_register_write(rtc, RTC_DAY, t->tm_mday);
	rtc_register_write(rtc, RTC_MON, t->tm_mon);
	rtc_register_write(rtc, RTC_YEAR, t->tm_year);

	/* Stop the clock */
	reg_a = rtc_register_read(rtc, RTC_STATUS_A);
	rtc_register_write(rtc, RTC_STATUS_A, RTC_MASK_CLK_STOP | reg_a);
	
	/* Enable updates */
	rtc_register_write(rtc, RTC_STATUS_B, reg_b);
	rtc_register_write(rtc, RTC_STATUS_A, reg_a);

	fibril_mutex_unlock(&rtc->mutex);

	return rc;
}

/** Check if the tm structure contains valid values
 *
 * @param t     The tm structure to check
 * @param epoch The RTC epoch year
 *
 * @return      EOK on success or EINVAL
 */
static int
rtc_tm_sanity_check(struct tm *t, int epoch)
{
	int ndays;

	if (t->tm_sec < 0 || t->tm_sec > 59)
		return EINVAL;
	else if (t->tm_min < 0 || t->tm_min > 59)
		return EINVAL;
	else if (t->tm_hour < 0 || t->tm_hour > 23)
		return EINVAL;
	else if (t->tm_mday < 1 || t->tm_mday > 31)
		return EINVAL;
	else if (t->tm_mon < 0 || t->tm_mon > 11)
		return EINVAL;
	else if (epoch == 2000 && t->tm_year < 100)
		return EINVAL;
	else if (t->tm_year < 0 || t->tm_year > 199)
		return EINVAL;

	if (t->tm_mon == 1/* FEB */ && is_leap_year(t->tm_year))
		ndays = 29;
	else
		ndays = days_month[t->tm_mon];

	if (t->tm_mday > ndays)
		return EINVAL;

	return EOK;
}

/** Check if a year is a leap year
 *
 * @param year   The year to check
 *
 * @return       true if it is a leap year, false otherwise
 */
static bool
is_leap_year(int year)
{
	bool r = false;

	if (year % 4 == 0) {
		if (year % 100 == 0)
			r = year % 400 == 0;
		else
			r = true;
	}

	return r;
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

/** The dev_remove callback for the rtc driver
 *
 * @param dev   The RTC device
 *
 * @return      EOK on success or a negative error code
 */
static int
rtc_dev_remove(ddf_dev_t *dev)
{
	rtc_t *rtc = RTC_FROM_DEV(dev);
	int rc;

	fibril_mutex_lock(&rtc->mutex);
	if (rtc->client_connected) {
		fibril_mutex_unlock(&rtc->mutex);
		return EBUSY;
	}

	rtc->removed = true;
	fibril_mutex_unlock(&rtc->mutex);

	rc = ddf_fun_unbind(rtc->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to unbind function");
		return rc;
	}

	ddf_fun_destroy(rtc->fun);
	rtc_dev_cleanup(rtc);

	return rc;
}

/** Default handler for client requests not handled
 *  by the standard interface
 */
static void
rtc_default_handler(ddf_fun_t *fun, ipc_callid_t callid, ipc_call_t *call)
{
	sysarg_t method = IPC_GET_IMETHOD(*call);
	rtc_t *rtc = RTC_FROM_FNODE(fun);
	bool batt_ok;

	switch (method) {
	case CLOCK_GET_BATTERY_STATUS:
		batt_ok = rtc_register_read(rtc, RTC_STATUS_D) &
		    RTC_BATTERY_OK;
		async_answer_1(callid, EOK, batt_ok);
		break;
	default:
		async_answer_0(callid, ENOTSUP);
	}
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
		rc = ELIMIT;
	else if (rtc->removed)
		rc = ENXIO;
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
static unsigned 
bcd2bin(unsigned bcd)
{
	return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xf);
}

/** Convert from binary mode to BCD mode
 *
 * @param bcd   The number in binary mode to convert
 *
 * @return      The converted value
 */
static unsigned
bin2bcd(unsigned binary)
{
	return ((binary / 10) << 4) + (binary % 10);
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
