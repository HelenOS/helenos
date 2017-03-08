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
		vfs_sess = service_connect_blocking(SERVICE_VFS, INTERFACE_VFS,
		    0);
	
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

int _vfs_walk(int parent, const char *path, int flags)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, VFS_IN_WALK, parent, flags, &answer);
	sysarg_t rc = async_data_write_start(exch, path, str_size(path));
	vfs_exchange_end(exch);
		
	sysarg_t rc_orig;
	async_wait_for(req, &rc_orig);

	if (rc_orig != EOK) {
		return (int) rc_orig;
	}
		
	if (rc != EOK) {
		return (int) rc;
	}
	
	return (int) IPC_GET_ARG1(answer);
}

int vfs_lookup(const char *path)
{
	size_t size;
	char *p = vfs_absolutize(path, &size);
	if (!p) {
		return ENOMEM;
	}
	int rc = _vfs_walk(-1, p, 0);
	free(p);
	return rc;
}

int _vfs_open(int fildes, int mode)
{
	async_exch_t *exch = vfs_exchange_begin();
	sysarg_t rc = async_req_2_0(exch, VFS_IN_OPEN2, fildes, mode);
	vfs_exchange_end(exch);
	
	return (int) rc;
}

char *vfs_absolutize(const char *path, size_t *retlen)
{
	char *ncwd_path;
	char *ncwd_path_nc;

	fibril_mutex_lock(&cwd_mutex);
	size_t size = str_size(path);
	if (*path != '/') {
		if (cwd_path == NULL) {
			fibril_mutex_unlock(&cwd_mutex);
			return NULL;
		}
		ncwd_path_nc = malloc(cwd_size + 1 + size + 1);
		if (ncwd_path_nc == NULL) {
			fibril_mutex_unlock(&cwd_mutex);
			return NULL;
		}
		str_cpy(ncwd_path_nc, cwd_size + 1 + size + 1, cwd_path);
		ncwd_path_nc[cwd_size] = '/';
		ncwd_path_nc[cwd_size + 1] = '\0';
	} else {
		ncwd_path_nc = malloc(size + 1);
		if (ncwd_path_nc == NULL) {
			fibril_mutex_unlock(&cwd_mutex);
			return NULL;
		}
		ncwd_path_nc[0] = '\0';
	}
	str_append(ncwd_path_nc, cwd_size + 1 + size + 1, path);
	ncwd_path = canonify(ncwd_path_nc, retlen);
	if (ncwd_path == NULL) {
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
	if (ncwd_path == NULL) {
		fibril_mutex_unlock(&cwd_mutex);
		return NULL;
	}
	fibril_mutex_unlock(&cwd_mutex);
	return ncwd_path;
}

int vfs_mount(const char *fs_name, const char *mp, const char *fqsn,
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
	char *mpa = vfs_absolutize(mp, &mpa_size);
	if (mpa == NULL) {
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
	
	vfs_exchange_end(exch);
	free(mpa);
	async_wait_for(req, &rc);
	
	if ((rc != EOK) && (null_id != -1))
		loc_null_destroy(null_id);
	
	return (int) rc;
}

int vfs_unmount(const char *mp)
{
	sysarg_t rc;
	sysarg_t rc_orig;
	aid_t req;
	size_t mpa_size;
	char *mpa;
	
	mpa = vfs_absolutize(mp, &mpa_size);
	if (mpa == NULL)
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

static int walk_flags(int oflags)
{
	int flags = 0;
	if (oflags & O_CREAT) {
		if (oflags & O_EXCL) {
			flags |= WALK_MUST_CREATE;
		} else {
			flags |= WALK_MAY_CREATE;
		}
	}
	return flags;
}

/** Open file.
 *
 * @param path File path
 * @param oflag O_xxx flags
 * @param mode File mode (only with O_CREAT)
 *
 * @return Nonnegative file descriptor on success. On error -1 is returned
 *         and errno is set.
 */
int open(const char *path, int oflag, ...)
{
	// FIXME: Some applications call this incorrectly.
	if ((oflag & (O_RDONLY|O_WRONLY|O_RDWR)) == 0) {
		oflag |= O_RDWR;
	}

	assert((((oflag & O_RDONLY) != 0) + ((oflag & O_WRONLY) != 0) + ((oflag & O_RDWR) != 0)) == 1);
	
	size_t abs_size;
	char *abs = vfs_absolutize(path, &abs_size);
	if (!abs) {
		return ENOMEM;
	}
	
	int ret = _vfs_walk(-1, abs, walk_flags(oflag) | WALK_REGULAR);
	if (ret < 0) {
		return ret;
	}
	
	int mode =
		((oflag & O_RDWR) ? MODE_READ|MODE_WRITE : 0) |
		((oflag & O_RDONLY) ? MODE_READ : 0) |
		((oflag & O_WRONLY) ? MODE_WRITE : 0) |
		((oflag & O_APPEND) ? MODE_APPEND : 0);
	
	int rc = _vfs_open(ret, mode); 
	if (rc < 0) {
		// _vfs_put(ret);
		close(ret);
		return rc;
	}
	
	if (oflag & O_TRUNC) {
		assert(oflag & O_WRONLY || oflag & O_RDWR);
		assert(!(oflag & O_APPEND));
		
		// _vfs_resize
		(void) ftruncate(ret, 0);
	}

	return ret;
}

/** Close file.
 *
 * @param fildes File descriptor
 * @return Zero on success. On error -1 is returned and errno is set.
 */
int close(int fildes)
{
	sysarg_t rc;
	
	async_exch_t *exch = vfs_exchange_begin();
	rc = async_req_1_0(exch, VFS_IN_CLOSE, fildes);
	vfs_exchange_end(exch);
	
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	
	return 0;
}

/** Read bytes from file.
 *
 * Read up to @a nbyte bytes from file. The actual number of bytes read
 * may be lower, but greater than zero if there are any bytes available.
 * If there are no bytes available for reading, then the function will
 * return success with zero bytes read.
 *
 * @param fildes File descriptor
 * @param buf Buffer
 * @param nbyte Maximum number of bytes to read
 * @param nread Place to store actual number of bytes read (0 or more)
 *
 * @return EOK on success, non-zero error code on error.
 */
static int _read_short(int fildes, void *buf, size_t nbyte, ssize_t *nread)
{
	sysarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	if (nbyte > DATA_XFER_LIMIT)
		nbyte = DATA_XFER_LIMIT;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_READ, fildes, &answer);
	rc = async_data_read_start(exch, (void *) buf, nbyte);

	vfs_exchange_end(exch);
	
	if (rc == EOK) {
		async_wait_for(req, &rc);
	} else {
		async_forget(req);
	}
	
	if (rc != EOK)
		return rc;
	
	*nread = (ssize_t) IPC_GET_ARG1(answer);
	return EOK;
}

/** Write bytes to file.
 *
 * Write up to @a nbyte bytes from file. The actual number of bytes written
 * may be lower, but greater than zero.
 *
 * @param fildes File descriptor
 * @param buf Buffer
 * @param nbyte Maximum number of bytes to write
 * @param nread Place to store actual number of bytes written (0 or more)
 *
 * @return EOK on success, non-zero error code on error.
 */
static int _write_short(int fildes, const void *buf, size_t nbyte,
    ssize_t *nwritten)
{
	sysarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	if (nbyte > DATA_XFER_LIMIT)
		nbyte = DATA_XFER_LIMIT;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_WRITE, fildes, &answer);
	rc = async_data_write_start(exch, (void *) buf, nbyte);
	
	vfs_exchange_end(exch);
	
	if (rc == EOK) {
		async_wait_for(req, &rc);
	} else {
		async_forget(req);
	}

	if (rc != EOK)
		return rc;
	
	*nwritten = (ssize_t) IPC_GET_ARG1(answer);
	return EOK;
}

/** Read data.
 *
 * Read up to @a nbytes bytes from file if available. This function always reads
 * all the available bytes up to @a nbytes.
 *
 * @param fildes	File descriptor
 * @param buf		Buffer, @a nbytes bytes long
 * @param nbytes	Number of bytes to read
 *
 * @return		On success, nonnegative number of bytes read.
 *			On failure, -1 and sets errno.
 */
ssize_t read(int fildes, void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	size_t nread = 0;
	uint8_t *bp = (uint8_t *) buf;
	int rc;
	
	do {
		bp += cnt;
		nread += cnt;
		rc = _read_short(fildes, bp, nbyte - nread, &cnt);
	} while (rc == EOK && cnt > 0 && (nbyte - nread - cnt) > 0);
	
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	
	return nread + cnt;
}

/** Write data.
 *
 * This function fails if it cannot write exactly @a len bytes to the file.
 *
 * @param fildes	File descriptor
 * @param buf		Data, @a nbytes bytes long
 * @param nbytes	Number of bytes to write
 *
 * @return		On success, nonnegative number of bytes written.
 *			On failure, -1 and sets errno.
 */
ssize_t write(int fildes, const void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	ssize_t nwritten = 0;
	const uint8_t *bp = (uint8_t *) buf;
	int rc;

	do {
		bp += cnt;
		nwritten += cnt;
		rc = _write_short(fildes, bp, nbyte - nwritten, &cnt);
	} while (rc == EOK && ((ssize_t )nbyte - nwritten - cnt) > 0);

	if (rc != EOK) {
		errno = rc;
		return -1;
	}

	return nbyte;
}

/** Synchronize file.
 *
 * @param fildes File descriptor
 * @return 0 on success. On error returns -1 and sets errno.
 */
int fsync(int fildes)
{
	async_exch_t *exch = vfs_exchange_begin();
	sysarg_t rc = async_req_1_0(exch, VFS_IN_SYNC, fildes);
	vfs_exchange_end(exch);
	
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	
	return 0;
}

/** Seek to a position.
 *
 * @param fildes File descriptor
 * @param offset Offset
 * @param whence SEEK_SET, SEEK_CUR or SEEK_END
 *
 * @return On success the nonnegative offset from start of file. On error
 *         returns (off64_t)-1 and sets errno.
 */
off64_t lseek(int fildes, off64_t offset, int whence)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	sysarg_t newoff_lo;
	sysarg_t newoff_hi;
	sysarg_t rc = async_req_4_2(exch, VFS_IN_SEEK, fildes,
	    LOWER32(offset), UPPER32(offset), whence,
	    &newoff_lo, &newoff_hi);
	
	vfs_exchange_end(exch);
	
	if (rc != EOK) {
		errno = rc;
		return (off64_t) -1;
	}
	
	return (off64_t) MERGE_LOUP32(newoff_lo, newoff_hi);
}

