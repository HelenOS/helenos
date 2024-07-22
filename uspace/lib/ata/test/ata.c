/*
 * Copyright (c) 2024 Jiri Svoboda
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

#include <ata/ata.h>
#include <errno.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(ata);

static void test_write_data_16(void *, uint16_t *, size_t);
static void test_read_data_16(void *, uint16_t *, size_t);
static void test_write_cmd_8(void *, uint16_t, uint8_t);
static uint8_t test_read_cmd_8(void *, uint16_t);
static void test_write_ctl_8(void *, uint16_t, uint8_t);
static uint8_t test_read_ctl_8(void *, uint16_t);
static errno_t test_irq_enable(void *);
static errno_t test_irq_disable(void *);
static void test_dma_chan_setup(void *, void *, size_t, ata_dma_dir_t);
static void test_dma_chan_teardown(void *);
static errno_t test_add_device(void *, unsigned, void *);
static errno_t test_remove_device(void *, unsigned);
static void test_msg_note(void *, char *);
static void test_msg_error(void *, char *);
static void test_msg_warn(void *, char *);
static void test_msg_debug(void *, char *);

/** ata_channel_create() / ata_channel_destroy() can be called */
PCUT_TEST(channel_create_destroy)
{
	ata_params_t params;
	ata_channel_t *chan;
	errno_t rc;

	memset(&params, 0, sizeof(params));
	params.write_data_16 = test_write_data_16;
	params.read_data_16 = test_read_data_16;
	params.write_cmd_8 = test_write_cmd_8;
	params.read_cmd_8 = test_read_cmd_8;
	params.write_ctl_8 = test_write_ctl_8;
	params.read_ctl_8 = test_read_ctl_8;
	params.irq_enable = test_irq_enable;
	params.irq_disable = test_irq_disable;
	params.dma_chan_setup = test_dma_chan_setup;
	params.dma_chan_teardown = test_dma_chan_teardown;
	params.add_device = test_add_device;
	params.remove_device = test_remove_device;
	params.msg_note = test_msg_note;
	params.msg_error = test_msg_error;
	params.msg_warn = test_msg_warn;
	params.msg_debug = test_msg_debug;

	chan = NULL;
	rc = ata_channel_create(&params, &chan);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ata_channel_destroy(chan);
}

/** ata_channel_initialize() can be called */
PCUT_TEST(channel_initialize)
{
	ata_params_t params;
	ata_channel_t *chan;
	errno_t rc;

	memset(&params, 0, sizeof(params));
	params.write_data_16 = test_write_data_16;
	params.read_data_16 = test_read_data_16;
	params.write_cmd_8 = test_write_cmd_8;
	params.read_cmd_8 = test_read_cmd_8;
	params.write_ctl_8 = test_write_ctl_8;
	params.read_ctl_8 = test_read_ctl_8;
	params.irq_enable = test_irq_enable;
	params.irq_disable = test_irq_disable;
	params.dma_chan_setup = test_dma_chan_setup;
	params.dma_chan_teardown = test_dma_chan_teardown;
	params.add_device = test_add_device;
	params.remove_device = test_remove_device;
	params.msg_note = test_msg_note;
	params.msg_error = test_msg_error;
	params.msg_warn = test_msg_warn;
	params.msg_debug = test_msg_debug;

	chan = NULL;
	rc = ata_channel_create(&params, &chan);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ata_channel_initialize(chan);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ata_channel_destroy(chan);
}

static void test_write_data_16(void *arg, uint16_t *data, size_t nwords)
{
	(void)arg;
	(void)data;
	(void)nwords;
}

static void test_read_data_16(void *arg, uint16_t *buf, size_t nwords)
{
	(void)arg;
	(void)buf;
	(void)nwords;
}

static void test_write_cmd_8(void *arg, uint16_t off, uint8_t value)
{
	(void)arg;
	(void)off;
	(void)value;
}

static uint8_t test_read_cmd_8(void *arg, uint16_t off)
{
	(void)arg;

	if (off == REG_STATUS) {
		/*
		 * This allows us to pass device initialization without
		 * timing out.
		 */
		return SR_DRQ;
	}
	return 0;
}

static void test_write_ctl_8(void *arg, uint16_t off, uint8_t value)
{
	(void)arg;
	(void)off;
	(void)value;
}

static uint8_t test_read_ctl_8(void *arg, uint16_t off)
{
	(void)arg;
	(void)off;
	return 0;
}

static errno_t test_irq_enable(void *arg)
{
	(void)arg;
	return EOK;
}

static errno_t test_irq_disable(void *arg)
{
	(void)arg;
	return EOK;
}

static void test_dma_chan_setup(void *arg, void *buf, size_t buf_size,
    ata_dma_dir_t dir)
{
	(void)arg;
	(void)buf;
	(void)buf_size;
	(void)dir;
}

static void test_dma_chan_teardown(void *arg)
{
	(void)arg;
}

static errno_t test_add_device(void *arg, unsigned idx, void *charg)
{
	(void)arg;
	(void)idx;
	(void)charg;
	return EOK;
}

static errno_t test_remove_device(void *arg, unsigned idx)
{
	(void)arg;
	(void)idx;
	return EOK;
}

static void test_msg_note(void *arg, char *msg)
{
	(void)arg;
	(void)msg;
}

static void test_msg_error(void *arg, char *msg)
{
	(void)arg;
	(void)msg;
}

static void test_msg_warn(void *arg, char *msg)
{
	(void)arg;
	(void)msg;
}

static void test_msg_debug(void *arg, char *msg)
{
	(void)arg;
	(void)msg;
}

PCUT_EXPORT(ata);
