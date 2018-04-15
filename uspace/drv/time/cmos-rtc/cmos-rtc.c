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
#include <as.h>
#include <libarch/barrier.h>
#include <stdio.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/clock_dev.h>
#include <ops/battery_dev.h>
#include <fibril_synch.h>
#include <device/hw_res.h>
#include <macros.h>
#include <time.h>

#include "cmos-regs.h"

#define NAME "cmos-rtc"

#define REG_COUNT 2

#define REG_SEL_PORT(port)  (port)
#define REG_RW_PORT(port)   ((port) + 1)

typedef struct rtc {
	/** DDF device node */
	ddf_dev_t *dev;
	/** DDF function node */
	ddf_fun_t *fun;
	/** The fibril mutex for synchronizing the access to the device */
	fibril_mutex_t mutex;
	/** The base I/O address of the device registers */
	ioport8_t *io_addr;
	/** The I/O port used to access the CMOS registers */
	ioport8_t *port;
	/** true if device is removed */
	bool removed;
	/** number of connected clients */
	int clients_connected;
	/** time at which the system booted */
	struct timeval boot_time;
} rtc_t;

static rtc_t *dev_rtc(ddf_dev_t *dev);
static rtc_t *fun_rtc(ddf_fun_t *fun);
static errno_t
rtc_battery_status_get(ddf_fun_t *fun, battery_status_t *status);
static errno_t  rtc_time_get(ddf_fun_t *fun, struct tm *t);
static errno_t  rtc_time_set(ddf_fun_t *fun, struct tm *t);
static errno_t  rtc_dev_add(ddf_dev_t *dev);
static errno_t  rtc_dev_initialize(rtc_t *rtc);
static bool rtc_pio_enable(rtc_t *rtc);
static void rtc_dev_cleanup(rtc_t *rtc);
static errno_t  rtc_open(ddf_fun_t *fun);
static void rtc_close(ddf_fun_t *fun);
static bool rtc_update_in_progress(rtc_t *rtc);
static int  rtc_register_read(rtc_t *rtc, int reg);
static unsigned bcd2bin(unsigned bcd);
static unsigned bin2bcd(unsigned binary);
static errno_t rtc_dev_remove(ddf_dev_t *dev);
static void rtc_register_write(rtc_t *rtc, int reg, int data);
static bool is_battery_ok(rtc_t *rtc);
static errno_t  rtc_fun_online(ddf_fun_t *fun);
static errno_t  rtc_fun_offline(ddf_fun_t *fun);

static ddf_dev_ops_t rtc_dev_ops;

