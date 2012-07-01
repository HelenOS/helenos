/* 
 */

#ifndef KERN_SMP_CALL_H_
#define	KERN_SMP_CALL_H_

#include <adt/list.h>
#include <synch/spinlock.h>
#include <atomic.h>

typedef void (*smp_call_func_t)(void *);

typedef struct smp_call {
	smp_call_func_t func;
	void *arg;
	link_t calls_link;
	atomic_t pending;
} smp_call_t;



extern void smp_call(unsigned int, smp_call_func_t, void *);
extern void smp_call_async(unsigned int, smp_call_func_t, void *, smp_call_t *);
extern void smp_call_wait(smp_call_t *);

extern void smp_call_init(void);

#ifdef CONFIG_SMP
extern void smp_call_ipi_recv(void);
extern void arch_smp_call_ipi(unsigned int);
#endif




#endif	/* KERN_SMP_CALL_H_ */

