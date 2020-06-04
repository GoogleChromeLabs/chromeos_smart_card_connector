# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# All these tests rely on mkdir() working, and currently
# neither newlib or glibc has mkdir plumbed through to the
# IRT.
#autocompact_test
#corruption_test
#db_test
#issue178_test
EXECUTABLES="\
    issue200_test \
    c_test \
    table_test \
    arena_test \
    bloom_test \
    cache_test \
    coding_test \
    crc32c_test \
    dbformat_test \
    env_test \
    filename_test \
    filter_block_test \
    log_test \
    memenv_test \
    skiplist_test \
    version_edit_test \
    version_set_test \
    write_batch_test"

BUILD_DIR=${SRC_DIR}

BuildStep() {
  export TARGET_OS=NaCl
  export AR=${NACLAR}
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  LogExecute make clean
  LogExecute make -j${OS_JOBS} OPT=""
  LogExecute make -j${OS_JOBS} OPT="" tests programs
}

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}
  LogExecute cp libleveldb.a ${DESTDIR_LIB}
  LogExecute cp -r include/leveldb ${DESTDIR_INCLUDE}
}

TestStep() {
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    return
  fi

  for test_binary in $EXECUTABLES; do
    echo "***** Running $test_binary.sh"
    ./$test_binary.sh
  done
}
