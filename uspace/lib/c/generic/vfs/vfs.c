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

#include <vfs/canonify.h>
#include <vfs/vfs.h>
#include <vfs/vfs_sess.h>
#include <macros.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <fibril_synch.h>
#include <errno.h>
#include <assert.h>
#include <str.h>
#include <loc.h>
#include <ipc/vfs.h>
#include <ipc/loc.h>

static FIBRIL_MUTEX_INITIALIZE(vfs_mutex);
static async_sess_t *vfs_sess = NULL;

static FIBRIL_MUTEX_INITIALIZE(cwd_mutex);

static int cwd_fd = -1;
static char *cwd_path = NULL;
static size_t cwd_size = 0;

/** Start an async exchange on the VFS session.
 *
 * @return New exchange.
 *
 */
async_exch_t *vfs_exchange_begin(void)
{
	fibril_mutex_lock(&vfs_mutex);
	
	while (vfs_sess == NULL)
		vfs_sess = service_connect_blocking(EXCHANGE_PARALLEL, SERVICE_VFS,
		    0, 0);
	
	fibril_mutex_unlock(&vfs_mutex);
	
	return async_exchange_begin(vfs_sess);
}

/** Finish an async exchange on the VFS session.
 *
 * @param exch Exchange to be finished.
 *
 */
void vfs_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

char *absolutize(const char *path, size_t *retlen)
{
	char *ncwd_path;
	char *ncwd_path_nc;

	fibril_mutex_lock(&cwd_mutex);
	size_t size = str_size(path);
	if (*path != '/') {
		if (!cwd_path) {
			fibril_mutex_unlock(&cwd_mutex);
			return NULL;
		}
		ncwd_path_nc = malloc(cwd_size + 1 + size + 1);
		if (!ncwd_path_nc) {
			fibril_mutex_unlock(&cwd_mutex);
			return NULL;
		}
		str_cpy(ncwd_path_nc, cwd_size + 1 + size + 1, cwd_path);
		ncwd_path_nc[cwd_size] = '/';
		ncwd_path_nc[cwd_size + 1] = '\0';
	} else {
		ncwd_path_nc = malloc(size + 1);
		if (!ncwd_path_nc) {
			fibril_mutex_unlock(&cwd_mutex);
			return NULL;
		}
		ncwd_path_nc[0] = '\0';
	}
	str_append(ncwd_path_nc, cwd_size + 1 + size + 1, path);
	ncwd_path = canonify(ncwd_path_nc, retlen);
	if (!ncwd_path) {
		fibril_mutex_unlock(&cwd_mutex);
		free(ncwd_path_nc);
		return NULL;
	}
	/*
	 * We need to clone ncwd_path because canonify() works in-place and thus
	 * the address in ncwd_path need not be the same as ncwd_path_nc, even
	 * though they both point into the same dynamically allocated buffer.
	 */
	ncwd_path = str_dup(ncwd_path);
	free(ncwd_path_nc);
	if (!ncwd_path) {
		fibril_mutex_unlock(&cwd_mutex);
		return NULL;
	}
	fibril_mutex_unlock(&cwd_mutex);
	return ncwd_path;
}

