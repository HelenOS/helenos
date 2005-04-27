/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <arch/acpi/acpi.h>
#include <arch/bios/bios.h>

#define RSDP_SIGNATURE		"RSD PTR "
#define RSDP_REVISION_OFFS	15

struct acpi_rsdp *acpi_rsdp = NULL;

int rsdp_check(__u8 *rsdp) {
	struct acpi_rsdp *r = (struct acpi_rsdp *) rsdp;
	__u8 sum = 0;
	int i;
	
	for (i=0; i<20; i++)
		sum += rsdp[i];
		
	if (sum)	
		return 0; /* bad checksum */

	if (r->revision == 0)
		return 1; /* ACPI 1.0 */
		
	for (; i<r->length; i++)
		sum += rsdp[i];
		
	return !sum;
	
}

void acpi_init(void)
{
        __u8 *addr[2] = { NULL, (__u8 *) 0xe0000 };
        int i, j, length[2] = { 1024, 128*1024 };
	__u64 *sig = (__u64 *) RSDP_SIGNATURE;

        /*
	 * Find Root System Description Pointer
         * 1. search first 1K of EBDA
         * 2. search 128K starting at 0xe0000
         */

	addr[0] = (__u8 *) ebda;
	for (i = (ebda ? 0 : 1); i < 2; i++) {
                for (j = 0; j < length[i]; j += 16) {
                        if (*((__u64 *) &addr[i][j]) == *sig && rsdp_check(&addr[i][j])) {
                                acpi_rsdp = (struct acpi_rsdp *) &addr[i][j];
                                goto rsdp_found;
                        }
                }
        }

        return;

rsdp_found:
        printf("%L: ACPI Root System Description Pointer\n", acpi_rsdp);		
}
