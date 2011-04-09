
#include "stat.h"
#include <mem.h>

#undef stat
#undef fstat

static void stat_to_posix (struct posix_stat *dest, struct stat *src) {
	
	memset(dest, 0, sizeof(struct posix_stat));
	
	dest->st_dev = src->device;
	
	/* HelenOS doesn't support permissions, so we set them all */
	dest->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
	if (src->is_file)
		dest->st_mode |= S_IFREG;
	if (src->is_directory)
		dest->st_mode |= S_IFDIR;
	
	dest->st_nlink = src->lnkcnt;
	dest->st_size = src->size;
}

int posix_fstat(int fd, struct posix_stat *st) {
	struct stat hst;
	if (fstat(fd, &hst) == -1) {
		// FIXME: propagate a POSIX compatible errno
		return -1;
	}
	stat_to_posix(st, &hst);
	return 0;
}

int posix_stat(const char *path, struct posix_stat *st) {
	struct stat hst;
	if (stat(path, &hst) == -1) {
		// FIXME: propagate a POSIX compatible errno
		return -1;
	}
	stat_to_posix(st, &hst);
	return 0;
}

