/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UNMOUNT_ENTRY_H
#define UNMOUNT_ENTRY_H

/* Entry points for the unmount command */
extern int cmd_unmount(char **);
extern void help_cmd_unmount(unsigned int);

#endif /* UNMOUNT_ENTRY_H */
