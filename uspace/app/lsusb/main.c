/*
 * Copyright (c) 2010-2011 Vojtech Horky
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

/** @addtogroup lsusb
 * @{
 */
/**
 * @file
 * Listing of USB host controllers.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <getopt.h>
#include <devman.h>
#include <devmap.h>
#include <usb/host.h>

#define NAME "lsusb"

#define MAX_FAILED_ATTEMPTS 4
#define MAX_PATH_LENGTH 1024

int main(int argc, char *argv[])
{
	size_t class_index = 0;
	size_t failed_attempts = 0;

	while (failed_attempts < MAX_FAILED_ATTEMPTS) {
		class_index++;
		devman_handle_t hc_handle = 0;
		int rc = usb_ddf_get_hc_handle_by_class(class_index, &hc_handle);
		if (rc != EOK) {
			failed_attempts++;
			continue;
		}
		char path[MAX_PATH_LENGTH];
		rc = devman_get_device_path(hc_handle, path, MAX_PATH_LENGTH);
		if (rc != EOK) {
			continue;
		}
		printf(NAME ": host controller %zu is `%s'.\n",
		    class_index, path);
	}

	return 0;
}


/** @}
 */