int mount(const char *fs_name, const char *mp, const char *fqsn,
    const char *opts, unsigned int flags, unsigned int instance)
{
	int null_id = -1;
	char null[LOC_NAME_MAXLEN];
	
	if (str_cmp(fqsn, "") == 0) {
		/* No device specified, create a fresh
		   null/%d device instead */
		null_id = loc_null_create();
		
		if (null_id == -1)
			return ENOMEM;
		
		snprintf(null, LOC_NAME_MAXLEN, "null/%d", null_id);
		fqsn = null;
	}
	
	service_id_t service_id;
	int res = loc_service_get_id(fqsn, &service_id, flags);
	if (res != EOK) {
		if (null_id != -1)
			loc_null_destroy(null_id);
		
		return res;
	}
	
	size_t mpa_size;
	char *mpa = absolutize(mp, &mpa_size);
	if (!mpa) {
		if (null_id != -1)
			loc_null_destroy(null_id);
		
		return ENOMEM;
	}
	
	async_exch_t *exch = vfs_exchange_begin();

	sysarg_t rc_orig;
	aid_t req = async_send_3(exch, VFS_IN_MOUNT, service_id, flags,
	    instance, NULL);
	sysarg_t rc = async_data_write_start(exch, (void *) mpa, mpa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(mpa);
		async_wait_for(req, &rc_orig);
		
		if (null_id != -1)
			loc_null_destroy(null_id);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	rc = async_data_write_start(exch, (void *) opts, str_size(opts));
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(mpa);
		async_wait_for(req, &rc_orig);
		
		if (null_id != -1)
			loc_null_destroy(null_id);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	rc = async_data_write_start(exch, (void *) fs_name, str_size(fs_name));
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(mpa);
		async_wait_for(req, &rc_orig);
		
		if (null_id != -1)
			loc_null_destroy(null_id);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	/* Ask VFS whether it likes fs_name. */
	rc = async_req_0_0(exch, VFS_IN_PING);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(mpa);
		async_wait_for(req, &rc_orig);
		
		if (null_id != -1)
			loc_null_destroy(null_id);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	vfs_exchange_end(exch);
	free(mpa);
	async_wait_for(req, &rc);
	
	if ((rc != EOK) && (null_id != -1))
		loc_null_destroy(null_id);
	
	return (int) rc;
}

int unmount(const char *mp)
{
	sysarg_t rc;
	sysarg_t rc_orig;
	aid_t req;
	size_t mpa_size;
	char *mpa;
	
	mpa = absolutize(mp, &mpa_size);
	if (!mpa)
		return ENOMEM;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_0(exch, VFS_IN_UNMOUNT, NULL);
	rc = async_data_write_start(exch, (void *) mpa, mpa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(mpa);
		async_wait_for(req, &rc_orig);
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	

	vfs_exchange_end(exch);
	free(mpa);
	async_wait_for(req, &rc);
	
	return (int) rc;
}

static int open_internal(const char *abs, size_t abs_size, int lflag, int oflag)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	ipc_call_t answer;
	aid_t req = async_send_3(exch, VFS_IN_OPEN, lflag, oflag, 0, &answer);
	sysarg_t rc = async_data_write_start(exch, abs, abs_size);
	
	if (rc != EOK) {
		vfs_exchange_end(exch);

		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	vfs_exchange_end(exch);
	async_wait_for(req, &rc);
	
	if (rc != EOK)
	    return (int) rc;
	
	return (int) IPC_GET_ARG1(answer);
}

int open(const char *path, int oflag, ...)
{
	size_t abs_size;
	char *abs = absolutize(path, &abs_size);
	if (!abs)
		return ENOMEM;
	
	int ret = open_internal(abs, abs_size, L_FILE, oflag);
	free(abs);
	
	return ret;
}

int close(int fildes)
{
	sysarg_t rc;
	
	async_exch_t *exch = vfs_exchange_begin();
	rc = async_req_1_0(exch, VFS_IN_CLOSE, fildes);
	vfs_exchange_end(exch);
	
	return (int) rc;
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
	sysarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	if (nbyte > DATA_XFER_LIMIT)
		nbyte = DATA_XFER_LIMIT;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_READ, fildes, &answer);
	rc = async_data_read_start(exch, (void *) buf, nbyte);
	if (rc != EOK) {
		vfs_exchange_end(exch);

		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);

		if (rc_orig == EOK)
			return (ssize_t) rc;
		else
			return (ssize_t) rc_orig;
	}
	vfs_exchange_end(exch);
	async_wait_for(req, &rc);
	if (rc == EOK)
		return (ssize_t) IPC_GET_ARG1(answer);
	else
		return rc;
}

