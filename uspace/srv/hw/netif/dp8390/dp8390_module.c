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

/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  DP8390 network interface implementation.
 */

#include <assert.h>
#include <async.h>
#include <ddi.h>
#include <errno.h>
#include <malloc.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_err.h>
#include <net_messages.h>
#include <net_modules.h>
#include <packet/packet_client.h>
#include <adt/measured_strings.h>
#include <net_device.h>
#include <nil_interface.h>
#include <netif.h>
#include <netif_module.h>

#include "dp8390.h"
#include "dp8390_drv.h"
#include "dp8390_port.h"

/** DP8390 module name.
 */
#define NAME  "dp8390"

/** Returns the device from the interrupt call.
 *  @param[in] call The interrupt call.
 */
#define IRQ_GET_DEVICE(call)			(device_id_t) IPC_GET_METHOD(*call)

/** Returns the interrupt status register from the interrupt call.
 *  @param[in] call The interrupt call.
 */
#define IPC_GET_ISR(call)				(int) IPC_GET_ARG2(*call)

/** DP8390 kernel interrupt command sequence.
 */
static irq_cmd_t	dp8390_cmds[] = {
	{	.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 2
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 1,
		.srcarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** DP8390 kernel interrupt code.
 */
static irq_code_t	dp8390_code = {
	sizeof(dp8390_cmds) / sizeof(irq_cmd_t),
	dp8390_cmds
};

/** Network interface module global data.
 */
netif_globals_t netif_globals;

/** Handles the interrupt messages.
 *  This is the interrupt handler callback function.
 *  @param[in] iid The interrupt message identifier.
 *  @param[in] call The interrupt message.
 */
void irq_handler(ipc_callid_t iid, ipc_call_t * call);

/** Changes the network interface state.
 *  @param[in,out] device The network interface.
 *  @param[in] state The new state.
 *  @returns The new state.
 */
int change_state(device_ref device, device_state_t state);

int netif_specific_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count){
	return ENOTSUP;
}

int netif_get_device_stats(device_id_t device_id, device_stats_ref stats){
	ERROR_DECLARE;

	device_ref device;
	eth_stat_t * de_stat;

	if(! stats){
		return EBADMEM;
	}
	ERROR_PROPAGATE(find_device(device_id, &device));
	de_stat = &((dpeth_t *) device->specific)->de_stat;
	null_device_stats(stats);
	stats->receive_errors = de_stat->ets_recvErr;
	stats->send_errors = de_stat->ets_sendErr;
	stats->receive_crc_errors = de_stat->ets_CRCerr;
	stats->receive_frame_errors = de_stat->ets_frameAll;
	stats->receive_missed_errors = de_stat->ets_missedP;
	stats->receive_packets = de_stat->ets_packetR;
	stats->send_packets = de_stat->ets_packetT;
	stats->collisions = de_stat->ets_collision;
	stats->send_aborted_errors = de_stat->ets_transAb;
	stats->send_carrier_errors = de_stat->ets_carrSense;
	stats->send_heartbeat_errors = de_stat->ets_CDheartbeat;
	stats->send_window_errors = de_stat->ets_OWC;
	return EOK;
}

int netif_get_addr_message(device_id_t device_id, measured_string_ref address){
	ERROR_DECLARE;

	device_ref device;

	if(! address){
		return EBADMEM;
	}
	ERROR_PROPAGATE(find_device(device_id, &device));
	address->value = (char *) (&((dpeth_t *) device->specific)->de_address);
	address->length = CONVERT_SIZE(ether_addr_t, char, 1);
	return EOK;
}

void irq_handler(ipc_callid_t iid, ipc_call_t * call)
{
	device_ref device;
	dpeth_t * dep;
	packet_t received;
	device_id_t device_id;
	int phone;

	device_id = IRQ_GET_DEVICE(call);
	fibril_rwlock_write_lock(&netif_globals.lock);
	if(find_device(device_id, &device) != EOK){
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return;
	}
	dep = (dpeth_t *) device->specific;
	if (dep->de_mode != DEM_ENABLED){
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return;
	}
	assert(dep->de_flags &DEF_ENABLED);
	dep->de_int_pending = 0;
//	remove debug print:
#ifdef CONFIG_DEBUG
	printf("I%d: 0x%x\n", device_id, IPC_GET_ISR(call));
#endif
	dp_check_ints(dep, IPC_GET_ISR(call));
	if(dep->received_queue){
		received = dep->received_queue;
		phone = device->nil_phone;
		dep->received_queue = NULL;
		dep->received_count = 0;
		fibril_rwlock_write_unlock(&netif_globals.lock);
//	remove debug dump:
#ifdef CONFIG_DEBUG
	uint8_t * data;
	data = packet_get_data(received);
	printf("Receiving packet:\n\tid\t= %d\n\tlength\t= %d\n\tdata\t= %.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX\n\t\t%.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX\n", packet_get_id(received), packet_get_data_length(received), data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40], data[41], data[42], data[43], data[44], data[45], data[46], data[47], data[48], data[49], data[50], data[51], data[52], data[53], data[54], data[55], data[56], data[57], data[58], data[59]);
#endif
		nil_received_msg(phone, device_id, received, NULL);
	}else{
		fibril_rwlock_write_unlock(&netif_globals.lock);
	}
	ipc_answer_0(iid, EOK);
}

