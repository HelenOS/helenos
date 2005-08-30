include Makefile.config
include ../arch/$(ARCH)/Makefile.inc

sources=cpu/cpu.c \
	main/main.c \
	main/kinit.c \
	main/uinit.c \
	proc/scheduler.c \
	proc/thread.c \
	proc/task.c \
	mm/heap.c \
	mm/frame.c \
	mm/page.c \
	mm/tlb.c \
	mm/vm.c \
	lib/func.c \
	lib/list.c \
	lib/memstr.c \
	lib/the.c \
	debug/print.c \
	time/clock.c \
	time/timeout.c \
	time/delay.c \
	preempt/preemption.c \
	synch/spinlock.c \
	synch/condvar.c \
	synch/rwlock.c \
	synch/mutex.c \
	synch/semaphore.c \
	synch/waitq.c \
	smp/ipi.c \
	fb/font-8x16.c

ifdef DEBUG_SPINLOCK
CFLAGS+=-D$(DEBUG_SPINLOCK)
endif

ifdef USERSPACE
CFLAGS+=-D$(USERSPACE)
endif

ifdef TEST
test_objects:=$(addsuffix .o,$(basename ../test/$(TEST_DIR)/$(TEST_FILE)))
CFLAGS+=-D$(TEST)
endif
arch_objects:=$(addsuffix .o,$(basename $(arch_sources)))
objects:=$(addsuffix .o,$(basename $(sources)))

.PHONY : all config depend build clean dist-clean boot

all: dist-clean config depend build

-include Makefile.depend

config:
	find . ../include -name arch -type l -exec rm \{\} \;
	ln -s ../arch/$(ARCH)/src arch
	ln -s ../arch/$(ARCH)/include ../include/arch

depend:
	$(CC) $(CPPFLAGS) -M $(arch_sources) $(sources) >Makefile.depend

build: kernel.bin boot

clean:
	find . ../arch/$(ARCH)/src ../test -name '*.o' -exec rm \{\} \;
	-rm *.bin kernel.map
	$(MAKE) -C ../arch/$(ARCH)/boot clean

dist-clean:
	find . ../include -name arch -type l -exec rm \{\} \;
	-rm Makefile.depend
	-$(MAKE) clean

kernel.bin: $(arch_objects) $(objects) $(test_objects)
	$(LD) $(LFLAGS) $(arch_objects) $(objects) $(test_objects) -o $@ -Map kernel.map

%.s: %.S
	$(CC) $(CPPFLAGS) -E $< >$@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

KS=`cat kernel.bin | wc -c`

boot:
	$(MAKE) -C ../arch/$(ARCH)/boot build KERNEL_SIZE=$(KS)
