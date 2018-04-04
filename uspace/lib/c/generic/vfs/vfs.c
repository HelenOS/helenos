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
#include <vfs/canonify.h>
#include <vfs/vfs_mtab.h>
#include <vfs/vfs_sess.h>
#include <macros.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
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

/*
 * This file contains the implementation of the native HelenOS file system API.
 *
 * The API supports client-side file system roots, client-side IO cursors and
 * uses file handles as a primary means to refer to files. In order to call the
 * API functions, one just includes vfs/vfs.h.
 *
 * The API functions come in two main flavors:
 *
 * - functions that operate on integer file handles, such as:
 *   vfs_walk(), vfs_open(), vfs_read(), vfs_link(), ...
 *
 * - functions that operate on paths, such as:
 *   vfs_lookup(), vfs_link_path(), vfs_unlink_path(), vfs_rename_path(), ...
 *
 * There is usually a corresponding path function for each file handle function
 * that exists mostly as a convenience wrapper, except for cases when only a
 * path version exists due to file system consistency considerations (see
 * vfs_rename_path()). Sometimes one of the versions does not make sense, in
 * which case it is also omitted.
 *
 * Besides of that, the API provides some convenience wrappers for frequently
 * performed pairs of operations, for example there is a combo API for
 * vfs_lookup() and vfs_open(): vfs_lookup_open().
 *
 * Some of the functions here return a file handle that can be passed to other
 * functions. Note that a file handle does not automatically represent a file
 * from which one can read or to which one can write. In order to do so, the
 * file handle must be opened first for reading/writing using vfs_open().
 *
 * All file handles, no matter whether opened or not, must be eventually
 * returned to the system using vfs_put(). Non-returned file handles are in use
 * and consume system resources.
 *
 * Functions that return errno_t return an error code on error and do not
 * set errno. Depending on function, success is signalled by returning either
 * EOK or a non-negative file handle.
 *
 * An example life-cycle of a file handle is as follows:
 *
 * 	#include <vfs/vfs.h>
 *
 * 	int file = vfs_lookup("/foo/bar/foobar", WALK_REGULAR);
 * 	if (file < 0)
 * 		return file;
 * 	errno_t rc = vfs_open(file, MODE_READ);
 * 	if (rc != EOK) {
 * 		(void) vfs_put(file);
 *		return rc;
 * 	}
 * 	aoff64_t pos = 42;
 * 	char buf[512];
 *	size_t nread;
 * 	rc = vfs_read(file, &pos, buf, sizeof(buf), &nread);
 * 	if (rc != EOK) {
 * 		vfs_put(file);
 * 		return rc;
 * 	}
 *
 *	// buf is now filled with nread bytes from file
 *
 *	vfs_put(file);
 */

static FIBRIL_MUTEX_INITIALIZE(vfs_mutex);
static async_sess_t *vfs_sess = NULL;

static FIBRIL_MUTEX_INITIALIZE(cwd_mutex);

static int cwd_fd = -1;
static char *cwd_path = NULL;
static size_t cwd_size = 0;

static FIBRIL_MUTEX_INITIALIZE(root_mutex);
static int root_fd = -1;

static errno_t get_parent_and_child(const char *path, int *parent, char **child)
{
	size_t size;
	char *apath = vfs_absolutize(path, &size);
	if (!apath)
		return ENOMEM;

	char *slash = str_rchr(apath, L'/');
	if (slash == apath) {
		*parent = vfs_root();
		if (*parent < 0) {
			free(apath);
			return EBADF;
		}
		*child = apath;
		return EOK;
	} else {
		*slash = '\0';
		errno_t rc = vfs_lookup(apath, WALK_DIRECTORY, parent);
		if (rc != EOK) {
			free(apath);
			return rc;
		}
		*slash = '/';
		*child = str_dup(slash);
		free(apath);
		if (!*child) {
			vfs_put(*parent);
			return ENOMEM;
		}

		return rc;
	}

}

