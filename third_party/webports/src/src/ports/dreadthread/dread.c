/*
 * Copyright (c) 1994, 1996 by Bennet Yee.
 *
 * This material was developed by the author.  Permission to copy this
 * software, to redistribute it, and to use it for any purpose is hereby
 * granted, subject to the following five restrictions.
 *
 * 1. Any copy made of this software must include this copyright notice in
 * full.
 *
 * 2. All materials containing this software or derivatives thereof must
 * provide this software in source form or provide a means of obtaining
 * this software in source form; no fees may be charged for this software
 * except those for covering media costs and handling.
 *
 * 3. All materials developed as a consequence of the use of this software
 * shall duly acknowledge such use, in accordance with the usual standards
 * of acknowledging credit in academic research.
 *
 * 4. The author makes no warranty or representation that the operation
 * of this software will be error-free, and the author is under no
 * obligation to provide any services, by way of maintenance, update, or
 * otherwise.
 *
 * 5. In conjunction with products arising from the use of this material,
 * there shall be no use of the names of the author, of Carnegie-Mellon
 * University or University of California, nor of any adaptation thereof
 * in any advertising, promotional, or sales literature without prior
 * written consent from the author, Carnegie-Mellon University, and
 * University of California in each case.
 *
 * If you have questions, you can contact the author at bsy@cs.ucsd.edu
 *
 * Users of this software is requested to make their best efforts (a) to
 * return to the author any improvements or extensions that they make, so
 * that these may be included in future releases; (b) to document clearly
 * any improvements or extensions made to this software; and (c) to inform
 * the author of noteworthy uses of this software.  Any improvements or
 * extensions to this software may be placed under different copyright
 * restrictions by the author of those improvements or extensions.  That
 * those improvements or extensions be placed under the same copyright as
 * this software is encouraged.
 */

/*
 * dread thread -- portable mostly machine independent coroutine threads.
 *
 * Created, bsy@cs.cmu.edu, 1994.
 * Added stack overflow checks, bsy@cs.ucsd.edu, 1996.
 *
 * To do:
 *
 * I/O thread wait.
 * Preemption via timers / signals.
 */
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "dreadthread.h"

#define MAGIC_TEST      1

#define DEBUG           0
#define DEBUG_QUEUES    0
#define DEBUG_SIGBLOCK  0
#define SIG_DISABLE     0
#if DEBUG
# if 1
#  define DBOUT(args) do { printf args; fflush(stdout); } while (0)
# else
#  include <stdarg.h>
char dboutbuf[4096];
#  define DBOUT(args) do { dbprint args; } while (0)
void dbprint(char *fmt, ...)
{
  va_list alist;
  int nch;
  va_start(alist,fmt);
  nch = vsprintf(dboutbuf,fmt,alist);
  write(1,dboutbuf,nch);
  va_end(alist);
}
# endif
# define  SHOWTHREAD  do { \
    printf("Th: %p\n", (void *) dthr_cur_thread); \
  } while (0)
# if  !defined(SIG_DISABLE)
#  define SIG_DISABLE 1
# endif
#else
# define  DBOUT(args)
# define  SHOWTHREAD
#endif

#define DREAD_THREAD_CSW_CSW    0
#define DREAD_THREAD_CSW_NORM   1
#define DREAD_THREAD_CSW_CREATE   2
#define DREAD_THREAD_CSW_EXIT   3
#define DREAD_THREAD_CSW_MAX    4

struct dthr_thread            *dthr_cur_thread = 0;

static struct dthr_stack      dthr_topmost_stack;
static struct dthr_thread     dthr_topmost_thread;
static struct dthr_chain      dthr_free_stacks, dthr_active_stacks,
                              dthr_runq, dthr_newq;
static struct dthr_semaphore  dthr_newq_sema;
static struct dthr_event      dthr_newq_event;
static dthr_ctxt_t            dthr_deadlock;
int       (*dthr_on_deadlock)() = 0;

#if DEBUG_QUEUES
void dthr_show_queues(void)
{
#define SHOW(var) fprintf(stderr,"\n" #var ":\n"); dthr_chain_show(stderr,&var);
  SHOW(dthr_free_stacks);
  SHOW(dthr_active_stacks);
  SHOW(dthr_runq);
  SHOW(dthr_newq);
  SHOW(dthr_newq_event.threadq);
  SHOW(dthr_newq_sema.threadq);
}
#else
# define  dthr_show_queues()  do { ;} while (0)
#endif


/* may eventually want to sort stacks by addresses */
struct dthr_thread  *dthr_this_thread(void)
{
  int                         stack_var;
  register caddr_t            stack_addr = (caddr_t) &stack_var;
  register struct dthr_chain  *p;

