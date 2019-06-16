# Generic kernel sources
generic_src = []

# Generic kernel sources that are instrumented when CONFIG_TRACE is enabled.
# Must be disjoint with generic_src.
instrumentable_src = []


generic_src += files(
	'src/adt/bitmap.c',
	'src/adt/hash_table.c',
	'src/adt/list.c',
	'src/adt/odict.c',
	'src/cap/cap.c',
	'src/console/chardev.c',
	'src/console/console.c',
	'src/console/prompt.c',
	'src/cpu/cpu_mask.c',
	'src/ddi/irq.c',
	'src/debug/debug.c',
	'src/debug/panic.c',
	'src/debug/stacktrace.c',
	'src/debug/symtab.c',
	'src/ipc/event.c',
	'src/ipc/ipc.c',
	'src/ipc/ipcrsc.c',
	'src/ipc/irq.c',
	'src/ipc/ops/conctmeto.c',
	'src/ipc/ops/concttome.c',
	'src/ipc/ops/dataread.c',
	'src/ipc/ops/datawrite.c',
	'src/ipc/ops/debug.c',
	'src/ipc/ops/pagein.c',
	'src/ipc/ops/sharein.c',
	'src/ipc/ops/shareout.c',
	'src/ipc/ops/stchngath.c',
	'src/ipc/sysipc.c',
	'src/ipc/sysipc_ops.c',
	'src/lib/elf.c',
	'src/lib/gsort.c',
	'src/lib/halt.c',
	'src/lib/mem.c',
	'src/lib/memfnc.c',
	'src/lib/ra.c',
	'src/lib/rd.c',
	'src/lib/str.c',
	'src/lib/strtol.c',
	'src/lib/str_error.c',
	'src/lib/ubsan.c',
	'src/log/log.c',
	'src/main/shutdown.c',
	'src/main/uinit.c',
	'src/main/version.c',
	'src/mm/backend_anon.c',
	'src/mm/backend_elf.c',
	'src/mm/backend_phys.c',
	'src/mm/backend_user.c',
	'src/mm/km.c',
	'src/mm/malloc.c',
	'src/mm/reserve.c',
	'src/preempt/preemption.c',
	'src/printf/printf.c',
	'src/printf/printf_core.c',
	'src/printf/snprintf.c',
	'src/printf/vprintf.c',
	'src/printf/vsnprintf.c',
	'src/proc/program.c',
	'src/proc/scheduler.c',
	'src/proc/task.c',
	'src/proc/thread.c',
	'src/security/perm.c',
	'src/smp/ipi.c',
	'src/smp/smp.c',
	'src/synch/condvar.c',
	'src/synch/mutex.c',
	'src/synch/semaphore.c',
	'src/synch/smc.c',
	'src/synch/spinlock.c',
	'src/synch/syswaitq.c',
	'src/synch/waitq.c',
	'src/syscall/copy.c',
	'src/syscall/syscall.c',
	'src/sysinfo/stats.c',
	'src/time/clock.c',
	'src/time/delay.c',
	'src/time/timeout.c',
)

instrumentable_src += files(
	'src/cpu/cpu.c',
	'src/ddi/ddi.c',
	'src/interrupt/interrupt.c',
	'src/main/kinit.c',
	'src/main/main.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/slab.c',
	'src/mm/tlb.c',
	'src/proc/current.c',
	'src/sysinfo/sysinfo.c',
)

## Kernel console support
#

if CONFIG_KCONSOLE
	generic_src += files('src/console/cmd.c')
	instrumentable_src += files('src/console/kconsole.c')
endif

## Udebug interface sources
#

if CONFIG_UDEBUG
	generic_src += files(
		'src/ipc/kbox.c',
		'src/udebug/udebug.c',
		'src/udebug/udebug_ops.c',
		'src/udebug/udebug_ipc.c',
	)
endif
