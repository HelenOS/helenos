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

/** @addtogroup test_serial
 * @brief	test the serial port driver - read from the serial port
 * @{
 */ 
/**
 * @file
 */

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
#include <string.h>

#define NAME 		"test serial"


static void print_usage()
{
	printf("Usage: \n test_serial count \n where count is the number of characters to be read\n");	
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf(NAME ": incorrect number of arguments.\n");
		print_usage();
		return 0;		
	}
	
	long int cnt = strtol(argv[1], NULL, 10);
	
	int res;
	res = devman_get_phone(DEVMAN_CLIENT, IPC_FLAG_BLOCKING);
	device_handle_t handle;
	
	if (EOK != (res = devman_device_get_handle("/hw/pci0/00:01.0/com1", &handle, IPC_FLAG_BLOCKING))) {
		printf(NAME ": could not get the device handle, errno = %d.\n", -res);
		return 1;
	}
	
	printf(NAME ": trying to read %d characters from device with handle %d.\n", cnt, handle);	
	
	int phone;
	if (0 >= (phone = devman_device_connect(handle, IPC_FLAG_BLOCKING))) {
		printf(NAME ": could not connect to the device, errno = %d.\n", -res);
		devman_hangup_phone(DEVMAN_CLIENT);		
		return 2;
	}
	
	char *buf = (char *)malloc(cnt+1);
	if (NULL == buf) {
		printf(NAME ": failed to allocate the input buffer\n");
		ipc_hangup(phone);
		devman_hangup_phone(DEVMAN_CLIENT);
		return 3;
	}
	
	int total = 0;
	int read = 0;
	while (total < cnt) {		
		read = read_dev(phone, buf, cnt - total);
		if (0 > read) {
			printf(NAME ": failed read from device, errno = %d.\n", -read);
			ipc_hangup(phone);
			devman_hangup_phone(DEVMAN_CLIENT);
			free(buf);
			return 4;
		}		
		total += read;
		if (read > 0) {			
			buf[read] = 0;
			printf(buf);	
			// write data back to the device to test the opposite direction of data transfer
			write_dev(phone, buf, read);
		} else {	
			usleep(100000);			
		}	
	}
	
	char *the_end = "\n---------\nTHE END\n---------\n";
	write_dev(phone, the_end, str_size(the_end));
	
	devman_hangup_phone(DEVMAN_CLIENT);
	ipc_hangup(phone);
	free(buf);
	
	return 0;
}


/** @}
 */