/** The RTC device driver's standard operations */
static driver_ops_t rtc_ops = {
	.dev_add = rtc_dev_add,
	.dev_remove = rtc_dev_remove,
	.fun_online = rtc_fun_online,
	.fun_offline = rtc_fun_offline,
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

/** Battery powered device interface */
static battery_dev_ops_t rtc_battery_dev_ops = {
	.battery_status_get = rtc_battery_status_get,
	.battery_charge_level_get = NULL,
};

/** Obtain soft state structure from device node */
static rtc_t *
dev_rtc(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

/** Obtain soft state structure from function node */
static rtc_t *
fun_rtc(ddf_fun_t *fun)
{
	return dev_rtc(ddf_fun_get_dev(fun));
}

/** Initialize the RTC driver */
static void
rtc_init(void)
{
	ddf_log_init(NAME);

	rtc_dev_ops.open = rtc_open;
	rtc_dev_ops.close = rtc_close;

	rtc_dev_ops.interfaces[CLOCK_DEV_IFACE] = &rtc_clock_dev_ops;
	rtc_dev_ops.interfaces[BATTERY_DEV_IFACE] = &rtc_battery_dev_ops;
	rtc_dev_ops.default_handler = NULL;
}

/** Clean up the RTC soft state
 *
 * @param rtc  The RTC device
 */
static void
rtc_dev_cleanup(rtc_t *rtc)
{
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
	if (pio_enable((void *) rtc->io_addr, REG_COUNT,
	    (void **) &rtc->port)) {

		ddf_msg(LVL_ERROR, "Cannot map the port %lx"
		    " for device %s", (long unsigned int)rtc->io_addr,
		    ddf_dev_get_name(rtc->dev));
		return false;
	}

	return true;
}

/** Initialize the RTC device
 *
 * @param rtc  Pointer to the RTC device
 *
 * @return  EOK on success or an error code
 */
static errno_t
rtc_dev_initialize(rtc_t *rtc)
{
	errno_t rc;
	size_t i;
	hw_resource_t *res;
	bool ioport = false;
	async_sess_t *parent_sess;

	ddf_msg(LVL_DEBUG, "rtc_dev_initialize %s", ddf_dev_get_name(rtc->dev));

	rtc->boot_time.tv_sec = 0;
	rtc->boot_time.tv_usec = 0;
	rtc->clients_connected = 0;

	hw_resource_list_t hw_resources;
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));

	/* Connect to the parent's driver */

	parent_sess = ddf_dev_parent_sess_get(rtc->dev);
	if (parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed to connect to parent driver\
		    of device %s.", ddf_dev_get_name(rtc->dev));
		rc = ENOENT;
		goto error;
	}

	/* Get the HW resources */
	rc = hw_res_get_resource_list(parent_sess, &hw_resources);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to get HW resources\
		    for device %s", ddf_dev_get_name(rtc->dev));
		goto error;
	}

	for (i = 0; i < hw_resources.count; ++i) {
		res = &hw_resources.resources[i];

		if (res->res.io_range.size < REG_COUNT) {
			ddf_msg(LVL_ERROR, "I/O range assigned to \
			    device %s is too small",
			    ddf_dev_get_name(rtc->dev));
			rc = ELIMIT;
			continue;
		}

		rtc->io_addr = (ioport8_t *) (long) res->res.io_range.address;
		ioport = true;
		ddf_msg(LVL_NOTE, "Device %s was assigned I/O address "
		    "0x%lx", ddf_dev_get_name(rtc->dev),
		    (unsigned long int) rtc->io_addr);
		rc = EOK;
		break;
	}

	if (rc != EOK)
		goto error;

	if (!ioport) {
		/* No I/O address assigned to this device */
		ddf_msg(LVL_ERROR, "Missing HW resource for device %s",
		    ddf_dev_get_name(rtc->dev));
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
	pio_write_8(REG_SEL_PORT(rtc->port), reg);
	return pio_read_8(REG_RW_PORT(rtc->port));
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
	pio_write_8(REG_SEL_PORT(rtc->port), reg);
	pio_write_8(REG_RW_PORT(rtc->port), data);
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
	return rtc_register_read(rtc, RTC_STATUS_A) & RTC_A_UPDATE;
}

/** Read the current time from the CMOS
 *
 * @param fun  The RTC function
 * @param t    Pointer to the time variable
 *
 * @return  EOK on success or an error code
 */
