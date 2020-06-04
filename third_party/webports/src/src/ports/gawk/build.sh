# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# getopt.c and getopt1.c have been commented for build
NACLPORTS_CFLAGS+=" -Dmain=nacl_main"
if [ ${TOOLCHAIN} != "glibc" ]; then
  NACLPORTS_CFLAGS+=" -D__GNU_LIBRARY__ "
fi

NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"

#BuildStep() {
  # SetupCrossEnvironment
  # including readline because it is not linked by itself
  # ncurses also has to be linked otherwise multiple
  # undefined errors appear in linking readline
  #LogExecute make -j${OS_JOBS} \
    #LIBREADLINE="-lreadline -lncurses" gawk${NACL_EXEEXT}
#}
