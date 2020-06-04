/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _LIBC_SYMBOLS_H
#define _LIBC_SYMBOLS_H 1

#define libc_hidden_def(x)
#define libc_hidden_weak(x)
#define libc_hidden_proto(x)
#define libc_hidden_data_def(x)
#define libresolv_hidden_def(x)
#define libresolv_hidden_weak(x)
#define libresolv_hidden_data_def(x)
#define libresolv_hidden_proto(x)
#define static_link_warning(x)
#define libc_freeres_ptr(x) x
#define DL_CALL_FCT(x,y) x y
#define attribute_hidden
#define internal_function
#define __set_errno(x) (errno = (x))
#define weak_alias(name, aliasname) _weak_alias (name, aliasname)
#define _weak_alias(name, aliasname)                                  \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

#endif
