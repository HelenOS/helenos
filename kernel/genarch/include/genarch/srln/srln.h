/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 */

#ifndef KERN_SRLN_H_
#define KERN_SRLN_H_

#include <console/chardev.h>
#include <proc/thread.h>

typedef struct {
	thread_t *thread;

	indev_t *sink;
	indev_t raw;
} srln_instance_t;

extern srln_instance_t *srln_init(void);
extern indev_t *srln_wire(srln_instance_t *, indev_t *);

#endif

/** @}
 */
