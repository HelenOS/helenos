include Makefile.config
include arch/$(ARCH)/Makefile.inc
include genarch/Makefile.inc

sources=generic/src/cpu/cpu.c \
	generic/src/main/main.c \
	generic/src/main/kinit.c \
	generic/src/main/uinit.c \
	generic/src/proc/scheduler.c \
	generic/src/proc/thread.c \
	generic/src/proc/task.c \
	generic/src/proc/the.c \
	generic/src/mm/buddy.c \
	generic/src/mm/heap.c \
	generic/src/mm/frame.c \
	generic/src/mm/page.c \
	generic/src/mm/tlb.c \
	generic/src/mm/vm.c \
	generic/src/lib/func.c \
	generic/src/lib/list.c \
	generic/src/lib/memstr.c \
	generic/src/lib/sort.c \
	generic/src/debug/print.c \
	generic/src/debug/symtab.c \
	generic/src/time/clock.c \
	generic/src/time/timeout.c \
	generic/src/time/delay.c \
	generic/src/preempt/preemption.c \
	generic/src/synch/spinlock.c \
	generic/src/synch/condvar.c \
	generic/src/synch/rwlock.c \
	generic/src/synch/mutex.c \
	generic/src/synch/semaphore.c \
	generic/src/synch/waitq.c \
	generic/src/smp/ipi.c \
	generic/src/fb/font-8x16.c

# CFLAGS options same for all targets
CFLAGS+=-nostdinc -Igeneric/include/ -Werror-implicit-function-declaration -Wmissing-prototypes -Werror

ifdef DEBUG_SPINLOCK
CFLAGS+=-D$(DEBUG_SPINLOCK)
endif

ifdef USERSPACE
CFLAGS+=-D$(USERSPACE)
endif

ifdef TEST
test_objects:=$(addsuffix .o,$(basename test/$(TEST_DIR)/$(TEST_FILE)))
CFLAGS+=-D$(TEST)
endif
arch_objects:=$(addsuffix .o,$(basename $(arch_sources)))
genarch_objects:=$(addsuffix .o,$(basename $(genarch_sources)))
objects:=$(addsuffix .o,$(basename $(sources)))

.PHONY : all config depend build clean dist-clean boot

all: dist-clean config depend build

-include Makefile.depend

config:
	find generic/src/ generic/include/ -name arch -type l -exec rm \{\} \;
	find generic/src/ generic/include/ -name genarch -type l -exec rm \{\} \;	
	ln -s ../../arch/$(ARCH)/src/ generic/src/arch
	ln -s ../../arch/$(ARCH)/include/ generic/include/arch
	ln -s ../../genarch/src/ generic/src/genarch
	ln -s ../../genarch/include/ generic/include/genarch

depend:
	$(CC) $(CFLAGS) -M $(arch_sources) $(genarch_sources) $(sources) >Makefile.depend

build: kernel.bin boot

clean:
	find generic/src/ arch/$(ARCH)/src/ genarch/src/ test/ -name '*.o' -exec rm \{\} \;
	-rm *.bin kernel.map kernel.map.pre kernel.objdump generic/src/debug/real_map.bin
	$(MAKE) -C arch/$(ARCH)/boot/ clean

dist-clean:
	find generic/src/ generic/include/ -name arch -type l -exec rm \{\} \;
	find generic/src/ generic/include/ -name genarch -type l -exec rm \{\} \;	
	-rm Makefile.depend
	-$(MAKE) clean

generic/src/debug/real_map.bin: $(arch_objects) $(genarch_objects) $(objects) $(test_objects) arch/$(ARCH)/_link.ld 
	$(OBJCOPY) -I binary -O $(BFD_NAME) -B $(BFD_ARCH) --prefix-sections=symtab Makefile generic/src/debug/empty_map.o
	$(LD) -T arch/$(ARCH)/_link.ld $(LFLAGS) $(arch_objects) $(genarch_objects) $(objects) $(test_objects) generic/src/debug/empty_map.o -o $@ -Map kernel.map.pre
	$(OBJDUMP) -t $(arch_objects) $(genarch_objects) $(objects) $(test_objects) > kernel.objdump
	tools/genmap.py kernel.map.pre kernel.objdump generic/src/debug/real_map.bin 

generic/src/debug/real_map.o: generic/src/debug/real_map.bin
	$(OBJCOPY) -I binary -O $(BFD_NAME) -B $(BFD_ARCH) --prefix-sections=symtab $< $@


kernel.bin: $(arch_objects) $(genarch_objects) $(objects) $(test_objects) arch/$(ARCH)/_link.ld generic/src/debug/real_map.o
	$(LD) -T arch/$(ARCH)/_link.ld $(LFLAGS) $(arch_objects) $(genarch_objects) $(objects) $(test_objects) generic/src/debug/real_map.o -o $@ -Map kernel.map

%.o: %.S
	$(CC) $(ASFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

KS=`cat kernel.bin | wc -c`

boot:
	$(MAKE) -C arch/$(ARCH)/boot build KERNEL_SIZE=$(KS)
