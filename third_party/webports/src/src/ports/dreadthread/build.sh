# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  MakeDir ${SRC_DIR}
  ChangeDir ${SRC_DIR}
  ${NACLCC} -c ${START_DIR}/dread.c -o dread.o
  ${NACLCC} -c ${START_DIR}/dread_chain.c -o dread_chain.o
  ${NACLAR} rcs libdreadthread.a \
      dread.o \
      dread_chain.o
  ${NACLRANLIB} libdreadthread.a
}

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}
  LogExecute cp ${SRC_DIR}/libdreadthread.a ${DESTDIR_LIB}/
  LogExecute cp ${START_DIR}/dreadthread.h ${DESTDIR_INCLUDE}/
  LogExecute cp ${START_DIR}/dreadthread_ctxt.h ${DESTDIR_INCLUDE}/
  LogExecute cp ${START_DIR}/dreadthread_chain.h ${DESTDIR_INCLUDE}/
}
