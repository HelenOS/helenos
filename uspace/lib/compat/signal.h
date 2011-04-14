
#include "sys/types.h"
#include "time.h"
#include "errno.h"

/* HelenOS doesn't have signals, so calls to functions of this header
 * are just replaced with their respective failure return value.
 *
 * Other macros and constants are here just to satisfy the symbol resolver
 * and have no practical value whatsoever, until HelenOS implements some
 * equivalent of signals. Maybe something neat based on IPC could be devised
 * in the future?
 */

#define SIG_DFL ((void (*)(int)) 0)
#define SIG_ERR ((void (*)(int)) 0)
#define SIG_IGN ((void (*)(int)) 0)


#define signal(sig,func) (errno = ENOTSUP, SIG_ERR)
#define raise(sig) ((int)-1)

typedef int sig_atomic_t;

// full POSIX set
enum {
	SIGABRT,
	SIGALRM,
	SIGBUS,
	SIGCHLD,
	SIGCONT,
	SIGFPE,
	SIGHUP,
	SIGILL,
	SIGINT,
	SIGKILL,
	SIGPIPE,
	SIGQUIT,
	SIGSEGV,
	SIGSTOP,
	SIGTERM,
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
	SIGUSR1,
	SIGUSR2,
	SIGPOLL,
	SIGPROF,
	SIGSYS,
	SIGTRAP,
	SIGURG,
	SIGVTALRM,
	SIGXCPU,
	SIGXFSZ
};