static errno_t
rtc_time_get(ddf_fun_t *fun, struct tm *t)
{
	bool bcd_mode;
	bool pm_mode = false;
	rtc_t *rtc = fun_rtc(fun);

	fibril_mutex_lock(&rtc->mutex);

	if (rtc->boot_time.tv_sec) {
		/* There is no need to read the current time from the
		 * device because it has already been cached.
		 */

		struct timeval curtime;

		getuptime(&curtime);
		tv_add(&curtime, &rtc->boot_time);
		fibril_mutex_unlock(&rtc->mutex);

		return time_tv2tm(&curtime, t);
	}

	/* Check if the RTC battery is OK */
	if (!is_battery_ok(rtc)) {
		fibril_mutex_unlock(&rtc->mutex);
		return EIO;
	}

	/* Microseconds are below RTC's resolution, assume 0. */
	t->tm_usec = 0;

	/* now read the registers */
	do {
		/* Suspend until the update process has finished */
		while (rtc_update_in_progress(rtc))
			;

		t->tm_sec = rtc_register_read(rtc, RTC_SEC);
		t->tm_min = rtc_register_read(rtc, RTC_MIN);
		t->tm_hour = rtc_register_read(rtc, RTC_HOUR);
		t->tm_mday = rtc_register_read(rtc, RTC_DAY);
		t->tm_mon = rtc_register_read(rtc, RTC_MON);
		t->tm_year = rtc_register_read(rtc, RTC_YEAR);

		/* Now check if it is stable */
	} while (t->tm_sec != rtc_register_read(rtc, RTC_SEC) ||
	    t->tm_min != rtc_register_read(rtc, RTC_MIN) ||
	    t->tm_mday != rtc_register_read(rtc, RTC_DAY) ||
	    t->tm_mon != rtc_register_read(rtc, RTC_MON) ||
	    t->tm_year != rtc_register_read(rtc, RTC_YEAR));

	/* Check if the RTC is working in 12h mode */
	bool _12h_mode = !(rtc_register_read(rtc, RTC_STATUS_B) &
	    RTC_B_24H);
	if (_12h_mode) {
		/* The RTC is working in 12h mode, check if it is AM or PM */
		if (t->tm_hour & 0x80) {
			/* PM flag is active, it must be cleared */
			t->tm_hour &= ~0x80;
			pm_mode = true;
		}
	}

	/* Check if the RTC is working in BCD mode */
	bcd_mode = !(rtc_register_read(rtc, RTC_STATUS_B) & RTC_B_BCD);
	if (bcd_mode) {
		t->tm_sec = bcd2bin(t->tm_sec);
		t->tm_min = bcd2bin(t->tm_min);
		t->tm_hour = bcd2bin(t->tm_hour);
		t->tm_mday = bcd2bin(t->tm_mday);
		t->tm_mon = bcd2bin(t->tm_mon);
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
		t->tm_year += 100;
	}

	/* Try to normalize the content of the tm structure */
	time_t r = mktime(t);
	errno_t result;

	if (r < 0)
		result = EINVAL;
	else {
		struct timeval uptime;

		getuptime(&uptime);
		rtc->boot_time.tv_sec = r;
		rtc->boot_time.tv_usec = t->tm_usec;	/* normalized */
		tv_sub(&rtc->boot_time, &uptime);
		result = EOK;
	}

	fibril_mutex_unlock(&rtc->mutex);

	return result;
}

/** Set the time in the RTC
 *
 * @param fun  The RTC function
 * @param t    The time value to set
 *
 * @return  EOK or an error code
 */
static errno_t
rtc_time_set(ddf_fun_t *fun, struct tm *t)
{
	bool bcd_mode;
	time_t norm_time;
	struct timeval uptime;
	struct timeval ntv;
	int  reg_b;
	int  reg_a;
	int  epoch;
	rtc_t *rtc = fun_rtc(fun);

	/* Try to normalize the content of the tm structure */
	if ((norm_time = mktime(t)) < 0)
		return EINVAL;

	ntv.tv_sec = norm_time;
	ntv.tv_usec = t->tm_usec;
	getuptime(&uptime);

	if (tv_gteq(&uptime, &ntv)) {
		/* This is not acceptable */
		return EINVAL;
	}

	fibril_mutex_lock(&rtc->mutex);

	if (!is_battery_ok(rtc)) {
		fibril_mutex_unlock(&rtc->mutex);
		return EIO;
	}

	/* boot_time must be recomputed */
	rtc->boot_time.tv_sec = 0;
	rtc->boot_time.tv_usec = 0;

	/* Detect the RTC epoch */
	if (rtc_register_read(rtc, RTC_YEAR) < 100)
		epoch = 2000;
	else
		epoch = 1900;

	if (epoch == 2000 && t->tm_year < 100) {
		/* Can't set a year before the epoch */
		fibril_mutex_unlock(&rtc->mutex);
		return EINVAL;
	}

	t->tm_mon++; /* counts from 1, not from 0 */

	reg_b = rtc_register_read(rtc, RTC_STATUS_B);

	if (!(reg_b & RTC_B_24H)) {
		/* Force 24h mode of operation */
		reg_b |= RTC_B_24H;
		rtc_register_write(rtc, RTC_STATUS_B, reg_b);
	}

	if (epoch == 2000) {
		/* The RTC epoch is year 2000  but the tm_year
		 * field counts years since 1900.
		 */
		t->tm_year -= 100;
	}

	/* Check if the rtc is working in bcd mode */
	bcd_mode = !(reg_b & RTC_B_BCD);
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
	rtc_register_write(rtc, RTC_STATUS_B, reg_b | RTC_B_INH);

	/* Write current time to RTC */
	rtc_register_write(rtc, RTC_SEC, t->tm_sec);
	rtc_register_write(rtc, RTC_MIN, t->tm_min);
	rtc_register_write(rtc, RTC_HOUR, t->tm_hour);
	rtc_register_write(rtc, RTC_DAY, t->tm_mday);
	rtc_register_write(rtc, RTC_MON, t->tm_mon);
	rtc_register_write(rtc, RTC_YEAR, t->tm_year);

	/* Stop the clock */
	reg_a = rtc_register_read(rtc, RTC_STATUS_A);
	rtc_register_write(rtc, RTC_STATUS_A, RTC_A_CLK_STOP | reg_a);

	/* Enable updates */
	rtc_register_write(rtc, RTC_STATUS_B, reg_b);
	rtc_register_write(rtc, RTC_STATUS_A, reg_a);

	fibril_mutex_unlock(&rtc->mutex);

	return EOK;
}

