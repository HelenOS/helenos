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
#include <stdio.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/clock.h>
#include <fibril_synch.h>
#include <device/hw_res.h>
#include <devman.h>

#define NAME "cmos-rtc"

#define REG_COUNT 2

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
} rtc_t;


static int
rtc_time_get(ddf_fun_t *fun, time_t *t);

static int
rtc_time_set(ddf_fun_t *fun, time_t t);

static int
rtc_dev_add(ddf_dev_t *dev);

static int
rtc_dev_initialize(rtc_t *rtc);

static bool
rtc_pio_enable(rtc_t *rtc);


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

	rtc_dev_ops.open = NULL; /* XXX */
	rtc_dev_ops.close = NULL; /* XXX */

	rtc_dev_ops.interfaces[CLOCK_DEV_IFACE] = &rtc_clock_dev_ops;
	rtc_dev_ops.default_handler = NULL; /* XXX */
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
	/* XXX Do cleanup in case of failure */
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
		return ENOENT;
	}

	/* Get the HW resources */
	rc = hw_res_get_resource_list(rtc->dev->parent_sess, &hw_resources);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to get HW resources\
		    for device %s", rtc->dev->name);
		return rc;
	}

	for (i = 0; i < hw_resources.count; ++i) {
		res = &hw_resources.resources[i];

		if (res->type == IO_RANGE) {
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
		return ENOENT;
	}

	hw_res_clean_resource_list(&hw_resources);

	return EOK;
}

/** Read the current time from the CMOS
 *
 * @param fun  The RTC function
 * @param t    Pointer to the time variable
 *
 * @return  EOK on success or a negative error code
 */
static int
rtc_time_get(ddf_fun_t *fun, time_t *t)
{
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
rtc_time_set(ddf_fun_t *fun, time_t t)
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
	ddf_fun_t *fun;
	int rc;

	ddf_msg(LVL_DEBUG, "rtc_dev_add %s (handle = %d)",
	    dev->name, (int) dev->handle);

	rtc = ddf_dev_data_alloc(dev, sizeof(rtc_t));
	if (!rtc)
		return ENOMEM;

	rtc->dev = dev;
	fibril_mutex_initialize(&rtc->mutex);

	rc = rtc_dev_initialize(rtc);
	if (rc != EOK)
		return rc;

	/* XXX Need cleanup */
	if (!rtc_pio_enable(rtc))
		return EADDRNOTAVAIL;

	fun = ddf_fun_create(dev, fun_exposed, "a");
	if (!fun) {
		ddf_msg(LVL_ERROR, "Failed creating function");
		return ENOENT;
	}

	fun->ops = &rtc_dev_ops;
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function");
		return rc;
	}

	rtc->fun = fun;

	ddf_fun_add_to_category(fun, "clock");

	ddf_msg(LVL_NOTE, "Device %s successfully initialized",
	    dev->name);

	return rc;
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
