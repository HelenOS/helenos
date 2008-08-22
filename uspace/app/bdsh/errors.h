#ifndef ERRORS_H
#define ERRORS_H

/* Various error levels */
#define CL_EFATAL   -1
#define CL_EOK     0
#define CL_EFAIL   1
#define CL_EBUSY   2
#define CL_ENOENT  3
#define CL_ENOMEM  4
#define CL_EPERM   5
#define CL_ENOTSUP 6
#define CL_EEXEC   7
#define CL_EEXISTS 8

extern char *err2str(int);
extern void cli_error(int, const char *, ...);
extern void cli_verbose(const char *, ...);
#endif