/** Get the status of the real time clock battery
 *
 * @param fun    The RTC function
 * @param status The status of the battery
 *
 * @return       EOK on success or an error code
 */
static errno_t
rtc_battery_status_get(ddf_fun_t *fun, battery_status_t *status)
{
	rtc_t *rtc = fun_rtc(fun);

	fibril_mutex_lock(&rtc->mutex);
	const bool batt_ok = is_battery_ok(rtc);
	fibril_mutex_unlock(&rtc->mutex);

	*status = batt_ok ? BATTERY_OK : BATTERY_LOW;

	return EOK;
}

/** Check if the battery is working properly or not.
 *  The caller already holds the rtc->mutex lock.
 *
 *  @param rtc   The RTC instance.
 *
 *  @return      true if the battery is ok, false otherwise.
 */
static bool
is_battery_ok(rtc_t *rtc)
{
	return rtc_register_read(rtc, RTC_STATUS_D) & RTC_D_BATTERY_OK;
}

/** The dev_add callback of the rtc driver
 *
 * @param dev  The RTC device
 *
 * @return  EOK on success or an error code
 */
static errno_t
rtc_dev_add(ddf_dev_t *dev)
{
	rtc_t *rtc;
	ddf_fun_t *fun = NULL;
	errno_t rc;
	bool need_cleanup = false;

	ddf_msg(LVL_DEBUG, "rtc_dev_add %s (handle = %d)",
	    ddf_dev_get_name(dev), (int) ddf_dev_get_handle(dev));

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

	ddf_fun_set_ops(fun, &rtc_dev_ops);
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function");
		goto error;
	}

	rtc->fun = fun;

	ddf_fun_add_to_category(fun, "clock");

	ddf_msg(LVL_NOTE, "Device %s successfully initialized",
	    ddf_dev_get_name(dev));

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
 * @return      EOK on success or an error code
 */
static errno_t
rtc_dev_remove(ddf_dev_t *dev)
{
	rtc_t *rtc = dev_rtc(dev);
	errno_t rc;

	fibril_mutex_lock(&rtc->mutex);
	if (rtc->clients_connected > 0) {
		fibril_mutex_unlock(&rtc->mutex);
		return EBUSY;
	}

	rtc->removed = true;
	fibril_mutex_unlock(&rtc->mutex);

	rc = rtc_fun_offline(rtc->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to offline function");
		return rc;
	}

	rc = ddf_fun_unbind(rtc->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to unbind function");
		return rc;
	}

	ddf_fun_destroy(rtc->fun);
	rtc_dev_cleanup(rtc);

	return rc;
}

/** Open the device
 *
 * @param fun   The function node
 *
 * @return  EOK on success or an error code
 */
static errno_t
rtc_open(ddf_fun_t *fun)
{
	errno_t rc;
	rtc_t *rtc = fun_rtc(fun);

	fibril_mutex_lock(&rtc->mutex);

	if (rtc->removed)
		rc = ENXIO;
	else {
		rc = EOK;
		rtc->clients_connected++;
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
	rtc_t *rtc = fun_rtc(fun);

	fibril_mutex_lock(&rtc->mutex);

	rtc->clients_connected--;
	assert(rtc->clients_connected >= 0);

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

static errno_t
rtc_fun_online(ddf_fun_t *fun)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, "rtc_fun_online()");

	rc = ddf_fun_online(fun);
	if (rc == EOK)
		ddf_fun_add_to_category(fun, "clock");

	return rc;
}

static errno_t
rtc_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "rtc_fun_offline()");
	return ddf_fun_offline(fun);
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
