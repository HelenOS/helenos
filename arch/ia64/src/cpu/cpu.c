#include <cpu.h>
#include <print.h>
#include <panic.h>
#include <arch/types.h>

void cpu_arch_init(void)
{
	int psr = 0x2000;
    
	__asm__  volatile (
		"{mov psr.l = %0 ;;}\n"
		"{srlz.i;"
		"srlz.d ;;}"
		: 
		: "r" (psr)
	);

	/* Switch to register bank 1. */
	__asm__ volatile 
	(
	    "bsw.1;;\n"
	);             
	
}


