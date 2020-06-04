# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

# --with-build-sysroot is necessary to run "fixincl"
# properly. Without this option, GCC's build system tries to create
# "include-fixed" based on the host's include directory, which is
# not compatible with nacl-gcc.
EXTRA_CONFIGURE_ARGS="\
    --enable-languages=c,c++ --disable-nls \
    --target=x86_64-nacl \
    --disable-libstdcxx-pch --enable-threads=nacl"

# Force gcc to think that makeinfo is old so it won't build documentation.
# Actually the issue is that its too new on most installs these days
# (Ubunut/Trusty for example) and causes the build to break.
export gcc_cv_prog_makeinfo_modern=no

ConfigureStep() {
  DefaultConfigureStep
  for cache_file in $(find . -name config.cache); do
    Remove $cache_file
  done
}
