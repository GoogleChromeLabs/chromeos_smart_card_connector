/*
 * Copyright (c) 1985, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static const char sccsid[] = "@(#)res_mkquery.c	8.1 (Berkeley) 6/4/93";
static const char rcsid[] = "$BINDId: res_mkquery.c,v 8.12 1999/10/13 16:39:40 vixie Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "local.h"

/* Options.  Leave them on. */
/* #define DEBUG */

#ifdef _LIBC
# include <hp-timing.h>
# if HP_TIMING_AVAIL
#  define RANDOM_BITS(Var) { uint64_t v64; HP_TIMING_NOW (v64); Var = v64; }
# endif
#endif

/*
 * Form all types of queries.
 * Returns the size of the result or -1.
 */
int
res_nmkquery(res_state statp,
	     int op,			/* opcode of query */
	     const char *dname,		/* domain name */
	     int class, int type,	/* class and type of query */
	     const u_char *data,	/* resource record data */
	     int datalen,		/* length of data */
	     const u_char *newrr_in,	/* new rr for modify or append */
	     u_char *buf,		/* buffer to put query */
	     int buflen)		/* size of buffer */
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	u_char *dnptrs[20], **dpp, **lastdnptr;

#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf(";; res_nmkquery(%s, %s, %s, %s)\n",
		       _res_opcodes[op], dname, p_class(class), p_type(type));
#endif
	/*
	 * Initialize header fields.
	 */
	if ((buf == NULL) || (buflen < HFIXEDSZ))
		return (-1);
	memset(buf, 0, HFIXEDSZ);
	hp = (HEADER *) buf;
	/* We randomize the IDs every time.  The old code just
	   incremented by one after the initial randomization which
	   still predictable if the application does multiple
	   requests.  */
#if 0
	hp->id = htons(++statp->id);
#else
	hp->id = htons(statp->id);
	int randombits;
	do
	  {
#ifdef RANDOM_BITS
	    RANDOM_BITS (randombits);
#else
	    struct timeval tv;
	    gettimeofday (&tv, NULL);
	    randombits = (tv.tv_sec << 8) ^ tv.tv_usec;
#endif
	  }
	while ((randombits & 0xffff) == 0);
	statp->id = (statp->id + randombits) & 0xffff;
#endif
	hp->opcode = op;
	hp->rd = (statp->options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	cp = buf + HFIXEDSZ;
	buflen -= HFIXEDSZ;
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof dnptrs / sizeof dnptrs[0];
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case QUERY:	/*FALLTHROUGH*/
	case NS_NOTIFY_OP:
		if ((buflen -= QFIXEDSZ) < 0)
			return (-1);
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		__putshort(type, cp);
		cp += INT16SZ;
		__putshort(class, cp);
		cp += INT16SZ;
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		n = dn_comp((char *)data, cp, buflen, dnptrs, lastdnptr);
		if (n < 0)
			return (-1);
		cp += n;
		buflen -= n;
		__putshort(T_NULL, cp);
		cp += INT16SZ;
		__putshort(class, cp);
		cp += INT16SZ;
		__putlong(0, cp);
		cp += INT32SZ;
		__putshort(0, cp);
		cp += INT16SZ;
		hp->arcount = htons(1);
		break;

	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (buflen < 1 + RRFIXEDSZ + datalen)
			return (-1);
		*cp++ = '\0';	/* no domain name */
		__putshort(type, cp);
		cp += INT16SZ;
		__putshort(class, cp);
		cp += INT16SZ;
		__putlong(0, cp);
		cp += INT32SZ;
		__putshort(datalen, cp);
		cp += INT16SZ;
		if (datalen) {
			memcpy(cp, data, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

	default:
		return (-1);
	}
	return (cp - buf);
}
libresolv_hidden_def (res_nmkquery)
