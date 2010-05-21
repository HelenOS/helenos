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

/** @addtogroup shutters
 * @brief	closing/opening shutters 
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <ipc/ipc.h>
#include <sys/types.h>
#include <atomic.h>
#include <ipc/devmap.h>
#include <async.h>
#include <ipc/services.h>
#include <ipc/serial.h>
#include <as.h>
#include <sysinfo.h>
#include <errno.h>


#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <str.h>

#define NAME 		"shutters"
#define MYROOM 		2
#define UpSection1 	25
#define DwnSection1 26
#define UpSection2 	27
#define DwnSection2 28

static int device_get_handle(const char *name, dev_handle_t *handle);
static void print_usage();
static void move_shutters(const char * serial_dev_name, char room, char cmd);
static char getcmd(bool wnd, bool up);
static bool is_com_dev(const char *dev_name);

static int device_get_handle(const char *name, dev_handle_t *handle)
{
	int phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP, DEVMAP_CLIENT, 0);
	if (phone < 0)
		return phone;
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAP_DEVICE_GET_HANDLE, 0, 0, &answer);
	
	ipcarg_t retval = ipc_data_write_start(phone, name, str_length(name) + 1); 

	if (retval != EOK) {
		async_wait_for(req, NULL);
		ipc_hangup(phone);
		return retval;
	}

	async_wait_for(req, &retval);

	if (handle != NULL)
		*handle = -1;
	
	if (retval == EOK) {
		if (handle != NULL)
			*handle = (dev_handle_t) IPC_GET_ARG1(answer);
	}
	
	ipc_hangup(phone);
	return retval;
}

static void print_usage()
{
	printf("Usage: \n shutters comN shutter direction \n where 'comN' is a serial port, 'shutter' is either 'window' or 'door' and direction is 'up' or 'down'.\n");	
}

static void move_shutters(const char * serial_dev_name, char room, char cmd)
{
	dev_handle_t serial_dev_handle = -1;
	
	if (device_get_handle(serial_dev_name, &serial_dev_handle) !=  EOK) {
		printf(NAME ": could not get the handle of %s.\n", serial_dev_name);
		return;
	}
	
	printf(NAME ": got the handle of %s.\n", serial_dev_name);
	
	int dev_phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP, DEVMAP_CONNECT_TO_DEVICE, serial_dev_handle);
	if(dev_phone < 0) {
		printf(NAME ": could not connect to %s device.\n", serial_dev_name);
		return;
	}
	
	async_req_1_0(dev_phone, SERIAL_PUTCHAR, 0x55);
	async_req_1_0(dev_phone, SERIAL_PUTCHAR, room);
	async_req_1_0(dev_phone, SERIAL_PUTCHAR, cmd);
	async_req_1_0(dev_phone, SERIAL_PUTCHAR, 240);	
}

static char getcmd(bool wnd, bool up)
{
	char cmd = 0;
	if (wnd) {
		if (up) {
			cmd = UpSection2;
		}
		else {
			cmd = DwnSection2;
		}
	} else {
		if (up) {
			cmd = UpSection1;
		}
		else {
			cmd = DwnSection1;
		}
	}
	return cmd;
}

/*
 * The name of a serial device must be between 'com0' and 'com9'.
 */ 
static bool is_com_dev(const char *dev_name) 
{
	if (str_length(dev_name) != 4) {
		return false;
	}
	
	if (str_cmp("com0", dev_name) > 0) {
		return false;
	}
	
	if (str_cmp(dev_name, "com9") > 0) {
		return false;
	}
	
	return true;
}


int main(int argc, char *argv[])
{
	if (argc != 4) {
		printf(NAME ": incorrect number of arguments.\n");
		print_usage();
		return 0;		
	}	

	bool wnd, up;
	const char *serial_dev = argv[1];
	
	if (!is_com_dev(serial_dev)) {
		printf(NAME ": the first argument is not correct.\n");
		print_usage();
		return 0;	
	}
	
	if (str_cmp(argv[2], "window") == 0) {
		wnd = true;
	} else if (str_cmp(argv[2], "door") == 0) {
		wnd = false;
	} else {
		printf(NAME ": the second argument is not correct.\n");
		print_usage();
		return 0;
	}
	
	if (str_cmp(argv[2], "window") == 0) {
		wnd = true;
	} else if (str_cmp(argv[2], "door") == 0) {
		wnd = false;
	} else {
		printf(NAME ": the third argument is not correct.\n");
		print_usage();
		return 0;
	}
	
	if (str_cmp(argv[3], "up") == 0) {
		up = true;
	} else if (str_cmp(argv[3], "down") == 0) {
		up = false;
	} else {
		printf(NAME ": the second argument is not correct.\n");
		print_usage();
		return 0;
	}
	
	char cmd = getcmd(wnd, up);	
	move_shutters(serial_dev, MYROOM, cmd);
	
	return 0;
}

/** @}
 */
