/*
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * An USB hub port state machine.
 *
 * This helper structure solves a repeated problem in USB world: management of
 * USB ports. A port is an object which receives events (connect, disconnect,
 * reset) which are to be handled in an asynchronous way. The tricky part is
 * that response to events has to wait for different events - the most notable
 * being USB 2 port requiring port reset to be enabled. This problem is solved
 * by launching separate fibril for taking the port up.
 *
 * This subsystem abstracts the rather complicated state machine, and offers
 * a simple interface to announce events and leave the fibril management on the
 * library.
 */

#ifndef LIB_USB_PORT_H
#define LIB_USB_PORT_H

#include <fibril_synch.h>
#include <usb/usb.h>
#include <errno.h>

typedef enum {
	PORT_DISABLED,	/* No device connected. Fibril not running. */
	PORT_ENUMERATED,/* Device enumerated. Fibril finished succesfully. */
	PORT_CONNECTING,/* A connected event received, fibril running. */
	PORT_DISCONNECTING,/* A disconnected event received, fibril running. */
	PORT_ERROR,	/* An error "in-progress". Fibril still running. */
} usb_port_state_t;

typedef struct usb_port {
	/** Guarding all fields. Is locked in the connected op. */
	fibril_mutex_t guard;
	/** Current state of the port */
	usb_port_state_t state;
	/** CV signalled on fibril exit. */
	fibril_condvar_t finished_cv;
	/** CV signalled on enabled event. */
	fibril_condvar_t enabled_cv;
} usb_port_t;

/**
 * Callback to run the enumeration routine.
 * Called in separate fibril with guard locked.
 */
typedef int (*usb_port_enumerate_t)(usb_port_t *);

/**
 * Callback to run the enumeration routine. Called in the caller fibril,
 */
typedef void (*usb_port_remove_t)(usb_port_t *);

/* Following methods are intended to be called "from outside". */
void usb_port_init(usb_port_t *);
int usb_port_connected(usb_port_t *, usb_port_enumerate_t);
void usb_port_enabled(usb_port_t *);
void usb_port_disabled(usb_port_t *, usb_port_remove_t);
void usb_port_fini(usb_port_t *);

/* And these are to be called from the connected handler. */
int usb_port_condvar_wait_timeout(usb_port_t *port, fibril_condvar_t *, usec_t);

/**
 * Wait for the enabled event to come.
 *
 * @return Error code:
 *	EINTR if the device was disconnected in the meantime.
 *	ETIMEOUT if the enabled event didn't come in 2 seconds
 */
static inline int usb_port_wait_for_enabled(usb_port_t *port)
{
	return usb_port_condvar_wait_timeout(port, &port->enabled_cv, 2000000);
}

#endif

/**
 * @}
 */
