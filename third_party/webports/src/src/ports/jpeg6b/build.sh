# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="\
  cjpeg${NACL_EXEEXT} \
  djpeg${NACL_EXEEXT} \
  jpegtran${NACL_EXEEXT} \
  rdjpgcom${NACL_EXEEXT} \
  wrjpgcom${NACL_EXEEXT}"


BuildStep() {
  for exe in ${EXECUTABLES}; do
    rm -f ${exe} ${exe%%${NACL_EXEEXT}}
  done
  DefaultBuildStep
  for exe in ${EXECUTABLES}; do
    mv ${exe%%${NACL_EXEEXT}} ${exe}
  done
}


InstallStep() {
  MakeDir ${DESTDIR}/${PREFIX}/include
  MakeDir ${DESTDIR}/${PREFIX}/lib
  LogExecute make install-lib prefix=${DESTDIR}/${PREFIX}
}


TestStep() {
  if [ ${NACL_ARCH} = "pnacl" ]; then
    for arch in x86-32 x86-64; do
      for exe in ${EXECUTABLES}; do
        local exe_noext=${exe%.*}
        WriteLauncherScriptPNaCl ${exe_noext} ${exe_noext}.${arch}.nexe ${arch}
      done
      make test
    done
  else
    make test
  fi
}
