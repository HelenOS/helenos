


#include <arch/interrupt.h>
#include <print.h>
#include <debug.h>
#include <panic.h>
#include <arch/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>



#include <test.h>
#include <arch.h>
#include <arch/smp/atomic.h>
#include <proc/thread.h>




thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags);


static void thread(void *data)
{
  while(1) 
  {
    double e,d,le,f;
    le=-1;
    e=0;
    f=1;
    for(d=1;e!=le;d*=f,f+=1) {le=e;e=e+1/d;}
    
    if((int)(100000000*e)==271828182) printf("THREAD:%s e OK\n",(char*)data);
    else panic("THREAD:%s e Failed\n",(char*)data);

    
//    printf("100000000*e:%d\n");
  }
  //printf("TEST:%s\n",(char*)data);  
}



void test(void)
{
  thread_t *t;
  
  t=thread_create(thread, (void*)"0", TASK,0);
  thread_ready(t);



  t=thread_create(thread, (void*)"1", TASK,0);
  thread_ready(t);



  t=thread_create(thread, (void*)"2", TASK,0);
  thread_ready(t);

  t=thread_create(thread, (void*)"3", TASK,0);
  thread_ready(t);
  while(1);

}

