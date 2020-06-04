# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="
  src/tools/oggz${NACL_EXEEXT}
  src/tests/read-stop-ok${NACL_EXEEXT}
  src/examples/read-file${NACL_EXEEXT}
"

TESTS="
comment-test
io-count
io-read
io-read-single
io-run
io-seek
io-write-flush
io-write
read-generated
read-stop-err
read-stop-ok
write-bad-bos
write-bad-bytes
write-bad-eos
write-bad-granulepos
write-bad-guard
write-bad-packetno
write-bad-serialno
write-dup-bos
write-prefix
write-recursive
write-suffix
write-unmarked-guard
"

# This test requires some kind of file to operator on
#seek-stress

TestStep() {
  export SEL_LDR_LIB_PATH=$PWD/src/liboggz/.libs
  for test in ${TESTS}; do
    RunSelLdrCommand "src/tests/${test}${NACL_EXEEXT}"
  done
}
