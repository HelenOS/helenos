
#ifndef fpu_context_h
#define fpu_context_h


#include <arch/fpu_context.h>
#include <typedefs.h>

extern void fpu_context_save(fpu_context_t *);
extern void fpu_context_restore(fpu_context_t *);
extern void fpu_lazy_context_save(fpu_context_t *);
extern void fpu_lazy_context_restore(fpu_context_t *);



#endif /*fpu_context_h*/
