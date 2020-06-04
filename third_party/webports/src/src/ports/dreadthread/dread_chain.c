/*
 * Copyright (c) 1994 by Bennet Yee.
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
 * University, nor of any adaptation thereof in any advertising,
 * promotional, or sales literature without prior written consent from the
 * author and Carnegie-Mellon University in each case.
 *
 * If you have questions, you can contact the author at bsy@cs.cmu.edu
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

/* doubly linked object chain */

#if !defined(DREAD_THREAD_CHAIN_REAL_CODE)
# define  DREAD_THREAD_CHAIN_REAL_CODE   1
#endif
#if !defined(DREAD_THREAD_CHAIN_EXTINLINE)
# include "dreadthread_chain.h"
#endif

#include <stdio.h>

#define DREAD_THREAD_CHAIN_DEBUG 0
#if DREAD_THREAD_CHAIN_DEBUG
# define  DREAD_THREAD_CHAIN_DBOUT(args) do { \
    printf args; \
    fflush(stdout); \
  } while (0)
#else
# define  DREAD_THREAD_CHAIN_DBOUT(args)
#endif

DREAD_THREAD_CHAIN_EXTINLINE
int dthr_chain_empty(struct dthr_chain  *anchor)
{
  DREAD_THREAD_CHAIN_DBOUT(("dthr_chain_empty(0x%08lx) -> %d\n",
               (long) anchor,anchor->prev==anchor->next));
  return anchor->next == anchor;
}

DREAD_THREAD_CHAIN_EXTINLINE
struct dthr_chain  *dthr_chain_delete(struct dthr_chain  *link)
{
  DREAD_THREAD_CHAIN_DBOUT(("dthr_chain_delete(0x%08lx)\n",(long) link));
  link->prev->next = link->next;
  link->next->prev = link->prev;

  link->prev = link->next = link; /* not necy */
  return link;
}

DREAD_THREAD_CHAIN_EXTINLINE
struct dthr_chain  *dthr_chain_dequeue(struct dthr_chain *anchor)
{
  struct dthr_chain  *rv;

  rv = DREAD_THREAD_CHAIN_EMPTY(anchor) ?
      (struct dthr_chain *) 0 : dthr_chain_delete(anchor->next);
  DREAD_THREAD_CHAIN_DBOUT(("dthr_chain_dequeue(0x%08lx) -> 0x%08lx\n",
                            anchor,rv));
  return rv;
}

DREAD_THREAD_CHAIN_EXTINLINE
struct dthr_chain  *dthr_chain_init(struct dthr_chain  *anchor)
{
  anchor->prev = anchor->next = anchor;
  return anchor;
}

DREAD_THREAD_CHAIN_EXTINLINE
struct dthr_chain  *dthr_chain_insert_after(struct dthr_chain  *link,
                                            struct dthr_chain  *new_link)
{
  DREAD_THREAD_CHAIN_DBOUT(("dthr_chain_insert_after(0x%08lx,0x%08lx)\n",
                            (long) link,(long) new_link));
  new_link->next = link->next;
  new_link->prev = link;
  link->next->prev = new_link;
  link->next = new_link;

  return link;
}

DREAD_THREAD_CHAIN_EXTINLINE
struct dthr_chain  *dthr_chain_insert_before(struct dthr_chain *link,
                                             struct dthr_chain *new_link)
{
  DREAD_THREAD_CHAIN_DBOUT(("dthr_chain_insert_before(0x%08lx,0x%08lx)\n",
                            (long) link,(long) new_link));
  new_link->prev = link->prev;
  new_link->next = link;
  link->prev->next = new_link;
  link->prev = new_link;

  return link;
}

#if /* CHAIN_DEBUG && */ CHAIN_REAL_CODE
void  dthr_chain_show(FILE              *iop,
                      struct dthr_chain *anchor)
{
  struct dthr_chain  *p;

  fprintf(iop,"Anchor(0x%08lx)\n",(long) anchor);
  for (p = anchor->next; p != anchor; p = p->next)
    fprintf(iop,"       0x%08lx\n",(long) p);
    /*      Anchor( */
  fprintf(iop,"   End(0x%08lx)\n",(long) anchor);
  fprintf(iop,"REV Anchor(0x%08lx)\n",(long) anchor);
  for (p = anchor->prev; p != anchor; p = p->prev)
    fprintf(iop,"           0x%08lx\n",(long) p);
    /*      Anchor( */
  fprintf(iop,"REV    End(0x%08lx)\n",(long) anchor);
}
#endif
