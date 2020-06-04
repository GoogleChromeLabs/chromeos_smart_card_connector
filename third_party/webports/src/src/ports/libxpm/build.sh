# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The transtive dependencies of libxpm include nacl_io which is
# written in C++. Without this sxpm binary fails to link.
if [ "${NACL_SHARED}" != "1" ]; then
  NACLPORTS_LIBS+=" -l${NACL_CXX_LIB} -pthread"
fi
