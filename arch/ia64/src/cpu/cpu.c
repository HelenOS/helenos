#include <cpu.h>
#include <print.h>
#include <panic.h>
#include <arch/types.h>

extern int IVT;




void cpu_arch_init(void)
{



    int *p=&IVT;
    volatile __u64 hlp,hlp2;


    int psr = 0x2000;
    

	__asm__  volatile (
		"mov cr2 = %0;;\n"
		"mov psr.l = %1;;\n"
		"srlz.i;"
		"srlz.d;;"
		: 
		: "r" (p), "r" (psr)
	);



	/*Switch register bank of regs r16 .. r31 to 1 It is automaticly cleared on exception*/
	__asm__ volatile 
	(
	    "bsw.1;;\n"
	);             
	

}


