/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SLEEP_ENTRY_H
#define SLEEP_ENTRY_H

/* Entry points for the sleep command */
extern int cmd_sleep(char **);
extern void help_cmd_sleep(unsigned int);

#endif /* SLEEP_ENTRY_H */