  DBOUT(("dthr_this_thread:  scanning for stack addr %p\n",
         (void *) stack_addr));
  for (p = dthr_active_stacks.next; p != &dthr_active_stacks; p = p->next) {
#if MAGIC_TEST
    if (((struct dthr_stack *) p)->magic != DREAD_THREAD_STACK_MAGIC) {
      fprintf(stderr,
              "dthr_thread: stack descriptor corruption"
              " detected by dthr_this_thread()\n");
      abort();
    }
#endif
#if DREAD_THREAD_STACK_GROWS_DOWN
    if (((struct dthr_stack *) p)->stack_top <= stack_addr
        && stack_addr <= ((struct dthr_stack *) p)->stack_base)
      break;
#else
    if (((struct dthr_stack *) p)->stack_base <= stack_addr
        && stack_addr <= ((struct dthr_stack *) p)->stack_top)
      break;
#endif
  }
  if (DEBUG) {  /* DCE */
    if (p == &dthr_active_stacks) {
      printf("dthr_this_thread: NOT FOUND, assuming TOPMOST (%p)\n",
             (void *) &dthr_topmost_thread);
    } else {
      printf("dthr_this_thread: thread %p",
             (void *) ((struct dthr_stack *) p)->thread);
    }
  }
  return (p == &dthr_active_stacks) ?
      &dthr_topmost_thread : ((struct dthr_stack *) p)->thread;
}

static void dthr_csw(struct dthr_stack  *target, int  op);

void dthr_thread_yield(void)
{
  struct dthr_thread  *next_runnable;

  SHOWTHREAD;
  DBOUT(("dthr_thread_yield\n"));
  next_runnable = (struct dthr_thread *) DREAD_THREAD_CHAIN_DEQUEUE(&dthr_runq);
  if (next_runnable) {
#if MAGIC_TEST
    if (next_runnable->magic != DREAD_THREAD_TH_MAGIC) {
      fprintf(stderr,
              "dthr_thread:  thread structure corruption detected"
              " in dthr_thread_yield()\n");
      abort();
    }
#endif
    DBOUT((" found runnable thread %p\n",(void *) next_runnable));
    (void) dthr_chain_enqueue(&dthr_runq,&dthr_cur_thread->link);
    dthr_csw(next_runnable->stack,DREAD_THREAD_CSW_NORM);
  }
  SHOWTHREAD;
  DBOUT(("LV dthr_thread_yield\n"));
}

struct dthr_semaphore *dthr_semaphore_init(struct dthr_semaphore  *sema,
               int      init)
{
  sema->value = init;
  (void) dthr_chain_init(&sema->threadq);
  sema->magic = DREAD_THREAD_SEMA_MAGIC;
  return sema;
}

/* current thread must already be enqueued somewhere */
static void dthr_thread_sleep(int leave)
{
  struct dthr_thread  *next_thread;

  SHOWTHREAD;
  DBOUT(("dthr_thread_sleep(%d):",leave));
  dthr_show_queues();

  do {
    next_thread = (struct dthr_thread *) DREAD_THREAD_CHAIN_DEQUEUE(&dthr_runq);
    DBOUT((" %sthread found %p\n",next_thread?"":"NO ",(void *) next_thread));

  } while (!next_thread && dthr_on_deadlock && (*dthr_on_deadlock)());

  if (!next_thread) {
    dthr_load_ctxt(&dthr_deadlock,1);
    fprintf(stderr,"dthr_thread:  DEADLOCK load context returned\n");
    abort();
  }
#if MAGIC_TEST
  if (next_thread->magic != DREAD_THREAD_TH_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  thread structure corruption detected"
            " in dthr_thread_sleep(%d)\n",
            leave);
    abort();
  }
#endif
  dthr_csw(next_thread->stack,leave ? DREAD_THREAD_CSW_EXIT
           : DREAD_THREAD_CSW_NORM);
  SHOWTHREAD;
  DBOUT(("LV dthr_thread_sleep\n"));
}

int dthr_semaphore_try(struct dthr_semaphore  *sema)
{
  register int  rv;

  SHOWTHREAD;
  DBOUT(("dthr_semaphore_try(%p):",(void *) sema));
#if MAGIC_TEST
  if (sema->magic != DREAD_THREAD_SEMA_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  semaphore structure corruption detected"
            " in dthr_semaphore_try(%p)\n",
            (void *) sema);
    abort();
  }
#endif
  if (sema->value == 0) {
    rv = 0;
  } else {
    sema->value--;
    rv = 1;
  }
  DBOUT(("\n"));
  return rv;
}

static void dthr_semaphore_take_no_yield(struct dthr_semaphore  *sema)
{
  SHOWTHREAD;
  DBOUT(("dthr_semaphore_take_no_yield(%p)\n",(void *) sema));
#if MAGIC_TEST
  if (sema->magic != DREAD_THREAD_SEMA_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  semaphore structure corruption detected"
            " in dthr_semaphore_take_no_yield(%p),1\n",
            (void *) sema);
    abort();
  }
#endif
  while (sema->value == 0) {
    DBOUT(("dthr_semaphore_take_no_yield: not available\n"));
    dthr_chain_enqueue(&sema->threadq,&dthr_cur_thread->link);
    dthr_cur_thread->state = DREAD_THREAD_TH_SEMA_WAIT;
    dthr_thread_sleep(0);
#if MAGIC_TEST
    if (sema->magic != DREAD_THREAD_SEMA_MAGIC) {
      fprintf(stderr,
              "dthr_thread:  semaphore structure corruption detected"
              " in dthr_semaphore_take_no_yield(%p),2\n",
              (void *) sema);
      abort();
    }
#endif
  }
  sema->value--;
  SHOWTHREAD;
  DBOUT(("LV dthr_semaphore_take_no_yield\n"));
}

