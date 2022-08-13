/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief HelenOS input server.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>
#include <async.h>

#define NAME  "input"

extern bool irc_service;
extern async_sess_t *irc_sess;

#endif

/**
 * @}
 */
