# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

MAKE_TARGETS="CCLD=\$(CXX) all"
NACLPORTS_CPPFLAGS+=" -DNACL_SDK_VERSION=$NACL_SDK_VERSION"
if [ "${NACL_DEBUG}" = "1" ] ; then
  NACLPORTS_CPPFLAGS+=" -DSQLITE_DEBUG -DSQLITE_LOCK_TRACE"
fi
LIBS="-lnacl_io -pthread"
if [ "${NACL_SHARED}" = "1" ]; then
  EXECUTABLE_DIR=.libs
  LIBS+=" -ldl"
else
  EXTRA_CONFIGURE_ARGS=--disable-dynamic-extensions
  EXECUTABLE_DIR=.
fi
export LIBS

EXECUTABLES="sqlite3${NACL_EXEEXT}"

BuildStep() {
  # Remove shell.o between building ppapi and sel_ldr versions of sqlite.
  # This is the only object that depends on the PPAPI define and therefore
  # needs to be rebuilt.
  Remove shell.o
  DefaultBuildStep
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    local pexe=${EXECUTABLE_DIR}/sqlite3${NACL_EXEEXT}
    TranslateAndWriteLauncherScript ${pexe} x86-64 sqlite3.x86-64.nexe sqlite3
  fi

  # Build (at least shell.c) again but this time with nacl_io and -DPPAPI
  NEW_LIBS="${NACL_CLI_MAIN_LIB}"
  Banner "Building sqlite3_ppapi"
  sed -i.bak "s/sqlite3\$(EXEEXT)/sqlite3_ppapi\$(EXEEXT)/" Makefile
  sed -i.bak "s/CFLAGS = /CFLAGS = -DPPAPI /" Makefile
  sed -i.bak "s/-lnacl_io/${NEW_LIBS}/" Makefile
  Remove shell.o
  DefaultBuildStep
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}

  local exe=${PUBLISH_DIR}/sqlite3_ppapi_${NACL_ARCH}${NACL_EXEEXT}

  LogExecute mv ${EXECUTABLE_DIR}/sqlite3_ppapi${NACL_EXEEXT} ${exe}
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    LogExecute ${PNACLFINALIZE} ${exe}
  fi

  pushd ${PUBLISH_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      -L${DESTDIR_LIB} \
      sqlite3_ppapi*${NACL_EXEEXT} \
      -s . \
      -o sqlite.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py sqlite.nmf
  popd

  InstallNaClTerm ${PUBLISH_DIR}
}

RunTest() {
  naclport_test/test -g
}

TestStep() {
  MakeDir naclport_test

  if [[ ${NACL_ARCH} == "pnacl" ]]; then
    EXT=.bc
  else
    EXT=${NACL_EXEEXT}
  fi

  INCLUDES="-I${SRC_DIR}"
  LogExecute ${NACLCXX} ${INCLUDES} ${NACLPORTS_CPPFLAGS} \
    ${NACLPORTS_CFLAGS} ${NACLPORTS_LDFLAGS} \
    -DPPAPI -o naclport_test/test${EXT} \
    ${START_DIR}/test.cc sqlite3.o ${LIBS} -lgtest

  [[ ${NACL_ARCH} == "pnacl" ]] && ${PNACLFINALIZE} \
    -o naclport_test/test${NACL_EXEEXT} naclport_test/test${EXT}

  echo "Running test"

  if [ "${NACL_ARCH}" = "pnacl" ]; then
    local pexe=test${NACL_EXEEXT}
    (cd naclport_test;
     TranslateAndWriteLauncherScript ${pexe} x86-32 test.x86-32${EXT} \
       test)
    RunTest
    (cd naclport_test;
     TranslateAndWriteLauncherScript ${pexe} x86-64 test.x86-64${EXT} \
       test)
    RunTest
    echo "Tests OK"
  elif [ "$(uname -m)" = "${NACL_ARCH_ALT}" ]; then
    WriteLauncherScript naclport_test/test test${EXT}
    RunTest
    echo "Tests OK"
  fi
}
