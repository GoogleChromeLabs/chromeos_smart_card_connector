# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=python${NACL_EXEEXT}

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

# This build relies on certain host binaries and python's configure
# requires us to set --build= as well as --host=.
HOST_BUILD_DIR=${WORK_DIR}/build_host

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

SetOptFlags() {
  # Python build system sets its own opt flags
  return
}

ConfigureStep() {
  # We pre-seed configure with certain results that it cannot determine
  # since we are doing a cross compile.  The $CONFIG_SITE file is sourced
  # by configure early on.
  export CROSS_COMPILE=true
  export CONFIG_SITE=${START_DIR}/config.site
  # Disable ipv6 since configure claims it requires a working getaddrinfo
  # which we do not provide.  TODO(sbc): remove this once nacl_io supports
  # getaddrinfo.
  EXTRA_CONFIGURE_ARGS+=" --disable-ipv6"
  EXTRA_CONFIGURE_ARGS+=" --with-suffix=${NACL_EXEEXT}"
  EXTRA_CONFIGURE_ARGS+=" --build=x86_64-linux-gnu"
  if [ "${NACL_DEBUG}" = 1 ]; then
    EXTRA_CONFIGURE_ARGS+=" --with-pydebug"
  fi
  NACLPORTS_LIBS+=" -ltermcap"
  NACLPORTS_LIBS+=" ${NACL_CLI_MAIN_LIB}"
  if [ "${NACL_LIBC}" = "newlib" ]; then
    # When python builds with wait3/wait4 support it also expects struct rusage
    # to have certain fields and newlib lacks.
    export ac_cv_func_wait3=no
    export ac_cv_func_wait4=no
  fi
  EnableGlibcCompat
  DefaultConfigureStep
  if [ "${NACL_LIBC}" = "newlib" ]; then
    # For static linking we copy in a pre-baked Setup.local
    LogExecute cp ${START_DIR}/Setup.local Modules/
  fi
}

BuildStep() {
  export CROSS_COMPILE=true
  export MAKEFLAGS="PGEN=${NACL_HOST_PYBUILD}/Parser/pgen"
  SetupCrossEnvironment
  DefaultBuildStep
}

InstallStep() {
  export CROSS_COMPILE=true
  DefaultInstallStep
}

TestStep() {
  if [ ${NACL_ARCH} = "pnacl" ]; then
    local pexe=python${NACL_EXEEXT}
    local script=python
    # on Mac/Windows the folder called Python prevents us from creating a
    # script called python (lowercase).
    if [ ${OS_NAME} != "Linux" ]; then
      script+=".sh"
    fi
    TranslateAndWriteLauncherScript ${pexe} x86-64 python.x86-64.nexe ${script}
  fi
}

PublishStep() {
  local assembly_dir=${PUBLISH_DIR}
  MakeDir ${assembly_dir}


  ChangeDir ${assembly_dir}
  if [[ $TOOLCHAIN == pnacl ]]; then
    local tar_file=pydata.tar
    LogExecute cp ${INSTALL_DIR}${PREFIX}/bin/python${NACL_EXEEXT} \
        python${NACL_EXEEXT}
    LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
        python${NACL_EXEEXT} -s . -o python.nmf
  else
    local tar_file=_platform_specific/${NACL_ARCH}/pydata.tar
    MakeDir _platform_specific/${NACL_ARCH}
    LogExecute cp ${INSTALL_DIR}${PREFIX}/bin/python${NACL_EXEEXT} \
        _platform_specific/${NACL_ARCH}/python${NACL_EXEEXT}
    LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py --no-arch-prefix \
        _platform_specific/*/python${NACL_EXEEXT} -s . -o python.nmf
  fi

  LogExecute tar cf ${tar_file} -C ${INSTALL_DIR}${PREFIX} lib/python2.7
  LogExecute shasum ${tar_file} > ${tar_file}.hash

  LogExecute python ${TOOLS_DIR}/create_term.py python.nmf

  GenerateManifest ${START_DIR}/manifest.json ${assembly_dir}
  InstallNaClTerm ${assembly_dir}
  LogExecute cp ${START_DIR}/background.js ${assembly_dir}
  LogExecute cp ${START_DIR}/python.js ${assembly_dir}
  LogExecute cp ${START_DIR}/index.html ${assembly_dir}
  LogExecute cp ${START_DIR}/icon_16.png ${assembly_dir}
  LogExecute cp ${START_DIR}/icon_48.png ${assembly_dir}
  LogExecute cp ${START_DIR}/icon_128.png ${assembly_dir}
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip python.zip .
}
