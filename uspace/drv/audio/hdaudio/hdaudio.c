/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio driver
 */

#include <assert.h>
#include <bitops.h>
#include <ddi.h>
#include <device/hw_res.h>
#include <device/hw_res_parsed.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>

#include "hdactl.h"
#include "hdaudio.h"
#include "pcm_iface.h"
#include "spec/regs.h"

#define NAME "hdaudio"

static errno_t hda_dev_add(ddf_dev_t *dev);
static errno_t hda_dev_remove(ddf_dev_t *dev);
static errno_t hda_dev_gone(ddf_dev_t *dev);
static errno_t hda_fun_online(ddf_fun_t *fun);
static errno_t hda_fun_offline(ddf_fun_t *fun);

static void hdaudio_interrupt(ipc_call_t *, ddf_dev_t *);

static driver_ops_t driver_ops = {
	.dev_add = &hda_dev_add,
	.dev_remove = &hda_dev_remove,
	.dev_gone = &hda_dev_gone,
	.fun_online = &hda_fun_online,
	.fun_offline = &hda_fun_offline
};

static driver_t hda_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

ddf_dev_ops_t hda_pcm_ops = {
	.interfaces[AUDIO_PCM_BUFFER_IFACE] = &hda_pcm_iface
};

irq_pio_range_t hdaudio_irq_pio_ranges[] = {
	{
		.base = 0,
		.size = 8192
	}
};

irq_cmd_t hdaudio_irq_commands[] = {
	/* 0 */
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL, /* rirbsts */
		.dstarg = 2
	},
	/* 1 */
	{
		.cmd = CMD_AND,
		.value = BIT_V(uint8_t, rirbsts_intfl),
		.srcarg = 2,
		.dstarg = 3
	},
	/* 2 */
	{
		.cmd = CMD_PREDICATE,
		.value = 2,
		.srcarg = 3
	},
	/* 3 */
	{
		.cmd = CMD_PIO_WRITE_8,
		.addr = NULL, /* rirbsts */
		.value = BIT_V(uint8_t, rirbsts_intfl)
	},
	/* 4 */
	{
		.cmd = CMD_ACCEPT
	}
};

irq_cmd_t hdaudio_irq_commands_sdesc[] = {
	/* 0 */
	{
		.cmd = CMD_PIO_READ_32,
		.addr = NULL, /* intsts */
		.dstarg = 2
	},
	/* 1 */
	{
		.cmd = CMD_AND,
		.value = 0, /* 1 << idx */
		.srcarg = 2,
		.dstarg = 3,
	},
	/* 2 */
	{
		.cmd = CMD_PREDICATE,
		.value = 2,
		.srcarg = 3
	},
	/* 3 */
	{
		.cmd = CMD_PIO_WRITE_8,
		.addr = NULL, /* sdesc[x].sts */
		.value = 0x4 /* XXX sdesc.sts.BCIS */
	},
	/* 4 */
	{
		.cmd = CMD_ACCEPT
	}
};

static errno_t hda_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun_pcm = NULL;
	hda_t *hda = NULL;
	hw_res_list_parsed_t res;
	irq_code_t irq_code;
	irq_cmd_t *cmds = NULL;
	size_t ncmds_base;
	size_t ncmds_sdesc;
	size_t ncmds;
	int i;
	void *regs = NULL;
	errno_t rc;

	ddf_msg(LVL_NOTE, "hda_dev_add()");
	hw_res_list_parsed_init(&res);

	hda = ddf_dev_data_alloc(dev, sizeof(hda_t));
	if (hda == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.\n");
		rc = ENOMEM;
		goto error;
	}

	fibril_mutex_initialize(&hda->lock);

	ddf_msg(LVL_NOTE, "create parent sess");
	hda->parent_sess = ddf_dev_parent_sess_get(dev);
	if (hda->parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed connecting parent driver.\n");
		rc = ENOMEM;
		goto error;
	}

	ddf_msg(LVL_NOTE, "get HW res list");
	rc = hw_res_get_list_parsed(hda->parent_sess, &res, 0);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting resource list.\n");
		goto error;
	}

	if (res.mem_ranges.count != 1) {
		ddf_msg(LVL_ERROR, "Expected exactly one memory range.\n");
		rc = EINVAL;
		goto error;
	}

	hda->rwbase = RNGABS(res.mem_ranges.ranges[0]);
	hda->rwsize = RNGSZ(res.mem_ranges.ranges[0]);

	ddf_msg(LVL_NOTE, "hda reg base: %" PRIx64,
	     RNGABS(res.mem_ranges.ranges[0]));

	if (hda->rwsize < sizeof(hda_regs_t)) {
		ddf_msg(LVL_ERROR, "Memory range is too small.");
		rc = EINVAL;
		goto error;
	}

	ddf_msg(LVL_NOTE, "enable PIO");
	rc = pio_enable((void *)(uintptr_t)hda->rwbase, hda->rwsize, &regs);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling PIO range.");
		goto error;
	}

	hda->regs = (hda_regs_t *)regs;

	ddf_msg(LVL_NOTE, "IRQs: %zu", res.irqs.count);
	if (res.irqs.count != 1) {
		ddf_msg(LVL_ERROR, "Unexpected IRQ count %zu (!= 1)",
		    res.irqs.count);
		goto error;
	}
	ddf_msg(LVL_NOTE, "interrupt no: %d", res.irqs.irqs[0]);

	ncmds_base = sizeof(hdaudio_irq_commands) / sizeof(irq_cmd_t);
	ncmds_sdesc = sizeof(hdaudio_irq_commands_sdesc) / sizeof(irq_cmd_t);
	ncmds = ncmds_base + 30 * ncmds_sdesc;

	cmds = calloc(ncmds, sizeof(irq_cmd_t));
	if (cmds == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory");
		goto error;
	}

	irq_code.rangecount = sizeof(hdaudio_irq_pio_ranges) /
	    sizeof(irq_pio_range_t);
	irq_code.ranges = hdaudio_irq_pio_ranges;
	irq_code.cmdcount = ncmds;
	irq_code.cmds = cmds;

	hda_regs_t *rphys = (hda_regs_t *)(uintptr_t)hda->rwbase;
	hdaudio_irq_pio_ranges[0].base = (uintptr_t)hda->rwbase;

	memcpy(cmds, hdaudio_irq_commands, sizeof(hdaudio_irq_commands));
	cmds[0].addr = (void *)&rphys->rirbsts;
	cmds[3].addr = (void *)&rphys->rirbsts;

	for (i = 0; i < 30; i++) {
		memcpy(&cmds[ncmds_base + i * ncmds_sdesc],
		    hdaudio_irq_commands_sdesc, sizeof(hdaudio_irq_commands_sdesc));
		cmds[ncmds_base + i * ncmds_sdesc + 0].addr = (void *)&rphys->intsts;
		cmds[ncmds_base + i * ncmds_sdesc + 1].value = BIT_V(uint32_t, i);
		cmds[ncmds_base + i * ncmds_sdesc + 3].addr = (void *)&rphys->sdesc[i].sts;
	}

	ddf_msg(LVL_NOTE, "range0.base=%zu", hdaudio_irq_pio_ranges[0].base);

	rc = hw_res_enable_interrupt(hda->parent_sess, res.irqs.irqs[0]);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed enabling interrupt.: %s", str_error(rc));
		goto error;
	}

	int irq_cap;
	rc = register_interrupt_handler(dev, res.irqs.irqs[0],
	    hdaudio_interrupt, &irq_code, &irq_cap);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed registering interrupt handler: %s",
		    str_error_name(rc));
		goto error;
	}

	free(cmds);
	cmds = NULL;

	if (hda_ctl_init(hda) == NULL) {
		rc = EIO;
		goto error;
	}

	ddf_msg(LVL_NOTE, "create function");
	fun_pcm = ddf_fun_create(dev, fun_exposed, "pcm");
	if (fun_pcm == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'pcm'.");
		rc = ENOMEM;
		goto error;
	}

	hda->fun_pcm = fun_pcm;

	ddf_fun_set_ops(fun_pcm, &hda_pcm_ops);

	rc = ddf_fun_bind(fun_pcm);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'pcm'.");
		ddf_fun_destroy(fun_pcm);
		goto error;
	}

	ddf_fun_add_to_category(fun_pcm, "audio-pcm");

	hw_res_list_parsed_clean(&res);
	return EOK;
