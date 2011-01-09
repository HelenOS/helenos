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
 * @brief Test the serial port driver - loopback test
 * @{
 */
/**
 * @file
 */

#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ipc/ipc.h>
#include <sys/types.h>
#include <async.h>
#include <ipc/services.h>
#include <ipc/devman.h>
#include <devman.h>
#include <device/char.h>
#include <str.h>
#include <ipc/serial_ctl.h>
#include "../../tester.h"

#define DEFAULT_COUNT  1024
#define DEFAULT_SLEEP  100000
#define EOT            "####> End of transfer <####\n"

const char *test_serial1(void)
{
	size_t cnt;
	
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
	
	int res = devman_get_phone(DEVMAN_CLIENT, IPC_FLAG_BLOCKING);
	
	devman_handle_t handle;
	res = devman_device_get_handle("/hw/pci0/00:01.0/com1", &handle,
	    IPC_FLAG_BLOCKING);
	if (res != EOK)
		return "Could not get serial device handle";
	
	int phone = devman_device_connect(handle, IPC_FLAG_BLOCKING);
	if (phone < 0) {
		devman_hangup_phone(DEVMAN_CLIENT);
		return "Unable to connect to serial device";
	}
	
	char *buf = (char *) malloc(cnt + 1);
	if (buf == NULL) {
		ipc_hangup(phone);
		devman_hangup_phone(DEVMAN_CLIENT);
		return "Failed to allocate input buffer";
	}
	
	sysarg_t old_baud;
	sysarg_t old_par;
	sysarg_t old_stop;
	sysarg_t old_word_size;
	
	res = ipc_call_sync_0_4(phone, SERIAL_GET_COM_PROPS, &old_baud,
	    &old_par, &old_word_size, &old_stop);
	if (res != EOK) {
		free(buf);
		ipc_hangup(phone);
		devman_hangup_phone(DEVMAN_CLIENT);
		return "Failed to get old serial communication parameters";
	}
	
	res = ipc_call_sync_4_0(phone, SERIAL_SET_COM_PROPS, 1200,
	    SERIAL_NO_PARITY, 8, 1);
	if (EOK != res) {
		free(buf);
		ipc_hangup(phone);
		devman_hangup_phone(DEVMAN_CLIENT);
		return "Failed to set serial communication parameters";
	}
	
	TPRINTF("Trying to read %zu characters from serial device "
	    "(handle=%" PRIun ")\n", cnt, handle);
	
	size_t total = 0;
	while (total < cnt) {
		ssize_t read = char_dev_read(phone, buf, cnt - total);
		
		if (read < 0) {
			ipc_call_sync_4_0(phone, SERIAL_SET_COM_PROPS, old_baud,
			    old_par, old_word_size, old_stop);
			free(buf);
			ipc_hangup(phone);
			devman_hangup_phone(DEVMAN_CLIENT);
			return "Failed read from serial device";
		}
		
		if ((size_t) read > cnt - total) {
			ipc_call_sync_4_0(phone, SERIAL_SET_COM_PROPS, old_baud,
			    old_par, old_word_size, old_stop);
			free(buf);
			ipc_hangup(phone);
			devman_hangup_phone(DEVMAN_CLIENT);
			return "Read more data than expected";
		}
		
		TPRINTF("Read %zd bytes\n", read);
		
		if (read == 0)
			usleep(DEFAULT_SLEEP);
		else {
			buf[read] = 0;
			
			/*
			 * Write data back to the device to test the opposite
			 * direction of data transfer.
			 */
			ssize_t written = char_dev_write(phone, buf, read);
			
			if (written < 0) {
				ipc_call_sync_4_0(phone, SERIAL_SET_COM_PROPS, old_baud,
				    old_par, old_word_size, old_stop);
				free(buf);
				ipc_hangup(phone);
				devman_hangup_phone(DEVMAN_CLIENT);
				return "Failed write to serial device";
			}
			
			if (written != read) {
				ipc_call_sync_4_0(phone, SERIAL_SET_COM_PROPS, old_baud,
				    old_par, old_word_size, old_stop);
				free(buf);
				ipc_hangup(phone);
				devman_hangup_phone(DEVMAN_CLIENT);
				return "Written less data than read from serial device";
			}
			
			TPRINTF("Written %zd bytes\n", written);
		}
		
		total += read;
	}
	
	TPRINTF("Trying to write EOT banner to the serial device\n");
	
	size_t eot_size = str_size(EOT);
	ssize_t written = char_dev_write(phone, (void *) EOT, eot_size);
	
	ipc_call_sync_4_0(phone, SERIAL_SET_COM_PROPS, old_baud,
	    old_par, old_word_size, old_stop);
	free(buf);
	ipc_hangup(phone);
	devman_hangup_phone(DEVMAN_CLIENT);
	
	if (written < 0)
		return "Failed to write EOT banner to serial device";
	
	if ((size_t) written != eot_size)
		return "Written less data than the size of the EOT banner "
		    "to serial device";
	
	return NULL;
}

/** @}
 */