/** Truncate file to a specified length.
 *
 * Truncate file so that its size is exactly @a length
 *
 * @param fildes File descriptor
 * @param length Length
 *
 * @return 0 on success, -1 on error and sets errno.
 */
int ftruncate(int fildes, aoff64_t length)
{
	sysarg_t rc;
	
	async_exch_t *exch = vfs_exchange_begin();
	rc = async_req_3_0(exch, VFS_IN_TRUNCATE, fildes,
	    LOWER32(length), UPPER32(length));
	vfs_exchange_end(exch);
	
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	
	return 0;
}

/** Get file status.
 *
 * @param fildes File descriptor
 * @param stat Place to store file information
 *
 * @return 0 on success, -1 on error and sets errno.
 */
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
		
		if (rc_orig != EOK)
			rc = rc_orig;
		if (rc != EOK) {
			errno = rc;
			return -1;
		}
		
		return 0;
	}
	
	vfs_exchange_end(exch);
	async_wait_for(req, &rc);
	
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	
	return 0;
}

/** Get file status.
 *
 * @param path Path to file
 * @param stat Place to store file information
 *
 * @return 0 on success, -1 on error and sets errno.
 */
int stat(const char *path, struct stat *stat)
{
	size_t pa_size;
	char *pa = vfs_absolutize(path, &pa_size);
	if (!pa) {
		return ENOMEM;
	}
	
	int fd = _vfs_walk(-1, pa, 0);
	if (fd < 0) {
		return fd;
	}
	
	int rc = fstat(fd, stat);
	close(fd);
	return rc;
}