ssize_t write(int fildes, const void *buf, size_t nbyte) 
{
	sysarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	if (nbyte > DATA_XFER_LIMIT)
		nbyte = DATA_XFER_LIMIT;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_WRITE, fildes, &answer);
	rc = async_data_write_start(exch, (void *) buf, nbyte);
	if (rc != EOK) {
		vfs_exchange_end(exch);

		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);

		if (rc_orig == EOK)
			return (ssize_t) rc;
		else
			return (ssize_t) rc_orig;
	}
	vfs_exchange_end(exch);
	async_wait_for(req, &rc);
	if (rc == EOK)
		return (ssize_t) IPC_GET_ARG1(answer);
	else
		return -1;
}

/** Read entire buffer.
 *
 * In face of short reads this function continues reading until either
 * the entire buffer is read or no more data is available (at end of file).
 *
 * @param fildes	File descriptor
 * @param buf		Buffer, @a nbytes bytes long
 * @param nbytes	Number of bytes to read
 *
 * @return		On success, positive number of bytes read.
 *			On failure, negative error code from read().
 */
ssize_t read_all(int fildes, void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	size_t nread = 0;
	uint8_t *bp = (uint8_t *) buf;

	do {
		bp += cnt;
		nread += cnt;
		cnt = read(fildes, bp, nbyte - nread);
	} while (cnt > 0 && (nbyte - nread - cnt) > 0);

	if (cnt < 0)
		return cnt;

	return nread + cnt;
}

/** Write entire buffer.
 *
 * This function fails if it cannot write exactly @a len bytes to the file.
 *
 * @param fildes	File descriptor
 * @param buf		Data, @a nbytes bytes long
 * @param nbytes	Number of bytes to write
 *
 * @return		EOK on error, return value from write() if writing
 *			failed.
 */
ssize_t write_all(int fildes, const void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	ssize_t nwritten = 0;
	const uint8_t *bp = (uint8_t *) buf;

	do {
		bp += cnt;
		nwritten += cnt;
		cnt = write(fildes, bp, nbyte - nwritten);
	} while (cnt > 0 && ((ssize_t )nbyte - nwritten - cnt) > 0);

	if (cnt < 0)
		return cnt;

	if ((ssize_t)nbyte - nwritten - cnt > 0)
		return EIO;

	return nbyte;
}

int fsync(int fildes)
{
	async_exch_t *exch = vfs_exchange_begin();
	sysarg_t rc = async_req_1_0(exch, VFS_IN_SYNC, fildes);
	vfs_exchange_end(exch);
	
	return (int) rc;
}

off64_t lseek(int fildes, off64_t offset, int whence)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	sysarg_t newoff_lo;
	sysarg_t newoff_hi;
	sysarg_t rc = async_req_4_2(exch, VFS_IN_SEEK, fildes,
	    LOWER32(offset), UPPER32(offset), whence,
	    &newoff_lo, &newoff_hi);
	
	vfs_exchange_end(exch);
	
	if (rc != EOK)
		return (off64_t) -1;
	
	return (off64_t) MERGE_LOUP32(newoff_lo, newoff_hi);
}

int ftruncate(int fildes, aoff64_t length)
{
	sysarg_t rc;
	
	async_exch_t *exch = vfs_exchange_begin();
	rc = async_req_3_0(exch, VFS_IN_TRUNCATE, fildes,
	    LOWER32(length), UPPER32(length));
	vfs_exchange_end(exch);
	
	return (int) rc;
}

int fstat(int fildes, struct stat *stat)
{
	sysarg_t rc;
	aid_t req;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_FSTAT, fildes, NULL);
	rc = async_data_read_start(exch, (void *) stat, sizeof(struct stat));
	if (rc != EOK) {
		vfs_exchange_end(exch);

		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);

		if (rc_orig == EOK)
			return (ssize_t) rc;
		else
			return (ssize_t) rc_orig;
	}
	vfs_exchange_end(exch);
	async_wait_for(req, &rc);

	return rc;
}

