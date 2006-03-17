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

	printf("Name service started.\n");
	while (1) {
		callid = ipc_wait_for_call(&call, 0);
		printf("Received call from: %P..%llX\n", &call.taskid,call.taskid);
		switch (IPC_GET_METHOD(call.data)) {
		case IPC_M_CONNECTTOME:
			printf("Somebody wants to connect with phoneid %zd...accepting\n", IPC_GET_ARG3(call.data));
			service = IPC_GET_ARG3(call.data);
			retval = 0;
			break;
		case IPC_M_CONNECTMETO:
			printf("Somebody wants to connect to: %zd\n",
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
		case NS_PING_SVC:
			printf("Pinging service %d\n", service);
			ipc_call_sync(service, NS_PING, 0xbeef, 0);
			printf("Got pong\n");
			break;
		default:
			printf("Unknown method: %zd\n", IPC_GET_METHOD(call.data));
			retval = ENOENT;
			break;
		}
		ipc_answer(callid, retval, arg1, arg2);
	}
}
