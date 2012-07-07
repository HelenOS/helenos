
#ifndef KERN_WORKQUEUE_H_
#define KERN_WORKQUEUE_H_

#include <adt/list.h>
#include <proc/thread.h>

/* Fwd decl. */
struct work_item;
struct work_queue;
typedef struct work_queue work_queue_t;

typedef void (*work_func_t)(struct work_item *);

typedef struct work_item {
	link_t queue_link;
	work_func_t func;
	
#ifdef CONFIG_DEBUG
	/* Magic number for integrity checks. */
	uint32_t cookie;
#endif 
} work_t;



extern void workq_global_init(void);
extern void workq_global_worker_init(void);
extern void workq_global_stop(void);
extern int workq_global_enqueue_noblock(work_t *, work_func_t);
extern int workq_global_enqueue(work_t *, work_func_t);

extern struct work_queue * workq_create(const char *);
extern void workq_destroy(struct work_queue *);
extern int workq_init(struct work_queue *, const char *);
extern void workq_stop(struct work_queue *);
extern int workq_enqueue_noblock(struct work_queue *, work_t *, work_func_t);
extern int workq_enqueue(struct work_queue *, work_t *, work_func_t);

extern void workq_print_info(struct work_queue *);
extern void workq_global_print_info(void);


extern void workq_after_thread_ran(void);
extern void workq_before_thread_is_ready(thread_t *);

#endif /* KERN_WORKQUEUE_H_ */
