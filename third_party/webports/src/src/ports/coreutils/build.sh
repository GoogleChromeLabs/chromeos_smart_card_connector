# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BARE_EXECUTABLES="\
[ basename cat chgrp chmod chown cksum comm cp csplit cut date dd df dir
dircolors dirname du echo env expand expr factor false fmt fold ginstall head
id join kill link ln logname ls md5sum mkdir mkfifo mknod mv nl
nohup od paste pathchk pr printenv printf ptx pwd readlink rm rmdir seq
sha1sum shred sleep sort split stat stty sum sync tac tail tee
test touch tr true tsort tty unexpand uniq unlink uname vdir wc whoami yes"

EXECUTABLES=""
for exe in ${BARE_EXECUTABLES}; do
  EXECUTABLES+=" src/${exe}${NACL_EXEEXT}"
done

TRANSLATE_PEXES=no

NACLPORTS_LIBS+=" ${NACL_CLI_MAIN_LIB}"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

EnableGlibcCompat

ConfigureStep() {
  if [ "${TOOLCHAIN}" = "glibc" ]; then
    NACLPORTS_CPPFLAGS+=" -D_FORTIFY_SOURCE=0"

    # Glibc's *at functions don't work correctly since they rely on
    # /proc/self/fd/X.  This ensure thats gnulib's replacements are used
    # instead.
    export ac_cv_func_faccessat=no
    export ac_cv_func_fchdir=no
    # Force replacement fstatat to be used
    export gl_cv_func_fstatat_zero_flag=no

    # sigaction is provided by nacl_io but configure mis-detects it as missing
    export ac_cv_func_sigaction=yes

    # configure guesses this wrong since we are cross compiling
    export gl_cv_func_open_directory_works=yes
  fi

  export ac_list_mounted_fs=found

  # Ensure that configure know that we have a working getcwd() implementation
  export gl_cv_func_getcwd_null=yes
  export gl_cv_func_getcwd_path_max=yes
  export gl_cv_func_getcwd_abort_bug=no

  # Without this coreutils will link with -Wl,--as-needed which causes
  # incorrect link order with nacl_io and pthead.
  export gl_cv_prog_c_ignore_unused_libraries=none

  # Perl is used to generate man pages by running commands with --help, but
  # we are cross compiling so this doesn't work.  Telling configure that perl
  # is missing sovles this issues as the expense of missing man pages.
  export PERL=missing

  # TODO(bradnelson): Re-enable when bots have a newer texinfo.
  # Disabling texinfo, as the version on the bots is too old.
  export MAKEINFO=true

  DefaultConfigureStep
}
