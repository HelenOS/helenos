
#ifndef KERN_COMPILER_BARRIER_H_
#define KERN_COMPILER_BARRIER_H_

#define compiler_barrier() asm volatile ("" ::: "memory")

/** Forces the compiler to access (ie load/store) the variable only once. */
#define ACCESS_ONCE(var) (*((volatile typeof(var)*)&(var)))

#endif /* KERN_COMPILER_BARRIER_H_ */
