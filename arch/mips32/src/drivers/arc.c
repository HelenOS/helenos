/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#include <arch/drivers/arc.h>
#include <arch/mm/page.h>
#include <print.h>
#include <arch.h>
#include <arch/byteorder.h>

/* This is a good joke, SGI HAS different types than NT bioses... */
/* Here is the SGI type */
static char *basetypes[] = {
	"ExceptionBlock",
	"SystemParameterBlock",
	"FreeContiguous",
	"FreeMemory",
	"BadMemory",
	"LoadedProgram",
	"FirmwareTemporary",
	"FirmwarePermanent"
};

static arc_sbp *sbp = (arc_sbp *)PA2KA(0x1000);
static arc_func_vector_t *arc_entry; 

static void _arc_putchar(char ch);

/** Initialize ARC structure
 *
 * @return 0 - ARC OK, -1 - ARC does not exist
 */
int init_arc(void)
{
	if (sbp->signature != ARC_MAGIC) {
		sbp = NULL;
		return -1;
	}
	arc_entry = sbp->firmwarevector;

	arc_putchar('A');
	arc_putchar('R');
	arc_putchar('C');
	arc_putchar('\n');
}

/** Return true if ARC is available */
int arc_enabled(void)
{
	return sbp != NULL;
}

void arc_print_memory_map(void)
{
	arc_memdescriptor_t *desc;

	if (!sbp) {
		printf("ARC not enabled.\n");
		return;
	}

	printf("Memory map:\n");

	desc = arc_entry->getmemorydescriptor(NULL);
	while (desc) {
		printf("%s: %d (size: %dKB)\n",basetypes[desc->type],
		       desc->basepage * 4096,
		       desc->basecount*4);
		desc = arc_entry->getmemorydescriptor(desc);
	}
}

/** Print charactor to console */
void arc_putchar(char ch)
{
	__u32 cnt;
	pri_t pri;

	/* TODO: Should be spinlock? */
	pri = cpu_priority_high();
	arc_entry->write(1, &ch, 1, &cnt);
	cpu_priority_restore(pri);
	
}
