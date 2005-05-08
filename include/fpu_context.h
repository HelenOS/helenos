
#ifndef fpu_context_h
#define fpu_context_h

extern void fpu_context_save(void);
extern void fpu_context_restore(void);
extern void fpu_lazy_context_save(void);
extern void fpu_lazy_context_restore(void);


#endif /*fpu_context_h*/
