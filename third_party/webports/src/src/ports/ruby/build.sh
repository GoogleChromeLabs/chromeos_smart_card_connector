# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

MAKE_TARGETS="pprogram"
INSTALL_TARGETS="install-nodoc"

EXECUTABLES="ruby${NACL_EXEEXT} pepper-ruby${NACL_EXEEXT}"

ConfigureStep() {
  # We need to build a host version of ruby for use during the nacl
  # build.
  HOST_BUILD=${WORK_DIR}/build_host
  if [ ! -x ${HOST_BUILD}/inst/bin/ruby ]; then
    Banner "Building ruby for host"
    MakeDir ${HOST_BUILD}
    ChangeDir ${HOST_BUILD}
    CFLAGS="" LDFLAGS="" LogExecute ${SRC_DIR}/configure --prefix=$PWD/inst
    LogExecute make -j${OS_JOBS} miniruby
    LogExecute make -j${OS_JOBS} install-nodoc
    cd -
  fi

  # TODO(sbc): remove once getaddrinfo() is working
  EXTRA_CONFIGURE_ARGS=--disable-ipv6

  EnableGlibcCompat

  if [ "${NACL_LIBC}" = "newlib" ]; then
    EXTRA_CONFIGURE_ARGS+=" --with-static-linked-ext --with-newlib"
  else
    EXTRA_CONFIGURE_ARGS+=" --with-out-ext=openssl,digest/*"
  fi

  local conf_host=${NACL_CROSS_PREFIX}
  if [ ${NACL_ARCH} = "pnacl" ]; then
    # The PNaCl tools use "pnacl-" as the prefix, but config.sub
    # does not know about "pnacl".  It only knows about "le32-nacl".
    # Unfortunately, most of the config.subs here are so old that
    # it doesn't know about that "le32" either.  So we just say "nacl".
    conf_host="nacl"
  fi

  SetupCrossEnvironment
  LogExecute ${SRC_DIR}/configure \
    --host=${conf_host} \
    --prefix=/ \
    --with-baseruby=${HOST_BUILD}/inst/bin/ruby \
    --with-http=no \
    --with-html=no \
    --with-ftp=no \
    --${NACL_OPTION}-mmx \
    --${NACL_OPTION}-sse \
    --${NACL_OPTION}-sse2 \
    --${NACL_OPTION}-asm \
    --with-x=no \
    ${EXTRA_CONFIGURE_ARGS}
}

BuildStep() {
  DefaultBuildStep
  if [ $NACL_ARCH == "pnacl" ]; then
    # Just write the x86-64 version out for now.
    TranslateAndWriteLauncherScript ruby.pexe x86-64 ruby.x86-64.nexe ruby
  fi
}

InstallStep() {
  DESTDIR=${DESTDIR}/${PREFIX}
  DefaultInstallStep
}
