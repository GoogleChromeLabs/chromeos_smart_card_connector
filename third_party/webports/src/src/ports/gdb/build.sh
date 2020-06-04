# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_LIBS+=" -lncurses ${NACL_CLI_MAIN_LIB} -lm"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

EnableGlibcCompat

if [ "${NACL_LIBC}" = "newlib" ]; then
  # Since the final link is done with -lnacl_io and not -lglibc-compat
  # we disable getrlimit and setrlimit.  TODO(sbc): add these back if/when
  # nacl_io evolves to include these functions.
  export ac_cv_func_getrlimit=no
  export ac_cv_func_setrlimit=no
fi

ConfigureStep() {
  SetupCrossEnvironment

  LogExecute ${SRC_DIR}/configure --with-curses --with-expat \
      --disable-werror \
      --with-system-readline \
      --disable-libmcheck \
      --prefix=${PREFIX} \
      --enable-targets=arm-none-eabi-nacl \
      --host=${NACL_CROSS_PREFIX} \
      --target=x86_64-nacl \
      ${EXTRA_CONFIGURE_ARGS:-}

  # If the .info files don't exist, "make all" will try to recreate it with the
  # "makeinfo" tool, which isn't normally installed.
  # Just copy the ones from the repo to the build directory.
  MakeDir ${BUILD_DIR}/{gdb,bfd}/doc
  ChangeDir ${SRC_DIR}
  find gdb bfd -name '*.info' -exec cp {} ${BUILD_DIR}/{} \;
}

BuildStep() {
  # gdb configures its submodules at build time so we need to setup
  # the cross enrionment here.  Without this CPPFLAGS doesn't get set
  # in gdb/Makefile.
  SetupCrossEnvironment
  DefaultBuildStep

  # Build test module.
  LogExecute ${CXX} ${CPPFLAGS} ${CXXFLAGS} ${NACLPORTS_LDFLAGS} -g \
      ${START_DIR}/test_module.cc \
      -o ${BUILD_DIR}/test_module_${NACL_ARCH}.nexe -lppapi_cpp -lppapi
}

InstallStep() {
  cd gdb
  DefaultInstallStep
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}

  # Tests
  local TEST_OUT_DIR="${PUBLISH_DIR}/tests"
  MakeDir ${TEST_OUT_DIR}
  LogExecute cp ${BUILD_DIR}/test_module_*.nexe ${TEST_OUT_DIR}
  pushd ${TEST_OUT_DIR}
  python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      test_module_*${NACL_EXEEXT} \
      -s . \
      -o test_module.nmf
  popd

  # GDB App
  local GDB_APP_DIR="${PUBLISH_DIR}/gdb_app"
  MakeDir ${GDB_APP_DIR}/_platform_specific/${NACL_ARCH}
  LogExecute cp gdb/gdb.nexe \
      ${GDB_APP_DIR}/_platform_specific/${NACL_ARCH}/gdb${NACL_EXEEXT}

  pushd ${GDB_APP_DIR}
  python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      _platform_specific/*/gdb*${NACL_EXEEXT} \
      -s . \
      -o gdb.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py gdb.nmf
  popd
  LogExecute cp ${START_DIR}/background.js ${GDB_APP_DIR}
  LogExecute cp ${START_DIR}/ladybug_16.png ${GDB_APP_DIR}
  LogExecute cp ${START_DIR}/ladybug_48.png ${GDB_APP_DIR}
  LogExecute cp ${START_DIR}/ladybug_128.png ${GDB_APP_DIR}

  # Generate a manifest.json (with key included).
  GenerateManifest ${START_DIR}/manifest.json.template \
    ${GDB_APP_DIR} key=$(cat ${START_DIR}/key.txt)

  # Create uploadable version (key not included).
  local GDB_APP_UPLOAD_DIR="${PUBLISH_DIR}/gdb_app_upload"
  rm -rf ${GDB_APP_UPLOAD_DIR}
  LogExecute cp -r ${GDB_APP_DIR} ${GDB_APP_UPLOAD_DIR}
  GenerateManifest ${START_DIR}/manifest.json.template \
    ${GDB_APP_UPLOAD_DIR} key=
  # Zip for upload to the web store.
  pushd ${PUBLISH_DIR}
  rm -f gdb_app_upload.zip
  zip -r gdb_app_upload.zip gdb_app_upload/
  popd

  # Debug Extension
  local DEBUG_EXT_DIR="${PUBLISH_DIR}/debug_extension"
  MakeDir ${DEBUG_EXT_DIR}
  InstallNaClTerm ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/background.js ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/devtools.html ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/devtools.js ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/ladybug_gear_128.png ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/ladybug_gear_16.png ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/ladybug_gear_48.png ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/main.css ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/main.html ${DEBUG_EXT_DIR}
  LogExecute cp ${START_DIR}/extension/main.js ${DEBUG_EXT_DIR}
  GenerateManifest ${START_DIR}/extension/manifest.json.template \
    ${DEBUG_EXT_DIR} key=

  # Zip for upload to the web store.
  pushd ${PUBLISH_DIR}
  rm -f debug_extension.zip
  zip -r debug_extension.zip debug_extension/
  popd
}

PostInstallTestStep() {
  # Temporartily disabled until this gets fixed:
  # https://code.google.com/p/naclports/issues/detail?id=187
  return
  if [[ ${OS_NAME} == Darwin && ${NACL_ARCH} == x86_64 ]]; then
    echo "Skipping gdb/debug tests on unsupported mac + x86_64 configuration."
  elif [[ ${NACL_ARCH} == arm ]]; then
    echo "Skipping gdb/debug tests on arm for now."
  else
    LogExecute python ${START_DIR}/gdb_test.py -x -vv -a ${NACL_ARCH}
    LogExecute python ${START_DIR}/debugger_test.py -x -vv -a ${NACL_ARCH}
  fi
}
