/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KCON_ENTRY_H
#define KCON_ENTRY_H

/* Entry points for the kcon command */
extern int cmd_kcon(char **);
extern void help_cmd_kcon(unsigned int);

#endif /* KCON_ENTRY_H */
