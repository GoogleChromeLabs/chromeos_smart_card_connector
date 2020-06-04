#include <stdio.h>
#include <stdlib.h>
#include "dreadthread.h"

#define NTHREADS  10
#define STACKSIZE (32 * 1024)

#define MAGIC   0x8e7646c3ul

int nthreads = NTHREADS;
int stacksize = STACKSIZE;
int count = NTHREADS * 4;

int     all_at_once = 0;
int     all_go = 0;
struct dthr_event go_eva;
struct dthr_semaphore go_sema;

struct thread_specifics {
  unsigned long stack_use;
  int           id;
};

void myThread_work(void  *arg)
{
  unsigned int            barrier[10];
  struct thread_specifics *info;
  int                     i;

  for (i = 0; i < sizeof barrier/sizeof barrier[0]; i++)
    barrier[i] = MAGIC + i;
  if (all_at_once) {
    dthr_semaphore_take(&go_sema);
    while (!all_go) {
      dthr_event_wait(&go_eva,&go_sema);
      dthr_semaphore_take(&go_sema);
    }
    dthr_semaphore_drop(&go_sema);
  }

  info = (struct thread_specifics *) arg;
  printf(" Hello world from thread %d, info %p\n",info->id,info);
  for (i = count; --i >= 0; ) {
    dthr_thread_yield();
    printf(" %d",i); fflush(stdout);
  }
  for (i = 0; i < sizeof barrier/sizeof barrier[0]; i++) {
    if (barrier[i] != MAGIC + i) {
      fprintf(stderr,
              "BARRIER BREACHED!"
              "  Increase stack_est's per-thread stack size!\n");
      /* following may fail if info ptr is trashed */
      fprintf(stderr,"Thread %d\n",info->id);
      exit(1);
    }
  }
  putchar('\n');
}

void  mark_stack(void   *arg)
{
  register int  i;
  char          stack[1];
  /* stack is the only on-stack automatic */

  printf("mark_stack: %p\n",(void *) stack);
#if DREAD_THREAD_STACK_GROWS_DOWN
  for (i = stacksize/2; --i >= 0; )
    stack[-i] = i;
#else
  for (i = stacksize/2; --i >= 0; )
    stack[i] = i;
#endif
}

void  examine_stack(void  *arg)
{
  register int        i;
  register struct thread_specifics  *info = (struct thread_specifics *) arg;
  char          stack[1];

  printf("examine_stack: %p\n",(void *) stack);
#if DREAD_THREAD_STACK_GROWS_DOWN
  for (i = stacksize/2; --i >= 0; ) {
    if (stack[-i] != (char) i) {
      info->stack_use = i;
      return;
    }
  }
  info->stack_use = 0;
#else
  for (i = stacksize/2; --i >= 0; ) {
    if (stack[i] != (char) i) {
      info->stack_use = i;
      return;
    }
  }
  info->stack_use = 0;
#endif
}

void *myThread(void *arg)
{
  mark_stack(arg);  /* same number of params */
  myThread_work(arg);
  examine_stack(arg); /* same number of params */
  return 0;
}

struct thread_specifics *priv;

extern int  getopt();
extern char *optarg, *rindex();

char    *me;

void usage()
{
  fprintf(stderr,
          "Usage: %s [-aA] [-s stackbytes] [-t nthreads] [-c count]\n",
          me);
}

int main(int ac, char **av)
{
  int   opt;
  int   i;
  unsigned long offset;

  if (!(me = rindex(*av,'/'))) me = *av;
  else ++me;

  while ((opt = getopt(ac,av,"aAc:s:t:")) != EOF) switch (opt) {
  case 'a': all_at_once = 1;    break;
  case 'A': all_at_once = 0;    break;
  case 'c': count = atoi(optarg);   break;
  case 's': stacksize = atoi(optarg); break;
  case 't': nthreads = atoi(optarg);  break;
  default:  usage(); exit(1);
  }

  priv = (struct thread_specifics *) malloc(nthreads * sizeof *priv);
  if (!priv) {
    perror(me);
    fprintf(stderr,"%s:  insufficient space for thread private data\n", me);
    exit(1);
  }

  for (i = 0; i < nthreads; i++) {
    priv[i].id = i;
    (void) DThr_Thread_Run(myThread,(void *) &priv[i],stacksize);
  }

  printf("main: DThr_Thread_Run(0,0,0) ***\n");

  (void) DThr_Thread_Run(0,0,0);
  fprintf(stderr,"All threads exited!\n");

  for (i = 0; i < nthreads; i++) {
    offset = priv[i].stack_use;
    printf("%d: %lu 0x%08lx\n",i,offset,offset);
  }
  return 0;
}
