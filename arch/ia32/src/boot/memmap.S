


#include <arch/boot/memmap.h>

.global memmap_arch_init

.code16
.section K_TEXT_START

memmap_arch_init:
    
    ret