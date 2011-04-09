
#ifndef COMPAT_STAT_H
#define COMPAT_STAT_H

#include "../../c/include/sys/stat.h"
#include "time.h"

/* values are the same as on Linux */
#define S_IFMT     0170000   /* all file types */
#define S_IFSOCK   0140000   /* socket */
#define S_IFLNK    0120000   /* symbolic link */
#define S_IFREG    0100000   /* regular file */
#define S_IFBLK    0060000   /* block device */
#define S_IFDIR    0040000   /* directory */
#define S_IFCHR    0020000   /* character device */
#define S_IFIFO    0010000   /* FIFO */

#define S_ISUID    0004000   /* SUID */
#define S_ISGID    0002000   /* SGID */
#define S_ISVTX    0001000   /* sticky */

#define S_IRWXU    00700     /* owner permissions */
#define S_IRUSR    00400
#define S_IWUSR    00200
#define S_IXUSR    00100

#define S_IRWXG    00070     /* group permissions */
#define S_IRGRP    00040
#define S_IWGRP    00020
#define S_IXGRP    00010

#define S_IRWXO    00007     /* other permissions */
#define S_IROTH    00004
#define S_IWOTH    00002
#define S_IXOTH    00001

#define S_ISREG(m) ((m & S_IFREG) != 0)
#define S_ISDIR(m) ((m & S_IFDIR) != 0)
#define S_ISCHR(m) ((m & S_IFCHR) != 0)
#define S_ISBLK(m) ((m & S_IFBLK) != 0)
#define S_ISFIFO(m) ((m & S_IFIFO) != 0)
#define S_ISLNK(m) ((m & S_IFLNK) != 0)   /* symbolic link? (Not in POSIX.1-1996.) */
#define S_ISSOCK(m) ((m & S_IFSOCK) != 0) /* socket? (Not in POSIX.1-1996.) */

typedef devmap_handle_t dev_t;
typedef unsigned int ino_t;
typedef unsigned int nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef aoff64_t off_t;
typedef unsigned int blksize_t;
typedef unsigned int blkcnt_t;

struct posix_stat {
	struct stat sys_stat;

	dev_t     st_dev;     /* ID of device containing file */
	ino_t     st_ino;     /* inode number */
	mode_t    st_mode;    /* protection */
	nlink_t   st_nlink;   /* number of hard links */
	uid_t     st_uid;     /* user ID of owner */
	gid_t     st_gid;     /* group ID of owner */
	dev_t     st_rdev;    /* device ID (if special file) */
	off_t     st_size;    /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for file system I/O */
	blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
	time_t    st_atime;   /* time of last access */
	time_t    st_mtime;   /* time of last modification */
	time_t    st_ctime;   /* time of last status change */
};

extern int posix_fstat(int, struct posix_stat *);
extern int posix_stat(const char *, struct posix_stat *);

#define fstat posix_fstat
#define stat posix_stat

#endif /* COMPAT_STAT_H */

