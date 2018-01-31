/*
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup genarch
 * @{
 */
/** @file Ski console driver
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <stdio.h>

#include "ski-con.h"

#define NAME  "ski-con"

static errno_t ski_con_dev_add(ddf_dev_t *dev);
static errno_t ski_con_dev_remove(ddf_dev_t *dev);
static errno_t ski_con_dev_gone(ddf_dev_t *dev);
static errno_t ski_con_fun_online(ddf_fun_t *fun);
static errno_t ski_con_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = ski_con_dev_add,
	.dev_remove = ski_con_dev_remove,
	.dev_gone = ski_con_dev_gone,
	.fun_online = ski_con_fun_online,
	.fun_offline = ski_con_fun_offline
};

static driver_t ski_con_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t ski_con_dev_add(ddf_dev_t *dev)
{
	ski_con_t *ski_con;

        ddf_msg(LVL_DEBUG, "ski_con_dev_add(%p)", dev);
	ski_con = ddf_dev_data_alloc(dev, sizeof(ski_con_t));
	if (ski_con == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	ski_con->dev = dev;

	return ski_con_add(ski_con);
}

static errno_t ski_con_dev_remove(ddf_dev_t *dev)
{
        ski_con_t *ski_con = (ski_con_t *)ddf_dev_data_get(dev);

        ddf_msg(LVL_DEBUG, "ski_con_dev_remove(%p)", dev);

        return ski_con_remove(ski_con);
}

static errno_t ski_con_dev_gone(ddf_dev_t *dev)
{
        ski_con_t *ski_con = (ski_con_t *)ddf_dev_data_get(dev);

        ddf_msg(LVL_DEBUG, "ski_con_dev_gone(%p)", dev);

        return ski_con_gone(ski_con);
}

static errno_t ski_con_fun_online(ddf_fun_t *fun)
{
        ddf_msg(LVL_DEBUG, "ski_con_fun_online()");
        return ddf_fun_online(fun);
}

static errno_t ski_con_fun_offline(ddf_fun_t *fun)
{
        ddf_msg(LVL_DEBUG, "ski_con_fun_offline()");
        return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": Ski console driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&ski_con_driver);
}

/** @}
 */
