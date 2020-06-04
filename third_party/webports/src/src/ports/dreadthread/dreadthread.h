#if !defined(DREAD_THREAD_STACK_EXTRA)
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
 * Portable C-only coroutine threads.
 */

#include "dreadthread_chain.h"
#include "dreadthread_ctxt.h"

/* select(2) req'mt, and caddr_t */
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#define DREAD_THREAD_MAGIC    0x31415926ul
  /* for stack corruption test dthr_csw */
#define DREAD_THREAD_MAGIC2   0x27182818ul

#define DREAD_THREAD_STACK_GROWS_DOWN 1
#define DREAD_THREAD_STACK_EXTRA  (6*1024)
/*
 * see stack_est.c -- it can be used to determine pico's overhead by first
 * providing a very generous value here, building stack_est, and see what
 * it outputs.  Beware of limit stacksize / ulimit -s.
 */

/*
 * Private data.
 *
 * Describes stack allocation.
 * Stacks are never fragmented.  First fit.
 *
 * Each thread has signal state determining which signals are blocked.
 * This information is not available here -- list of blocked signals
 * are implicitly stored on the stack as part of the pico thread code
 * execution state.
 *
 * Signal handlers are still global.
 */
struct dthr_stack {
  struct dthr_chain   link;     /* free/active lists */
  unsigned long       magic;
#define DREAD_THREAD_STACK_MAGIC  0x62737920ul
  size_t              stack_size;
  caddr_t             stack_base,   /* approximate */
                      stack_top;
  struct dthr_thread  *thread;
  dthr_ctxt_t         regs,     /* user thread regs */
                      base;     /* stack reuse */
};

struct dthr_semaphore {
  unsigned long     magic;
#define DREAD_THREAD_SEMA_MAGIC   0x68657265ul
  int               value;      /* binary for lock */
  struct dthr_chain threadq;
};

struct dthr_event {
  unsigned long     magic;
#define DREAD_THREAD_EV_MAGIC   0x83651fc2ul
  struct dthr_chain threadq;
};

struct dthr_thread_exit {
  struct dthr_thread_exit *next;
  void                    (*fn)(struct dthr_thread *,void *);
  void                    *arg;
};

struct dthr_thread {
  /* Private stuff */
  struct dthr_chain       link; /* runq, runq_sema, runq_event */
  unsigned long           magic;
#define DREAD_THREAD_TH_MAGIC   0xe32f8c27ul
  void                    *(*fn)(void *);
  void                    *fn_arg;
  int                     stack_size;
  int                     state;
#define   DREAD_THREAD_TH_EXITED    -1  /* stack undefined */
#define   DREAD_THREAD_TH_RUNNABLE  0
#define   DREAD_THREAD_TH_SEMA_WAIT 1
#define   DREAD_THREAD_TH_EVENT_WAIT  2
  struct dthr_stack       *stack;
  struct dthr_semaphore   exit_sema;
  struct dthr_thread_exit *on_exit;
  jmp_buf                 io_timeout;
  /* Public stuff */
  void                    *exit_value;
  void                    *data;
  /*
   * For implementing mailboxes etc --
   * NOT avaiable for use during on-exit processing
   * User may also use an extended dthr_thread structure
   * with more data at the end to hold more per-thread
   * info; if this is made into a DLL, the magic number
   * will be used for version control to avoid stepping
   * on any such user data.
   */
};

extern struct dthr_thread *dthr_cur_thread;
struct dthr_thread *dthr_this_thread(void);
/* signal handlers? who needs it? */
void  dthr_thread_yield(void);
struct dthr_semaphore *dthr_semaphore_init(struct dthr_semaphore  *sema,
                                           int                    init);
int dthr_semaphore_try(struct dthr_semaphore  *sema);
void  dthr_semaphore_take(struct dthr_semaphore *sema);
void  dthr_semaphore_drop(struct dthr_semaphore *sema);

void  dthr_event_wait(struct dthr_event     *event,
                      struct dthr_semaphore *lock);
void  dthr_event_broadcast_no_yield(struct dthr_event *event);
void  dthr_event_broadcast(struct dthr_event *event);
void  dthr_event_signal_no_yield(struct dthr_event *event);
void  dthr_event_signal(struct dthr_event *event);

struct dthr_event *dthr_event_init(struct dthr_event *ev);

void  dthr_init(void);
void  dthr_thread_exit(void *status);
struct dthr_thread  *dthr_thread_init(struct dthr_thread  *th,
                                      void                *(*fn)(void *),
                                      void                *fn_arg,
                                      size_t              requested_stack_size);
int dthr_thread_on_exit(struct dthr_thread  *th,
                        void                (*fn)(struct dthr_thread *,void *),
                        void                *arg);

struct dthr_thread  *dthr_thread_run(struct dthr_thread *th);
struct dthr_thread  *dthr_thread_wait(struct dthr_thread *th);
struct dthr_thread  *dthr_thread_detach(struct dthr_thread *th);

void  dthr_thread_multithread(struct dthr_thread  *th);

int DThr_Thread_Run(void    *(*fn)(void *),
                    void    *fn_arg,
                    size_t  req_stack_size);

extern int  (*dthr_on_deadlock)();

#endif
