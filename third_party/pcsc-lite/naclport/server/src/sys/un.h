/* Copyright (C) 1991-2014 Free Software Foundation, Inc.
   Copyright (C) 2016 Google Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

// The <sys/un.h> file is missing from PNaCl, so the definitions necessary for
// pcsc-lite compilation are provided here.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SYS_UN_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SYS_UN_H_

/* Structure describing the address of an AF_LOCAL (aka AF_UNIX) socket.  */
struct sockaddr_un {
  unsigned short int sun_family;
  char sun_path[108]; /* Path name.  */
};

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SYS_UN_H_
