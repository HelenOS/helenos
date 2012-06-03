/*
 * Copyright (c) 2010 Lenka Trochtova
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

/**
 * @defgroup libdrv generic device driver support.
 * @brief HelenOS generic device driver support.
 * @{
 */

/** @file
 */

#include <async.h>
#include <errno.h>
#include <sys/types.h>

#include "ddf/interrupt.h"

static void driver_irq_handler(ipc_callid_t iid, ipc_call_t *icall);
static interrupt_context_t *create_interrupt_context(void);
static void delete_interrupt_context(interrupt_context_t *ctx);
static void init_interrupt_context_list(interrupt_context_list_t *list);
static void add_interrupt_context(interrupt_context_list_t *list,
    interrupt_context_t *ctx);
static void remove_interrupt_context(interrupt_context_list_t *list,
    interrupt_context_t *ctx);
static interrupt_context_t *find_interrupt_context_by_id(
    interrupt_context_list_t *list, int id);
static interrupt_context_t *find_interrupt_context(
    interrupt_context_list_t *list, ddf_dev_t *dev, int irq);
int register_interrupt_handler(ddf_dev_t *dev, int irq,
    interrupt_handler_t *handler, irq_code_t *pseudocode);
int unregister_interrupt_handler(ddf_dev_t *dev, int irq);

/** Interrupts */
static interrupt_context_list_t interrupt_contexts;

static irq_cmd_t default_cmds[] = {
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t default_pseudocode = {
	0,
	NULL,
	sizeof(default_cmds) / sizeof(irq_cmd_t),
	default_cmds
};

void interrupt_init(void)
{
	/* Initialize the list of interrupt contexts. */
	init_interrupt_context_list(&interrupt_contexts);
	
	/* Set generic interrupt handler. */
	async_set_interrupt_received(driver_irq_handler);
}

static void driver_irq_handler(ipc_callid_t iid, ipc_call_t *icall)
{
	int id = (int)IPC_GET_IMETHOD(*icall);
	interrupt_context_t *ctx;
	
	ctx = find_interrupt_context_by_id(&interrupt_contexts, id);
	if (ctx != NULL && ctx->handler != NULL)
		(*ctx->handler)(ctx->dev, iid, icall);
}

static interrupt_context_t *create_interrupt_context(void)
{
	interrupt_context_t *ctx;
	
	ctx = (interrupt_context_t *) malloc(sizeof(interrupt_context_t));
	if (ctx != NULL)
		memset(ctx, 0, sizeof(interrupt_context_t));
	
	return ctx;
}

static void delete_interrupt_context(interrupt_context_t *ctx)
{
	if (ctx != NULL)
		free(ctx);
}

static void init_interrupt_context_list(interrupt_context_list_t *list)
{
	memset(list, 0, sizeof(interrupt_context_list_t));
	fibril_mutex_initialize(&list->mutex);
	list_initialize(&list->contexts);
}

static void add_interrupt_context(interrupt_context_list_t *list,
    interrupt_context_t *ctx)
{
	fibril_mutex_lock(&list->mutex);
	ctx->id = list->curr_id++;
	list_append(&ctx->link, &list->contexts);
	fibril_mutex_unlock(&list->mutex);
}

static void remove_interrupt_context(interrupt_context_list_t *list,
    interrupt_context_t *ctx)
{
	fibril_mutex_lock(&list->mutex);
	list_remove(&ctx->link);
	fibril_mutex_unlock(&list->mutex);
}

static interrupt_context_t *find_interrupt_context_by_id(
    interrupt_context_list_t *list, int id)
{
	interrupt_context_t *ctx;
	
	fibril_mutex_lock(&list->mutex);
	
	list_foreach(list->contexts, link) {
		ctx = list_get_instance(link, interrupt_context_t, link);
		if (ctx->id == id) {
			fibril_mutex_unlock(&list->mutex);
			return ctx;
		}
	}
	
	fibril_mutex_unlock(&list->mutex);
	return NULL;
}

static interrupt_context_t *find_interrupt_context(
    interrupt_context_list_t *list, ddf_dev_t *dev, int irq)
{
	interrupt_context_t *ctx;
	
	fibril_mutex_lock(&list->mutex);
	
	list_foreach(list->contexts, link) {
		ctx = list_get_instance(link, interrupt_context_t, link);
		if (ctx->irq == irq && ctx->dev == dev) {
			fibril_mutex_unlock(&list->mutex);
			return ctx;
		}
	}
	
	fibril_mutex_unlock(&list->mutex);
	return NULL;
}


int register_interrupt_handler(ddf_dev_t *dev, int irq,
    interrupt_handler_t *handler, irq_code_t *pseudocode)
{
	interrupt_context_t *ctx = create_interrupt_context();
	
	ctx->dev = dev;
	ctx->irq = irq;
	ctx->handler = handler;
	
	add_interrupt_context(&interrupt_contexts, ctx);
	
	if (pseudocode == NULL)
		pseudocode = &default_pseudocode;
	
	int res = irq_register(irq, dev->handle, ctx->id, pseudocode);
	if (res != EOK) {
		remove_interrupt_context(&interrupt_contexts, ctx);
		delete_interrupt_context(ctx);
	}
	
	return res;
}

int unregister_interrupt_handler(ddf_dev_t *dev, int irq)
{
	interrupt_context_t *ctx = find_interrupt_context(&interrupt_contexts,
	    dev, irq);
	int res = irq_unregister(irq, dev->handle);
	
	if (ctx != NULL) {
		remove_interrupt_context(&interrupt_contexts, ctx);
		delete_interrupt_context(ctx);
	}
	
	return res;
}

/**
 * @}
 */
