#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dreadthread.h"

#define NTHREADS  1000
#define STACKSIZE (8 * 1024)
#define COUNT     100

int                   count = COUNT;
int                   nthreads = NTHREADS;
size_t                stacksize = STACKSIZE;

int                   all_at_once = 0;
int                   all_go = 0;
struct dthr_event     go_eva;
struct dthr_semaphore go_sema;

struct thread_specifics {
  int     thread_num;
  int     counter;
  double  val;
};

void *myThread(void *arg)
{
  int                     index = (uintptr_t) arg;
  struct thread_specifics *info;
  int                     th_num;
  double                  v;

  if (all_at_once) {
    dthr_semaphore_take(&go_sema);
    while (!all_go) {
      dthr_event_wait(&go_eva,&go_sema);
    }
    dthr_semaphore_drop(&go_sema);
  }

  info = (struct thread_specifics *) dthr_cur_thread->data;
  th_num = info->thread_num;
  if (index != th_num) {
    fprintf(stderr,"Thread %d: arg %d != thread data\n",th_num,index);
  }
  v = info->val;
  while (info->counter < count) {
    dthr_thread_yield();
    printf("Thread 0x%lx, 0x%lx\n",
           (unsigned long) (uintptr_t) dthr_cur_thread,
           (unsigned long) (uintptr_t) dthr_this_thread());
    printf("Thread %d: count %d, val-1.0=%g\n",th_num,info->counter,v-1.0);
    v = sqrt(v);
    info->counter++;
  }
  return 0;
}

struct dthr_thread  *th, main_th;
struct thread_specifics *priv;

void *goForIt()
{
  int i;

  if (all_at_once) {
    dthr_semaphore_init(&go_sema,1);
    dthr_event_init(&go_eva);
  }
  for (i = 0; i < nthreads; i++)
    (void) dthr_thread_detach(dthr_thread_run(&th[i]));
  (void) dthr_thread_detach(dthr_cur_thread);
  if (all_at_once) {
    dthr_semaphore_take(&go_sema);
    all_go = 1;
    dthr_semaphore_drop(&go_sema);
    dthr_event_broadcast(&go_eva);
  }
  return 0;
}

char    *me;

void usage()
{
  fprintf(stderr,
          "Usage: %s [-aA] [-s stackbytes] [-c count] [-t nthreads]\n",
          me);
}

int main(int ac, char **av)
{
  int opt;
  int i;

  if (!(me = strrchr(*av,'/'))) me = *av;
  else ++me;

  while ((opt = getopt(ac,av,"aAc:s:t:")) != EOF) {
    switch (opt) {
      case 'a': all_at_once = 1;
        break;
      case 'A': all_at_once = 0;
        break;
      case 'c': count = atoi(optarg);
        break;
      case 's': stacksize = strtoull(optarg, (char **) 0, 0);
        break;
      case 't': nthreads = atoi(optarg);
        break;
      default:  usage(); exit(1);
    }
  }

  th = (struct dthr_thread *) malloc(nthreads * sizeof *th);
  if (!th) {
    perror(me);
    fprintf(stderr,"%s:  insufficient space for thread descriptors\n",me);
    exit(1);
  }
  priv = (struct thread_specifics *) malloc(nthreads * sizeof *priv);
  if (!priv) {
    perror(me);
    fprintf(stderr,"%s:  insufficient space for thread private data\n",me);
    exit(1);
  }

  dthr_init();

  for (i = 0; i < nthreads; i++) {
    (void) dthr_thread_init(&th[i],myThread,(void *) (uintptr_t) i,stacksize);

    th[i].data = (void *) &priv[i];
    priv[i].thread_num = i;
    priv[i].counter = 0;
    priv[i].val = 100.0 * i;
  }

  printf("main: dthr_thread_init (main thread)\n");
  dthr_thread_init(&main_th, goForIt, (void *) 0, stacksize);
  printf("main: dthr_thread_multithread ***\n");

  dthr_thread_multithread(&main_th);
  fprintf(stderr,"All threads exited, all done!\n");
  return 0;
}
