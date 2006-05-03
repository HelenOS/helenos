#include <ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ns.h>
#include <errno.h>
/*
irq_cmd_t msim_cmds[1] = {
	{ CMD_MEM_READ_1, (void *)0xB0000000, 0 }
};

irq_code_t msim_kbd = {
	1,
	msim_cmds
};
*/
/*
irq_cmd_t i8042_cmds[1] = {
	{ CMD_PORT_READ_1, (void *)0x60, 0 }
};

irq_code_t i8042_kbd = {
	1,
	i8042_cmds
};
*/

static int service;

int main(int argc, char **argv)
{
	ipc_call_t call;
	ipc_callid_t callid;
	
	ipcarg_t retval, arg1, arg2;

	printf("NS:Name service started.\n");
//	ipc_register_irq(2, &msim_kbd);
//	ipc_register_irq(1, &i8042_kbd);
	while (1) {
		callid = ipc_wait_for_call(&call, 0);
		printf("NS:Call phone=%lX..", call.phoneid);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_INTERRUPT:
			printf("GOT INTERRUPT: %c\n", IPC_GET_ARG2(call));
			break;
		case IPC_M_PHONE_HUNGUP:
			printf("Phone hung up.\n");
			retval = 0;
			break;
		case IPC_M_CONNECT_TO_ME:
			printf("Somebody connecting phid=%zd.\n", IPC_GET_ARG3(call));
			service = IPC_GET_ARG3(call);
			retval = 0;
			break;
		case IPC_M_CONNECT_ME_TO:
			printf("Connectme(%P)to: %zd\n",
			       IPC_GET_ARG3(call), IPC_GET_ARG1(call));
			retval = 0;
			break;
		case NS_PING:
			printf("Ping...%P %P\n", IPC_GET_ARG1(call),
			       IPC_GET_ARG2(call));
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
			printf("Unknown method: %zd\n", IPC_GET_METHOD(call));
			retval = ENOENT;
			break;
		}
		if (! (callid & IPC_CALLID_NOTIFICATION)) {
			printf("Answerinh\n");
			ipc_answer(callid, retval, arg1, arg2);
		}
	}
}
