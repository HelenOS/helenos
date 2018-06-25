/*
 * Copyright (c) 2014 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup taskdump
 * @{
 */
/** @file
 */

#include <adt/list.h>
#include <context.h>
#include <errno.h>
#include <fibril.h>
#include <fibrildump.h>
#include <stacktrace.h>
#include <stdio.h>
#include <stdbool.h>
#include <symtab.h>
#include <taskdump.h>
#include <udebug.h>

struct fibril {
	link_t all_link;
	context_t ctx;
	uint8_t __opaque[];
};

static errno_t fibrildump_read_uintptr(void *, uintptr_t, uintptr_t *);

static stacktrace_ops_t fibrildump_st_ops = {
	.read_uintptr = fibrildump_read_uintptr
};

static errno_t fibrildump_read_uintptr(void *arg, uintptr_t addr, uintptr_t *data)
{
	async_sess_t *sess = (async_sess_t *)arg;

	return udebug_mem_read(sess, data, addr, sizeof(uintptr_t));
}

static errno_t read_link(async_sess_t *sess, uintptr_t addr, link_t *link)
{
	errno_t rc;

	rc = udebug_mem_read(sess, (void *)link, addr, sizeof(link_t));
	return rc;
}

static errno_t read_fibril(async_sess_t *sess, uintptr_t addr, fibril_t *fibril)
{
	errno_t rc;

	rc = udebug_mem_read(sess, (void *)fibril, addr, sizeof(fibril_t));
	return rc;
}

errno_t fibrils_dump(symtab_t *symtab, async_sess_t *sess)
{
	uintptr_t fibril_list_addr;
	link_t link;
	fibril_t fibril;
	uintptr_t addr, fibril_addr;
	uintptr_t pc, fp;
	errno_t rc;

	/*
	 * If we for whatever reason could not obtain symbols table from the binary,
	 * we cannot dump fibrils.
	 */
	if (symtab == NULL) {
		return EIO;
	}

	rc = symtab_name_to_addr(symtab, "fibril_list", &fibril_list_addr);
	if (rc != EOK)
		return EIO;

	addr = fibril_list_addr;
	while (true) {
		rc = read_link(sess, addr, &link);
		if (rc != EOK)
			return EIO;

		addr = (uintptr_t) link.next;
		if (addr == fibril_list_addr)
			break;

		fibril_addr = (uintptr_t) list_get_instance((void *)addr,
		    fibril_t, all_link);
		printf("Fibril %p:\n", (void *) fibril_addr);
		rc = read_fibril(sess, fibril_addr, &fibril);
		if (rc != EOK)
			return EIO;

		pc = context_get_pc(&fibril.ctx);
		fp = context_get_fp(&fibril.ctx);
		if (0)
			stacktrace_print_generic(&fibrildump_st_ops, sess,
			    fp, pc);
		td_stacktrace(fp, pc);
	}

	return EOK;
}

/** @}
 */
