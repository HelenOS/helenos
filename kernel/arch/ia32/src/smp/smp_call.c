#include <smp/smp_call.h>
#include <arch/smp/apic.h>
#include <arch/interrupt.h>
#include <cpu.h>

#ifdef CONFIG_SMP

void arch_smp_call_ipi(unsigned int cpu_id)
{
	(void) l_apic_send_custom_ipi(cpus[cpu_id].arch.id, VECTOR_SMP_CALL_IPI);
}

#endif /* CONFIG_SMP */

