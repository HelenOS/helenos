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

#include <stdlib.h>
#include <fibril.h>
#include <assert.h>
#include <usb/debug.h>

#include <usb/port.h>

void usb_port_init(usb_port_t *port)
{
	fibril_mutex_initialize(&port->guard);
	fibril_condvar_initialize(&port->finished_cv);
	fibril_condvar_initialize(&port->enabled_cv);
}

struct enumerate_worker_args {
	usb_port_t *port;
	usb_port_enumerate_t handler;
};

static int enumerate_worker(void *arg)
{
	struct enumerate_worker_args * const args = arg;
	usb_port_t *port = args->port;
	usb_port_enumerate_t handler = args->handler;
	free(args);

	fibril_mutex_lock(&port->guard);

	if (port->state == PORT_ERROR) {
		/*
		 * The device was removed faster than this fibril acquired the
		 * mutex.
		 */
		port->state = PORT_DISABLED;
		goto out;
	}

	assert(port->state == PORT_CONNECTING);

	port->state = handler(port)
		? PORT_DISABLED
		: PORT_ENUMERATED;

out:
	fibril_condvar_broadcast(&port->finished_cv);
	fibril_mutex_unlock(&port->guard);
	return EOK; // This is a fibril worker. No one will read the value.
}

int usb_port_connected(usb_port_t *port, usb_port_enumerate_t handler)
{
	assert(port);
	int ret = ENOMEM;

	fibril_mutex_lock(&port->guard);

	if (port->state != PORT_DISABLED) {
		usb_log_warning("a connected event come for port that is not disabled.");
		ret = EINVAL;
		goto out;
	}

	struct enumerate_worker_args *args = malloc(sizeof(*args));
	if (!args)
		goto out;

	fid_t fibril = fibril_create(&enumerate_worker, args);
	if (!fibril) {
		free(args);
		goto out;
	}

	args->port = port;
	args->handler = handler;

	port->state = PORT_CONNECTING;
	fibril_add_ready(fibril);
out:
	fibril_mutex_unlock(&port->guard);
	return ret;
}

void usb_port_enabled(usb_port_t *port)
{
	assert(port);

	fibril_mutex_lock(&port->guard);
	fibril_condvar_broadcast(&port->enabled_cv);
	fibril_mutex_unlock(&port->guard);
}

struct remove_worker_args {
	usb_port_t *port;
	usb_port_remove_t handler;
};

static int remove_worker(void *arg)
{
	struct remove_worker_args * const args = arg;
	usb_port_t *port = args->port;
	usb_port_remove_t handler = args->handler;
	free(args);

	fibril_mutex_lock(&port->guard);
	assert(port->state == PORT_DISCONNECTING);

	handler(port);

	port->state = PORT_DISABLED;
	fibril_condvar_broadcast(&port->finished_cv);
	fibril_mutex_unlock(&port->guard);
	return EOK;
}

static void fork_remove_worker(usb_port_t *port, usb_port_remove_t handler)
{
	struct remove_worker_args *args = malloc(sizeof(*args));
	if (!args)
		return;

	fid_t fibril = fibril_create(&remove_worker, args);
	if (!fibril) {
		free(args);
		return;
	}

	args->port = port;
	args->handler = handler;

	port->state = PORT_DISCONNECTING;
	fibril_add_ready(fibril);
}

void usb_port_disabled(usb_port_t *port, usb_port_remove_t handler)
{
	assert(port);
	fibril_mutex_lock(&port->guard);

	switch (port->state) {
	case PORT_ENUMERATED:
		fork_remove_worker(port, handler);
		break;

	case PORT_CONNECTING:
		port->state = PORT_ERROR;
		fibril_condvar_broadcast(&port->enabled_cv);
		/* fallthrough */
	case PORT_ERROR:
		fibril_condvar_wait(&port->finished_cv, &port->guard);
		/* fallthrough */
	case PORT_DISCONNECTING:
	case PORT_DISABLED:
		break;
	}

	fibril_mutex_unlock(&port->guard);
}

void usb_port_fini(usb_port_t *port)
{
	assert(port);

	fibril_mutex_lock(&port->guard);
	switch (port->state) {
	case PORT_ENUMERATED:
		/*
		 * We shall inform the HC that the device is gone.
		 * However, we can't wait for it, because if the device is hub,
		 * it would have to use the same IPC handling fibril as we do.
		 * But we cannot even defer it to another fibril, because then
		 * the HC would assume our driver didn't cleanup properly, and
		 * will remove those devices by himself.
		 *
		 * So the solutions seems to be to behave like a bad driver and
		 * leave the work for HC.
		 */
		port->state = PORT_DISABLED;
		/* fallthrough */
	case PORT_DISABLED:
		break;

	/* We first have to stop the fibril in progress. */
	case PORT_CONNECTING:
		port->state = PORT_ERROR;
		fibril_condvar_broadcast(&port->enabled_cv);
		/* fallthrough */
	case PORT_ERROR:
	case PORT_DISCONNECTING:
		fibril_condvar_wait(&port->finished_cv, &port->guard);
		break;
	}
	fibril_mutex_unlock(&port->guard);
}

int usb_port_condvar_wait_timeout(usb_port_t *port, fibril_condvar_t *cv, suseconds_t timeout)
{
	assert(port);
	assert(port->state == PORT_CONNECTING);
	assert(fibril_mutex_is_locked(&port->guard));

	if (fibril_condvar_wait_timeout(cv, &port->guard, timeout))
		return ETIMEOUT;

	return port->state == PORT_CONNECTING ? EOK : EINTR;
}

/**
 * @}
 */