int stat(const char *path, struct stat *stat)
{
	sysarg_t rc;
	sysarg_t rc_orig;
	aid_t req;
	
	size_t pa_size;
	char *pa = absolutize(path, &pa_size);
	if (!pa)
		return ENOMEM;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_0(exch, VFS_IN_STAT, NULL);
	rc = async_data_write_start(exch, pa, pa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(pa);
		async_wait_for(req, &rc_orig);
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	rc = async_data_read_start(exch, stat, sizeof(struct stat));
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(pa);
		async_wait_for(req, &rc_orig);
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	vfs_exchange_end(exch);
	free(pa);
	async_wait_for(req, &rc);
	return rc;
}

DIR *opendir(const char *dirname)
{
	DIR *dirp = malloc(sizeof(DIR));
	if (!dirp)
		return NULL;
	
	size_t abs_size;
	char *abs = absolutize(dirname, &abs_size);
	if (!abs) {
		free(dirp);
		return NULL;
	}
	
	int ret = open_internal(abs, abs_size, L_DIRECTORY, 0);
	free(abs);
	
	if (ret < 0) {
		free(dirp);
		return NULL;
	}
	
	dirp->fd = ret;
	return dirp;
}

struct dirent *readdir(DIR *dirp)
{
	ssize_t len = read(dirp->fd, &dirp->res.d_name[0], NAME_MAX + 1);
	if (len <= 0)
		return NULL;
	return &dirp->res;
}

void rewinddir(DIR *dirp)
{
	(void) lseek(dirp->fd, 0, SEEK_SET);
}

int closedir(DIR *dirp)
{
	(void) close(dirp->fd);
	free(dirp);
	return 0;
}

int mkdir(const char *path, mode_t mode)
{
	sysarg_t rc;
	aid_t req;
	
	size_t pa_size;
	char *pa = absolutize(path, &pa_size);
	if (!pa)
		return ENOMEM;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_MKDIR, mode, NULL);
	rc = async_data_write_start(exch, pa, pa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(pa);

		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);

		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	vfs_exchange_end(exch);
	free(pa);
	async_wait_for(req, &rc);
	return rc;
}

static int _unlink(const char *path, int lflag)
{
	sysarg_t rc;
	aid_t req;
	
	size_t pa_size;
	char *pa = absolutize(path, &pa_size);
	if (!pa)
		return ENOMEM;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_UNLINK, lflag, NULL);
	rc = async_data_write_start(exch, pa, pa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(pa);

		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);

		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	vfs_exchange_end(exch);
	free(pa);
	async_wait_for(req, &rc);
	return rc;
}

int unlink(const char *path)
{
	return _unlink(path, L_NONE);
}

int rmdir(const char *path)
{
	return _unlink(path, L_DIRECTORY);
}

