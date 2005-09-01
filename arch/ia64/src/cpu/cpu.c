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
		"mov r15 = %0;;"
		"mov cr2 = r15;;"
		"mov psr.l = %1;;"
		: 
		: "r" (p), "r" (psr)
		: "r15"
	);



	/*Switch register bank of regs r16 .. r31 to 1 It is automaticly cleared on exception*/
	__asm__ volatile ("bsw.1;;");             
	

}


void gugux_pokus(void);
void gugux_pokus(void)
{
    panic("\n\nGUGUX Exception\n\n");
}



