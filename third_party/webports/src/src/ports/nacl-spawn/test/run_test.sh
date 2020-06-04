#!/bin/sh
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -eu

cd $(dirname $0)

# Compares the first string argument with the result of a command
# specified by the second argument. All line breaks in the command
# output will be replaced by whitespaces.
AssertCommandResultEquals() {
  local expected="$1"
  local command="$2"
  echo "[ RUN      ] $command"
  local actual="$(python ${NACL_SDK_ROOT}/tools/sel_ldr.py ${command})"
  if [ "x${expected}" != "x${actual}" ]; then
    echo "command failed: $command"
    echo "  expected=$expected"
    echo "  actual=$actual"
    exit 1
  fi
  echo "[  PASSED  ]"
}

# test_exe depends on libtest1.so and libtest2.so.
# libtest1.so depends on libtest3.so.
# libtest2.so also depends on libtest3.so.
# libtest3.so depends on nothing.

AssertCommandResultEquals "" "./elf_reader libtest3.so"
AssertCommandResultEquals "libtest3.so" "./elf_reader libtest2.so"
AssertCommandResultEquals "libtest1.so libtest2.so" "./elf_reader test_exe"
AssertCommandResultEquals "libtest3.so" "./library_dependencies libtest3.so"
AssertCommandResultEquals "./libtest3.so libtest1.so" \
    "./library_dependencies libtest1.so"
AssertCommandResultEquals \
    "./libtest1.so ./libtest2.so ./libtest3.so test_exe" \
    "./library_dependencies test_exe"