int rename(const char *old, const char *new)
{
	sysarg_t rc;
	sysarg_t rc_orig;
	aid_t req;
	
	size_t olda_size;
	char *olda = absolutize(old, &olda_size);
	if (!olda)
		return ENOMEM;

	size_t newa_size;
	char *newa = absolutize(new, &newa_size);
	if (!newa) {
		free(olda);
		return ENOMEM;
	}
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_0(exch, VFS_IN_RENAME, NULL);
	rc = async_data_write_start(exch, olda, olda_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(olda);
		free(newa);
		async_wait_for(req, &rc_orig);
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	rc = async_data_write_start(exch, newa, newa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(olda);
		free(newa);
		async_wait_for(req, &rc_orig);
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	vfs_exchange_end(exch);
	free(olda);
	free(newa);
	async_wait_for(req, &rc);
	return rc;
}

int remove(const char *path)
{
	return unlink(path);
}

int chdir(const char *path)
{
	size_t abs_size;
	char *abs = absolutize(path, &abs_size);
	if (!abs)
		return ENOMEM;
	
	int fd = open_internal(abs, abs_size, L_DIRECTORY, O_DESC);
	
	if (fd < 0) {
		free(abs);
		return ENOENT;
	}
	
	fibril_mutex_lock(&cwd_mutex);
	
	if (cwd_fd >= 0)
		close(cwd_fd);
	
	
	if (cwd_path)
		free(cwd_path);
	
	cwd_fd = fd;
	cwd_path = abs;
	cwd_size = abs_size;
	
	fibril_mutex_unlock(&cwd_mutex);
	return EOK;
}

char *getcwd(char *buf, size_t size)
{
	if (size == 0)
		return NULL;
	
	fibril_mutex_lock(&cwd_mutex);
	
	if ((cwd_size == 0) || (size < cwd_size + 1)) {
		fibril_mutex_unlock(&cwd_mutex);
		return NULL;
	}
	
	str_cpy(buf, size, cwd_path);
	fibril_mutex_unlock(&cwd_mutex);
	
	return buf;
}

async_sess_t *fd_session(exch_mgmt_t mgmt, int fildes)
{
	struct stat stat;
	int rc = fstat(fildes, &stat);
	if (rc != 0) {
		errno = rc;
		return NULL;
	}
	
	if (!stat.service) {
		errno = ENOENT;
		return NULL;
	}
	
	return loc_service_connect(mgmt, stat.service, 0);
}

int dup2(int oldfd, int newfd)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	sysarg_t ret;
	sysarg_t rc = async_req_2_1(exch, VFS_IN_DUP, oldfd, newfd, &ret);
	
	vfs_exchange_end(exch);
	
	if (rc == EOK)
		return (int) ret;
	
	return (int) rc;
}

int fd_wait(void)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	sysarg_t ret;
	sysarg_t rc = async_req_0_1(exch, VFS_IN_WAIT_HANDLE, &ret);
	
	vfs_exchange_end(exch);
	
	if (rc == EOK)
		return (int) ret;
	
	return (int) rc;
}

int get_mtab_list(list_t *mtab_list)
{
	sysarg_t rc;
	aid_t req;
	size_t i;
	sysarg_t num_mounted_fs;
	
	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_0(exch, VFS_IN_MTAB_GET, NULL);

	/* Ask VFS how many filesystems are mounted */
	rc = async_req_0_1(exch, VFS_IN_PING, &num_mounted_fs);
	if (rc != EOK)
		goto exit;

	for (i = 0; i < num_mounted_fs; ++i) {
		mtab_ent_t *mtab_ent;

		mtab_ent = malloc(sizeof(mtab_ent_t));
		if (!mtab_ent) {
			rc = ENOMEM;
			goto exit;
		}

		memset(mtab_ent, 0, sizeof(mtab_ent_t));

		rc = async_data_read_start(exch, (void *) mtab_ent->mp,
		    MAX_PATH_LEN);
		if (rc != EOK)
			goto exit;

		rc = async_data_read_start(exch, (void *) mtab_ent->opts,
			MAX_MNTOPTS_LEN);
		if (rc != EOK)
			goto exit;

		rc = async_data_read_start(exch, (void *) mtab_ent->fs_name,
			FS_NAME_MAXLEN);
		if (rc != EOK)
			goto exit;

		sysarg_t p[2];

		rc = async_req_0_2(exch, VFS_IN_PING, &p[0], &p[1]);
		if (rc != EOK)
			goto exit;

		mtab_ent->instance = p[0];
		mtab_ent->service_id = p[1];

		link_initialize(&mtab_ent->link);
		list_append(&mtab_ent->link, mtab_list);
	}

exit:
	async_wait_for(req, &rc);
	vfs_exchange_end(exch);
	return rc;
}

int statfs(const char *path, struct statfs *st)
{
	sysarg_t rc, rc_orig;
	aid_t req;
	size_t pa_size;

	char *pa = absolutize(path, &pa_size);
	if (!pa)
		return ENOMEM;

	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_0(exch, VFS_IN_STATFS, NULL);
	rc = async_data_write_start(exch, pa, pa_size);
	if (rc != EOK)
		goto exit;

	rc = async_data_read_start(exch, (void *) st, sizeof(*st));

exit:
	vfs_exchange_end(exch);
	free(pa);
	async_wait_for(req, &rc_orig);
	return (int) (rc_orig != EOK ? rc_orig : rc);
}

/** @}
 */
