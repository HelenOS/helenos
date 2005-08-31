#include <cpu.h>
#include <print.h>
#include <panic.h>

extern int IVT;




void cpu_arch_init(void)
{



    int *p=&IVT;


	__asm__ (
		"mov r15 = %0;;"
		"mov cr2 = r15;;"
		: 
		: "r" (p)
		: "r15"
	);

}


void gugux_pokus(void);
void gugux_pokus(void)
{
    panic("\n\nGUGUX Exception\n\n");
}



