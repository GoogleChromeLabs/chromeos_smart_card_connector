# Generated automatically from Makefile.in by configure.
# Makefile.in for thttpd
#
# Copyright © 1995,1998 by Jef Poskanzer <jef@mail.acme.com>.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

# Various configurable paths (remember to edit Makefile.in, not Makefile)

# Top level hierarchy.
prefix = /usr/local
exec_prefix = ${prefix}
# Pathname of directory to install the binary.
BINDIR = ${exec_prefix}/sbin
# Pathname of directory to install the man page.
MANDIR = ${prefix}/man
# Pathname of directory to install the CGI programs.
WEBDIR = $(prefix)/www

# CONFIGURE: The group that the web directory belongs to.  This is so that
# the makeweb program can be installed set-group-id to that group, and make
# subdirectories.  If you're not going to use makeweb, ignore this.
WEBGROUP =	www

# CONFIGURE: Directory for CGI executables.
CGIBINDIR =	$(WEBDIR)/cgi-bin

# You shouldn't need to edit anything below here.

CC =		$(NACLCC)
CXX =		$(NACLCXX)
CCOPT =		-O0
DEFS =		-DHAVE_DAEMON=1 -DHAVE__PROGNAME=1 -DHAVE_GRP_H=1 -DHAVE_MEMORY_H=1 -DHAVE_PATHS_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_DIRENT_H=1 -DHAVE_LIBCRYPT=1 -DHAVE_STRERROR=1 -DHAVE_VSNPRINTF=1  -DHAVE_SETSID=1 -DHAVE_GETADDRINFO=1 -DHAVE_GETNAMEINFO=1 -DHAVE_GAI_STRERROR=1 -DHAVE_SIGSET=1 -DHAVE_ATOLL=1 -DHAVE_UNISTD_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_SELECT=1 -DHAVE_TM_GMTOFF=1 -DHAVE_INT64T=1 -DHAVE_SOCKLENT=1 
INCLS =		-I. $(NACLPORTS_CPPFLAGS)
CFLAGS =	$(CCOPT) $(DEFS) $(NACLPORTS_CFLAGS) $(INCLS)
LDFLAGS =	-lcrypt -ljsoncpp ${NACL_CLI_MAIN_LIB} $(NACLPORTS_LDFLAGS) -pthread
LIBS =
NETLIBS =	
INSTALL =	/usr/bin/install -c

.c.o:
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

.cpp.o:
	@rm -f $@
	$(CXX) $(CFLAGS) -c $*.cpp

SRC =		thttpd.c libhttpd.c fdwatch.c mmc.c timers.c match.c tdate_parse.c
SRCP =		syslog.cpp

OBJ =		$(SRC:.c=.o) 
OBJP =		$(SRCP:.cpp=.o) 

ALL =		thttpd

GENHDR =	mime_encodings.h mime_types.h

CLEANFILES =	$(ALL) $(OBJP) $(OBJ) $(GENSRC) $(GENHDR)

all:		this
this:		$(ALL)

thttpd: $(OBJ) $(OBJP)
	@rm -f $@
	$(CXX) $(CFLAGS) -o $@ $(OBJ) $(OBJP) $(LDFLAGS) $(LIBS) $(NETLIBS)

mime_encodings.h:	mime_encodings.txt
	rm -f mime_encodings.h
	sed < mime_encodings.txt > mime_encodings.h \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'

mime_types.h:	mime_types.txt
	rm -f mime_types.h
	sed < mime_types.txt > mime_types.h \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'


install:	installthis install-man

installthis:
	-mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 555 -o bin -g bin thttpd $(DESTDIR)$(BINDIR)

install-man:
	-mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 444 -o bin -g bin thttpd.8 $(DESTDIR)$(MANDIR)/man8


clean:
	rm -f $(CLEANFILES)

distclean:
	rm -f $(CLEANFILES) Makefile config.cache config.log config.status tags

tags:
	ctags -wtd *.c *.h

tar:
	@name=`sed -n -e '/SERVER_SOFTWARE/!d' -e 's,.*thttpd/,thttpd-,' -e 's, .*,,p' version.h` ; \
	  rm -rf $$name ; \
	  mkdir $$name ; \
	  tar cf - `cat FILES` | ( cd $$name ; tar xfBp - ) ; \
	  chmod 644 $$name/Makefile.in $$name/config.h $$name/mime_encodings.txt $$name/mime_types.txt ; \
	  chmod 755 $$name/cgi-bin $$name/cgi-src $$name/contrib $$name/contrib/redhat-rpm $$name/extras $$name/scripts ; \
	  tar cf $$name.tar $$name ; \
	  rm -rf $$name ; \
	  gzip $$name.tar

thttpd.o:	config.h version.h libhttpd.h fdwatch.h mmc.h timers.h match.h \
            my_syslog.h
libhttpd.o:	config.h version.h libhttpd.h mime_encodings.h mime_types.h \
		mmc.h timers.h match.h tdate_parse.h my_syslog.h
syslog.o: my_syslog.h
fdwatch.o:	fdwatch.h
mmc.o:		mmc.h libhttpd.h my_syslog.h
timers.o:	timers.h
match.o:	match.h
tdate_parse.o:	tdate_parse.h