/** Open directory.
 *
 * @param dirname Directory pathname
 *
 * @return Non-NULL pointer on success. On error returns @c NULL and sets errno.
 */
DIR *opendir(const char *dirname)
{
	DIR *dirp = malloc(sizeof(DIR));
	if (!dirp) {
		errno = ENOMEM;
		return NULL;
	}
	
	size_t abs_size;
	char *abs = vfs_absolutize(dirname, &abs_size);
	if (abs == NULL) {
		free(dirp);
		errno = ENOMEM;
		return NULL;
	}
	
	int ret = _vfs_walk(-1, abs, WALK_DIRECTORY);
	free(abs);
	
	if (ret < EOK) {
		free(dirp);
		errno = ret;
		return NULL;
	}
	
	int rc = _vfs_open(ret, MODE_READ);
	if (rc < 0) {
		free(dirp);
		close(ret);
		errno = rc;
		return NULL;
	}
	
	dirp->fd = ret;
	return dirp;
}

/** Read directory entry.
 *
 * @param dirp Open directory
 * @return Non-NULL pointer to directory entry on success. On error returns
 *         @c NULL and sets errno.
 */
struct dirent *readdir(DIR *dirp)
{
	int rc;
	ssize_t len;
	
	rc = _read_short(dirp->fd, &dirp->res.d_name[0], NAME_MAX + 1, &len);
	if (rc != EOK) {
		errno = rc;
		return NULL;
	}
	
