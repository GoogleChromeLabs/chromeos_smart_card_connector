# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# zlib doesn't support custom build directories so we have
# to build directly in the source dir.
BUILD_DIR=${SRC_DIR}
EXECUTABLES="minigzip${NACL_EXEEXT} example${NACL_EXEEXT}"
if [ "${NACL_SHARED}" = "1" ]; then
  EXECUTABLES+=" libz.so.1"
fi

ConfigureStep() {
  local CONFIGURE_FLAGS="--prefix='${PREFIX}'"
  if [ "${TOOLCHAIN}" = "emscripten" ]; then
    # The emscripten toolchain will happily accept -shared (although it
    # emits a warning if the output name ends with .so).  This means that
    # zlib's configure script tries to build shared as well as static
    # libraries until we explicitly disable shared libraries like this.
    CONFIGURE_FLAGS+=" --static"
  fi
  LogExecute rm -f libz.*
  SetupCrossEnvironment
  CFLAGS="${CPPFLAGS} ${CFLAGS}"
  CXXFLAGS="${CPPFLAGS} ${CXXFLAGS}"
  CHOST=${NACL_CROSS_PREFIX} LogExecute ./configure ${CONFIGURE_FLAGS}
}

RunMinigzip() {
  echo "Running minigzip test"
  export SEL_LDR_LIB_PATH=.
  if echo "hello world" | ./minigzip | ./minigzip -d; then
    echo '  *** minigzip test OK ***'
  else
    echo '  *** minigzip test FAILED ***'
    exit 1
  fi
  unset SEL_LDR_LIB_PATH
}

RunExample() {
  echo "Running exmple test"
  export SEL_LDR_LIB_PATH=.
  # This second test does not yet work on nacl (gzopen fails)
  if ./example; then \
    echo '  *** zlib test OK ***'; \
  else \
    echo '  *** zlib test FAILED ***'; \
    exit 1
  fi
  unset SEL_LDR_LIB_PATH
}

TestStep() {
 if [ "${NACL_ARCH}" = "pnacl" ]; then
    local minigzip_pexe="minigzip${NACL_EXEEXT}"
    local example_pexe="example${NACL_EXEEXT}"
    local minigzip_script="minigzip"
    local example_script="example"
    TranslateAndWriteLauncherScript "${minigzip_pexe}" x86-32 \
      minigzip.x86-32.nexe "${minigzip_script}"
    RunMinigzip
    TranslateAndWriteLauncherScript "${minigzip_pexe}" x86-64 \
      minigzip.x86-64.nexe "${minigzip_script}"
    RunMinigzip
    TranslateAndWriteLauncherScript "${example_pexe}" x86-32 \
      example.x86-32.nexe "${example_script}"
    RunExample
    TranslateAndWriteLauncherScript "${example_pexe}" x86-64 \
      example.x86-64.nexe "${example_script}"
    RunExample
  else
    RunMinigzip
    RunExample
  fi
}
