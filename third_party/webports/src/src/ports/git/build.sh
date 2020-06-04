# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="git git-remote-http git-remote-https"

BUILD_DIR=${SRC_DIR}
export EXTLIBS="${NACL_CLI_MAIN_LIB}"

if [ "${NACL_SHARED}" != "1" ]; then
  # These are needed so that the configure can detect libcurl when statically
  # linked.
  NACLPORTS_LIBS+=" -lcurl -lssl -lcrypto -lz"
fi

if [ ${OS_NAME} = "Darwin" ]; then
  # gettext (msgfmt) doesn't exist on darwin by default.  homebrew installs
  # it to /usr/local/opt/gettext, and we need it to be in the PATH when
  # building git
  export PATH=${PATH}:/usr/local/opt/gettext/bin
fi

if [ "${NACL_LIBC}" = "newlib" ]; then
  export NO_RT_LIBRARY=1
  EXTLIBS+=" -lglibc-compat"
fi
export CROSS_COMPILE=1
export NEEDS_CRYPTO_WITH_SSL=YesPlease

EnableGlibcCompat

ConfigureStep() {
  NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"
  autoconf

  if [ "${NACL_LIBC}" = "glibc" ]; then
    # Because libcrypto.a needs dlsym we need to add this explicitly.
    # This is not normally needed when libcyrpto is a shared library.
    NACLPORTS_LDFLAGS+=" -ldl"
  fi

  DefaultConfigureStep
}

BuildStep() {
  SetupCrossEnvironment
  export CCLD=${CXX}
  # Git's build doesn't support building outside the source tree.
  # Do a clean to make rebuild after failure predictable.
  LogExecute make clean
  DefaultBuildStep
}

InstallStep() {
  SetupCrossEnvironment
  export CCLD=${CXX}
  DefaultInstallStep
  # Remove some of the git symlinks to save some space (since symlinks are
  # currnely only supported via file copying).
  for f in ${DESTDIR}${PREFIX}/libexec/git-core/*; do
    if [ -L $f ]; then
      Remove $f
    fi
  done
}