	(void) len;
	return &dirp->res;
}

/** Rewind directory position to the beginning.
 *
 * @param dirp Open directory
 */
void rewinddir(DIR *dirp)
{
	(void) lseek(dirp->fd, 0, SEEK_SET);
}

/** Close directory.
 *
 * @param dirp Open directory
 * @return 0 on success. On error returns -1 and sets errno.
 */
int closedir(DIR *dirp)
{
	int rc;
	
	rc = close(dirp->fd);
	free(dirp);

	/* On error errno was set by close() */
	return rc;
}

/** Create directory.
 *
 * @param path Path
 * @param mode File mode
 * @return 0 on success. On error returns -1 and sets errno.
 */
int mkdir(const char *path, mode_t mode)
{
	size_t pa_size;
	char *pa = vfs_absolutize(path, &pa_size);
	if (!pa) {
		return ENOMEM;
	}
	
	int ret = _vfs_walk(-1, pa, WALK_MUST_CREATE | WALK_DIRECTORY);
	if (ret < 0) {
		return ret;
	}
	
	close(ret);
	return EOK;
}

static int _vfs_unlink2(int parent, const char *path, int expect, int wflag)
{
	sysarg_t rc;
	aid_t req;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_3(exch, VFS_IN_UNLINK2, parent, expect, wflag, NULL);
	rc = async_data_write_start(exch, path, str_size(path));
	
	vfs_exchange_end(exch);
	
	sysarg_t rc_orig;
	async_wait_for(req, &rc_orig);
	
	if (rc_orig != EOK) {
		return (int) rc_orig;
	}
	return rc;
}

/** Unlink file or directory.
 *
 * @param path Path
 * @return EOk on success, error code on error
 */
int unlink(const char *path)
{
	size_t pa_size;
	char *pa = vfs_absolutize(path, &pa_size);
	if (!pa) {
		return ENOMEM;
	}
	
	return _vfs_unlink2(-1, pa, -1, 0);
}

/** Remove empty directory.
 *
 * @param path Path
 * @return 0 on success. On error returns -1 and sets errno.
 */
int rmdir(const char *path)
{
	size_t pa_size;
	char *pa = vfs_absolutize(path, &pa_size);
	if (!pa) {
		return ENOMEM;
	}
	
	return _vfs_unlink2(-1, pa, -1, WALK_DIRECTORY);
}

/** Rename directory entry.
 *
 * @param old Old name
 * @param new New name
 *
 * @return 0 on success. On error returns -1 and sets errno.
 */
int rename(const char *old, const char *new)
{
	sysarg_t rc;
	sysarg_t rc_orig;
	aid_t req;
	
	size_t olda_size;
	char *olda = vfs_absolutize(old, &olda_size);
	if (olda == NULL) {
		errno = ENOMEM;
		return -1;
	}

	size_t newa_size;
	char *newa = vfs_absolutize(new, &newa_size);
	if (newa == NULL) {
		free(olda);
		errno = ENOMEM;
		return -1;
	}
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_RENAME, -1, NULL);
	rc = async_data_write_start(exch, olda, olda_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(olda);
		free(newa);
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		if (rc != EOK) {
			errno = rc;
			return -1;
		}
		return 0;
	}
	rc = async_data_write_start(exch, newa, newa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(olda);
		free(newa);
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		if (rc != EOK) {
			errno = rc;
			return -1;
		}
		return 0;
	}
	vfs_exchange_end(exch);
	free(olda);
	free(newa);
	async_wait_for(req, &rc);

	if (rc != EOK) {
		errno = rc;
		return -1;
	}

	return 0;
}

/** Remove directory entry.
 *
 * @param path Path
 * @return 0 on success. On error returns -1 and sets errno.
 */
int remove(const char *path)
{
	return unlink(path);
}

/** Change working directory.
 *
 * @param path Path
 * @return 0 on success. On error returns -1 and sets errno.
 */