int netif_probe_message(device_id_t device_id, int irq, uintptr_t io){
	ERROR_DECLARE;

	device_ref device;
	dpeth_t * dep;

	device = (device_ref) malloc(sizeof(device_t));
	if(! device){
		return ENOMEM;
	}
	dep = (dpeth_t *) malloc(sizeof(dpeth_t));
	if(! dep){
		free(device);
		return ENOMEM;
	}
	bzero(device, sizeof(device_t));
	bzero(dep, sizeof(dpeth_t));
	device->device_id = device_id;
	device->nil_phone = -1;
	device->specific = (void *) dep;
	device->state = NETIF_STOPPED;
	dep->de_irq = irq;
	dep->de_mode = DEM_DISABLED;
	//TODO address?
	if(ERROR_OCCURRED(pio_enable((void *) io, DP8390_IO_SIZE, (void **) &dep->de_base_port))
		|| ERROR_OCCURRED(do_probe(dep))){
		free(dep);
		free(device);
		return ERROR_CODE;
	}
	if(ERROR_OCCURRED(device_map_add(&netif_globals.device_map, device->device_id, device))){
		free(dep);
		free(device);
		return ERROR_CODE;
	}
	return EOK;
}

int netif_send_message(device_id_t device_id, packet_t packet, services_t sender){
	ERROR_DECLARE;

	device_ref device;
	dpeth_t * dep;
	packet_t next;

	ERROR_PROPAGATE(find_device(device_id, &device));
	if(device->state != NETIF_ACTIVE){
		netif_pq_release(packet_get_id(packet));
		return EFORWARD;
	}
	dep = (dpeth_t *) device->specific;
	// process packet queue
	do{
		next = pq_detach(packet);
//		remove debug dump:
#ifdef CONFIG_DEBUG
		uint8_t * data;
		data = packet_get_data(packet);
		printf("Sending packet:\n\tid\t= %d\n\tlength\t= %d\n\tdata\t= %.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX\n\t\t%.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX:%.2hhX %.2hhX %.2hhX %.2hhX\n", packet_get_id(packet), packet_get_data_length(packet), data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40], data[41], data[42], data[43], data[44], data[45], data[46], data[47], data[48], data[49], data[50], data[51], data[52], data[53], data[54], data[55], data[56], data[57], data[58], data[59]);
#endif
		if(do_pwrite(dep, packet, FALSE) != EBUSY){
			netif_pq_release(packet_get_id(packet));
		}
		packet = next;
	}while(packet);
	return EOK;
}

int netif_start_message(device_ref device){
	ERROR_DECLARE;

	dpeth_t * dep;

	if(device->state != NETIF_ACTIVE){
		dep = (dpeth_t *) device->specific;
		dp8390_cmds[0].addr = (void *) (uintptr_t) (dep->de_dp8390_port + DP_ISR);
		dp8390_cmds[2].addr = dp8390_cmds[0].addr;
		ERROR_PROPAGATE(ipc_register_irq(dep->de_irq, device->device_id, device->device_id, &dp8390_code));
		if(ERROR_OCCURRED(do_init(dep, DL_BROAD_REQ))){
			ipc_unregister_irq(dep->de_irq, device->device_id);
			return ERROR_CODE;
		}
		return change_state(device, NETIF_ACTIVE);
	}
	return EOK;
}

int netif_stop_message(device_ref device){
	dpeth_t * dep;

	if(device->state != NETIF_STOPPED){
		dep = (dpeth_t *) device->specific;
		do_stop(dep);
		ipc_unregister_irq(dep->de_irq, device->device_id);
		return change_state(device, NETIF_STOPPED);
	}
	return EOK;
}

int change_state(device_ref device, device_state_t state)
{
	if (device->state != state) {
		device->state = state;
		
		printf("%s: State changed to %s\n", NAME,
		    (state == NETIF_ACTIVE) ? "active" : "stopped");
		
		return state;
	}
	
	return EOK;
}

int netif_initialize(void){
	ipcarg_t phonehash;

	async_set_interrupt_received(irq_handler);

	return REGISTER_ME(SERVICE_DP8390, &phonehash);
}

#ifdef CONFIG_NETWORKING_modular

#include <netif_standalone.h>

/** Default thread for new connections.
 *
 *  @param[in] iid The initial message identifier.
 *  @param[in] icall The initial message call structure.
 *
 */
static void netif_client_connection(ipc_callid_t iid, ipc_call_t * icall)
{
	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	ipc_answer_0(iid, EOK);
	
	while(true) {
		ipc_call_t answer;
		int answer_count;
		
		/* Clear the answer structure */
		refresh_answer(&answer, &answer_count);
		
		/* Fetch the next message */
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		/* Process the message */
		int res = netif_module_message(NAME, callid, &call, &answer,
		    &answer_count);
		
		/* End if said to either by the message or the processing result */
		if ((IPC_GET_METHOD(call) == IPC_M_PHONE_HUNGUP) || (res == EHANGUP))
			return;
		
		/* Answer the message */
		answer_call(callid, res, &answer, answer_count);
	}
}

/** Starts the module.
 *
 *  @param argc The count of the command line arguments. Ignored parameter.
 *  @param argv The command line parameters. Ignored parameter.
 *
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module start function.
 *
 */
int main(int argc, char *argv[])
{
	ERROR_DECLARE;
	
	/* Start the module */
	if (ERROR_OCCURRED(netif_module_start(netif_client_connection)))
		return ERROR_CODE;
	
	return EOK;
}

#endif /* CONFIG_NETWORKING_modular */


/** @}
 */
