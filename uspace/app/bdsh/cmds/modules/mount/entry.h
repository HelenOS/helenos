/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOUNT_ENTRY_H
#define MOUNT_ENTRY_H

/* Entry points for the mount command */
extern int cmd_mount(char **);
extern void help_cmd_mount(unsigned int);

#endif /* MOUNT_ENTRY_H */
