# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"
NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"

EXECUTABLES="tests/devenv_small_test_${NACL_ARCH}${NACL_EXEEXT} \
             jseval/jseval_${NACL_ARCH}${NACL_EXEEXT}"

STORAGE_URL=https://naclports.storage.googleapis.com/builds
SDK_VERSION=pepper_48
BUILT_REVISION=trunk-703-gdb4edd8
DEFAULT_SOURCE=${STORAGE_URL}/${SDK_VERSION}/${BUILT_REVISION}/publish
LOCAL_SOURCE=http://localhost:5103

BuildStep() {
  SetupCrossEnvironment

  # Build jseval module.
  MakeDir ${BUILD_DIR}/jseval
  LogExecute ${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} -O2 \
      ${START_DIR}/jseval.c \
      -o ${BUILD_DIR}/jseval/jseval_${NACL_ARCH}${NACL_EXEEXT} \
      ${EXTRA_LIBS}

  # Build test module.
  MakeDir ${BUILD_DIR}/tests
  LogExecute ${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -g \
      ${START_DIR}/tests/devenv_small_test.cc \
      -o ${BUILD_DIR}/tests/devenv_small_test_${NACL_ARCH}${NACL_EXEEXT} \
      -lgtest ${EXTRA_LIBS}
}

InstallStep() {
  return
}

#
# $1: Name of directory in which to create the conf file.
# $2: The http address of repo.
#
CreateRepoConfFile() {
  MakeDir $1
  local PKG_ARCH=${NACL_ARCH}
  if [ "${PKG_ARCH}" = "x86_64" ]; then
    PKG_ARCH=x86-64
  fi
  if [ "${NACL_ARCH}" = "${TOOLCHAIN}" ]; then
    local url="$2/pkg_${PKG_ARCH}"
  else
    local url="$2/pkg_${TOOLCHAIN}_${PKG_ARCH}"
  fi

  cat > $1/NaCl.conf <<EOF
NaCl: {
    url: $url,
    MIRROR_TYPE: HTTP,
}
EOF
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}

  local APP_DIR=${PUBLISH_DIR}/app
  MakeDir ${APP_DIR}

  # Set up files for bootstrap.
  local BASH_DIR=${NACL_PACKAGES_PUBLISH}/bash/${TOOLCHAIN}/bash_multiarch
  local PKG_DIR=${NACL_PACKAGES_PUBLISH}/pkg/${TOOLCHAIN}
  local GETURL_DIR=${NACL_PACKAGES_PUBLISH}/geturl/${TOOLCHAIN}
  local UNZIP_DIR=${NACL_PACKAGES_PUBLISH}/unzip/${TOOLCHAIN}

  LogExecute cp -fR ${BASH_DIR}/* ${APP_DIR}
  LogExecute cp -fR ${PKG_DIR}/* ${APP_DIR}
  LogExecute cp -fR ${GETURL_DIR}/* ${APP_DIR}
  LogExecute cp -fR ${UNZIP_DIR}/* ${APP_DIR}

  # Install jseval only for pnacl (as it can't really work otherwise).
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    LogExecute ${PNACLFINALIZE} \
        ${BUILD_DIR}/jseval/jseval_${NACL_ARCH}${NACL_EXEEXT} \
        -o ${APP_DIR}/jseval_${NACL_ARCH}${NACL_EXEEXT}
    cd ${APP_DIR}
    LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
        jseval_${NACL_ARCH}${NACL_EXEEXT} \
        -s . \
        -o jseval.nmf
  fi

  # Install the HTML/JS for the terminal.
  # TODO(gdeepti): Extend mounter to the other create_term.py apps.
  ChangeDir ${APP_DIR}
  LogExecute python ${TOOLS_DIR}/create_term.py -i whitelist.js \
      -i devenv.js -i mounter.js -s mounter.css bash.nmf
  LogExecute cp bash.nmf sh.nmf
  InstallNaClTerm ${APP_DIR}

  # Create Nacl.conf file
  CreateRepoConfFile "${APP_DIR}/repos_local_${NACL_ARCH}" "${LOCAL_SOURCE}"
  CreateRepoConfFile "${APP_DIR}/repos_${NACL_ARCH}" "${DEFAULT_SOURCE}"

  RESOURCES="background.js mounter.css mounter.js bash.js bashrc which
      install-base-packages.sh graphical.html devenv.js whitelist.js
      devenv_16.png devenv_48.png devenv_128.png"
  for resource in ${RESOURCES}; do
    LogExecute install -m 644 ${START_DIR}/${resource} ${APP_DIR}/
  done
  sed "s/@TOOLCHAIN@/${TOOLCHAIN}/g" \
    ${START_DIR}/setup-environment > ${APP_DIR}/setup-environment

  # Generate manifest.txt so that nacl_io can list the directory
  rm -f manifest.txt
  (cd ${APP_DIR} && ${NACL_SDK_ROOT}/tools/genhttpfs.py . -r > ../manifest.txt)
  mv ../manifest.txt .

  # Generate a manifest.json.
  GenerateManifest ${START_DIR}/manifest.json.template ${APP_DIR} \
    key=$(cat ${START_DIR}/key.txt)

  # Create uploadable version (key not included).
  local APP_UPLOAD_DIR="${PUBLISH_DIR}/devenv_app_upload"
  rm -rf ${APP_UPLOAD_DIR}
  LogExecute cp -r ${APP_DIR} ${APP_UPLOAD_DIR}
  GenerateManifest ${START_DIR}/manifest.json.template \
    ${APP_UPLOAD_DIR} key=

  # Zip the full app for upload.
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip devenv_app_upload.zip devenv_app_upload

  # Copy the files for DevEnvWidget.
  local WIDGET_DIR=${PUBLISH_DIR}/devenvwidget
  MakeDir ${WIDGET_DIR}
  LogExecute cp -r ${START_DIR}/devenvwidget/* ${WIDGET_DIR}

  # Install tests.
  MakeDir ${PUBLISH_DIR}/tests
  LogExecute cp -r ${BUILD_DIR}/tests/* ${PUBLISH_DIR}/tests
  cd ${PUBLISH_DIR}/tests
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    LogExecute ${PNACLFINALIZE} devenv_small_test_${NACL_ARCH}${NACL_EXEEXT}
  fi
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      devenv_small_test_${NACL_ARCH}${NACL_EXEEXT} \
      -s . \
      -o devenv_small_test.nmf
  LogExecute mv devenv_small_test_${NACL_ARCH}${NACL_EXEEXT} \
      devenv_small_test_${NACL_ARCH}

  Remove devenv_small_test.zip
  LogExecute zip -r devenv_small_test.zip *
}

PostInstallTestStep() {
  local arches=
  if [[ ${OS_NAME} == Darwin && ${NACL_ARCH} == x86_64 ]]; then
    echo "Skipping devenv tests on unsupported mac + x86_64 configuration."
  elif [[ ${NACL_ARCH} == arm ]]; then
    echo "Skipping devenv tests on arm for now."
  elif [[ ${NACL_ARCH} == pnacl ]]; then
    arches="i686 x86_64"
  else
    arches="${NACL_ARCH}"
  fi
  for arch in ${arches}; do
    LogExecute python ${START_DIR}/devenv_small_test.py -x -v -a ${arch}
    if [[ ${NACL_ARCH} == pnacl ]]; then
      LogExecute python ${START_DIR}/jseval_test.py -x -v -a ${arch}
    fi
    # Run large and io2014 tests only on the buildbots (against pinned revs).
    # Temporarily disabled until the latest set of pkg files is uplaoded
    # by the buildbots
    # TODO(sbc): reenable this.
    #if [[ "${BUILDBOT_BUILDERNAME:-}" != "" ]]; then
      #LogExecute python ${START_DIR}/../devenv/devenv_large_test.py \
        #-x -vv -a ${arch}
      #LogExecute python ${START_DIR}/../devenv/io2014_test.py \
        #-x -vv -a ${arch}
    #fi
  done
}