/** Make a potentially relative path absolute
 *
 * This function coverts a current-working-directory-relative path into a
 * well-formed, absolute path. The caller is responsible for deallocating the
 * returned buffer.
 *
 * @param[in] path      Path to be absolutized
 * @param[out] retlen   Length of the absolutized path
 *
 * @return              New buffer holding the absolutized path or NULL
 */
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

/** Clone a file handle
 *
 * The caller can choose whether to clone an existing file handle into another
 * already existing file handle (in which case it is first closed) or to a new
 * file handle allocated either from low or high indices.
 *
 * @param file_from     Source file handle
 * @param file_to       Destination file handle or -1
 * @param high          If file_to is -1, high controls whether the new file
 *                      handle will be allocated from high indices
 *
 * @return              New file handle on success or an error code
 */
errno_t vfs_clone(int file_from, int file_to, bool high, int *handle)
{
	assert(handle != NULL);

	async_exch_t *vfs_exch = vfs_exchange_begin();
	sysarg_t ret;
	errno_t rc = async_req_3_1(vfs_exch, VFS_IN_CLONE, (sysarg_t) file_from,
	    (sysarg_t) file_to, (sysarg_t) high, &ret);
	vfs_exchange_end(vfs_exch);

	if (rc == EOK) {
		*handle = ret;
	}
	return rc;
}

/** Get current working directory path
 *
 * @param[out] buf      Buffer
 * @param size          Size of @a buf
 *
 * @return              EOK on success or a non-error code
 */
errno_t vfs_cwd_get(char *buf, size_t size)
{
	fibril_mutex_lock(&cwd_mutex);

	if ((cwd_size == 0) || (size < cwd_size + 1)) {
		fibril_mutex_unlock(&cwd_mutex);
		return ERANGE;
	}

	str_cpy(buf, size, cwd_path);
	fibril_mutex_unlock(&cwd_mutex);

	return EOK;
}

/** Change working directory
 *
 * @param path  Path of the new working directory
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_cwd_set(const char *path)
{
	size_t abs_size;
	char *abs = vfs_absolutize(path, &abs_size);
	if (!abs)
		return ENOMEM;

	int fd;
	errno_t rc = vfs_lookup(abs, WALK_DIRECTORY, &fd);
	if (rc != EOK) {
		free(abs);
		return rc;
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
	return EOK;
}

/** Start an async exchange on the VFS session
 *
 * @return      New exchange
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

/** Finish an async exchange on the VFS session
 *
 * @param exch  Exchange to be finished
 */
void vfs_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Open session to service represented by a special file
 *
 * Given that the file referred to by @a file represents a service,
 * open a session to that service.
 *
 * @param file  File handle representing a service
 * @param iface Interface to connect to (XXX Should be automatic)
 *
 * @return      Session pointer on success.
 * @return      @c NULL or error.
 */
async_sess_t *vfs_fd_session(int file, iface_t iface)
{
	vfs_stat_t stat;
	errno_t rc = vfs_stat(file, &stat);
	if (rc != EOK)
		return NULL;

	if (stat.service == 0)
		return NULL;

	return loc_service_connect(stat.service, iface, 0);
}

/** Determine if a device contains the specified file system type. If so,
 * return identification information.
 *
 * @param fs_name File system name
 * @param serv    Service representing the mountee
 * @param info    Place to store volume identification information
 *
 * @return                      EOK on success or an error code
 */
errno_t vfs_fsprobe(const char *fs_name, service_id_t serv,
    vfs_fs_probe_info_t *info)
{
	errno_t rc;

	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_begin();
	aid_t req = async_send_1(exch, VFS_IN_FSPROBE, serv, &answer);

	rc = async_data_write_start(exch, (void *) fs_name,
	    str_size(fs_name));

	async_wait_for(req, &rc);

	if (rc != EOK) {
		vfs_exchange_end(exch);
		return rc;
	}

	rc = async_data_read_start(exch, info, sizeof(*info));
	vfs_exchange_end(exch);

	return rc;
}


