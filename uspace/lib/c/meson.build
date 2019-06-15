# libarch
arch_src = []
subdir(join_paths('arch', h_arch))

c_args = [ '-fno-builtin', '-D_LIBC_SOURCE' ]

root_path = '..' / '..' / '..'

incdirs = [
	'include',
	'arch' / h_arch / 'include',
	root_path / 'abi' / 'arch' / h_arch / 'include',
	root_path / 'abi' / 'include',
]

includes += include_directories(incdirs, is_system: true)

allow_shared = true

# FIXME: symlinks from uspace to kernel will break in future Meson version
#        we should instead move the duplicated library parts into a shared location.

if CONFIG_DEVEL_FILES
	foreach idir : incdirs
		install_subdir(idir, install_dir: 'include' / 'libc', strip_directory: true)
	endforeach
endif

src = [ arch_src ]

src += files(
	'generic/libc.c',
	'generic/ddi.c',
	'generic/as.c',
	'generic/bd.c',
	'generic/bd_srv.c',
	'generic/perm.c',
	'generic/cap.c',
	'generic/clipboard.c',
	'generic/config.c',
	'generic/context.c',
	'generic/corecfg.c',
	'generic/ctype.c',
	'generic/devman.c',
	'generic/device/hw_res.c',
	'generic/device/hw_res_parsed.c',
	'generic/device/pio_window.c',
	'generic/device/clock_dev.c',
	'generic/device/led_dev.c',
	'generic/dirent.c',
	'generic/dhcp.c',
	'generic/dnsr.c',
	'generic/dlfcn.c',
	'generic/elf/elf.c',
	'generic/elf/elf_load.c',
	'generic/elf/elf_mod.c',
	'generic/event.c',
	'generic/errno.c',
	'generic/gsort.c',
	'generic/inttypes.c',
	'generic/ipc_test.c',
	'generic/loc.c',
	'generic/mem.c',
	'generic/str.c',
	'generic/string.c',
	'generic/str_error.c',
	'generic/strtol.c',
	'generic/l18n/langs.c',
	'generic/pcb.c',
	'generic/smc.c',
	'generic/task.c',
	'generic/imath.c',
	'generic/inet/addr.c',
	'generic/inet/endpoint.c',
	'generic/inet/host.c',
	'generic/inet/hostname.c',
	'generic/inet/hostport.c',
	'generic/inet/tcp.c',
	'generic/inet/udp.c',
	'generic/inet.c',
	'generic/inetcfg.c',
	'generic/inetping.c',
	'generic/io/asprintf.c',
	'generic/io/input.c',
	'generic/io/io.c',
	'generic/io/chardev.c',
	'generic/io/chardev_srv.c',
	'generic/io/chargrid.c',
	'generic/io/output.c',
	'generic/io/printf.c',
	'generic/io/log.c',
	'generic/io/logctl.c',
	'generic/io/label.c',
	'generic/io/kio.c',
	'generic/io/klog.c',
	'generic/io/serial.c',
	'generic/io/snprintf.c',
	'generic/io/vprintf.c',
	'generic/io/vsnprintf.c',
	'generic/io/printf_core.c',
	'generic/io/con_srv.c',
	'generic/io/console.c',
	'generic/io/table.c',
	'generic/io/visualizer.c',
	'generic/io/window.c',
	'generic/iplink.c',
	'generic/iplink_srv.c',
	'generic/irc.c',
	'generic/irq.c',
	'generic/ieee_double.c',
	'generic/power_of_ten.c',
	'generic/double_to_str.c',
	'generic/malloc.c',
	'generic/rndgen.c',
	'generic/stdio/scanf.c',
	'generic/stdio/sprintf.c',
	'generic/stdio/sscanf.c',
	'generic/stdio/sstream.c',
	'generic/stdio/vsprintf.c',
	'generic/thread/fibril.c',
	'generic/thread/fibril_synch.c',
	'generic/thread/thread.c',
	'generic/thread/tls.c',
	'generic/thread/futex.c',
	'generic/thread/mpsc.c',
	'generic/sysinfo.c',
	'generic/ipc.c',
	'generic/ns.c',
	'generic/async/client.c',
	'generic/async/server.c',
	'generic/async/ports.c',
	'generic/loader.c',
	'generic/getopt.c',
	'generic/adt/checksum.c',
	'generic/adt/circ_buf.c',
	'generic/adt/list.c',
	'generic/adt/hash_table.c',
	'generic/adt/odict.c',
	'generic/adt/prodcons.c',
	'generic/time.c',
	'generic/tmpfile.c',
	'generic/stdio.c',
	'generic/stdlib.c',
	'generic/udebug.c',
	'generic/vfs/canonify.c',
	'generic/vfs/inbox.c',
	'generic/vfs/mtab.c',
	'generic/vfs/vfs.c',
	'generic/setjmp.c',
	'generic/stack.c',
	'generic/stacktrace.c',
	'generic/arg_parse.c',
	'generic/stats.c',
	'generic/assert.c',
	'generic/bsearch.c',
	'generic/pci.c',
	'generic/pio_trace.c',
	'generic/qsort.c',
	'generic/ubsan.c',
	'generic/uuid.c',
	'generic/vbd.c',
	'generic/vol.c',
)

if CONFIG_RTLD
	src += files(
		'generic/rtld/rtld.c',
		'generic/rtld/dynamic.c',
		'generic/rtld/module.c',
		'generic/rtld/symbol.c',
	)
endif

test_src = files(
	'test/adt/circ_buf.c',
	'test/adt/odict.c',
	'test/cap.c',
	'test/casting.c',
	'test/double_to_str.c',
	'test/fibril/timer.c',
	'test/getopt.c',
	'test/gsort.c',
	'test/ieee_double.c',
	'test/imath.c',
	'test/inttypes.c',
	'test/io/table.c',
	'test/main.c',
	'test/mem.c',
	'test/perf.c',
	'test/perm.c',
	'test/qsort.c',
	'test/sprintf.c',
	'test/stdio/scanf.c',
	'test/stdio.c',
	'test/stdlib.c',
	'test/str.c',
	'test/string.c',
	'test/strtol.c',
	'test/uuid.c',
)

# Startfiles.
# This is what other systems know as crt*.o files, i.e. a set of object files
# that are automatically built into every program binary (but not libraries).
# We haven't yet installed the object files where the compiler can find them,
# so we have to add them manually.
#

start_src = [ arch_start_src, files('generic/crt/crt1.c') ]

libstartfiles = static_library('startfiles', start_src,
	include_directories: includes,
	c_args: c_args,
	install: CONFIG_DEVEL_FILES,
	install_dir: 'lib',
	pic: false,
)

startfiles = libstartfiles.extract_all_objects()
