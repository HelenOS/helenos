/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#ifndef MATCH_H_
#define MATCH_H_

#include <stdbool.h>

#include "devman.h"

#define MATCH_EXT ".ma"

extern int get_match_score(driver_t *, dev_node_t *);
extern bool parse_match_ids(char *, match_id_list_t *);
extern bool read_match_ids(const char *, match_id_list_t *);
extern char *read_match_id(char **);
extern char *read_id(const char **);

#endif

/** @}
 */
