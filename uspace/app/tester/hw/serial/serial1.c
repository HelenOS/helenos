/*
 * Copyright (c) 2009 Lenka Trochtova
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

/** @addtogroup tester
 * @{
 */
/**
 * @file
 * @brief Test the serial port driver - loopback test
 */

#include <async.h>
#include <errno.h>
#include <io/chardev.h>
#include <io/serial.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <str.h>
#include "../../tester.h"

#define DEFAULT_COUNT  1024
#define DEFAULT_SLEEP  100000
#define EOT            "####> End of transfer <####\n"

const char *test_serial1(void)
{
	size_t cnt;
	serial_t *serial;
	chardev_t *chardev;
	errno_t rc;
	size_t nread;
	size_t nwritten;

	if (test_argc < 1)
		cnt = DEFAULT_COUNT;
	else
		switch (str_size_t(test_argv[0], NULL, 0, true, &cnt)) {
		case EOK:
			break;
		case EINVAL:
			return "Invalid argument, unsigned integer expected";
		case EOVERFLOW:
			return "Argument size overflow";
		default:
			return "Unexpected argument error";
		}

	service_id_t svc_id;
	errno_t res = loc_service_get_id("devices/\\hw\\pci0\\00:01.0\\com1\\a",
	    &svc_id, IPC_FLAG_BLOCKING);
	if (res != EOK)
		return "Failed getting serial port service ID";

	async_sess_t *sess = loc_service_connect(svc_id, INTERFACE_DDF,
	    IPC_FLAG_BLOCKING);
	if (sess == NULL)
		return "Failed connecting to serial device";

	res = chardev_open(sess, &chardev);
	if (res != EOK) {
		async_hangup(sess);
		return "Failed opening serial port";
	}

	res = serial_open(sess, &serial);
	if (res != EOK) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed opening serial port";
	}

	char *buf = (char *) malloc(cnt + 1);
	if (buf == NULL) {
		chardev_close(chardev);
		serial_close(serial);
		async_hangup(sess);
		return "Failed allocating input buffer";
	}

	unsigned old_baud;
	serial_parity_t old_par;
	unsigned old_stop;
	unsigned old_word_size;

	res = serial_get_comm_props(serial, &old_baud, &old_par,
	    &old_word_size, &old_stop);
	if (res != EOK) {
		free(buf);
		chardev_close(chardev);
		serial_close(serial);
		async_hangup(sess);
		return "Failed to get old serial communication parameters";
	}

	res = serial_set_comm_props(serial, 1200, SERIAL_NO_PARITY, 8, 1);
	if (EOK != res) {
		free(buf);
		chardev_close(chardev);
		serial_close(serial);
		async_hangup(sess);
		return "Failed setting serial communication parameters";
	}

	TPRINTF("Trying reading %zu characters from serial device "
	    "(svc_id=%" PRIun ")\n", cnt, svc_id);

	size_t total = 0;
	while (total < cnt) {

		rc = chardev_read(chardev, buf, cnt - total, &nread,
		    chardev_f_none);
		if (rc != EOK) {
			(void) serial_set_comm_props(serial, old_baud,
			    old_par, old_word_size, old_stop);

			free(buf);
			chardev_close(chardev);
			serial_close(serial);
			async_hangup(sess);
			return "Failed reading from serial device";
		}

		if (nread > cnt - total) {
			(void) serial_set_comm_props(serial, old_baud,
			    old_par, old_word_size, old_stop);

			free(buf);
			chardev_close(chardev);
			serial_close(serial);
			async_hangup(sess);
			return "Read more data than expected";
		}

		TPRINTF("Read %zd bytes\n", nread);

		buf[nread] = 0;

		/*
		 * Write data back to the device to test the opposite
		 * direction of data transfer.
		 */
		rc = chardev_write(chardev, buf, nread, &nwritten);
		if (rc != EOK) {
			(void) serial_set_comm_props(serial, old_baud,
			    old_par, old_word_size, old_stop);

			free(buf);
			chardev_close(chardev);
			serial_close(serial);
			async_hangup(sess);
			return "Failed writing to serial device";
		}

		if (nwritten != nread) {
			(void) serial_set_comm_props(serial, old_baud,
			    old_par, old_word_size, old_stop);

			free(buf);
			chardev_close(chardev);
			serial_close(serial);
			async_hangup(sess);
			return "Written less data than read from serial device";
		}

		TPRINTF("Written %zd bytes\n", nwritten);

		total += nread;
	}

	TPRINTF("Trying to write EOT banner to the serial device\n");

	size_t eot_size = str_size(EOT);
	rc = chardev_write(chardev, (void *) EOT, eot_size, &nwritten);

	(void) serial_set_comm_props(serial, old_baud, old_par, old_word_size,
	    old_stop);

	free(buf);
	chardev_close(chardev);
	serial_close(serial);
	async_hangup(sess);

	if (rc != EOK)
		return "Failed to write EOT banner to serial device";

	if (nwritten != eot_size)
		return "Written less data than the size of the EOT banner "
		    "to serial device";

	return NULL;
}

/** @}
 */