void  dthr_semaphore_take(struct dthr_semaphore *sema)
{
  SHOWTHREAD;
  DBOUT(("dthr_semaphore_take(%p)\n",(void *) sema));
  /* yield -before- taking the semaphore! */
  dthr_thread_yield();
  dthr_semaphore_take_no_yield(sema);
  SHOWTHREAD;
  DBOUT(("LV dthr_semaphore_take\n"));
}

static void dthr_semaphore_drop_no_yield(struct dthr_semaphore  *sema)
{
  struct dthr_thread  *waker;

  SHOWTHREAD;
  DBOUT(("dthr_semaphore_drop_no_yield(%p)\n",(void *) sema));
#if MAGIC_TEST
  if (sema->magic != DREAD_THREAD_SEMA_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  semaphore structure corruption detected"
            " in dthr_semaphore_drop_no_yield(%p)\n",
            (void *) sema);
    abort();
  }
#endif
  ++sema->value;
  waker = (struct dthr_thread *) DREAD_THREAD_CHAIN_DEQUEUE(&sema->threadq);
  if (waker) {
    waker->state = DREAD_THREAD_TH_RUNNABLE;
    dthr_chain_enqueue(&dthr_runq,&waker->link);
  }
  SHOWTHREAD;
  DBOUT(("LV dthr_semaphore_drop_no_yield\n"));
}

void  dthr_semaphore_drop(struct dthr_semaphore *sema)
{
  SHOWTHREAD;
  DBOUT(("dthr_semaphore_drop(%p)\n",(void *) sema));
  dthr_semaphore_drop_no_yield(sema);
  dthr_thread_yield();
  SHOWTHREAD;
  DBOUT(("LV dthr_semaphore_drop\n"));
}

void  dthr_event_wait(struct dthr_event *event,
      struct dthr_semaphore *lock)
{
  SHOWTHREAD;
  DBOUT(("dthr_event_wait(%p,%p)\n",(void *) event,(void *) lock));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif

  DBOUT(("p_e_w: enqueuing current thread %p\n",(void *) dthr_cur_thread));
#if MAGIC_TEST
  if (event->magic != DREAD_THREAD_EV_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  event structure corruption detected"
            " in dthr_event_wait(%p,%p)\n",
            (void *) event,(void *) lock);
    abort();
  }
