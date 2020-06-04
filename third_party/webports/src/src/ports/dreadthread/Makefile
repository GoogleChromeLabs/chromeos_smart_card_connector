CC=gcc
CFLAGS=-g -Wall -Dposix_signals
SRCS=dread.c dread_chain.c
OBJS=$(SRCS:c=o)
HDRS=dreadthread.h dreadthread_ctxt.h dreadthread_chain.h
TEST_PROGS=test test2 stack_est test4
# test3 used pico_select, not avail in NaCl
TEST_PROG_OBJS=$(TEST_PROGS:%=%.o)

all:	libdreadthread.a

test_progs:	$(TEST_PROGS)

$(TEST_PROG_OBJS):	$(HDRS)

install:	libdreadthread.a $(HDRS)
	rm -f $(LIBDIR)/libdreadthread.a
	cp -p libdreadthread.a $(LIBDIR)/libdreadthread.a
	ranlib $(LIBDIR)/libdreadthread.a
	for i in $(HDRS); do bn=`basename $$i`; rm -f $(INCDIR)/$$bn; cp -p $$i $(INCDIR)/$$bn; done

$(OBJS):		$(HDRS)

libdreadthread.a:	$(OBJS) $(MD_OBJS)
	ar ru libdreadthread.a $(OBJS) $(MD_OBJS)
	ranlib libdreadthread.a

%:	%.o	libdreadthread.a
	$(CC) $(LDFLAGS) $(CFLAGS) -o $* $*.o libdreadthread.a -lm $(LIBES)

clean:
	rm -f *.o stack_est test test2 test3 test4 libdreadthread.a *~ core
