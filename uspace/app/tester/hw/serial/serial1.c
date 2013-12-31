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
#include <sys/types.h>
#include <async.h>
#include <ipc/services.h>
#include <loc.h>
#include <char_dev_iface.h>
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
	
	service_id_t svc_id;
	int res = loc_service_get_id("devices/\\hw\\pci0\\00:01.0\\com1\\a",
	    &svc_id, IPC_FLAG_BLOCKING);
	if (res != EOK)
		return "Failed getting serial port service ID";
	
	async_sess_t *sess = loc_service_connect(EXCHANGE_SERIALIZE, svc_id,
	    IPC_FLAG_BLOCKING);
	if (!sess)
		return "Failed connecting to serial device";
	
	char *buf = (char *) malloc(cnt + 1);
	if (buf == NULL) {
		async_hangup(sess);
		return "Failed allocating input buffer";
	}
	
	sysarg_t old_baud;
	sysarg_t old_par;
	sysarg_t old_stop;
	sysarg_t old_word_size;
	
	async_exch_t *exch = async_exchange_begin(sess);
	res = async_req_0_4(exch, SERIAL_GET_COM_PROPS, &old_baud,
	    &old_par, &old_word_size, &old_stop);
	async_exchange_end(exch);
	
	if (res != EOK) {
		free(buf);
		async_hangup(sess);
		return "Failed to get old serial communication parameters";
	}
	
	exch = async_exchange_begin(sess);
	res = async_req_4_0(exch, SERIAL_SET_COM_PROPS, 1200,
	    SERIAL_NO_PARITY, 8, 1);
	async_exchange_end(exch);
	
	if (EOK != res) {
		free(buf);
		async_hangup(sess);
		return "Failed setting serial communication parameters";
	}
	
	TPRINTF("Trying reading %zu characters from serial device "
	    "(svc_id=%" PRIun ")\n", cnt, svc_id);
	
	size_t total = 0;
	while (total < cnt) {
		ssize_t read = char_dev_read(sess, buf, cnt - total);
		
		if (read < 0) {
			exch = async_exchange_begin(sess);
			async_req_4_0(exch, SERIAL_SET_COM_PROPS, old_baud,
			    old_par, old_word_size, old_stop);
			async_exchange_end(exch);
			
			free(buf);
			async_hangup(sess);
			return "Failed reading from serial device";
		}
		
		if ((size_t) read > cnt - total) {
			exch = async_exchange_begin(sess);
			async_req_4_0(exch, SERIAL_SET_COM_PROPS, old_baud,
			    old_par, old_word_size, old_stop);
			async_exchange_end(exch);
			
			free(buf);
			async_hangup(sess);
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
			ssize_t written = char_dev_write(sess, buf, read);
			
			if (written < 0) {
				exch = async_exchange_begin(sess);
				async_req_4_0(exch, SERIAL_SET_COM_PROPS, old_baud,
				    old_par, old_word_size, old_stop);
				async_exchange_end(exch);
				
				free(buf);
				async_hangup(sess);
				return "Failed writing to serial device";
			}
			
			if (written != read) {
				exch = async_exchange_begin(sess);
				async_req_4_0(exch, SERIAL_SET_COM_PROPS, old_baud,
				    old_par, old_word_size, old_stop);
				async_exchange_end(exch);
				
				free(buf);
				async_hangup(sess);
				return "Written less data than read from serial device";
			}
			
			TPRINTF("Written %zd bytes\n", written);
		}
		
		total += read;
	}
	
	TPRINTF("Trying to write EOT banner to the serial device\n");
	
	size_t eot_size = str_size(EOT);
	ssize_t written = char_dev_write(sess, (void *) EOT, eot_size);
	
	exch = async_exchange_begin(sess);
	async_req_4_0(exch, SERIAL_SET_COM_PROPS, old_baud,
	    old_par, old_word_size, old_stop);
	async_exchange_end(exch);
	
	free(buf);
	async_hangup(sess);
	
	if (written < 0)
		return "Failed to write EOT banner to serial device";
	
	if ((size_t) written != eot_size)
		return "Written less data than the size of the EOT banner "
		    "to serial device";
	
	return NULL;
}

/** @}
 */