#endif
  DBOUT(("p_e_w: about to enqueue this thread onto event\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
  dthr_chain_enqueue(&event->threadq,&dthr_cur_thread->link);
  dthr_cur_thread->state = DREAD_THREAD_TH_EVENT_WAIT;
  DBOUT(("p_e_w: dropping lock %p\n",(void *) lock));
  dthr_semaphore_drop_no_yield(lock);

  DBOUT(("p_e_w: about to go sleeping\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
  DBOUT(("p_e_w: sleeping\n"));
  dthr_thread_sleep(0);
  DBOUT(("p_e_w: awaken, retaking lock %p\n",(void *) lock));
  dthr_semaphore_take_no_yield(lock);

  DBOUT(("about to lv dthr_event_wait, SHOWTHREAD/chain_show first\n"));
  SHOWTHREAD;
  DBOUT(("about to lv dthr_event_wait, calling chain show prior to exiting\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
  DBOUT(("LV dthr_event_wait\n"));
}

static void dthr_eventq_to_runq(struct dthr_event *event,
            unsigned int  max)
{
  struct dthr_thread  *th;

#if MAGIC_TEST
  if (event->magic != DREAD_THREAD_EV_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  event structure corruption detected"
            " in dthr_eventq_to_runq(%p,%d)\n",
            (void *) event,max);
    abort();
  }
#endif
  while (max > 0 &&
         NULL != (th = (struct dthr_thread *)
                  DREAD_THREAD_CHAIN_DEQUEUE(&event->threadq))) {
#if MAGIC_TEST
    if (th->magic != DREAD_THREAD_TH_MAGIC) {
      fprintf(stderr,
              "dthr_thread:  thread structure corruption detected"
              " in dthr_eventq_to_runq(%p,%d)\n",
              (void *) event,max);
      abort();
    }
#endif
#if DEBUG
    dthr_chain_show(stderr,&event->threadq);
#endif
    th->state = DREAD_THREAD_TH_RUNNABLE;
    dthr_chain_enqueue(&dthr_runq,&th->link);
    DBOUT(("dthr_eventq_move: %p is now runnable\n",(void *) th));
    --max;
  }
}

void  dthr_event_broadcast_no_yield(struct dthr_event *event)
{
  SHOWTHREAD;
  DBOUT(("dthr_event_broadcast_no_yield(%p)\n",(void *) event));

#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
  dthr_eventq_to_runq(event,UINT_MAX);
  SHOWTHREAD;
  DBOUT(("LV dthr_event_broadcast\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
}

void  dthr_event_broadcast(struct dthr_event  *event)
{
  SHOWTHREAD;
  DBOUT(("dthr_event_broadcast(%p)\n",(void *) event));

  dthr_eventq_to_runq(event,UINT_MAX);
  dthr_thread_yield();
  SHOWTHREAD;
  DBOUT(("LV dthr_event_broadcast\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
}

void  dthr_event_signal_no_yield(struct dthr_event  *event)
{
  SHOWTHREAD;
  DBOUT(("dthr_event_signal_no_yield(%p)\n",(void *) event));

#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
  dthr_eventq_to_runq(event,1);
  SHOWTHREAD;
  DBOUT(("LV dthr_event_signal_no_yield\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
}

void  dthr_event_signal(struct dthr_event *event)
{
  SHOWTHREAD;
  DBOUT(("dthr_event_signal(%p)\n",(void *) event));

  dthr_eventq_to_runq(event,1);
  dthr_thread_yield();
  SHOWTHREAD;
  DBOUT(("LV dthr_event_signal\n"));
#if DEBUG
  dthr_chain_show(stderr,&event->threadq);
#endif
}

struct dthr_event *dthr_event_init(struct dthr_event  *ev)
{
  (void) dthr_chain_init(&ev->threadq);
  ev->magic = DREAD_THREAD_EV_MAGIC;
  return ev;
}

/*
 * Should be first dthr_* routine to be called.
 */
void  dthr_init(void)
{
#if DEBUG
  setbuf(stdout,0);
#endif
  (void) dthr_chain_init(&dthr_free_stacks);
  (void) dthr_chain_init(&dthr_active_stacks);
  (void) dthr_chain_init(&dthr_runq);
  (void) dthr_chain_init(&dthr_newq);
  dthr_topmost_thread.stack = 0;
  dthr_topmost_thread.magic = DREAD_THREAD_TH_MAGIC;
  (void) dthr_semaphore_init(&dthr_newq_sema,1);
  (void) dthr_event_init(&dthr_newq_event);
}

void  dthr_thread_exit(void *status)
{
  struct dthr_thread_exit   *x;

  SHOWTHREAD;
  DBOUT(("dthr_thread_exit\n"));
  while ((x = dthr_cur_thread->on_exit) != 0) {
    dthr_cur_thread->on_exit = x->next;
    (*x->fn)(dthr_cur_thread,x->arg);
    free(x);
  }
#if MAGIC_TEST
  if (dthr_cur_thread->magic != DREAD_THREAD_TH_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  thread structure corruption detected"
            " in dthr_thread_exit(%p)\n",
            status);
      abort();
    }
#endif
  dthr_cur_thread->exit_value = status;
  dthr_cur_thread->state = DREAD_THREAD_TH_EXITED;

  SHOWTHREAD;
  DBOUT(("dthr_thread_exit -> sleep"));
  dthr_thread_sleep(1);

  fprintf(stderr,"dthr_thread_exit:  exited thread still running\n");
  abort();
}

struct dthr_thread  *dthr_thread_init(struct dthr_thread  *th,
                                      void                *(*fn)(void *),
                                      void                *fn_arg,
                                      size_t              requested_stack_size)
{
  DBOUT(("dthr_thread_init(%p,%p,%p,%p)\n",
         (void *) th,(void *) fn,(void *) fn_arg,
         (void *) requested_stack_size));
  th->fn = fn;
  th->fn_arg = fn_arg;
  th->stack_size = requested_stack_size;

  th->exit_value = (void *) 0;
  th->state = DREAD_THREAD_TH_RUNNABLE;
  th->stack = 0;
  (void) dthr_semaphore_init(&th->exit_sema,0);
  th->on_exit = 0;
  th->magic = DREAD_THREAD_TH_MAGIC;
  /*
   * no stack bound to this thread yet
   */
  return th;
}

int dthr_thread_on_exit(struct dthr_thread  *th,
          void    (*fn)(struct dthr_thread *,void *),
          void    *arg)
{
  struct dthr_thread_exit *x;

  DBOUT(("on exit thread %p, fn %p\n",(void *) th,(void *) fn));
  if (!(x = (struct dthr_thread_exit *) malloc(sizeof *x))) return 0;
#if MAGIC_TEST
  if (th->magic != DREAD_THREAD_TH_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  thread structure corruption detected"
            " in dthr_thread_on_exit(%p,%p,%p)\n",
            (void *) th,(void *) fn,(void *) arg);
    abort();
  }
#endif

  x->next = th->on_exit;
  x->fn = fn;
  x->arg = arg;
  th->on_exit = x;
  return 1;
}

/*
 * Place onto new thread queue.  Topmost thread creates stack space and
 * moves thread from new queue to run queue.
 */
struct dthr_thread  *dthr_thread_run(struct dthr_thread *th)
{
#if MAGIC_TEST
  if (th->magic != DREAD_THREAD_TH_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  thread structure corruption detected"
            " in dthr_thread_run(%p)\n",
            (void *) th);
    abort();
  }
#endif
  SHOWTHREAD;
  DBOUT(("dthr_thread_run(%p)\n",(void *) th));
  dthr_semaphore_take_no_yield(&dthr_newq_sema);
  dthr_chain_enqueue(&dthr_newq,&th->link);
  dthr_event_signal_no_yield(&dthr_newq_event);
  dthr_semaphore_drop_no_yield(&dthr_newq_sema);
  dthr_thread_yield();
  SHOWTHREAD;
  DBOUT(("LV dthr_thread_run\n"));
  return th;
}

struct dthr_thread  *dthr_thread_wait(struct dthr_thread *th)
{
#if MAGIC_TEST
  if (th->magic != DREAD_THREAD_TH_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  thread structure corruption detected"
            " in dthr_thread_wait(%p)\n",
            (void *) th);
    abort();
  }
#endif
  dthr_semaphore_take(&th->exit_sema);
  dthr_semaphore_drop(&th->exit_sema);
  return th;
}

struct dthr_thread  *dthr_thread_detach(struct dthr_thread *th)
{
#if MAGIC_TEST
  if (th->magic != DREAD_THREAD_TH_MAGIC) {
    fprintf(stderr,
            "dthr_thread:  thread structure corruption detected"
            " in dthr_thread_detach(%p)\n",
            (void *) th);
    abort();
  }
#endif
  dthr_semaphore_drop(&th->exit_sema);
  /* self cleanup? */
  return th;
}

/*
 * Returns a stack descriptor from free list -- best fit.
 */
static struct dthr_stack  *dthr_find_free_stack(int size)
{
  struct dthr_stack *stk, *best_so_far;
  int     best_size_diff = INT_MAX, size_diff;

  DBOUT(("dthr_find_free_stack\n"));
  for (best_so_far = 0, stk = (struct dthr_stack *) dthr_free_stacks.next;
       (struct dthr_chain *) stk != &dthr_free_stacks;
       stk = (struct dthr_stack *) stk->link.next) {
#if MAGIC_TEST
    if (stk->magic != DREAD_THREAD_STACK_MAGIC) {
      fprintf(stderr,
              "dthr_thread: stack descriptor corruption detected"
              " by dthr_find_free_stack(0x%08x)\n",
              size);
      abort();
    }
#endif
    if ((size_diff = stk->stack_size - size) > 0
        && size_diff < best_size_diff) {
      best_size_diff = size_diff;
      best_so_far = stk;
    }
  }
  DBOUT(("dthr_find_free_stack -> %p\n",(void *) best_so_far));
  return best_so_far;
}

static void dthr_thread_launcher(void);

static void dthr_new_topmost_thread(struct dthr_stack *stk,
                                    size_t            size_wanted,
                                    int               first_time,
                                    dthr_ctxt_t       *continuation)
{
  int         base_candidate;
  caddr_t     stack_pos;
  int         sofar;
#if DEBUG
  static int  counter;
#endif

#if MAGIC_TEST
  if (stk->magic != DREAD_THREAD_STACK_MAGIC) {
    fprintf(stderr,
            "dthr_thread: stack descriptor corruption detected"
            " by dthr_new_topmost_thread(%p,0x%08lx,%d,%p)\n",
            (void *) stk,(unsigned long) size_wanted,
            first_time,(void *) continuation);
    abort();
  }
#endif
  /*
   * The new thread is on the run queue, and when the
   * topmost launcher thread finishes, the new thread
   * will context switch normally and be run.
   */
  if (first_time) {
#if DEBUG
    DBOUT(("dthr_new_topmost_thread(%p,0x%08x,%d)\n",
           (void *) stk,size_wanted,first_time));
    DBOUT(("dthr_new_topmost_thread: stack at %p, prev stack_base at %p\n",
           (void *) &base_candidate,(void *) stk->stack_base));
    counter = 0;
#endif
    DBOUT(("dthr_new_topmost_thread.save_ctxt(%p)\n",(void *) &stk->regs));
    if (dthr_save_ctxt(&stk->regs)) {
      DBOUT(("dthr_new_topmost_thread: topmost split, thread %p on the way\n",
             (void *) stk->thread));
      DBOUT(("dthr_new_topmost_thread.load_ctxt(%p,1)\n",
             (void *) continuation));
      dthr_load_ctxt(continuation,1);
    }
  }

  stack_pos = (void *) &base_candidate;
#if DREAD_THREAD_STACK_GROWS_DOWN
  sofar = stk->stack_base - stack_pos;
#else
  sofar = stack_pos - stk->stack_base;
#endif

#if DEBUG
  ++counter;
  if (!(counter % 0x10)) {
    printf("dthr_new_topmost_thread: stack 0x%08x so far\n",sofar);
  }
#endif
  if (sofar < size_wanted + DREAD_THREAD_STACK_EXTRA) {
    dthr_new_topmost_thread(stk,size_wanted,0,continuation);
    /* compiler should not tail recurse here! */
  }
  DBOUT(("dthr_new_topmost_thread: enough stack!\n"));
  /*
   * Actual thread_fork happens here.  The stack has grown to the
   * appropriate size, and now we can reset the topmost-stack descriptor
   * to updated values.
   */
  DBOUT(("dthr_new_topmost_thread:  marking thread %p as runnable\n",
         (void *) stk->thread));
  dthr_chain_enqueue(&dthr_runq,&stk->thread->link);

  DBOUT(("dthr_new_topmost_thread -> launcher\n"));
  dthr_thread_launcher();
}

static void dthr_thread_launcher(void)
{
  unsigned long           magic = DREAD_THREAD_MAGIC2;
  struct dthr_thread      *new_th;
  struct dthr_stack       *new_stk;
  dthr_ctxt_t             continuation;
  struct dthr_thread_exit *x;

  DBOUT(("dthr_thread_launcher\n"));

  dthr_topmost_stack.stack_size = 0;
  dthr_topmost_stack.stack_base = (void *) &magic;
  dthr_topmost_stack.stack_top = 0;
  dthr_topmost_stack.thread = &dthr_topmost_thread;
  dthr_topmost_stack.magic = DREAD_THREAD_STACK_MAGIC;

  dthr_topmost_thread.stack = &dthr_topmost_stack;

  DBOUT(("p_t_l: grabbing new thread queue lock\n"));
  dthr_semaphore_take_no_yield(&dthr_newq_sema);
  for (;;) {
    DBOUT(("p_t_l: getting thread\n"));
    dthr_show_queues();
    while ((new_th = (struct dthr_thread *)
            DREAD_THREAD_CHAIN_DEQUEUE(&dthr_newq)) != 0) {
#if MAGIC_TEST
      if (new_th->magic != DREAD_THREAD_TH_MAGIC) {
        fprintf(stderr,
                "dthr_thread:  thread structure corruption detected"
                " in dthr_thread_launcher(), thread scan\n");
        abort();
      }
#endif
      DBOUT(("p_t_l: launching thread %p\n",(void *) new_th));
      /* dthr_find_free_stack does magic test */
      if (0 != (new_stk = dthr_find_free_stack(new_th->stack_size))) {
        /*
         * Found a stack for this thread,
         * so now we can just bind them together
         * and context switch to the base of
         * the stack to launch the thread.
         */
        DBOUT(("p_t_l:  free stack available on free list %p\n",
               (void *) new_stk));
        DBOUT(("p_t_l:  reverting regs\n"));
        memcpy((void *) &new_stk->regs,(void *) &new_stk->base,
               sizeof new_stk->regs);
        new_stk->thread = new_th;
        new_th->stack = new_stk;
        new_th->state = DREAD_THREAD_TH_RUNNABLE;
        dthr_chain_enqueue(&dthr_runq,&new_th->link);
      } else {
        DBOUT(("p_t_l:  no sufficiently large stack on free list,"
               " allocating new stack descriptor\n"));
        new_stk = (struct dthr_stack *) malloc(sizeof *new_stk);
        if (!new_stk) {
          fprintf(stderr,
                  "dthr_thread:  top_thread:  No space for stack descriptor\n");
          abort();
        }
        /* copy and correct */
        new_stk->stack_base = dthr_topmost_stack.stack_base;
        new_stk->stack_size = new_th->stack_size;
#if DREAD_THREAD_STACK_GROWS_DOWN
        new_stk->stack_top = new_stk->stack_base - new_stk->stack_size;
#else
        new_stk->stack_top = new_stk->stack_base + new_stk->stack_size;
#endif
        new_stk->magic = DREAD_THREAD_STACK_MAGIC;

        /*
         * Turn into new thread; note that temporarily
         * a stack will reference a thread that is not
         * in the run/sema/event queues and is not the
         * current thread.
         */
        new_stk->thread = new_th;
        new_th->stack = new_stk;
        new_th->state = DREAD_THREAD_TH_RUNNABLE;
        /*
         * Ensure that dthr_this_thread will make sense
         */
        dthr_chain_push(&dthr_active_stacks,&new_stk->link);
        /*
         * Grow stack; dthr_topmost_stack is updated
         * as a side effect.
         */
        DBOUT(("p_t_l: dropping new thread queue lock\n"));
        dthr_semaphore_drop_no_yield(&dthr_newq_sema);
        DBOUT(("p_t_l: growing stack\n"));
        DBOUT(("p_t_l.save_ctxt(%p) [cont]\n",(void *) &continuation));
        if (!dthr_save_ctxt(&continuation))
          dthr_new_topmost_thread(new_stk,new_th->stack_size,1,&continuation);
        for (;;) {
          /* base, allowing for stack reuse */
          DBOUT(("p_t_l.dthr_save_ctxt(%p) [base]\n",(void *) &new_stk->base));
          (void) dthr_save_ctxt(&new_stk->base);

          /* launch the thread */
          new_th = new_stk->thread;
          dthr_cur_thread = new_th;
#if MAGIC_TEST
          if (new_th->magic != DREAD_THREAD_TH_MAGIC) {
            fprintf(stderr,
                    "dthr_thread:  thread structure corruption detected"
                    " in dthr_thread_launcher(), base launch\n");
            abort();
          }
#endif
          new_th->exit_value = (*new_th->fn)(new_th->fn_arg);

          while ((x = new_th->on_exit) != 0) {
            new_th->on_exit = x->next;
            (*x->fn)(new_th,x->arg);
            free(x);
          }

          DBOUT(("p_t_l: thread %p exited\n",(void *) dthr_cur_thread));
          /* same as dthr_thread_exit here */
          new_th->state = DREAD_THREAD_TH_EXITED;

          dthr_thread_sleep(1);
          fprintf(stderr,"dthr_thread_launcher: exited thread still runs\n");
          abort();
        }
      }
    }
    /*
     * wait for more threads to launch
     */
    DBOUT(("p_t_l: waiting for a new thread to launch\n"));
    dthr_event_wait(&dthr_newq_event,&dthr_newq_sema);
    DBOUT(("p_t_l: event wait returned\n"));
  }
}

/*
 * We force user to pass us a runnable thread directly
 * rather than just placing it on the run queue
 * in order to programmatically enforce that there must
 * be at least one runnable thread when the process
 * goes multithreaded.
 */
void  dthr_thread_multithread(struct dthr_thread  *th)
{
  DBOUT(("dthr_thread_multithread\n"));

  dthr_show_queues();

  /* no need to lock it yet, since we are only thread running */
  dthr_chain_enqueue(&dthr_newq,&th->link);
  dthr_cur_thread = &dthr_topmost_thread;

  dthr_show_queues();
  DBOUT(("dthr_thread_multithread -> launch\n"));
  if (!dthr_save_ctxt(&dthr_deadlock))
    dthr_thread_launcher();
}



/*
 * Context switch to thread th_next, doing op to current thread.
 * Interrupts are masked.
 */
static void dthr_csw(struct dthr_stack  *target, int  op)
{
  register struct dthr_stack  *this_stack;
  register int      rv;

  unsigned long     magic = DREAD_THREAD_MAGIC;

  dthr_show_queues();
  DBOUT(("dthr_csw(%p,%d)\n",(void *) target,op));
  this_stack = dthr_cur_thread->stack;
  DBOUT((" old stack = %p\n",(void *) this_stack));
  if (!this_stack) {
    fprintf(stderr,"dthr_csw:  no current stack\n");
    abort();
  }
  if (op == DREAD_THREAD_CSW_EXIT) {
    DBOUT((" leaving exited thread\n"));

    dthr_cur_thread->stack = 0;

    memset(&this_stack->regs,0,sizeof this_stack->regs);
    dthr_show_queues();
    (void) dthr_chain_delete(&this_stack->link);
    dthr_show_queues();
    dthr_chain_enqueue(&dthr_free_stacks,&this_stack->link);
    dthr_show_queues();

    dthr_cur_thread->fn = 0;
    dthr_cur_thread->fn_arg = 0;
    dthr_cur_thread->stack_size = 0;

    DBOUT(("dthr_csw.long(%p,DREAD_THREAD_CSW_NORM)\n",(void *) &target->regs));
    dthr_load_ctxt(&target->regs,DREAD_THREAD_CSW_NORM);
    fprintf(stderr,"dthr_csw:  exit load context returned\n");
    abort();
  }

  DBOUT(("dthr_csw.save_ctxt(%p)\n",(void *) &this_stack->regs));
  switch (rv = dthr_save_ctxt(&this_stack->regs)) {
  case DREAD_THREAD_CSW_CSW:
    DBOUT(("dthr_csw.load_ctxt(%p,%d)\n",(void *) &target->regs,op));
    dthr_load_ctxt(&target->regs,op);
    fprintf(stderr,"dthr_csw:  csw failed\n");
    abort();
    break;
  case DREAD_THREAD_CSW_CREATE:
    if (magic != DREAD_THREAD_MAGIC) {
      fprintf(stderr,
              "dthr_csw:  stack corruption detected, thread ID %p (tomost)\n",
              (void *) dthr_cur_thread);
      abort();
    }
    if ((dthr_cur_thread = this_stack->thread) != &dthr_topmost_thread) {
      fprintf(stderr,"dthr_csw:  create csw, am not topmost\n");
      abort();
    }
    DBOUT(("dthr_csw/ret: cur thread %p\n",(void *) dthr_cur_thread));
    return;
  case DREAD_THREAD_CSW_NORM:
    if (magic != DREAD_THREAD_MAGIC) {
      fprintf(stderr,
              "dthr_csw:  stack corruption detected, thread %p\n",
              (void *) dthr_cur_thread);
      abort();
    }
    dthr_cur_thread = this_stack->thread;
    DBOUT(("dthr_csw/ret: cur thread %p\n",(void *) dthr_cur_thread));
    return;
  default:
    fprintf(stderr,"dthr_csw:  Unexpected op code %d\n",rv);
    abort();
  }
}

/*
 * A simpler, higher level threads interface:  one function,
 * DThr_Thread_Run, which queues threads to be run before going
 * multithreaded, uses NULL pointers in arglist to go multithreaded,
 * and once multithreaded may be called to spawn additional
 * threads.  The threads are automatically garbage collected.
 *
 * This has to be inside of the abstraction barrier to permit reuse of
 * the dthr_thread data structure.  Otherwise we would declare a union...
 * We also stomp on the user data field.  We assume that that field is
 * needed by the user only for the duration of the thread function but
 * not during on-exit routines.
 */

static struct dthr_thread     *dthr_thread_q = 0;
static int                    DThr_Thread_State = 0;
static struct dthr_semaphore  to_be_freed_sema, numDThr_Threads;
static struct dthr_event      to_be_freed_ev;
static struct dthr_thread     *to_be_freed = 0;

/*
 * Things break horribly if we yield here, since the malloc'ed thread
 * descriptor could get free'd prior to actually finishing up with
 * the thread.  In particular, we do not wish the reaper thread to
 * run until we have csw'd out of the exiting thread.
 */
static void mark_of_death(struct dthr_thread *th, void *arg)
{
  DBOUT(("thread %p, going to the guillotine\n", (void *) th));
  dthr_semaphore_take_no_yield(&to_be_freed_sema);
  th->data = (void *) to_be_freed;
  to_be_freed = th;
  dthr_semaphore_drop_no_yield(&to_be_freed_sema);
  dthr_event_signal_no_yield(&to_be_freed_ev);
}

static void reaper(void)
{
  struct dthr_thread  *th;

  dthr_semaphore_take(&to_be_freed_sema);
  for (;;) {
    while ((th = to_be_freed) != 0) {
      to_be_freed = (struct dthr_thread *) th->data;
      DBOUT(("Grim reaper:  got thread %p\n",(void *) th));
      free(th);
      dthr_semaphore_drop(&numDThr_Threads);
    }
    if (!dthr_semaphore_try(&numDThr_Threads)) break; /* thread exits */
    DBOUT(("reaper event wait\n"));
    dthr_event_wait(&to_be_freed_ev,&to_be_freed_sema);
    DBOUT(("reaper event occurred\n"));
  }
  dthr_semaphore_drop(&to_be_freed_sema);
}

static void *DThr_MultiThread(void *arg)
{
  struct dthr_thread  *th;

  DBOUT(("Entered DThr_MultiThread\n"));
  while ((th = dthr_thread_q) != 0) {
    dthr_thread_q = (struct dthr_thread *) th->data;
    dthr_thread_init(th,th->fn,th->fn_arg,th->stack_size);
    if (!dthr_thread_on_exit(th,mark_of_death,0)) {
      perror("DThr_Thread");
      fprintf(stderr,
              "DThr_Thread:  startup failure while setting up on-exit fn\n");
      abort();
    }
    (void) dthr_thread_run(th);
    (void) dthr_thread_detach(th);
  }
  reaper();
  DBOUT(("Leaving DThr_MultiThread\n"));
  return 0;
}

int DThr_Thread_Run(void    *(*fn)(void *),
                    void    *fn_arg,
                    size_t  req_stack_size)
{
  struct dthr_thread  main_th, *th;

  DBOUT(("Entered DThr_Thread_Run\n"));
  switch (DThr_Thread_State) {
  case 0:
    dthr_init();
    (void) dthr_semaphore_init(&to_be_freed_sema,0);
    (void) dthr_semaphore_init(&numDThr_Threads,0);
    (void) dthr_event_init(&to_be_freed_ev);
    DThr_Thread_State = 1;
    /* fall thru */
  case 1:
    if (!fn) {
      /* begin multithreading */
      dthr_thread_init(&main_th,DThr_MultiThread,fn_arg, 0x1000);
      DThr_Thread_State = 2;
      dthr_thread_multithread(&main_th);
      return 1;
    }
    th = (struct dthr_thread *) malloc(sizeof *th);
    if (!th) return 0;
    th->fn = fn;
    th->fn_arg = fn_arg;
    th->stack_size = req_stack_size;

    th->data = (void *) dthr_thread_q;
    dthr_thread_q = th;

    dthr_semaphore_drop(&numDThr_Threads);

    DBOUT(("Leaving DThr_Thread_Run\n"));
    return 1;
  case 2:
    th = (struct dthr_thread *) malloc(sizeof *th);
    if (!th) return 0;
    th->fn = fn;
    th->fn_arg = fn_arg;
    th->stack_size = req_stack_size;

    dthr_semaphore_drop(&numDThr_Threads);

    if (!dthr_thread_on_exit(th,mark_of_death,0)) {
      perror("DThr_Thread");
      fprintf(stderr,
              "DThr_Thread:  runtime failure while setting up on-exit fn\n");
      abort();
    }
    (void) dthr_thread_run(th);
    (void) dthr_thread_detach(th);

    DBOUT(("Leaving DThr_Thread_Run\n"));
    return 1;
  }
  /* NOTREACHED -- make lint happy */
  return 1;
}
