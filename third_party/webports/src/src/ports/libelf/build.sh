# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INSTALL_TARGETS="install instroot=${DESTDIR}"

if [[ ${NACL_ARCH} == pnacl ]]; then
  export libelf_cv_int32=int
  export libelf_cv_int16=short
fi
