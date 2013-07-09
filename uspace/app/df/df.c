/*
 * Copyright (c) 2013 Manuele Conti
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

/** @addtogroup df
 * @brief Df utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <errno.h>
#include <adt/list.h>
#include <vfs/vfs.h>

#define NAME  "df"

#define HEADER_TABLE "Filesystem    512-blocks      Used      Available  Used%  Mounted on"

#define PERCENTAGE(x, tot) ((long) (100L * (x) / (tot)))  

int main(int argc, char *argv[])
{
	struct statfs st;

	LIST_INITIALIZE(mtab_list);
	get_mtab_list(&mtab_list);
	printf("%s\n", HEADER_TABLE);
	list_foreach(mtab_list, cur) {
		mtab_ent_t *mtab_ent = list_get_instance(cur, mtab_ent_t,
		    link);
		statfs(mtab_ent->mp, &st);
		printf("block size:%ld\n", st.f_bsize);
		printf("%13s %15lld %9lld %9lld %3ld%% %s\n", 
			mtab_ent->fs_name,
			(long long) st.f_blocks * st.f_bsize,
			(long long) st.f_bfree * st.f_bsize,
			(long long) (st.f_blocks - st.f_bfree) * st.f_bsize,
			(st.f_blocks)?PERCENTAGE(st.f_blocks - st.f_bfree, st.f_blocks):0L,
			mtab_ent->mp);
	}
	putchar('\n');	
	return 0;
}

/** @}
 */
