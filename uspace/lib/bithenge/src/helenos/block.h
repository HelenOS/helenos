/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Access block devices as blobs.
 */

#ifndef BITHENGE_BLOCK_H_
#define BITHENGE_BLOCK_H_

#include <loc.h>
#include <bithenge/tree.h>

errno_t bithenge_new_block_blob(bithenge_node_t **, service_id_t);

#endif

/** @}
 */