int chdir(const char *path)
{
	size_t abs_size;
	char *abs = vfs_absolutize(path, &abs_size);
	if (!abs)
		return ENOMEM;
	
	int fd = _vfs_walk(-1, abs, WALK_DIRECTORY);
	if (fd < 0) {
		free(abs);
		errno = fd;
		return -1;
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
	return 0;
}

/** Get current working directory path.
 *
 * @param buf Buffer
 * @param size Size of @a buf
 * @return On success returns @a buf. On failure returns @c NULL and sets errno.
 */
char *getcwd(char *buf, size_t size)
{
	if (size == 0) {
		errno = EINVAL;
		return NULL;
	}
	
	fibril_mutex_lock(&cwd_mutex);
	
	if ((cwd_size == 0) || (size < cwd_size + 1)) {
		fibril_mutex_unlock(&cwd_mutex);
		errno = ERANGE;
		return NULL;
	}
	
	str_cpy(buf, size, cwd_path);
	fibril_mutex_unlock(&cwd_mutex);
	
	return buf;
}

/** Open session to service represented by a special file.
 *
 * Given that the file referred to by @a fildes represents a service,
 * open a session to that service.
 *
 * @param fildes File descriptor
 * @param iface Interface to connect to (XXX Should be automatic)
 * @return On success returns session pointer. On error returns @c NULL.
 */
async_sess_t *vfs_fd_session(int fildes, iface_t iface)
{
	struct stat stat;
	int rc = fstat(fildes, &stat);
	if (rc != 0)
		return NULL;
	
	if (stat.service == 0)
		return NULL;
	
	return loc_service_connect(stat.service, iface, 0);
}

/** Duplicate open file.
 *
 * Duplicate open file under a new file descriptor.
 *
 * @param oldfd Old file descriptor
 * @param newfd New file descriptor
 * @return 0 on success. On error -1 is returned and errno is set
 */
int dup2(int oldfd, int newfd)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	sysarg_t ret;
	sysarg_t rc = async_req_2_1(exch, VFS_IN_DUP, oldfd, newfd, &ret);
	
	vfs_exchange_end(exch);
	
	if (rc == EOK)
		rc = ret;
	
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	
	return 0;
}

int vfs_get_mtab_list(list_t *mtab_list)
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
		if (mtab_ent == NULL) {
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

/** Get filesystem statistics.
 *
 * @param path Mount point path
 * @param st Buffer for storing information
 * @return 0 on success. On error -1 is returned and errno is set.
 */
int statfs(const char *path, struct statfs *st)
{
	size_t pa_size;
	char *pa = vfs_absolutize(path, &pa_size);
	if (!pa) {
		errno = ENOMEM;
		return -1;
	}
	
	int fd = _vfs_walk(-1, pa, 0);
	if (fd < 0) {
		free(pa);
		errno = fd;
		return -1;
	}

	free(pa);
	
	sysarg_t rc, ret;
	aid_t req;

	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_1(exch, VFS_IN_STATFS, fd, NULL);
	rc = async_data_read_start(exch, (void *) st, sizeof(*st));

	vfs_exchange_end(exch);
	async_wait_for(req, &ret);
	close(fd);

	rc = (ret != EOK ? ret : rc);
	if (rc != EOK) {
		errno = rc;
		return -1;
	}

	return 0;
}

int vfs_pass_handle(async_exch_t *vfs_exch, int file, async_exch_t *exch)
{
	return async_state_change_start(exch, VFS_PASS_HANDLE, (sysarg_t)file, 0, vfs_exch);
}

int vfs_receive_handle(bool high_descriptor)
{
	ipc_callid_t callid;
	if (!async_state_change_receive(&callid, NULL, NULL, NULL)) {
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}

	async_exch_t *vfs_exch = vfs_exchange_begin();

	async_state_change_finalize(callid, vfs_exch);

	sysarg_t ret;
	sysarg_t rc = async_req_1_1(vfs_exch, VFS_IN_WAIT_HANDLE, high_descriptor, &ret);

	async_exchange_end(vfs_exch);

	if (rc != EOK) {
		return rc;
	}
	return ret;
}

int vfs_clone(int file, bool high_descriptor)
{
	async_exch_t *vfs_exch = vfs_exchange_begin();
	int rc = async_req_2_0(vfs_exch, VFS_IN_CLONE, (sysarg_t) file, (sysarg_t) high_descriptor);
	vfs_exchange_end(vfs_exch);
	return rc;
}

/** @}
 */