/** Return a list of currently available file system types
 *
 * @param fstypes Points to structure where list of filesystem types is
 *        stored. It is read as a null-terminated list of strings
 *        fstypes->fstypes[0..]. To free the list use vfs_fstypes_free().
 *
 * @return                      EOK on success or an error code
 */
errno_t vfs_fstypes(vfs_fstypes_t *fstypes)
{
	sysarg_t size;
	char *buf;
	char dummybuf[1];
	size_t count, i;

	async_exch_t *exch = vfs_exchange_begin();
	errno_t rc = async_req_0_1(exch, VFS_IN_FSTYPES, &size);

	if (rc != EOK) {
		vfs_exchange_end(exch);
		return rc;
	}

	buf = malloc(size);
	if (buf == NULL) {
		buf = dummybuf;
		size = 1;
	}

	rc = async_data_read_start(exch, buf, size);
	vfs_exchange_end(exch);

	if (buf == dummybuf)
		return ENOMEM;

	/*
	 * Buffer should contain a number of null-terminated strings.
	 * Count them so that we can allocate an index
	 */
	count = 0;
	i = 0;
	while (i < size) {
		if (buf[i] == '\0')
			++count;
		++i;
	}

	if (count == 0) {
		free(buf);
		return EIO;
	}

	fstypes->fstypes = calloc(sizeof(char *), count + 1);
	if (fstypes->fstypes == NULL) {
		free(buf);
		return ENOMEM;
	}

	/* Now fill the index */
	if (buf[0] != '\0')
		fstypes->fstypes[0] = &buf[0];
	count = 0;
	i = 0;
	while (i < size) {
		if (buf[i] == '\0')
			fstypes->fstypes[++count] = &buf[i + 1];
		++i;
	}
	fstypes->fstypes[count] = NULL;
	fstypes->buf = buf;
	fstypes->size = size;

	return rc;
}

/** Free list of file system types.
 *
 * @param fstypes List of file system types
 */
void vfs_fstypes_free(vfs_fstypes_t *fstypes)
{
	free(fstypes->buf);
	fstypes->buf = NULL;
	free(fstypes->fstypes);
	fstypes->fstypes = NULL;
	fstypes->size = 0;
}

/** Link a file or directory
 *
 * Create a new name and an empty file or an empty directory in a parent
 * directory. If child with the same name already exists, the function returns
 * a failure, the existing file remains untouched and no file system object
 * is created.
 *
 * @param parent        File handle of the parent directory node
 * @param child         New name to be linked
 * @param kind          Kind of the object to be created: KIND_FILE or
 *                      KIND_DIRECTORY
 * @param[out] linkedfd If not NULL, will receive a file handle to the linked
 *                      child
 * @return              EOK on success or an error code
 */
errno_t vfs_link(int parent, const char *child, vfs_file_kind_t kind, int *linkedfd)
{
	int flags = (kind == KIND_DIRECTORY) ? WALK_DIRECTORY : WALK_REGULAR;
	int file = -1;
	errno_t rc = vfs_walk(parent, child, WALK_MUST_CREATE | flags, &file);
	if (rc != EOK)
		return rc;

	if (linkedfd)
		*linkedfd = file;
	else
		vfs_put(file);

	return EOK;
}

/** Link a file or directory
 *
 * Create a new name and an empty file or an empty directory at given path.
 * If a link with the same name already exists, the function returns
 * a failure, the existing file remains untouched and no file system object
 * is created.
 *
 * @param path          New path to be linked
 * @param kind          Kind of the object to be created: KIND_FILE or
 *                      KIND_DIRECTORY
 * @param[out] linkedfd If not NULL, will receive a file handle to the linked
 *                      child
 * @return              EOK on success or an error code
 */
errno_t vfs_link_path(const char *path, vfs_file_kind_t kind, int *linkedfd)
{
	char *child;
	int parent;
	errno_t rc = get_parent_and_child(path, &parent, &child);
	if (rc != EOK)
		return rc;

	rc = vfs_link(parent, child, kind, linkedfd);

	free(child);
	vfs_put(parent);
	return rc;
}

