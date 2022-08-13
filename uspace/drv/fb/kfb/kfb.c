/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kfb
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <stdio.h>
#include "port.h"
#include "kfb.h"

static errno_t kgraph_dev_add(ddf_dev_t *dev)
{
	port_init(dev);
	printf("%s: Accepting connections\n", NAME);
	return EOK;
}

static driver_ops_t kgraph_driver_ops = {
	.dev_add = kgraph_dev_add,
};

static driver_t kgraph_driver = {
	.name = NAME,
	.driver_ops = &kgraph_driver_ops
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS kernel framebuffer driver\n", NAME);
	return ddf_driver_main(&kgraph_driver);
}

/** @}
 */
