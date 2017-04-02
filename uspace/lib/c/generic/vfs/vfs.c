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
#include <vfs/vfs_mtab.h>
#include <vfs/vfs_sess.h>
#include <macros.h>
#include <stdlib.h>
#include <dirent.h>
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

static FIBRIL_MUTEX_INITIALIZE(root_mutex);
static int root_fd = -1;

int vfs_root(void)
{
	fibril_mutex_lock(&root_mutex);	
	int r;
	if (root_fd < 0)
		r = ENOENT;
	else
		r = vfs_clone(root_fd, -1, true);
	fibril_mutex_unlock(&root_mutex);
	return r;
}

void vfs_root_set(int nroot)
{
	fibril_mutex_lock(&root_mutex);
	if (root_fd >= 0)
		vfs_put(root_fd);
	root_fd = vfs_clone(nroot, -1, true);
	fibril_mutex_unlock(&root_mutex);
}

/** Start an async exchange on the VFS session.
 *
 * @return New exchange.
 *
 */
async_exch_t *vfs_exchange_begin(void)
{
	fibril_mutex_lock(&vfs_mutex);
	
	while (vfs_sess == NULL) {
		vfs_sess = service_connect_blocking(SERVICE_VFS, INTERFACE_VFS,
		    0);
	}
	
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

int vfs_walk(int parent, const char *path, int flags)
{
	async_exch_t *exch = vfs_exchange_begin();
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, VFS_IN_WALK, parent, flags, &answer);
	sysarg_t rc = async_data_write_start(exch, path, str_size(path));
	vfs_exchange_end(exch);
		
	sysarg_t rc_orig;
	async_wait_for(req, &rc_orig);

	if (rc_orig != EOK)
		return (int) rc_orig;
		
	if (rc != EOK)
		return (int) rc;
	
	return (int) IPC_GET_ARG1(answer);
}

int vfs_lookup(const char *path, int flags)
{
	size_t size;
	char *p = vfs_absolutize(path, &size);
	if (!p)
		return ENOMEM;
	int root = vfs_root();
	if (root < 0) {
		free(p);
		return ENOENT;
	}
	int rc = vfs_walk(root, p, flags);
	vfs_put(root);
	free(p);
	return rc;
}

int vfs_open(int file, int mode)
{
	async_exch_t *exch = vfs_exchange_begin();
	int rc = async_req_2_0(exch, VFS_IN_OPEN, file, mode);
	vfs_exchange_end(exch);
	
	return rc;
}

