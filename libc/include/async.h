#ifndef _libc_ASYNC_H_
#define _libc_ASYNC_H_

#include <ipc/ipc.h>
#include <psthread.h>
#include <sys/time.h>
#include <atomic.h>

typedef ipc_callid_t aid_t;

int async_manager(void);
ipc_callid_t async_get_call(ipc_call_t *data);


aid_t async_send_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
		   ipc_call_t *dataptr);
void async_wait_for(aid_t amsgid, ipcarg_t *result);
int async_wait_timeout(aid_t amsgid, ipcarg_t *retval, suseconds_t timeout);

/** Pseudo-synchronous message sending
 *
 * Send message through IPC, wait in the event loop, until it is received
 *
 * @return Return code of message
 */
static inline ipcarg_t sync_send_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2, ipcarg_t *r1, ipcarg_t *r2)
{
	ipc_call_t result;
	ipcarg_t rc;

	aid_t eid = async_send_2(phoneid, method, arg1, arg2, &result);
	async_wait_for(eid, &rc);
	if (r1) 
		*r1 = IPC_GET_ARG1(result);
	if (r2)
		*r2 = IPC_GET_ARG2(result);
	return rc;
}


pstid_t async_new_connection(ipcarg_t in_phone_hash,ipc_callid_t callid, 
			     ipc_call_t *call,
			     void (*cthread)(ipc_callid_t,ipc_call_t *));
void async_usleep(suseconds_t timeout);
void async_create_manager(void);
void async_destroy_manager(void);
int _async_init(void);

/* Should be defined by application */
void client_connection(ipc_callid_t callid, ipc_call_t *call) __attribute__((weak));
void interrupt_received(ipc_call_t *call)  __attribute__((weak));


extern atomic_t async_futex;

#endif
