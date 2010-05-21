#ifndef LIBC_FB_H_
#define LIBC_FB_H_

#include <ipc/ipc.h>

typedef enum {
	SERIAL_PUTCHAR = IPC_FIRST_USER_METHOD,
	SERIAL_GETCHAR,
	SERIAL_READ,
	SERIAL_WRITE,
} serial_request_t;


#endif