int vfs_lookup_open(const char *path, int flags, int mode)
{
	int file = vfs_lookup(path, flags);
	if (file < 0)
		return file;

	int rc = vfs_open(file, mode);
	if (rc != EOK) {
		vfs_put(file);
		return rc;
	}
	
	return file;
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

int vfs_mount(int mp, const char *fs_name, service_id_t serv, const char *opts,
    unsigned int flags, unsigned int instance, int *mountedfd)
{
	sysarg_t rc, rc1;
	
	if (!mountedfd)
		flags |= VFS_MOUNT_NO_REF;
	if (mp < 0)
		flags |= VFS_MOUNT_CONNECT_ONLY;
	
	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_begin();
	aid_t req = async_send_4(exch, VFS_IN_MOUNT, mp, serv, flags, instance,
	    &answer);

	rc1 = async_data_write_start(exch, (void *) opts, str_size(opts));
	if (rc1 == EOK) {
		rc1 = async_data_write_start(exch, (void *) fs_name,
		    str_size(fs_name));
	}

	vfs_exchange_end(exch);

	async_wait_for(req, &rc);

	if (mountedfd)
		*mountedfd = (int) IPC_GET_ARG1(answer);
	
	if (rc != EOK)
		return rc;
	return rc1;
}

int vfs_unmount(int mp)
{
	async_exch_t *exch = vfs_exchange_begin();
	int rc = async_req_1_0(exch, VFS_IN_UNMOUNT, mp);
	vfs_exchange_end(exch);
	return rc;
}

int vfs_mount_path(const char *mp, const char *fs_name, const char *fqsn,
    const char *opts, unsigned int flags, unsigned int instance)
{
	int null_id = -1;
	char null[LOC_NAME_MAXLEN];
	
	if (str_cmp(fqsn, "") == 0) {
		/*
		 * No device specified, create a fresh null/%d device instead.
		*/
		null_id = loc_null_create();
		
		if (null_id == -1)
			return ENOMEM;
		
		snprintf(null, LOC_NAME_MAXLEN, "null/%d", null_id);
		fqsn = null;
	}
	
	if (flags & IPC_FLAG_BLOCKING)
		flags = VFS_MOUNT_BLOCKING;
	else
		flags = 0;
	
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
	
	fibril_mutex_lock(&root_mutex);
	
	int rc;
	
	if (str_cmp(mpa, "/") == 0) {
		/* Mounting root. */
		
		if (root_fd >= 0) {
			fibril_mutex_unlock(&root_mutex);
			if (null_id != -1)
				loc_null_destroy(null_id);
			return EBUSY;
		}
		
		int root;
		rc = vfs_mount(-1, fs_name, service_id, opts, flags, instance,
		    &root);
		if (rc == EOK)
			root_fd = root;
	} else {
		if (root_fd < 0) {
			fibril_mutex_unlock(&root_mutex);
			if (null_id != -1)
				loc_null_destroy(null_id);
			return EINVAL;
		}
		
		int mpfd = vfs_walk(root_fd, mpa, WALK_DIRECTORY);
		if (mpfd >= 0) {
			rc = vfs_mount(mpfd, fs_name, service_id, opts, flags,
			    instance, NULL);
			vfs_put(mpfd);
		} else {
			rc = mpfd;
		}
	}
	
	fibril_mutex_unlock(&root_mutex);
	
	if ((rc != EOK) && (null_id != -1))
		loc_null_destroy(null_id);
	
	return (int) rc;
}

int vfs_unmount_path(const char *mpp)
{
	int mp = vfs_lookup(mpp, WALK_MOUNT_POINT | WALK_DIRECTORY);
	if (mp < 0)
		return mp;
	
	int rc = vfs_unmount(mp);
	vfs_put(mp);
	return rc;
}

/** Close file.
 *
 * @param fildes File descriptor
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_put(int fildes)
{
	sysarg_t rc;
	
	async_exch_t *exch = vfs_exchange_begin();
	rc = async_req_1_0(exch, VFS_IN_PUT, fildes);
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
 * @param file File descriptor
 * @param pos Position to read from
 * @param buf Buffer
 * @param nbyte Maximum number of bytes to read
 * @param nread Place to store actual number of bytes read (0 or more)
 *
 * @return EOK on success, non-zero error code on error.
 */
int vfs_read_short(int file, aoff64_t pos, void *buf, size_t nbyte,
    ssize_t *nread)
{
	sysarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	if (nbyte > DATA_XFER_LIMIT)
		nbyte = DATA_XFER_LIMIT;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_3(exch, VFS_IN_READ, file, LOWER32(pos),
	    UPPER32(pos), &answer);
	rc = async_data_read_start(exch, (void *) buf, nbyte);

	vfs_exchange_end(exch);
	
	if (rc == EOK)
		async_wait_for(req, &rc);
	else
		async_forget(req);
	
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
 * @param file File descriptor
 * @param pos Position to write to
 * @param buf Buffer
 * @param nbyte Maximum number of bytes to write
 * @param nread Place to store actual number of bytes written (0 or more)
 *
 * @return EOK on success, non-zero error code on error.
 */
int vfs_write_short(int file, aoff64_t pos, const void *buf, size_t nbyte,
    ssize_t *nwritten)
{
	sysarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	if (nbyte > DATA_XFER_LIMIT)
		nbyte = DATA_XFER_LIMIT;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_3(exch, VFS_IN_WRITE, file, LOWER32(pos),
	    UPPER32(pos), &answer);
	rc = async_data_write_start(exch, (void *) buf, nbyte);
	
	vfs_exchange_end(exch);
	
	if (rc == EOK)
		async_wait_for(req, &rc);
	else
		async_forget(req);

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
 * @param pos		Pointer to position to read from 
 * @param buf		Buffer, @a nbytes bytes long
 * @param nbytes	Number of bytes to read
 *
 * @return		On success, non-negative number of bytes red.
 *			On failure, a negative error code.
 */
ssize_t vfs_read(int file, aoff64_t *pos, void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	size_t nread = 0;
	uint8_t *bp = (uint8_t *) buf;
	int rc;
	
	do {
		bp += cnt;
		nread += cnt;
		*pos += cnt;
		rc = vfs_read_short(file, *pos, bp, nbyte - nread, &cnt);
	} while (rc == EOK && cnt > 0 && (nbyte - nread - cnt) > 0);
	
	if (rc != EOK)
		return rc;
	
	*pos += cnt;
	return nread + cnt;
}

/** Write data.
 *
 * This function fails if it cannot write exactly @a len bytes to the file.
 *
 * @param file		File descriptor
 * @param pos		Pointer to position to write to
 * @param buf		Data, @a nbytes bytes long
 * @param nbytes	Number of bytes to write
 *
 * @return		On success, non-negative number of bytes written.
 *			On failure, a negative error code.
 */
ssize_t vfs_write(int file, aoff64_t *pos, const void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	ssize_t nwritten = 0;
	const uint8_t *bp = (uint8_t *) buf;
	int rc;

	do {
		bp += cnt;
		nwritten += cnt;
		*pos += cnt;
		rc = vfs_write_short(file, *pos, bp, nbyte - nwritten, &cnt);
	} while (rc == EOK && ((ssize_t )nbyte - nwritten - cnt) > 0);

	if (rc != EOK)
		return rc;

	*pos += cnt;
	return nbyte;
}

/** Synchronize file.
 *
 * @param fildes File descriptor
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_sync(int file)
{
	async_exch_t *exch = vfs_exchange_begin();
	sysarg_t rc = async_req_1_0(exch, VFS_IN_SYNC, file);
	vfs_exchange_end(exch);
	
	return rc;
}

/** Truncate file to a specified length.
 *
 * Truncate file so that its size is exactly @a length
 *
 * @param fildes File descriptor
 * @param length Length
 *
 * @return EOK on success or a negative erroc code otherwise.
 */
int vfs_resize(int file, aoff64_t length)
{
	sysarg_t rc;
	
	async_exch_t *exch = vfs_exchange_begin();
	rc = async_req_3_0(exch, VFS_IN_RESIZE, file, LOWER32(length),
	    UPPER32(length));
	vfs_exchange_end(exch);
	
	return rc;
}

/** Get file status.
 *
 * @param file File descriptor
 * @param stat Place to store file information
 *
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_stat(int file, struct stat *stat)
{
	sysarg_t rc;
	aid_t req;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_1(exch, VFS_IN_STAT, file, NULL);
	rc = async_data_read_start(exch, (void *) stat, sizeof(struct stat));
	if (rc != EOK) {
		vfs_exchange_end(exch);
		
		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);
		
		if (rc_orig != EOK)
			rc = rc_orig;
		
		return rc;
	}
	
	vfs_exchange_end(exch);
	async_wait_for(req, &rc);
	
	return rc;
}

/** Get file status.
 *
 * @param path Path to file
 * @param stat Place to store file information
 *
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_stat_path(const char *path, struct stat *stat)
{
	int file = vfs_lookup(path, 0);
	if (file < 0)
		return file;
	
	int rc = vfs_stat(file, stat);

	vfs_put(file);

	return rc;
}

static int get_parent_and_child(const char *path, char **child)
{
	size_t size;
	char *apath = vfs_absolutize(path, &size);
	if (!apath)
		return ENOMEM;

	char *slash = str_rchr(apath, L'/');
	int parent;
	if (slash == apath) {
		parent = vfs_root();
		*child = apath;
	} else {
		*slash = '\0';
		parent = vfs_lookup(apath, WALK_DIRECTORY);
		if (parent < 0) {
			free(apath);
			return parent;
		}
		*slash = '/';
		*child = str_dup(slash);
		free(apath);
		if (!*child) {
			vfs_put(parent);
			return ENOMEM;
		}
	}

	return parent;
}

int vfs_link(int parent, const char *child, vfs_file_kind_t kind)
{
	int flags = (kind == KIND_DIRECTORY) ? WALK_DIRECTORY : WALK_REGULAR;
	int file = vfs_walk(parent, child, WALK_MUST_CREATE | flags);

	if (file < 0)
		return file;

	vfs_put(file);

	return EOK;
}

/** Link a file or directory.
 *
 * @param path Path
 * @param kind Kind of the file to be created.
 * @return EOK on success or a negative error code otherwise
 */
int vfs_link_path(const char *path, vfs_file_kind_t kind)
{
	char *child;
	int parent = get_parent_and_child(path, &child);
	if (parent < 0)
		return parent;

	int rc = vfs_link(parent, child, kind);

	free(child);
	vfs_put(parent);
	return rc;
}	

int vfs_unlink(int parent, const char *child, int expect)
{
	sysarg_t rc;
	aid_t req;
	
	async_exch_t *exch = vfs_exchange_begin();
	
	req = async_send_2(exch, VFS_IN_UNLINK, parent, expect, NULL);
	rc = async_data_write_start(exch, child, str_size(child));
	
	vfs_exchange_end(exch);
	
	sysarg_t rc_orig;
	async_wait_for(req, &rc_orig);
	
	if (rc_orig != EOK)
		return (int) rc_orig;
	return rc;
}

/** Unlink a file or directory.
 *
 * @param path Path
 * @return EOK on success or a negative error code otherwise
 */
int vfs_unlink_path(const char *path)
{
	int expect = vfs_lookup(path, 0);
	if (expect < 0)
		return expect;

	char *child;
	int parent = get_parent_and_child(path, &child);
	if (parent < 0) {
		vfs_put(expect);
		return parent;
	}

	int rc = vfs_unlink(parent, child, expect);
	
	free(child);
	vfs_put(parent);
	vfs_put(expect);
	return rc;
}

/** Rename directory entry.
 *
 * @param old Old name
 * @param new New name
 *
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_rename_path(const char *old, const char *new)
{
	sysarg_t rc;
	sysarg_t rc_orig;
	aid_t req;
	
	size_t olda_size;
	char *olda = vfs_absolutize(old, &olda_size);
	if (olda == NULL)
		return ENOMEM;

	size_t newa_size;
	char *newa = vfs_absolutize(new, &newa_size);
	if (newa == NULL) {
		free(olda);
		return ENOMEM;
	}
	
	async_exch_t *exch = vfs_exchange_begin();
	int root = vfs_root();
	if (root < 0) {
		free(olda);
		free(newa);
		return ENOENT;
	}
	
	req = async_send_1(exch, VFS_IN_RENAME, root, NULL);
	rc = async_data_write_start(exch, olda, olda_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(olda);
		free(newa);
		vfs_put(root);
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		return rc;
	}
	rc = async_data_write_start(exch, newa, newa_size);
	if (rc != EOK) {
		vfs_exchange_end(exch);
		free(olda);
		free(newa);
		vfs_put(root);
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		return rc;
	}
	vfs_exchange_end(exch);
	free(olda);
	free(newa);
	vfs_put(root);
	async_wait_for(req, &rc);

	return rc;
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
	if (!abs) {
		errno = ENOMEM;
		return -1;
	}
	
	int fd = vfs_lookup(abs, WALK_DIRECTORY);
	if (fd < 0) {
		free(abs);
		errno = fd;
		return -1;
	}
	
	fibril_mutex_lock(&cwd_mutex);
	
	if (cwd_fd >= 0)
		vfs_put(cwd_fd);
	
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
async_sess_t *vfs_fd_session(int file, iface_t iface)
{
	struct stat stat;
	int rc = vfs_stat(file, &stat);
	if (rc != 0)
		return NULL;
	
	if (stat.service == 0)
		return NULL;
	
	return loc_service_connect(stat.service, iface, 0);
}

static void process_mp(const char *path, struct stat *stat, list_t *mtab_list)
{
	mtab_ent_t *ent;

	ent = (mtab_ent_t *) malloc(sizeof(mtab_ent_t));
	if (!ent)
		return;

	link_initialize(&ent->link);
	str_cpy(ent->mp, sizeof(ent->mp), path);
	ent->service_id = stat->service_id;

	struct statfs stfs;
	if (vfs_statfs_path(path, &stfs) == EOK)
		str_cpy(ent->fs_name, sizeof(ent->fs_name), stfs.fs_name);
	else
		str_cpy(ent->fs_name, sizeof(ent->fs_name), "?");
	
	list_append(&ent->link, mtab_list);
}

static int vfs_get_mtab_visit(const char *path, list_t *mtab_list,
    fs_handle_t fs_handle, service_id_t service_id)
{
	DIR *dir;
	struct dirent *dirent;

	dir = opendir(path);
	if (!dir)
		return ENOENT;

	while ((dirent = readdir(dir)) != NULL) {
		char *child;
		struct stat st;
		int rc;

		rc = asprintf(&child, "%s/%s", path, dirent->d_name);
		if (rc < 0) {
			closedir(dir);
			return rc;
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
		if (rc != 0) {
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

int vfs_get_mtab_list(list_t *mtab_list)
{
	struct stat st;

	int rc = vfs_stat_path("/", &st);
	if (rc != 0)
		return rc;

	process_mp("/", &st, mtab_list);

	return vfs_get_mtab_visit("/", mtab_list, st.fs_handle, st.service_id);
}

/** Get filesystem statistics.
 *
 * @param file File located on the queried file system
 * @param st Buffer for storing information
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_statfs(int file, struct statfs *st)
{
	sysarg_t rc, ret;
	aid_t req;

	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_1(exch, VFS_IN_STATFS, file, NULL);
	rc = async_data_read_start(exch, (void *) st, sizeof(*st));

	vfs_exchange_end(exch);
	async_wait_for(req, &ret);

	rc = (ret != EOK ? ret : rc);

	return rc;
}
/** Get filesystem statistics.
 *
 * @param path Mount point path
 * @param st Buffer for storing information
 * @return EOK on success or a negative error code otherwise.
 */
int vfs_statfs_path(const char *path, struct statfs *st)
{
	int file = vfs_lookup(path, 0);
	if (file < 0)
		return file;
	
	int rc = vfs_statfs(file, st);

	vfs_put(file);

	return rc; 
}

int vfs_pass_handle(async_exch_t *vfs_exch, int file, async_exch_t *exch)
{
	return async_state_change_start(exch, VFS_PASS_HANDLE, (sysarg_t) file,
	    0, vfs_exch);
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
	sysarg_t rc = async_req_1_1(vfs_exch, VFS_IN_WAIT_HANDLE,
	    high_descriptor, &ret);

	async_exchange_end(vfs_exch);

	if (rc != EOK)
		return rc;
	return ret;
}

int vfs_clone(int file_from, int file_to, bool high_descriptor)
{
	async_exch_t *vfs_exch = vfs_exchange_begin();
	int rc = async_req_3_0(vfs_exch, VFS_IN_CLONE, (sysarg_t) file_from,
	    (sysarg_t) file_to, (sysarg_t) high_descriptor);
	vfs_exchange_end(vfs_exch);
	return rc;
}

/** @}
 */
