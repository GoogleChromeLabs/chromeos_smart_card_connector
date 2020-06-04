# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="twm${NACL_EXEEXT}"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

NACLPORTS_LIBS+="\
  -lXext -lXmu -lSM -lICE -lXt -lX11 -lxcb -lXau ${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  NACLPORTS_CFLAGS+=" -Wno-return-type -Wno-parentheses -Wno-dangling-else"
  NACLPORTS_CPPFLAGS+=" -std=gnu89"
fi

BuildStep() {
  RC=deftwmrc.c
  rm -f ${RC}
  echo 'char *defTwmrc[] = {' >>${RC}
  sed -e '/^#/d' -e 's/"/\\"/g' -e 's/^/    "/' -e 's/$/",/' \
    ${SRC_DIR}/system.twmrc >>${RC}
  echo '    (char *) 0 };' >>${RC}

  LogExecute flex ${SRC_DIR}/lex.l
  LogExecute bison --defines=gram.h ${SRC_DIR}/gram.y
  SetupCrossEnvironment
  LogExecute ${CC} ${CPPFLAGS} ${CFLAGS} -o twm ${SRC_DIR}/*.c *.c -I. \
    -I${SRC_DIR} ${LDFLAGS} ${LIBS}
  LogExecute ${CC} ${CPPFLAGS} ${CFLAGS} -o twm${NACL_EXEEXT} ${SRC_DIR}/*.c\
   *.c -I. -I${SRC_DIR} ${LDFLAGS} ${LIBS}
}

InstallStep() {
  MakeDir ${DESTDIR_BIN}
  LogExecute cp -f ${BUILD_DIR}/twm${NACL_EXEEXT} ${DESTDIR_BIN}/twm
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  ChangeDir ${PUBLISH_DIR}
  LogExecute cp -f ${BUILD_DIR}/twm${NACL_EXEEXT} twm_${NACL_ARCH}${NACL_EXEEXT}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py\
     twm_*${NACL_EXEEXT} -s . -o twm.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py -n twm twm.nmf
  InstallNaClTerm .
  LogExecute cp -f ${START_DIR}/*.js .
}