/** Lookup a path relative to the local root
 *
 * @param path  Path to be looked up
 * @param flags Walk flags
 * @param[out] handle Pointer to variable where handle is to be written.
 *
 * @return      EOK on success or an error code.
 */
errno_t vfs_lookup(const char *path, int flags, int *handle)
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

	// XXX: Workaround for GCC diagnostics.
	*handle = -1;

	errno_t rc = vfs_walk(root, p, flags, handle);
	vfs_put(root);
	free(p);
	return rc;
}

/** Lookup a path relative to the local root and open the result
 *
 * This function is a convenience combo for vfs_lookup() and vfs_open().
 *
 * @param path  Path to be looked up
 * @param flags Walk flags
 * @param mode  Mode in which to open file in
 * @param[out] handle Pointer to variable where handle is to be written.
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_lookup_open(const char *path, int flags, int mode, int *handle)
{
	int file;
	errno_t rc = vfs_lookup(path, flags, &file);
	if (rc != EOK)
		return rc;

	rc = vfs_open(file, mode);
	if (rc != EOK) {
		vfs_put(file);
		return rc;
	}

	*handle = file;
	return EOK;
}

/** Mount a file system
 *
 * @param[in] mp                File handle representing the mount-point
 * @param[in] fs_name           File system name
 * @param[in] serv              Service representing the mountee
 * @param[in] opts              Mount options for the endpoint file system
 * @param[in] flags             Mount flags
 * @param[in] instance          Instance number of the file system server
 * @param[out] mountedfd        File handle of the mounted root if not NULL
 *
 * @return                      EOK on success or an error code
 */
errno_t vfs_mount(int mp, const char *fs_name, service_id_t serv, const char *opts,
    unsigned int flags, unsigned int instance, int *mountedfd)
{
	errno_t rc, rc1;

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

/** Mount a file system
 *
 * @param[in] mp                Path representing the mount-point
 * @param[in] fs_name           File system name
 * @param[in] fqsn              Fully qualified service name of the mountee
 * @param[in] opts              Mount options for the endpoint file system
 * @param[in] flags             Mount flags
 * @param[in] instance          Instance number of the file system server
 *
 * @return                      EOK on success or an error code
 */
errno_t vfs_mount_path(const char *mp, const char *fs_name, const char *fqsn,
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
	errno_t res = loc_service_get_id(fqsn, &service_id, flags);
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

	errno_t rc;

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

		int mpfd;
		rc = vfs_walk(root_fd, mpa, WALK_DIRECTORY, &mpfd);
		if (rc == EOK) {
			rc = vfs_mount(mpfd, fs_name, service_id, opts, flags,
			    instance, NULL);
			vfs_put(mpfd);
		}
	}

	fibril_mutex_unlock(&root_mutex);

	if ((rc != EOK) && (null_id != -1))
		loc_null_destroy(null_id);

	return (errno_t) rc;
}


/** Open a file handle for I/O
 *
 * @param file  File handle to enable I/O on
 * @param mode  Mode in which to open file in
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_open(int file, int mode)
{
	async_exch_t *exch = vfs_exchange_begin();
	errno_t rc = async_req_2_0(exch, VFS_IN_OPEN, file, mode);
	vfs_exchange_end(exch);

	return rc;
}

/** Pass a file handle to another VFS client
 *
 * @param vfs_exch      Donor's VFS exchange
 * @param file          Donor's file handle to pass
 * @param exch          Exchange to the acceptor
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_pass_handle(async_exch_t *vfs_exch, int file, async_exch_t *exch)
{
	return async_state_change_start(exch, VFS_PASS_HANDLE, (sysarg_t) file,
	    0, vfs_exch);
}

/** Stop working with a file handle
 *
 * @param file  File handle to put
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_put(int file)
{
	async_exch_t *exch = vfs_exchange_begin();
	errno_t rc = async_req_1_0(exch, VFS_IN_PUT, file);
	vfs_exchange_end(exch);

	return rc;
}

/** Receive a file handle from another VFS client
 *
 * @param high   If true, the received file handle will be allocated from high
 *               indices
 * @param[out] handle  Received handle.
 *
 * @return       EOK on success or an error code
 */