error:
	if (fun_pcm != NULL)
		ddf_fun_destroy(fun_pcm);
	if (hda != NULL) {
		if (hda->ctl != NULL)
			hda_ctl_fini(hda->ctl);
	}
	free(cmds);
	// pio_disable(regs);
	hw_res_list_parsed_clean(&res);

	ddf_msg(LVL_NOTE, "Failing hda_dev_add() -> %s", str_error_name(rc));
	return rc;
}

static errno_t hda_dev_remove(ddf_dev_t *dev)
{
	hda_t *hda = (hda_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "hda_dev_remove(%p)", dev);

	if (hda->fun_pcm != NULL) {
		rc = ddf_fun_offline(hda->fun_pcm);
		if (rc != EOK)
			return rc;

		rc = ddf_fun_unbind(hda->fun_pcm);
		if (rc != EOK)
			return rc;
	}

	hda_ctl_fini(hda->ctl);
	// pio_disable(regs);
	return EOK;
}

static errno_t hda_dev_gone(ddf_dev_t *dev)
{
	hda_t *hda = (hda_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "hda_dev_remove(%p)", dev);

	if (hda->fun_pcm != NULL) {
		rc = ddf_fun_unbind(hda->fun_pcm);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

static errno_t hda_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "hda_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t hda_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "hda_fun_offline()");
	return ddf_fun_offline(fun);
}

static void hdaudio_interrupt(ipc_call_t *icall, ddf_dev_t *dev)
{
	hda_t *hda = (hda_t *)ddf_dev_data_get(dev);

	if (0) ddf_msg(LVL_NOTE, "## interrupt ##");
//	ddf_msg(LVL_NOTE, "interrupt arg4=0x%x", (int)IPC_GET_ARG4(*icall));
	hda_ctl_interrupt(hda->ctl);

	if (IPC_GET_ARG3(*icall) != 0) {
		/* Buffer completed */
		hda_lock(hda);
		if (hda->playing) {
			hda_pcm_event(hda, PCM_EVENT_FRAMES_PLAYED);
			hda_pcm_event(hda, PCM_EVENT_FRAMES_PLAYED);
			hda_pcm_event(hda, PCM_EVENT_FRAMES_PLAYED);
			hda_pcm_event(hda, PCM_EVENT_FRAMES_PLAYED);
		} else if (hda->capturing) {
			hda_pcm_event(hda, PCM_EVENT_FRAMES_CAPTURED);
			hda_pcm_event(hda, PCM_EVENT_FRAMES_CAPTURED);
			hda_pcm_event(hda, PCM_EVENT_FRAMES_CAPTURED);
			hda_pcm_event(hda, PCM_EVENT_FRAMES_CAPTURED);
		}

		hda_unlock(hda);
	}
}

void hda_lock(hda_t *hda)
{
	fibril_mutex_lock(&hda->lock);
}

void hda_unlock(hda_t *hda)
{
	fibril_mutex_unlock(&hda->lock);
}

int main(int argc, char *argv[])
{
	printf(NAME ": High Definition Audio driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&hda_driver);
}

/** @}
 */
