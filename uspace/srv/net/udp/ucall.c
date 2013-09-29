/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup tcp
 * @{
 */

/**
 * @file UDP user calls
 */

#include <io/log.h>
#include <macros.h>

#include "assoc.h"
#include "msg.h"
#include "udp_type.h"
#include "ucall.h"

udp_error_t udp_uc_create(udp_assoc_t **assoc)
{
	udp_assoc_t *nassoc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_create()");
	nassoc = udp_assoc_new(NULL, NULL);
	if (nassoc == NULL)
		return UDP_ENORES;

	udp_assoc_add(nassoc);
	*assoc = nassoc;
	return UDP_EOK;
}

void udp_uc_set_iplink(udp_assoc_t *assoc, service_id_t iplink)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_set_iplink(%p, %zu)",
	    assoc, iplink);

	udp_assoc_set_iplink(assoc, iplink);
}

udp_error_t udp_uc_set_foreign(udp_assoc_t *assoc, udp_sock_t *fsock)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_set_foreign(%p, %p)", assoc, fsock);

	udp_assoc_set_foreign(assoc, fsock);
	return UDP_EOK;
}

udp_error_t udp_uc_set_local(udp_assoc_t *assoc, udp_sock_t *lsock)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_set_local(%p, %p)", assoc, lsock);
	
	udp_assoc_set_local(assoc, lsock);
	return UDP_EOK;
}

udp_error_t udp_uc_set_local_port(udp_assoc_t *assoc, uint16_t lport)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_set_local(%p, %" PRIu16 ")", assoc, lport);
	
	udp_assoc_set_local_port(assoc, lport);
	return UDP_EOK;
}

udp_error_t udp_uc_send(udp_assoc_t *assoc, udp_sock_t *fsock, void *data,
    size_t size, xflags_t flags)
{
	int rc;
	udp_msg_t msg;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: udp_uc_send()", assoc->name);

	msg.data = data;
	msg.data_size = size;

	rc = udp_assoc_send(assoc, fsock, &msg);
	switch (rc) {
	case ENOMEM:
		return UDP_ENORES;
	case EINVAL:
		return UDP_EUNSPEC;
	case EIO:
		return UDP_ENOROUTE;
	}
	return UDP_EOK;
}

udp_error_t udp_uc_receive(udp_assoc_t *assoc, void *buf, size_t size,
    size_t *rcvd, xflags_t *xflags, udp_sock_t *fsock)
{
	size_t xfer_size;
	udp_msg_t *msg;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: udp_uc_receive()", assoc->name);
	rc = udp_assoc_recv(assoc, &msg, fsock);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_recv -> %d", rc);
	switch (rc) {
	case EOK:
		break;
	case ECONNABORTED:
		return UDP_ERESET;
	default:
		assert(false);
	}

	xfer_size = min(size, msg->data_size);
	memcpy(buf, msg->data, xfer_size);
	*rcvd = xfer_size;
	udp_msg_delete(msg);

	return UDP_EOK;
}

void udp_uc_status(udp_assoc_t *assoc, udp_assoc_status_t *astatus)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_status()");
//	cstatus->cstate = conn->cstate;
}

void udp_uc_destroy(udp_assoc_t *assoc)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_uc_destroy()");
	udp_assoc_reset(assoc);
	udp_assoc_remove(assoc);
	udp_assoc_delete(assoc);
}

void udp_uc_reset(udp_assoc_t *assoc)
{
	udp_assoc_reset(assoc);
}

/**
 * @}
 */
