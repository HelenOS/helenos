
#ifndef LIBC_VFS_INBOX_H_
#define LIBC_VFS_INBOX_H_

enum {
	INBOX_MAX_ENTRIES = 256,
};

extern int inbox_set(const char *name, int file);
extern int inbox_get(const char *name);

extern int inbox_list(const char **names, int capacity);

#endif
