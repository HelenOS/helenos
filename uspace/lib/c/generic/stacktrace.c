/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <io/kio.h>

#define STACK_FRAMES_MAX 20

static errno_t stacktrace_read_uintptr(void *arg, uintptr_t addr, uintptr_t *data);

static stacktrace_ops_t basic_ops = {
	.read_uintptr = stacktrace_read_uintptr,
	.printf = printf,
};

static stacktrace_ops_t kio_ops = {
	.read_uintptr = stacktrace_read_uintptr,
	.printf = kio_printf,
};

void stacktrace_print_generic(stacktrace_ops_t *ops, void *arg, uintptr_t fp,
    uintptr_t pc)
{
	int cnt = 0;
	stacktrace_t st;
	uintptr_t nfp;
	errno_t rc;

	st.op_arg = arg;
	st.ops = ops;

	while (cnt++ < STACK_FRAMES_MAX && stacktrace_fp_valid(&st, fp)) {
		ops->printf("%p: %p()\n", (void *) fp, (void *) pc);
		rc =  stacktrace_ra_get(&st, fp, &pc);
		if (rc != EOK)
			break;
		rc = stacktrace_fp_prev(&st, fp, &nfp);
		if (rc != EOK)
			break;
		fp = nfp;
	}
}

void stacktrace_print_fp_pc(uintptr_t fp, uintptr_t pc)
{
	stacktrace_print_generic(&basic_ops, NULL, fp, pc);
}

void stacktrace_kio_print(void)
{
	stacktrace_prepare();
	stacktrace_print_generic(&kio_ops, NULL, stacktrace_fp_get(), stacktrace_pc_get());

	/*
	 * Prevent the tail call optimization of the previous call by
	 * making it a non-tail call.
	 */

	kio_printf("-- end of stack trace --\n");
}

void stacktrace_print(void)
{
	stacktrace_prepare();
	stacktrace_print_fp_pc(stacktrace_fp_get(), stacktrace_pc_get());

	/*
	 * Prevent the tail call optimization of the previous call by
	 * making it a non-tail call.
	 */

	printf("-- end of stack trace --\n");
}

static errno_t stacktrace_read_uintptr(void *arg, uintptr_t addr, uintptr_t *data)
{
	(void) arg;
	*data = *((uintptr_t *) addr);
	return EOK;
}

/** @}
 */
