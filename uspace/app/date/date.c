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


#include <stdio.h>
#include <devman.h>
#include <device/clock_dev.h>
#include <errno.h>
#include <loc.h>
#include <time.h>

#define NAME   "date"

int
main(int argc, char **argv)
{
	int rc;
	category_id_t cat_id;
	size_t svc_cnt;
	service_id_t  *svc_ids = NULL;
	char *svc_name = NULL;
	char *devpath;
	devman_handle_t devh;
	struct tm t;

	/* Get the id of the clock category */
	rc = loc_category_get_id("clock", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf(NAME ": Cannot get clock category id\n");
		goto exit;
	}

	/* Get the list of available services in the clock category */
	rc = loc_category_get_svcs(cat_id, &svc_ids, &svc_cnt);
	if (rc != EOK) {
		printf(NAME ": Cannot get the list of services in category \
		    clock\n");
		goto exit;
	}

	/* Check if the are available services in the clock category */
	if (svc_cnt == 0) {
		printf(NAME ": No available service found in \
		    the clock category\n");
		goto exit;
	}

	/* Get the name of the clock service */
	rc = loc_service_get_name(svc_ids[0], &svc_name);
	if (rc != EOK) {
		printf(NAME ": Cannot get the name of the service\n");
		goto exit;
	}

	const char delim = '/';
	devpath = str_chr(svc_name, delim);

	if (!devpath) {
		printf(NAME ": Device name format not recognized\n");
		goto exit;
	}

	/* Skip the delimiter */
	devpath++;

	printf("Found device %s\n", devpath);

	/* Get the device's handle */
	rc = devman_fun_get_handle("/hw/pci0/00:01.0/cmos-rtc/a", &devh,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf(NAME ": Cannot open the device\n");
		goto exit;
	}

	/* Now connect to the device */
	async_sess_t *sess = devman_device_connect(EXCHANGE_SERIALIZE,
	    devh, IPC_FLAG_BLOCKING);
	if (!sess) {
		printf(NAME ": Cannot connect to the device\n");
		goto exit;
	}

	/* Read the current date */
	rc = clock_dev_time_get(sess, &t);
	if (rc != EOK) {
		printf(NAME ": Cannot read the current time\n");
		goto exit;
	}

	printf("%02d/%02d/%d ", t.tm_mday, t.tm_mon + 1, 1900 + t.tm_year);
	printf("%02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);

exit:
	free(svc_name);
	free(svc_ids);
	return rc;
}

