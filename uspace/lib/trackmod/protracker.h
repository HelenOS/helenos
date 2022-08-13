/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup trackmod
 * @{
 */
/**
 * @file Protracker module (.mod).
 */

#ifndef PROTRACKER_H
#define PROTRACKER_H

#include "types/trackmod.h"

extern errno_t trackmod_protracker_load(char *, trackmod_module_t **);

#endif

/** @}
 */
