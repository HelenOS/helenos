/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Generic module functions implementation.
 */

#include <async.h>
#include <malloc.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <sys/time.h>

#include <net_err.h>
#include <net_modules.h>

/** The time between connect requests in microseconds.
 */
#define MODULE_WAIT_TIME	(10 * 1000)

void answer_call(ipc_callid_t callid, int result, ipc_call_t * answer, int answer_count){
	// choose the most efficient answer function
	if(answer || (! answer_count)){
		switch(answer_count){
			case 0:
				ipc_answer_0(callid, (ipcarg_t) result);
				break;
			case 1:
				ipc_answer_1(callid, (ipcarg_t) result, IPC_GET_ARG1(*answer));
				break;
			case 2:
				ipc_answer_2(callid, (ipcarg_t) result, IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer));
				break;
			case 3:
				ipc_answer_3(callid, (ipcarg_t) result, IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer), IPC_GET_ARG3(*answer));
				break;
			case 4:
				ipc_answer_4(callid, (ipcarg_t) result, IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer), IPC_GET_ARG3(*answer), IPC_GET_ARG4(*answer));
				break;
			case 5:
			default:
				ipc_answer_5(callid, (ipcarg_t) result, IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer), IPC_GET_ARG3(*answer), IPC_GET_ARG4(*answer), IPC_GET_ARG5(*answer));
				break;
		}
	}
}

/** Create bidirectional connection with the needed module service and registers the message receiver.
 *
 * @param[in] need            The needed module service.
 * @param[in] arg1            The first parameter.
 * @param[in] arg2            The second parameter.
 * @param[in] arg3            The third parameter.
 * @param[in] client_receiver The message receiver.
 *
 * @return The phone of the needed service.
 * @return Other error codes as defined for the ipc_connect_to_me() function.
 *
 */
int bind_service(services_t need, ipcarg_t arg1, ipcarg_t arg2, ipcarg_t arg3,
    async_client_conn_t client_receiver)
{
	return bind_service_timeout(need, arg1, arg2, arg3, client_receiver, 0);
}

/** Create bidirectional connection with the needed module service and registers the message receiver.
 *
 * @param[in] need            The needed module service.
 * @param[in] arg1            The first parameter.
 * @param[in] arg2            The second parameter.
 * @param[in] arg3            The third parameter.
 * @param[in] client_receiver The message receiver.
 * @param[in] timeout         The connection timeout in microseconds.
 *                            No timeout if set to zero (0).
 *
 * @return The phone of the needed service.
 * @return ETIMEOUT if the connection timeouted.
 * @return Other error codes as defined for the ipc_connect_to_me() function.
 *
 */
int bind_service_timeout(services_t need, ipcarg_t arg1, ipcarg_t arg2,
    ipcarg_t arg3, async_client_conn_t client_receiver, suseconds_t timeout)
{
	ERROR_DECLARE;
	
	/* Connect to the needed service */
	int phone = connect_to_service_timeout(need, timeout);
	if (phone >= 0) {
		/* Request the bidirectional connection */
		ipcarg_t phonehash;
		if (ERROR_OCCURRED(ipc_connect_to_me(phone, arg1, arg2, arg3,
		    &phonehash))) {
			ipc_hangup(phone);
			return ERROR_CODE;
		}
		async_new_connection(phonehash, 0, NULL, client_receiver);
	}
	
	return phone;
}

int connect_to_service(services_t need){
	return connect_to_service_timeout(need, 0);
}

int connect_to_service_timeout(services_t need, suseconds_t timeout){
	int phone;

	// if no timeout is set
	if (timeout <= 0){
		return async_connect_me_to_blocking(PHONE_NS, need, 0, 0);
	}

	while(true){
		phone = async_connect_me_to(PHONE_NS, need, 0, 0);
		if((phone >= 0) || (phone != ENOENT)){
			return phone;
		}

		// end if no time is left
		if(timeout <= 0){
			return ETIMEOUT;
		}

		// wait the minimum of the module wait time and the timeout
		usleep((timeout <= MODULE_WAIT_TIME) ? timeout : MODULE_WAIT_TIME);
		timeout -= MODULE_WAIT_TIME;
	}
}

int data_receive(void ** data, size_t * length){
	ERROR_DECLARE;

	ipc_callid_t callid;

	if(!(data && length)){
		return EBADMEM;
	}

	// fetch the request
	if(! async_data_write_receive(&callid, length)){
		return EINVAL;
	}

	// allocate the buffer
	*data = malloc(*length);
	if(!(*data)){
		return ENOMEM;
	}

	// fetch the data
	if(ERROR_OCCURRED(async_data_write_finalize(callid, * data, * length))){
		free(data);
		return ERROR_CODE;
	}
	return EOK;
}

int data_reply(void * data, size_t data_length){
	size_t length;
	ipc_callid_t callid;

	// fetch the request
	if(! async_data_read_receive(&callid, &length)){
		return EINVAL;
	}

	// check the requested data size
	if(length < data_length){
		async_data_read_finalize(callid, data, length);
		return EOVERFLOW;
	}

	// send the data
	return async_data_read_finalize(callid, data, data_length);
}

void refresh_answer(ipc_call_t * answer, int * answer_count){

	if(answer_count){
		*answer_count = 0;
	}

	if(answer){
		IPC_SET_RETVAL(*answer, 0);
		// just to be precize
		IPC_SET_METHOD(*answer, 0);
		IPC_SET_ARG1(*answer, 0);
		IPC_SET_ARG2(*answer, 0);
		IPC_SET_ARG3(*answer, 0);
		IPC_SET_ARG4(*answer, 0);
		IPC_SET_ARG5(*answer, 0);
	}
}

/** @}
 */
