/*
 * Copyright (c) 2016 Jakub Jermar
 * Copyright (c) 2017 Jiri Svoboda
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

/** @file
 */

#include <async.h>
#include <config.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/chardev.h>
#include <loc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "../ctl/serial.h"
#include "../output.h"
#include "chardev.h"

enum {
	chardev_buf_size = 4096
};

static char *console;

static async_sess_t *sess;
static chardev_t *chardev;
static service_id_t serial_cat_id;
static service_id_t console_cat_id;

static uint8_t chardev_buf[chardev_buf_size];
static size_t chardev_bused;

static FIBRIL_MUTEX_INITIALIZE(discovery_lock);
static bool discovery_finished;
static FIBRIL_CONDVAR_INITIALIZE(discovery_cv);

static void chardev_flush(void)
{
	size_t nwr;

	if (chardev_bused == 0)
		return;

	chardev_write(chardev, chardev_buf, chardev_bused, &nwr);
	/* XXX Handle error */

	chardev_bused = 0;
}

static void chardev_putchar(wchar_t ch)
{
	if (chardev_bused == chardev_buf_size)
		chardev_flush();
	if (!ascii_check(ch))
		ch = '?';
	chardev_buf[chardev_bused++] = (uint8_t) ch;
}

static void chardev_control_puts(const char *str)
{
	const char *p;

	p = str;
	while (*p != '\0')
		chardev_putchar(*p++);
}

static bool find_output_dev(service_id_t *svcid)
{
	service_id_t *svc;
	size_t svcs;
	errno_t rc;

	rc = loc_category_get_svcs(serial_cat_id, &svc, &svcs);
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		printf("%s: Failed to get services\n", NAME);
		return false;
	}

	for (size_t i = 0; i < svcs; i++) {
		char *name;

		rc = loc_service_get_name(svc[i], &name);
		if (rc != EOK)
			continue;

		if (!str_cmp(console, name)) {
			/*
			 * This is the serial console service that the user
			 * wanted to use.
			 */
			*svcid = svc[i];
			free(svc);
			return true;
		}

		free(name);
	}

	free(svc);

	/* Look for any service in the 'console' category */

	rc = loc_category_get_svcs(console_cat_id, &svc, &svcs);
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		printf("%s: Failed to get services\n", NAME);
		return false;
	}

	if (svcs > 0) {
		*svcid = svc[0];
		free(svc);
		return true;
	}

	free(svc);
	return false;
}

/*
 * This callback scans all the services in the 'serial' category, hoping to see
 * the single one the user wishes to use as a serial console. If it spots it, it
 * connects to it and registers it as an output device. Then it unblocks the
 * fibril blocked in chardev_init().
 */
static void check_for_dev(void)
{
	errno_t rc;
	bool found;
	service_id_t sid;

	fibril_mutex_lock(&discovery_lock);
	if (discovery_finished) {
		// TODO: no need to receive these callbacks anymore
		fibril_mutex_unlock(&discovery_lock);
		return;
	}

	found = find_output_dev(&sid);
	if (!found) {
		fibril_mutex_unlock(&discovery_lock);
		return;
	}

	printf("%s: Connecting service %zu\n", NAME, sid);
	char *name;
	rc = loc_service_get_name(sid, &name);
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		return;
	}
	printf("%s: Service name is %s\n", NAME, name);
	free(name);

	sess = loc_service_connect(sid, INTERFACE_DDF, IPC_FLAG_BLOCKING);
	if (!sess) {
		fibril_mutex_unlock(&discovery_lock);
		printf("%s: Failed connecting to device\n", NAME);
		return;
	}

	rc = chardev_open(sess, &chardev);
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		printf("%s: Failed opening character device\n", NAME);
		return;
	}

	serial_init(chardev_putchar, chardev_control_puts, chardev_flush);

	discovery_finished = true;
	fibril_condvar_signal(&discovery_cv);
	fibril_mutex_unlock(&discovery_lock);
}

errno_t chardev_init(void)
{
	if (!config_key_exists("console")) {
		console = NULL;
#ifdef MACHINE_ski
		/* OK */
#elif defined(UARCH_sparc64) && defined(PROCESSOR_sun4v)
		/* OK */
#elif defined(MACHINE_msim)
		/* OK */
#else
		return EOK;
#endif
	} else {
		console = config_get_value("console");
		if (!console)
			return EOK;
	}

	errno_t rc = loc_category_get_id("serial", &serial_cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed to get \"serial\" category ID.\n", NAME);
		return rc;
	}

	rc = loc_category_get_id("console", &console_cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed to get \"console\" category ID.\n", NAME);
		return rc;
	}

	rc = loc_register_cat_change_cb(check_for_dev);
	if (rc != EOK) {
		printf("%s: Failed to register callback for device discovery.\n",
		    NAME);
		return rc;
	}

	check_for_dev();

	fibril_mutex_lock(&discovery_lock);
	while (!discovery_finished)
		fibril_condvar_wait(&discovery_cv, &discovery_lock);
	fibril_mutex_unlock(&discovery_lock);

	return EOK;
}

/** @}
 */
