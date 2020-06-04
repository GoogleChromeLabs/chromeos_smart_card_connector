#!/bin/bash
# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Samba doesn't seem to support building outside the source tree.
BUILD_DIR=${SRC_DIR}

#NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

#NACLPORTS_LDFLAGS+="${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat

ConfigureStep() {
  conf_build=$(/bin/sh "${SCRIPT_DIR}/config.guess")

  SetupCrossEnvironment

  local CONFIGURE=${NACL_CONFIGURE_PATH:-${SRC_DIR}/configure}
  local conf_host=${NACL_CROSS_PREFIX}
  if [ "${NACL_ARCH}" = "pnacl" -o "${NACL_ARCH}" = "emscripten" ]; then
    # The PNaCl tools use "pnacl-" as the prefix, but config.sub
    # does not know about "pnacl".  It only knows about "le32-nacl".
    # Unfortunately, most of the config.subs here are so old that
    # it doesn't know about that "le32" either.  So we just say "nacl".
    conf_host="nacl"
  fi

  # Inject a shim that speed up pnacl invocations for configure.
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    local PNACL_CONF_SHIM="${TOOLS_DIR}/pnacl-configure-shim.py"
    CC="${PNACL_CONF_SHIM} ${CC}"
  fi

  CC="${START_DIR}/cc_shim.sh ${CC}"

  # Specify both --build and --host options.  This forces autoconf into cross
  # compile mode.  This is useful since the autodection doesn't always works.
  # For example a trivial PNaCl binary can sometimes run on the linux host if
  # it has the correct LLVM bimfmt support. What is more, autoconf will
  # generate a warning if only --host is specified.
  LogExecute "${CONFIGURE}" \
    --build=${conf_build} \
    --hostcc=gcc \
    --cross-compile \
    --cross-answers=${START_DIR}/answers \
    --prefix=${PREFIX}
}

BuildStep() {
  WAF_ARGS="--targets=smbclient"
  if [ "${VERBOSE:-}" = "1" ]; then
    WAF_ARGS+=" -v"
  fi
  WAF_MAKE=1 LogExecute python ./buildtools/bin/waf build ${WAF_ARGS}
}

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}

  # Copy the .so files.
  LogExecute find bin/default/ -name *.so -exec \
      cp -vuni '{}' "${DESTDIR_LIB}" ";"

  # Copy the headers files.
  MakeDir ${DESTDIR_INCLUDE}/samba
  LogExecute cp -r bin/default/include/public/* ${DESTDIR_INCLUDE}/samba

  # Make the symlinks for the versioned .so files.
  # TODO(zentaro): Generate this mapping automatically.
  #
  # List created with;
  # find out/build/samba/samba-4.1.16/bin/ -name *.so.* -exec ls -l '{}' ";" |
  #     awk -F ' ' '{print $9, $11}'
  ChangeDir ${DESTDIR_LIB}

  LogExecute ln -sf libsmbclient.so libsmbclient.so.0
  LogExecute ln -sf libsmbconf.so libsmbconf.so.0
  LogExecute ln -sf libsamba-util.so libsamba-util.so.0
  LogExecute ln -sf libndr.so libndr.so.0
  LogExecute ln -sf libdcerpc-binding.so libdcerpc-binding.so.0
  LogExecute ln -sf libsamdb.so libsamdb.so.0
  LogExecute ln -sf libndr-nbt.so libndr-nbt.so.0
  LogExecute ln -sf libwbclient.so libwbclient.so.0
  LogExecute ln -sf libndr-standard.so libndr-standard.so.0
  LogExecute ln -sf libgensec.so libgensec.so.0
  LogExecute ln -sf libkrb5-samba4.so libkrb5-samba4.so.26
  LogExecute ln -sf libtalloc.so libtalloc.so.2
  LogExecute ln -sf libntdb.so libntdb.so.0
  LogExecute ln -sf libtdb.so libtdb.so.1
  LogExecute ln -sf libroken-samba4.so libroken-samba4.so.19
  LogExecute ln -sf libcom_err-samba4.so libcom_err-samba4.so.0
  LogExecute ln -sf libldb.so libldb.so.1
  LogExecute ln -sf libhx509-samba4.so libhx509-samba4.so.5
  LogExecute ln -sf libasn1-samba4.so libasn1-samba4.so.8
  LogExecute ln -sf libwind-samba4.so libwind-samba4.so.0
  LogExecute ln -sf libtevent.so libtevent.so.0
  LogExecute ln -sf libgssapi-samba4.so libgssapi-samba4.so.2
  LogExecute ln -sf libhcrypto-samba4.so libhcrypto-samba4.so.5
  LogExecute ln -sf libheimbase-samba4.so libheimbase-samba4.so.1
  LogExecute ln -sf libndr-krb5pac.so libndr-krb5pac.so.0
  LogExecute ln -sf libtevent-util.so libtevent-util.so.0
  LogExecute ln -sf libsamba-credentials.so libsamba-credentials.so.0
  LogExecute ln -sf libsamba-hostconfig.so libsamba-hostconfig.so.0
}
