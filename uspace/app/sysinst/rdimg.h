/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup sysinst
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef RDIMG_H
#define RDIMG_H

#include <errno.h>
#include <task.h>

/** Open RAM disk image */
typedef struct {
	/** Task ID of file_bd providing the RAM disk */
	task_id_t filebd_tid;
} rd_img_t;

extern errno_t rd_img_open(const char *, char **, rd_img_t **);
extern errno_t rd_img_close(rd_img_t *);

#endif

/** @}
 */
