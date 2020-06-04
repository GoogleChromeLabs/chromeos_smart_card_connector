#!/bin/bash
# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

is_host=0

for x in $*; do
  if [ ${x} = -D_SAMBA_HOSTCC_ ]; then
    is_host=1
  fi
  if [[ ${x} = *asn1/gen_template_132.o ]]; then
    is_host=1
  fi
  if [[ ${x} = */compile_et_134.o ]]; then
    is_host=1
  fi
done


if [ ${is_host} = 1 ]; then
  shift
  gcc $*
else
  $*
fi
