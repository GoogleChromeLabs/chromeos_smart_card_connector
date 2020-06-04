#if !defined(DREAD_THREAD_CTXT_MAGIC)
/*
 * Context switch routine.
 */

#define DREAD_THREAD_CTXT_MAGIC   1

#if i386 || sparc || __x86_64__ || __x86_32__ || __native_client__
# include <setjmp.h>
# if  posix_signals
#  define DREAD_THREAD_MD_SAVE(regs)   setjmp((regs)->r)
#  define DREAD_THREAD_MD_LOAD(regs,val) longjmp((regs)->r,val)
# else
#  define DREAD_THREAD_MD_SAVE(regs)   _setjmp((regs)->r)
#  define DREAD_THREAD_MD_LOAD(regs,val) _longjmp((regs)->r,val)
# endif
#elif mips
# include <setjmp.h>
/*
 * With Mach on pmax/mips (which may have incorporated DEC's Ultrix md libc
 * code), the default longjmp incorporated error checking to prevent context
 * switching to stack frames with higher addresses.
 */
# define  DREAD_THREAD_MD_SAVE(regs)   setjmp((regs)->r)
# define  DREAD_THREAD_MD_LOAD(regs,val) mylongjmp((regs)->r,val)
#else
# error "What kind of machine am I being compiled on?"
#endif

#if DREAD_THREAD_CTXT_MAGIC
typedef struct {
  unsigned long magic1;
# define  DREAD_THREAD_CTXT_MAGIC_1 0x38127483ul
  jmp_buf   r;
  unsigned long magic2;
# define  DREAD_THREAD_CTXT_MAGIC_2 0xc843fa73ul
} dthr_ctxt_t;
# define  dthr_save_ctxt(regs) \
  ((regs)->magic1 = DREAD_THREAD_CTXT_MAGIC_1, \
   (regs)->magic2 = DREAD_THREAD_CTXT_MAGIC_2, \
   DREAD_THREAD_MD_SAVE((regs)))
# define  dthr_load_ctxt(regs,val)  do { \
    if ((regs)->magic1 != DREAD_THREAD_CTXT_MAGIC_1 || \
        (regs)->magic2 != DREAD_THREAD_CTXT_MAGIC_2) { \
      fprintf(stderr,"dthr_load_ctxt detected context corruption\n"); \
      abort(); \
      /*segv blocked?*/ \
      exit(1); \
    } \
    DREAD_THREAD_MD_LOAD((regs),val); \
  } while (0)

#else

typedef struct {
  jmp_buf   r;
} dthr_ctxt_t;
# define  dthr_save_ctxt(regs)      DREAD_THREAD_MD_SAVE((regs))
# define  dthr_load_ctxt(regs,val)  DREAD_THREAD_MD_LOAD((regs),val)
#endif

#endif
