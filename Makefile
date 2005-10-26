include Makefile.config
include arch/$(ARCH)/Makefile.inc
include genarch/Makefile.inc

sources=src/cpu/cpu.c \
	src/main/main.c \
	src/main/kinit.c \
	src/main/uinit.c \
	src/proc/scheduler.c \
	src/proc/thread.c \
	src/proc/task.c \
	src/proc/the.c \
	src/mm/buddy.c \
	src/mm/heap.c \
	src/mm/frame.c \
	src/mm/page.c \
	src/mm/tlb.c \
	src/mm/vm.c \
	src/lib/func.c \
	src/lib/list.c \
	src/lib/memstr.c \
	src/lib/sort.c \
	src/debug/print.c \
	src/debug/symtab.c \
	src/time/clock.c \
	src/time/timeout.c \
	src/time/delay.c \
	src/preempt/preemption.c \
	src/synch/spinlock.c \
	src/synch/condvar.c \
	src/synch/rwlock.c \
	src/synch/mutex.c \
	src/synch/semaphore.c \
	src/synch/waitq.c \
	src/smp/ipi.c \
	src/fb/font-8x16.c

# CFLAGS options same for all targets
CFLAGS+=-nostdinc -Iinclude/ -Werror-implicit-function-declaration -Wmissing-prototypes -Werror

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
	find src/ include/ -name arch -type l -exec rm \{\} \;
	find src/ include/ -name genarch -type l -exec rm \{\} \;	
	ln -s ../arch/$(ARCH)/src/ src/arch
	ln -s ../arch/$(ARCH)/include/ include/arch
	ln -s ../genarch/src/ src/genarch
	ln -s ../genarch/include/ include/genarch

depend:
	$(CC) $(CFLAGS) -M $(arch_sources) $(genarch_sources) $(sources) >Makefile.depend

build: kernel.bin boot

clean:
	find src/ arch/$(ARCH)/src/ genarch/src/ test/ -name '*.o' -exec rm \{\} \;
	-rm *.bin kernel.map kernel.map.pre kernel.objdump src/debug/real_map.bin
	$(MAKE) -C arch/$(ARCH)/boot/ clean

dist-clean:
	find src/ include/ -name arch -type l -exec rm \{\} \;
	find src/ include/ -name genarch -type l -exec rm \{\} \;	
	-rm Makefile.depend
	-$(MAKE) clean

src/debug/real_map.bin: $(arch_objects) $(genarch_objects) $(objects) $(test_objects) arch/$(ARCH)/_link.ld 
	$(OBJCOPY) -I binary -O $(BFD_NAME) -B $(BFD_ARCH) --prefix-sections=symtab Makefile src/debug/empty_map.o
	$(LD) -T arch/$(ARCH)/_link.ld $(LFLAGS) $(arch_objects) $(genarch_objects) $(objects) $(test_objects) src/debug/empty_map.o -o $@ -Map kernel.map.pre
	$(OBJDUMP) -t $(arch_objects) $(genarch_objects) $(objects) $(test_objects) > kernel.objdump
	tools/genmap.py kernel.map.pre kernel.objdump src/debug/real_map.bin 

src/debug/real_map.o: src/debug/real_map.bin
	$(OBJCOPY) -I binary -O $(BFD_NAME) -B $(BFD_ARCH) --prefix-sections=symtab $< $@


kernel.bin: $(arch_objects) $(genarch_objects) $(objects) $(test_objects) arch/$(ARCH)/_link.ld src/debug/real_map.o
	$(LD) -T arch/$(ARCH)/_link.ld $(LFLAGS) $(arch_objects) $(genarch_objects) $(objects) $(test_objects) src/debug/real_map.o -o $@ -Map kernel.map

%.o: %.S
	$(CC) $(ASFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

KS=`cat kernel.bin | wc -c`

boot:
	$(MAKE) -C arch/$(ARCH)/boot build KERNEL_SIZE=$(KS)
