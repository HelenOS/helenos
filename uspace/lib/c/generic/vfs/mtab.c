/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <vfs/vfs.h>
#include <vfs/vfs_mtab.h>
#include <macros.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <str.h>

static void process_mp(const char *path, vfs_stat_t *stat, list_t *mtab_list)
{
	mtab_ent_t *ent;

	ent = (mtab_ent_t *) malloc(sizeof(mtab_ent_t));
	if (!ent)
		return;

	link_initialize(&ent->link);
	str_cpy(ent->mp, sizeof(ent->mp), path);
	ent->service_id = stat->service_id;

	vfs_statfs_t stfs;
	if (vfs_statfs_path(path, &stfs) == EOK)
		str_cpy(ent->fs_name, sizeof(ent->fs_name), stfs.fs_name);
	else
		str_cpy(ent->fs_name, sizeof(ent->fs_name), "?");
	
	list_append(&ent->link, mtab_list);
}

static errno_t vfs_get_mtab_visit(const char *path, list_t *mtab_list,
    fs_handle_t fs_handle, service_id_t service_id)
{
	DIR *dir;
	struct dirent *dirent;

	dir = opendir(path);
	if (!dir)
		return ENOENT;

	while ((dirent = readdir(dir)) != NULL) {
		char *child;
		vfs_stat_t st;
		errno_t rc;
		int ret;

		ret = asprintf(&child, "%s/%s", path, dirent->d_name);
		if (ret < 0) {
			closedir(dir);
			return ENOMEM;
		}

		char *pa = vfs_absolutize(child, NULL);
		if (!pa) {
			free(child);
			closedir(dir);
			return ENOMEM;
		}

		free(child);
		child = pa;

		rc = vfs_stat_path(child, &st);
		if (rc != EOK) {
			free(child);
			closedir(dir);
			return rc;
		}

		if (st.fs_handle != fs_handle || st.service_id != service_id) {
			/*
			 * We have discovered a mountpoint.
			 */
			process_mp(child, &st, mtab_list);
		}

		if (st.is_directory) {
			(void) vfs_get_mtab_visit(child, mtab_list,
			    st.fs_handle, st.service_id);
		}

		free(child);
	}

	closedir(dir);
	return EOK;
}

errno_t vfs_get_mtab_list(list_t *mtab_list)
{
	vfs_stat_t st;

	errno_t rc = vfs_stat_path("/", &st);
	if (rc != EOK)
		return rc;

	process_mp("/", &st, mtab_list);

	return vfs_get_mtab_visit("/", mtab_list, st.fs_handle, st.service_id);
}

/** @}
 */
