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
 * @brief Chardev keyboard port driver.
 */

#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <io/chardev.h>
#include <loc.h>
#include <stdio.h>
#include "../input.h"
#include "../kbd_port.h"
#include "../kbd.h"

static errno_t kbd_port_fibril(void *);

static errno_t chardev_port_init(kbd_dev_t *);
static void chardev_port_write(uint8_t);

kbd_port_ops_t chardev_port = {
	.init = chardev_port_init,
	.write = chardev_port_write
};

static kbd_dev_t *kbd_dev;
static async_sess_t *dev_sess;
static chardev_t *chardev;

/** List of devices to try connecting to. */
static const char *in_devs[] = {
	/** S3C24xx UART - Openmoko debug console */
	"char/s3c24xx_uart",
	/** Ski console, MSIM console, Sun4v console */
	"devices/\\hw\\console\\a"
};

static const unsigned int num_devs = sizeof(in_devs) / sizeof(in_devs[0]);

static errno_t chardev_port_init(kbd_dev_t *kdev)
{
	service_id_t service_id;
	unsigned int i;
	fid_t fid;
	errno_t rc;

	kbd_dev = kdev;
again:
	for (i = 0; i < num_devs; i++) {
		rc = loc_service_get_id(in_devs[i], &service_id, 0);
		if (rc == EOK)
			break;
	}

	if (i >= num_devs) {
		/* XXX This is just a hack. */
		printf("%s: No input device found, sleep for retry.\n", NAME);
		async_usleep(1000 * 1000);
		goto again;
	}

	dev_sess = loc_service_connect(service_id, INTERFACE_DDF,
	    IPC_FLAG_BLOCKING);
	if (dev_sess == NULL) {
		printf("%s: Failed connecting to device\n", NAME);
		return ENOENT;
	}

	rc = chardev_open(dev_sess, &chardev);
	if (rc != EOK) {
		printf("%s: Failed opening character device\n", NAME);
		async_hangup(dev_sess);
		return ENOMEM;
	}

	fid = fibril_create(kbd_port_fibril, NULL);
	if (fid == 0) {
		printf("%s: Failed creating fibril\n", NAME);
		chardev_close(chardev);
		async_hangup(dev_sess);
		return ENOMEM;
	}

	fibril_add_ready(fid);

	printf("%s: Found input device '%s'\n", NAME, in_devs[i]);
	return 0;
}

static void chardev_port_write(uint8_t data)
{
	errno_t rc;
	size_t nwr;

	rc = chardev_write(chardev, &data, sizeof(data), &nwr);
	if (rc != EOK || nwr != sizeof(data)) {
		printf("%s: Failed writing to character device\n", NAME);
		return;
	}
}

static errno_t kbd_port_fibril(void *arg)
{
	errno_t rc;
	size_t nread;
	uint8_t b;

	while (true) {
		rc = chardev_read(chardev, &b, sizeof(b), &nread);
		if (rc != EOK || nread != sizeof(b)) {
			printf("%s: Error reading data", NAME);
			continue;
		}

		kbd_push_data(kbd_dev, b);
	}

	return 0;
}

/**
 * @}
 */
