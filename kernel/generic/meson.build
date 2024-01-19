#
# Copyright (c) 2019 Jiří Zárevúcky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Generic kernel sources
generic_src = []

# Generic kernel sources that are instrumented when CONFIG_TRACE is enabled.
# Must be disjoint with generic_src.
instrumentable_src = []


generic_src += files(
	'common/adt/bitmap.c',
	'common/adt/hash_table.c',
	'common/adt/list.c',
	'common/adt/odict.c',
	'common/printf/printf_core.c',
	'common/stdc/calloc.c',
	'common/stdc/ctype.c',
	'common/stdc/mem.c',
	'common/gsort.c',
	'common/str.c',
	'common/str_error.c',
	'common/strtol.c',

	'src/cap/cap.c',
	'src/console/chardev.c',
	'src/console/console.c',
	'src/console/prompt.c',
	'src/cpu/cpu_mask.c',
	'src/ddi/irq.c',
	'src/debug/line.c',
	'src/debug/names.c',
	'src/debug/panic.c',
	'src/debug/profile.c',
	'src/debug/sections.c',
	'src/debug/stacktrace.c',
	'src/debug/symtab.c',
	'src/debug/util.c',
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
	'src/lib/halt.c',
	'src/lib/mem.c',
	'src/lib/ra.c',
	'src/lib/rd.c',
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
	'src/printf/printf.c',
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
	'src/synch/irq_spinlock.c',
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
