#include <libarch/types.h>
#include <unistd.h>

#define EMFILE -17

typedef int fd_t;


typedef ssize_t (*pwritefn_t)(void *, const void *, size_t);
typedef char (*preadfn_t)(void);

fd_t open(const char *fname, int flags);
