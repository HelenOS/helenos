#include <io/io.h>
#include <io/stream.h>
#include <string.h>
#include <malloc.h>
#include <libc.h>
#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/fb.h>
#include <ipc/services.h>

#define FDS 32

typedef struct stream_t {
	pwritefn_t w;
	preadfn_t r;
	void * param;
} stream_t;


typedef struct vfb_descriptor_t {
	int phone;
	int vfb;
} vfb_descriptor_t;


stream_t streams[FDS] = {{0, 0, 0}};

/*
ssize_t write_stdout(void *param, const void * buf, size_t count);
ssize_t write_stdout(void *param, const void * buf, size_t count)
{
	return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}*/

static void vfb_send_char(vfb_descriptor_t *d, char c)
{
	ipcarg_t r0,r1;
	ipc_call_sync_2(d->phone, FB_PUTCHAR, d->vfb, c, &r0, &r1);
}
				
static ssize_t write_vfb(void *param, const void *buf, size_t count)
{
	int i;
	for (i = 0; i < count; i++)
		vfb_send_char((vfb_descriptor_t *) param, ((char *) buf)[i]);
	
	return count;
	//return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}


static ssize_t write_stderr(void *param, const void *buf, size_t count)
{
	return count;
	//return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}


stream_t open_vfb(void)
{
	stream_t stream;
	vfb_descriptor_t *vfb;
	int phoneid;
	int res;
	ipcarg_t vfb_no;
	
	while ((phoneid = ipc_connect_me_to(PHONE_NS, SERVICE_VIDEO, 0)) < 0) {
		volatile int a;
		
		for (a = 0; a < 1048576; a++);
	}
	
	ipc_call_sync(phoneid, FB_GET_VFB, 0, &vfb_no);
	vfb = malloc(sizeof(vfb_descriptor_t));
	
	vfb->phone = phoneid;
	vfb->vfb = vfb_no;
	
	stream.w = write_vfb;
	stream.param = vfb;
	return stream;
}


fd_t open(const char *fname, int flags)
{
	int c = 0;
	
	while (((streams[c].w) || (streams[c].r)) && (c < FDS))
		c++;
	if (c == FDS)
		return EMFILE;
	
	if (!strcmp(fname, "stdin")) {
		streams[c].r = (preadfn_t)1;
		return c;
	}
	
	if (!strcmp(fname, "stdout")) {
		//streams[c].w = write_stdout;
		//return c;
		streams[c] = open_vfb();
		return c;
	}
	
	if (!strcmp(fname, "stderr")) {
		streams[c].w = write_stderr;
		return c;
	}
}


ssize_t write(int fd, const void *buf, size_t count)
{
	if (fd < FDS)
		return streams[fd].w(streams[fd].param, buf, count);
	
	return 0;
}
