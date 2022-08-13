/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ELF_LINUX_H_
#define _LIBC_ELF_LINUX_H_

#include <elf/elf.h>
#include <libarch/elf_linux.h>

/*
 * Note types
 */
#define NT_PRSTATUS	1

typedef int pid_t;
typedef struct {
	long tv_sec;
	long tv_usec;
} linux_timeval_t;

typedef struct {
	int sig_info[3];
	short cursig;
	unsigned long sigpend;
	unsigned long sighold;
	pid_t pid;
	pid_t ppid;
	pid_t pgrp;
	pid_t sid;
	linux_timeval_t pr_utime;
	linux_timeval_t pr_stime;
	linux_timeval_t pr_cutime;
	linux_timeval_t pr_sid;
	elf_regs_t regs;
	int fpvalid;
} elf_prstatus_t;

#endif

/** @}
 */
