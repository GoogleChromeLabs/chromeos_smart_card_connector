# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  export CC_FOR_BUILD=clang
else
  export CC_FOR_BUILD=gcc
fi

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
export LIBS="${NACL_CLI_MAIN_LIB}"

EXECUTABLES="tshark${NACL_EXEEXT}"

EXTRA_CONFIGURE_ARGS="
--enable-extra-compiler-warnings
--disable-wireshark
--disable-editcap
--disable-capinfos
--disable-captype
--disable-mergecap
--disable-reordercap
--disable-text2pcap
--disable-dftest
--disable-randpkt
--disable-dumpcap
--disable-rawshark
--disable-ipv6
--without-gcrypt
--without-plugins
--without-pcap
--without-zlib
--disable-glibtest
--disable-gtktest
"
