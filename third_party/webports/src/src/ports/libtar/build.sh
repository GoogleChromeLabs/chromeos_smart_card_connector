# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CPPFLAGS+=" -DMAXPATHLEN=512 -DHAVE_STDARG_H"

if [ "${NACL_LIBC}" == "bionic" ]; then
  NACLPORTS_CPPFLAGS+=" -I${START_DIR}"
fi

if [ "${NACL_SHARED}" = "1" ]; then
  NACLPORTS_CFLAGS+=" -fPIC"
fi

if [ "${NACL_DEBUG}" = "1" ]; then
  NACLPORTS_CPPFLAGS+=" -DDEBUG"
fi

export compat_cv_func_snprintf_works=yes

InstallStep() {
  DefaultInstallStep
  LogExecute cp ${START_DIR}/tar.h ${NACLPORTS_INCLUDE}/
}
