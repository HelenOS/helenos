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
 * Access files as blobs.
 */

#ifndef BITHENGE_FILE_H_
#define BITHENGE_FILE_H_

#include <stdio.h>
#include "blob.h"

errno_t bithenge_new_file_blob(bithenge_node_t **, const char *);
errno_t bithenge_new_file_blob_from_fd(bithenge_node_t **, int);
errno_t bithenge_new_file_blob_from_file(bithenge_node_t **, FILE *);

#endif

/** @}
 */
