#if !defined(DREAD_THREAD_CHAIN_EMPTY)
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
 *  * this software is encouraged.
 */

/*
 * User must keep track of offset from chain structure to base of actual
 * structure.  Usually the struct chain is first in the derived class, so
 * the offset is zero.
 */
struct dthr_chain {
  struct dthr_chain  *prev,*next;
};

#define DREAD_THREAD_CHAIN_EMPTY(chainp)   ((chainp)->next == (chainp))

struct dthr_chain  *dthr_chain_init(struct dthr_chain  *anchor);
struct dthr_chain  *dthr_chain_insert_after(struct dthr_chain  *link,
                                            struct dthr_chain  *new_link);
struct dthr_chain  *dthr_chain_insert_before(struct dthr_chain *link,
                                             struct dthr_chain *new_link);
struct dthr_chain  *dthr_chain_delete(struct dthr_chain  *link);

#define dthr_chain_insert(anchor,link) dthr_chain_insert_after(anchor,link)
#define dthr_chain_push(anchor,link)   dthr_chain_insert_after(anchor,link)
#define dthr_chain_enqueue(anchor,link)  dthr_chain_insert_before(anchor,link)
#define DREAD_THREAD_CHAIN_DEQUEUE(anchor)   \
  (DREAD_THREAD_CHAIN_EMPTY(anchor) ?                                 \
   (struct dthr_chain *) 0 :                                            \
   dthr_chain_delete((anchor)->next))
#include <stdio.h>
void  dthr_chain_show(FILE              *iop,
                      struct dthr_chain *anchor);

#if defined(__GNUC__)
# if  defined(DREAD_THREAD_CHAIN_REAL_CODE)
#  define DREAD_THREAD_CHAIN_EXTINLINE
# else
#  define DREAD_THREAD_CHAIN_EXTINLINE   extern inline
#  define DREAD_THREAD_CHAIN_REAL_CODE   0
   /*
    * For inlined functions.  For make, this also means that anything that
    * depends on chain.h will also depend on chain.c, but that's not easily
    * expressible (if at all) in most make languages.
    */
#  include "dread_chain.c"
# endif
#else
# define  DREAD_THREAD_CHAIN_EXTINLINE
#endif

/*
 * chain loops:
 *
 * for (p = anchor.next; p != &anchor; p = p->next) {
 *  ...
 * }
 */
#endif
