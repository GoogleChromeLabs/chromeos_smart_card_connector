#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dreadthread.h"

#define NTHREADS  1000
#define STACKSIZE (8 * 1024)
#define COUNT     100

int                   count = COUNT;
int                   nthreads = NTHREADS;
int                   stacksize = STACKSIZE;

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

  info = (struct thread_specifics *) arg;
  th_num = info->thread_num;

  v = info->val;
  while (info->counter < count) {
    dthr_thread_yield();
    printf("Thread %d: count %d, val-1.0=%g\n",th_num,info->counter,v-1.0);
    v = sqrt(v);
    info->counter++;
  }
  return 0;
}

struct thread_specifics *priv;
char                    *me;

void usage()
{
  fprintf(stderr,
          "Usage: %s [-aA] [-s stackbytes] [-c count] [-t nthreads]\n",
          me);
}

int main(int ac, char **av)
{
  int   opt;
  int   i;

  if (!(me = strrchr(*av,'/'))) me = *av;
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
    priv[i].thread_num = i;
    priv[i].counter = 0;
    priv[i].val = 100.0 * i;

    (void) DThr_Thread_Run(myThread,(void *) &priv[i],stacksize);
  }

  printf("main: DThr_Thread_Run(0,0,0) ***\n");

  (void) DThr_Thread_Run(0,0,0);
  fprintf(stderr,"All threads exited, all done!\n");
  return 0;
}
