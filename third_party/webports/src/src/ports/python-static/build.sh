# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=python${NACL_EXEEXT}

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

# Currently this package only builds on linux.
# The build relies on certain host binaries and python's configure
# requires us to set --build= as well as --host=.
#
# The module downloader is patterned after the Bochs image downloading step.

SetOptFlags() {
  # Python build system sets its own opt flags
  return
}

ConfigureStep() {
  SetupCrossEnvironment
  export CROSS_COMPILE=true
  # We pre-seed configure with certain results that it cannot determine
  # since we are doing a cross compile.  The $CONFIG_SITE file is sourced
  # by configure early on.
  export CONFIG_SITE=${START_DIR}/config.site
  # Disable ipv6 since configure claims it requires a working getaddrinfo
  # which we do not provide.  TODO(sbc): remove this once nacl_io supports
  # getaddrinfo.
  EXTRA_CONFIGURE_ARGS="--disable-ipv6"
  EXTRA_CONFIGURE_ARGS+=" --with-suffix=${NACL_EXEEXT}"
  EXTRA_CONFIGURE_ARGS+=" --build=i686-linux-gnu --disable-shared --enable-static"
  if [ "${NACL_DEBUG}" = 1 ]; then
    EXTRA_CONFIGURE_ARGS+=" --with-pydebug"
  fi
  export SO=.a
  export MAKEFLAGS="PGEN=${NACL_HOST_PYBUILD}/Parser/pgen"
  export LIBS="-ltermcap"
  export DYNLOADFILE=dynload_ppapi.o
  export MACHDEP=ppapi
  export LINKCC=${NACLCXX}
  EnableGlibcCompat
  LogExecute cp ${START_DIR}/dynload_ppapi.c ${SRC_DIR}/Python/
  # This next step is costly, but it sets the environment variables correctly.
  DefaultConfigureStep

  LogExecute cp ${START_DIR}/Setup.local Modules/
  cat ${DEST_PYTHON_OBJS}/*.list >> Modules/Setup.local
  PY_MOD_LIBS=""
  for MODFILE in ${DEST_PYTHON_OBJS}/*.libs ; do
    if [ -e ${MODFILE} ]; then
      source ${MODFILE}
    fi
  done
  LogExecute rm -vf libpython2.7.a
  PY_LINK_LINE+="ppapi_wrapper ${DEST_PYTHON_OBJS}/\*/\*.o"
  PY_LINK_LINE+=" ${PY_MOD_LIBS} -lz -lbz2 ${NACL_CLI_MAIN_LIB_CPP}"
  if [ "${NACL_LIBC}" = "newlib" ]; then
    PY_LINK_LINE+=" -lglibc-compat"
  fi
  echo ${PY_LINK_LINE} >> Modules/Setup.local
  # At this point we use the existing environment variables from
  # DefaultConfigureStep to build our destination Python modules
}

BuildStep() {
  SetupCrossEnvironment
  export CROSS_COMPILE=true
  export MAKEFLAGS="PGEN=${NACL_HOST_PYBUILD}/Parser/pgen"
  DefaultBuildStep
  ChangeDir ${BUILD_DIR}
  Banner "Rebuilding libpython2.7.a"
  ${AR} cr libpython2.7.a ${DEST_PYTHON_OBJS}/*/*.o
  ${RANLIB} libpython2.7.a
  # To avoid rebuilding python.nexe with the new libpython2.7.a and duplicating
  # symbols
  LogExecute touch python${NACL_EXEEXT}
  # The modules get built with SO=so, but they need to be SO=a inside the
  # destination filesystem.
  for fn in $(find ${NACL_DEST_PYROOT}/${SITE_PACKAGES} -name "*.so"); do
    LogExecute touch ${fn%%so}a
    LogExecute rm -v ${fn}
  done
}
