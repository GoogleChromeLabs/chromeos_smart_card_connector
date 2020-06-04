/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_NETDB_H
#define GLIBCEMU_NETDB_H 1

/* Redirect incorrect declarations to an alternate name. */
#define getaddrinfo getaddrinfo_wrong
#define getnameinfo getnameinfo_wrong
/* Redirect h_addr to an alternate name to keep the struct correctly sized. */
#ifdef h_addr
# undef h_addr
#endif
#define h_addr h_addr_pad

#include_next <netdb.h>

/* Remove all the macros we added. */
#undef getaddrinfo
#undef getnameinfo
#undef h_addr
/* Defined h_addr as in glibc. */
#define h_addr h_addr_list[0]

__BEGIN_DECLS

void herror(const char *);
const char *hstrerror(int);

/* Declare these two correctly. */
int getaddrinfo(const char *, const char *,
                const struct addrinfo *, struct addrinfo **);
int getnameinfo(const struct sockaddr *, socklen_t, char *,
                socklen_t, char *, socklen_t, unsigned int);

__END_DECLS

#endif  /* GLIBCEMU_NETDB_H */
