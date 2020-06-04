/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains macros from BSD for converting little-endian
 * or big-endian to host-endian and back.  Although these are not
 * part of glibc some software assumes that they exist when __GLIBC__
 * is *not* defined so they allow such software to build with newlib.
 */

#ifndef GLIBCEMU_SYS_ENDIAN_H
#define GLIBCEMU_SYS_ENDIAN_H 1

#include <machine/endian.h>

#define __byte_swap_constant_16(x) \
    ( (((unsigned short)(x) & 0xff) << 8) | \
       ((unsigned short)(x) >> 8) )

#define __byte_swap_constant_32(x) \
    ( (((unsigned int)(x) & 0xff) << 24) | \
      (((unsigned int)(x) << 8) & 0x00ff0000) | \
      (((unsigned int)(x) >> 8) & 0x0000ff00) | \
       ((unsigned int)(x) >> 24) )


_ELIDABLE_INLINE unsigned short int
__inline_bswap_16 (unsigned short int x)
{
    return __byte_swap_constant_16(x);
}

#define __byte_swap_16(x) \
  (__builtin_constant_p(x) ? __byte_swap_constant_16(x) : __inline_bswap_16(x))

#define __byte_swap_32(x) \
  (__builtin_constant_p(x) ? __byte_swap_constant_32(x) : __builtin_bswap32(x))


#if BYTE_ORDER == LITTLE_ENDIAN
#define htole16(x) (x)
#define htole32(x) (x)
#define letoh16(x) (x)
#define letoh32(x) (x)
#define htobe16(x) (__byte_swap_16(x))
#define htobe32(x) (__byte_swap_32(x))
#define betoh16(x) (__byte_swap_16(x))
#define betoh32(x) (__byte_swap_32(x))
#elif BYTE_ORDER == LITTLE_ENDIAN
#else
/* Native client is defined as always little endian */
#error "Expected BYTE_ORDER == LITTLE_ENDIAN... "
#endif

#endif /* GLIBCEMU_SYS_ENDIAN_H */
