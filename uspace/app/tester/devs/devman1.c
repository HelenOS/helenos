/*
 * Copyright (c) 2011 Vojtech Horky
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
 * @brief Test devman service.
 * @{
 */
/**
 * @file
 */

#include <inttypes.h>
#include <errno.h>
#include <str_error.h>
#include <sys/types.h>
#include <async.h>
#include <devman.h>
#include <str.h>
#include <vfs/vfs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../tester.h"

#define DEVICE_PATH_NORMAL "/virt/null/a"
#define DEVICE_CLASS "virt-null"
#define DEVICE_CLASS_NAME "1"
#define DEVICE_PATH_CLASSES DEVICE_CLASS "/" DEVICE_CLASS_NAME

const char *test_devman1(void)
{
	devman_handle_t handle_primary;
	devman_handle_t handle_class;
	
	int rc;
	
	TPRINTF("Asking for handle of `%s'...\n", DEVICE_PATH_NORMAL);
	rc = devman_device_get_handle(DEVICE_PATH_NORMAL, &handle_primary, 0);
	if (rc != EOK) {
		TPRINTF(" ...failed: %s.\n", str_error(rc));
		if (rc == ENOENT) {
			TPRINTF("Have you compiled the test drivers?\n");
		}
		return "Failed getting device handle";
	}

	TPRINTF("Asking for handle of `%s' by class..\n", DEVICE_PATH_CLASSES);
	rc = devman_device_get_handle_by_class(DEVICE_CLASS, DEVICE_CLASS_NAME,
	    &handle_class, 0);
	if (rc != EOK) {
		TPRINTF(" ...failed: %s.\n", str_error(rc));
		return "Failed getting device class handle";
	}

	TPRINTF("Received handles %" PRIun " and %" PRIun ".\n",
	    handle_primary, handle_class);
	if (handle_primary != handle_class) {
		return "Retrieved different handles for the same device";
	}

	return NULL;
}

/** @}
 */