errno_t vfs_receive_handle(bool high, int *handle)
{
	cap_call_handle_t chandle;
	if (!async_state_change_receive(&chandle, NULL, NULL, NULL)) {
		async_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	async_exch_t *vfs_exch = vfs_exchange_begin();

	async_state_change_finalize(chandle, vfs_exch);

	sysarg_t ret;
	errno_t rc = async_req_1_1(vfs_exch, VFS_IN_WAIT_HANDLE, high, &ret);

	async_exchange_end(vfs_exch);

	if (rc == EOK) {
		*handle = (int) ret;
	}

	return rc;
}

/** Read data
 *
 * Read up to @a nbytes bytes from file if available. This function always reads
 * all the available bytes up to @a nbytes.
 *
 * @param file          File handle to read from
 * @param[inout] pos    Position to read from, updated by the actual bytes read
 * @param buf		Buffer, @a nbytes bytes long
 * @param nbytes	Number of bytes to read
 * @param nread		Place to store number of bytes actually read
 *
 * @return              On success, EOK and @a *nread is filled with number
 *			of bytes actually read.
 * @return              On failure, an error code
 */
errno_t vfs_read(int file, aoff64_t *pos, void *buf, size_t nbyte, size_t *nread)
{
	ssize_t cnt = 0;
	size_t nr = 0;
	uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	do {
		bp += cnt;
		nr += cnt;
		*pos += cnt;
		rc = vfs_read_short(file, *pos, bp, nbyte - nr, &cnt);
	} while (rc == EOK && cnt > 0 && (nbyte - nr - cnt) > 0);

	if (rc != EOK) {
		*nread = nr;
		return rc;
	}

	nr += cnt;
	*pos += cnt;
	*nread = nr;
	return EOK;
}

/** Read bytes from a file
 *
 * Read up to @a nbyte bytes from file. The actual number of bytes read
 * may be lower, but greater than zero if there are any bytes available.
 * If there are no bytes available for reading, then the function will
 * return success with zero bytes read.
 *
 * @param file          File handle to read from
 * @param[in] pos       Position to read from
 * @param buf           Buffer to read from
 * @param nbyte         Maximum number of bytes to read
 * @param[out] nread	Actual number of bytes read (0 or more)
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_read_short(int file, aoff64_t pos, void *buf, size_t nbyte,
    ssize_t *nread)
{
	errno_t rc;
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

/** Rename a file or directory
 *
 * There is no file-handle-based variant to disallow attempts to introduce loops
 * and breakage in the directory tree when relinking eg. a node under its own
 * descendant.  The path-based variant is not susceptible because the VFS can
 * prevent this lexically by comparing the paths.
 *
 * @param old   Old path
 * @param new   New path
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_rename_path(const char *old, const char *new)
{
	errno_t rc;
	errno_t rc_orig;
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

/** Resize file to a specified length
 *
 * Resize file so that its size is exactly @a length.
 *
 * @param file          File handle to resize
 * @param length        New length
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_resize(int file, aoff64_t length)
{
	async_exch_t *exch = vfs_exchange_begin();
	errno_t rc = async_req_3_0(exch, VFS_IN_RESIZE, file, LOWER32(length),
	    UPPER32(length));
	vfs_exchange_end(exch);

	return rc;
}

/** Return a new file handle representing the local root
 *
 * @return      A clone of the local root file handle or -1
 */
int vfs_root(void)
{
	fibril_mutex_lock(&root_mutex);
	int fd;
	if (root_fd < 0) {
		fd = -1;
	} else {
		errno_t rc = vfs_clone(root_fd, -1, true, &fd);
		if (rc != EOK) {
			fd = -1;
		}
	}
	fibril_mutex_unlock(&root_mutex);
	return fd;
}

/** Set a new local root
 *
 * Note that it is still possible to have file handles for other roots and pass
 * them to the API functions. Functions like vfs_root() and vfs_lookup() will
 * however consider the file set by this function to be the root.
 *
 * @param nroot The new local root file handle
 *
 * @return  Error code
 */
errno_t vfs_root_set(int nroot)
{
	int new_root;
	errno_t rc = vfs_clone(nroot, -1, true, &new_root);
	if (rc != EOK) {
		return rc;
	}

	fibril_mutex_lock(&root_mutex);
	if (root_fd >= 0)
		vfs_put(root_fd);
	root_fd = new_root;
	fibril_mutex_unlock(&root_mutex);

	return EOK;
}

/** Get file information
 *
 * @param file          File handle to get information about
 * @param[out] stat     Place to store file information
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_stat(int file, vfs_stat_t *stat)
{
	errno_t rc;
	aid_t req;

	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_1(exch, VFS_IN_STAT, file, NULL);
	rc = async_data_read_start(exch, (void *) stat, sizeof(vfs_stat_t));
	if (rc != EOK) {
		vfs_exchange_end(exch);

		errno_t rc_orig;
		async_wait_for(req, &rc_orig);

		if (rc_orig != EOK)
			rc = rc_orig;

		return rc;
	}

	vfs_exchange_end(exch);
	async_wait_for(req, &rc);

	return rc;
}

/** Get file information
 *
 * @param path          File path to get information about
 * @param[out] stat     Place to store file information
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_stat_path(const char *path, vfs_stat_t *stat)
{
	int file;
	errno_t rc = vfs_lookup(path, 0, &file);
	if (rc != EOK)
		return rc;

	rc = vfs_stat(file, stat);

	vfs_put(file);

	return rc;
}

/** Get filesystem statistics
 *
 * @param file          File located on the queried file system
 * @param[out] st       Buffer for storing information
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_statfs(int file, vfs_statfs_t *st)
{
	errno_t rc, ret;
	aid_t req;

	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_1(exch, VFS_IN_STATFS, file, NULL);
	rc = async_data_read_start(exch, (void *) st, sizeof(*st));

	vfs_exchange_end(exch);
	async_wait_for(req, &ret);

	rc = (ret != EOK ? ret : rc);

	return rc;
}

/** Get filesystem statistics
 *
 * @param file          Path pointing to the queried file system
 * @param[out] st       Buffer for storing information
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_statfs_path(const char *path, vfs_statfs_t *st)
{
	int file;
	errno_t rc = vfs_lookup(path, 0, &file);
	if (rc != EOK)
		return rc;

	rc = vfs_statfs(file, st);

	vfs_put(file);

	return rc;
}

/** Synchronize file
 *
 * @param file  File handle to synchronize
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_sync(int file)
{
	async_exch_t *exch = vfs_exchange_begin();
	errno_t rc = async_req_1_0(exch, VFS_IN_SYNC, file);
	vfs_exchange_end(exch);

	return rc;
}

/** Unlink a file or directory
 *
 * Unlink a name from a parent directory. The caller can supply the file handle
 * of the unlinked child in order to detect a possible race with vfs_link() and
 * avoid unlinking a wrong file. If the last link for a file or directory is
 * removed, the FS implementation will deallocate its resources.
 *
 * @param parent        File handle of the parent directory node
 * @param child         Old name to be unlinked
 * @param expect        File handle of the unlinked child
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_unlink(int parent, const char *child, int expect)
{
	errno_t rc;
	aid_t req;

	async_exch_t *exch = vfs_exchange_begin();

	req = async_send_2(exch, VFS_IN_UNLINK, parent, expect, NULL);
	rc = async_data_write_start(exch, child, str_size(child));

	vfs_exchange_end(exch);

	errno_t rc_orig;
	async_wait_for(req, &rc_orig);

	if (rc_orig != EOK)
		return (errno_t) rc_orig;
	return rc;
}

/** Unlink a file or directory
 *
 * Unlink a path. If the last link for a file or directory is removed, the FS
 * implementation will deallocate its resources.
 *
 * @param path          Old path to be unlinked
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_unlink_path(const char *path)
{
	int expect;
	errno_t rc = vfs_lookup(path, 0, &expect);
	if (rc != EOK)
		return rc;

	char *child;
	int parent;
	rc = get_parent_and_child(path, &parent, &child);
	if (rc != EOK) {
		vfs_put(expect);
		return rc;
	}

	rc = vfs_unlink(parent, child, expect);

	free(child);
	vfs_put(parent);
	vfs_put(expect);
	return rc;
}

/** Unmount a file system
 *
 * @param mp    File handle representing the mount-point
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_unmount(int mp)
{
	async_exch_t *exch = vfs_exchange_begin();
	errno_t rc = async_req_1_0(exch, VFS_IN_UNMOUNT, mp);
	vfs_exchange_end(exch);
	return rc;
}

/** Unmount a file system
 *
 * @param mpp   Mount-point path
 *
 * @return      EOK on success or an error code
 */
errno_t vfs_unmount_path(const char *mpp)
{
	int mp;
	errno_t rc = vfs_lookup(mpp, WALK_MOUNT_POINT | WALK_DIRECTORY, &mp);
	if (rc != EOK)
		return rc;

	rc = vfs_unmount(mp);
	vfs_put(mp);
	return rc;
}

/** Walk a path starting in a parent node
 *
 * @param parent        File handle of the parent node where the walk starts
 * @param path          Parent-relative path to be walked
 * @param flags         Flags influencing the walk
 * @param[out] handle   File handle representing the result on success.
 *
 * @return              Error code.
 */
errno_t vfs_walk(int parent, const char *path, int flags, int *handle)
{
	async_exch_t *exch = vfs_exchange_begin();

	ipc_call_t answer;
	aid_t req = async_send_2(exch, VFS_IN_WALK, parent, flags, &answer);
	errno_t rc = async_data_write_start(exch, path, str_size(path));
	vfs_exchange_end(exch);

	errno_t rc_orig;
	async_wait_for(req, &rc_orig);

	if (rc_orig != EOK)
		return (errno_t) rc_orig;

	if (rc != EOK)
		return (errno_t) rc;

	*handle = (int) IPC_GET_ARG1(answer);
	return EOK;
}

/** Write data
 *
 * This function fails if it cannot write exactly @a len bytes to the file.
 *
 * @param file          File handle to write to
 * @param[inout] pos    Position to write to, updated by the actual bytes
 *                      written
 * @param buf           Data, @a nbytes bytes long
 * @param nbytes        Number of bytes to write
 * @param nwritten	Place to store number of bytes written
 *
 * @return		On success, EOK, @a *nwr is filled with number
 *			of bytes written
 * @return              On failure, an error code
 */
errno_t vfs_write(int file, aoff64_t *pos, const void *buf, size_t nbyte,
    size_t *nwritten)
{
	ssize_t cnt = 0;
	ssize_t nwr = 0;
	const uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	do {
		bp += cnt;
		nwr += cnt;
		*pos += cnt;
		rc = vfs_write_short(file, *pos, bp, nbyte - nwr, &cnt);
	} while (rc == EOK && ((ssize_t)nbyte - nwr - cnt) > 0);

	if (rc != EOK) {
		*nwritten = nwr;
		return rc;
	}

	nwr += cnt;
	*pos += cnt;
	*nwritten = nwr;
	return EOK;
}

/** Write bytes to a file
 *
 * Write up to @a nbyte bytes from file. The actual number of bytes written
 * may be lower, but greater than zero.
 *
 * @param file          File handle to write to
 * @param[in] pos       Position to write to
 * @param buf           Buffer to write to
 * @param nbyte         Maximum number of bytes to write
 * @param[out] nread    Actual number of bytes written (0 or more)
 *
 * @return              EOK on success or an error code
 */
errno_t vfs_write_short(int file, aoff64_t pos, const void *buf, size_t nbyte,
    ssize_t *nwritten)
{
	errno_t rc;
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

/** @}
 */
