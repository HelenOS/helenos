#include <smp/smp_call.h>
#include <arch.h>
#include <config.h>
#include <preemption.h>
#include <compiler/barrier.h>
#include <arch/barrier.h>
#include <arch/asm.h>  /* interrupt_disable */


static void call_start(smp_call_t *call_info, smp_call_func_t func, void *arg);
static void call_done(smp_call_t *call_info);
static void call_wait(smp_call_t *call_info);


void smp_call_init(void)
{
	ASSERT(CPU);
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	
	spinlock_initialize(&CPU->smp_calls_lock, "cpu[].smp_calls_lock");
	list_initialize(&CPU->smp_pending_calls);
}

/** Invokes a function on a specific cpu and waits for it to complete.
 * 
 * Calls @a func on the CPU denoted by its logical id @cpu_id . 
 * The function will execute with interrupts disabled. It should 
 * be a quick and simple function and must never block. 
 * 
 * If @a cpu_id is the local CPU, the function will be invoked
 * directly.
 * 
 * @param cpu_id Destination CPU's logical id (eg CPU->id)
 * @param func Function to call.
 * @param arg Argument to pass to the user supplied function @a func.
 */
void smp_call(unsigned int cpu_id, smp_call_func_t func, void *arg)
{
	smp_call_t call_info;
	smp_call_async(cpu_id, func, arg, &call_info);
	smp_call_wait(&call_info);
}

/** Invokes a function on a specific cpu asynchronously.
 * 
 * Calls @a func on the CPU denoted by its logical id @cpu_id . 
 * The function will execute with interrupts disabled. It should 
 * be a quick and simple function and must never block. 
 * 
 * Pass @a call_info to smp_call_wait() in order to wait for 
 * @a func to complete.
 * 
 * @a call_info must be valid until @a func returns.
 * 
 * If @a cpu_id is the local CPU, the function will be invoked
 * directly. If the destination cpu id @a cpu_id is invalid
 * or denotes an inactive cpu, the call is discarded immediately.
 * 
 * Interrupts must be enabled. Otherwise you run the risk
 * of a deadlock.
 * 
 * @param cpu_id Destination CPU's logical id (eg CPU->id).
 * @param func Function to call.
 * @param arg Argument to pass to the user supplied function @a func.
 * @param call_info Use it to wait for the function to complete. Must
 *          be valid until the function completes.
 */
void smp_call_async(unsigned int cpu_id, smp_call_func_t func, void *arg, 
	smp_call_t *call_info)
{
	/* todo: doc deadlock */
	ASSERT(!interrupts_disabled());
	ASSERT(call_info != NULL);
	
	/* Discard invalid calls. */
	if (config.cpu_count <= cpu_id || !cpus[cpu_id].active) {
		call_start(call_info, func, arg);
		call_done(call_info);
		return;
	}
	
	/* Protect cpu->id against migration. */
	preemption_disable();

	call_start(call_info, func, arg);
	
	if (cpu_id != CPU->id) {
#ifdef CONFIG_SMP
		spinlock_lock(&cpus[cpu_id].smp_calls_lock);
		list_append(&call_info->calls_link, &cpus[cpu_id].smp_pending_calls);
		spinlock_unlock(&cpus[cpu_id].smp_calls_lock);

		/*
		 * If a platform supports SMP it must implement arch_smp_call_ipi().
		 * It should issue an IPI an cpu_id and invoke smp_call_ipi_recv()
		 * on cpu_id in turn. 
		 * 
		 * Do not implement as just an empty dummy function. Instead
		 * consider providing a full implementation or at least a version 
		 * that panics if invoked. Note that smp_call_async() never
		 * calls arch_smp_call_ipi() on uniprocessors even if CONFIG_SMP.
		 */
		arch_smp_call_ipi(cpu_id);
#endif
	} else {
		/* Invoke local smp calls in place. */
		ipl_t ipl = interrupts_disable();
		func(arg);
		interrupts_restore(ipl);
		
		call_done(call_info);
	}
	
	preemption_enable();
}

/** Waits for a function invoked on another CPU asynchronously to complete.
 * 
 * Does not sleep but rather spins.
 * 
 * Example usage:
 * @code
 * void hello(void *p) {
 *     puts((char*)p);
 * }
 * 
 * smp_call_t call_info;
 * smp_call_async(cpus[2].id, hello, "hi!\n", &call_info);
 * // Do some work. In the meantime, hello() is executed on cpu2.
 * smp_call_wait(&call_info);
 * @endcode
 * 
 * @param call_info Initialized by smp_call_async().
 */
void smp_call_wait(smp_call_t *call_info)
{
	call_wait(call_info);
}

#ifdef CONFIG_SMP

/** Architecture independent smp call IPI handler.
 * 
 * Interrupts must be disabled. Tolerates spurious calls.
 */
void smp_call_ipi_recv(void)
{
	ASSERT(interrupts_disabled());
	ASSERT(CPU);
	
	list_t calls_list;
	list_initialize(&calls_list);
	
	spinlock_lock(&CPU->smp_calls_lock);
	list_splice(&CPU->smp_pending_calls, &calls_list.head);
	spinlock_unlock(&CPU->smp_calls_lock);

	/* Walk the list manually, so that we can safely remove list items. */
	for (link_t *cur = calls_list.head.next, *next = cur->next; 
		!list_empty(&calls_list); cur = next, next = cur->next) {
		
		smp_call_t *call_info = list_get_instance(cur, smp_call_t, calls_link);
		list_remove(cur);
		
		call_info->func(call_info->arg);
		call_done(call_info);
	}
}

#endif /* CONFIG_SMP */

static void call_start(smp_call_t *call_info, smp_call_func_t func, void *arg)
{
	link_initialize(&call_info->calls_link);
	call_info->func = func;
	call_info->arg = arg;
	
	/*
	 * We can't use standard spinlocks here because we want to lock
	 * the structure on one cpu and unlock it on another (without
	 * messing up the preemption count).
	 */
	atomic_set(&call_info->pending, 1);
	
	/* Let initialization complete before continuing. */
	memory_barrier();
}

static void call_done(smp_call_t *call_info)
{
	/* 
	 * Separate memory accesses of the called function from the 
	 * announcement of its completion.
	 */
	memory_barrier();
	atomic_set(&call_info->pending, 0);
}

static void call_wait(smp_call_t *call_info)
{
	do {
		/* 
		 * Ensure memory accesses following call_wait() are ordered
		 * after completion of the called function on another cpu. 
		 * Also, speed up loading of call_info->pending.
		 */
		memory_barrier();
	} while (atomic_get(&call_info->pending));
}



