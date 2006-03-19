#include <ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ns.h>
#include <errno.h>

static int service;

int main(int argc, char **argv)
{
	ipc_call_t call;
	ipc_callid_t callid;
	
	ipcarg_t retval, arg1, arg2;

	printf("NS:Name service started.\n");
	while (1) {
		call.taskid = -1;
		callid = ipc_wait_for_call(&call, 0);
		printf("NS:Call task=%llX,phone=%lX..", 
		       call.taskid,call.data.phoneid);
		switch (IPC_GET_METHOD(call.data)) {
		case IPC_M_PHONE_HUNGUP:
			printf("Phone hung up.\n");
			retval = 0;
			break;
		case IPC_M_CONNECT_TO_ME:
			printf("Somebody connecting phid=%zd.\n", IPC_GET_ARG3(call.data));
			service = IPC_GET_ARG3(call.data);
			retval = 0;
			break;
		case IPC_M_CONNECT_ME_TO:
			printf("Connectmeto: %zd\n",
			       IPC_GET_ARG1(call.data));
			retval = 0;
			break;
		case NS_PING:
			printf("Ping...%P %P\n", IPC_GET_ARG1(call.data),
			       IPC_GET_ARG2(call.data));
			retval = 0;
			arg1 = 0xdead;
			arg2 = 0xbeef;
			break;
		case NS_HANGUP:
			printf("Closing connection.\n");
			retval = EHANGUP;
			break;
		case NS_PING_SVC:
			printf("NS:Pinging service %d\n", service);
			ipc_call_sync(service, NS_PING, 0xbeef, 0);
			printf("NS:Got pong\n");
			break;
		default:
			printf("Unknown method: %zd\n", IPC_GET_METHOD(call.data));
			retval = ENOENT;
			break;
		}
		ipc_answer(callid, retval, arg1, arg2);
	}
}
