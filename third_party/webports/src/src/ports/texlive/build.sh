# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TLNET_URL=http://storage.googleapis.com/naclports/mirror/texlive-20150523

SCHEME="small"

NACLPORTS_CFLAGS+=" -I${NACLPORTS_INCLUDE}"
NACLPORTS_CXXFLAGS+=" -I${NACLPORTS_INCLUDE}"

EXTRA_CONFIGURE_ARGS="--disable-native-texlive-build \
                      --enable-cxx-runtime-hack \
                      --disable-ipc \
                      --disable-lcdf-typetools \
                      --disable-luajittex \
                      --disable-mktexmf-default \
                      --disable-mktexpk-default \
                      --disable-mktextfm-default \
                      --disable-mkocp-default \
                      --disable-mktexfmt-default \
                      --disable-dependency-tracking \
                      --disable-linked-scripts \
                      --disable-shared \
                      --disable-largefile \
                      --with-banner-add=/NaCl \
                      --without-x"

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat

if [ "${NACL_LIBC}" = "newlib" ]; then
  NACLPORTS_CFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
  NACLPORTS_CXXFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
  NACLPORTS_LIBS+=" -lm"
fi

BuildHostBinaries() {
  local host_build=${WORK_DIR}/build_host
  if [ ! -x ${host_build}/texk/web2c/tangleboot ]; then
    Banner "Building texlive for host"
    MakeDir ${host_build}
    ChangeDir ${host_build}
    LIBS="" EXTRA_LIBS="" LogExecute ${SRC_DIR}/configure \
      --disable-all-pkgs --enable-web2c
    LIBS="" EXTRA_LIBS="" LogExecute make -j${OS_JOBS}
    cd -
  fi
  EXTRA_CONFIGURE_ARGS="${EXTRA_CONFIGURE_ARGS} \
                        OTANGLE=${host_build}/texk/web2c/otangle \
                        TIE=${host_build}/texk/web2c/tie \
                        CTANGLE=${host_build}/texk/web2c/ctangle \
                        CTANGLEBOOT=${host_build}/texk/web2c/ctangleboot \
                        TANGLE=${host_build}/texk/web2c/tangle \
                        TANGLEBOOT=${host_build}/texk/web2c/tangleboot"
}

ConfigureStep() {
  # We need to build a host version of TeX Live for tangle, ctangle, otangle,
  # and tie which are used during the NaCl build.
  # TODO(phosek): a better solution would be to distribute pexe's for these
  # tools which would be translated for the host and ran using sel_ldr
  BuildHostBinaries

  local build_host=$(${SRC_DIR}/build-aux/config.guess)
  EXTRA_CONFIGURE_ARGS="${EXTRA_CONFIGURE_ARGS} \
                        --build=${build_host} \
                        BUILDCC=cc \
                        BUILDCXX=c++ \
                        BUILDAR=ar \
                        BUILDRANLIB=ranlib \
                        BUILDLIBS="

  # TODO(phosek): we need to hardcode the package path because kpathsea which
  # is normally responsible for resolving paths requires fork/spawn and pipes;
  # once nacl_io has support for pipes, we could remove this bit
  sed -i "s+\$PACKAGEDIR+/packages/texlive.${NACL_ARCH}+g" \
    ${SRC_DIR}/texk/kpathsea/texmf.cnf

  export ac_exeext=${NACL_EXEEXT}
  DefaultConfigureStep
}

BuildStep() {
  export CONFIG_SITE
  DefaultBuildStep
}

InstallStep() {
  INSTALL_TARGETS="install-strip texlinks"
  DefaultInstallStep
  Remove ${DESTDIR}${PREFIX}/bin/mktexfmt
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  local ARCH_DIR=${PUBLISH_DIR}/${NACL_ARCH}

  ChangeDir ${PUBLISH_DIR}
  local INSTALL_TL="install-tl-unx.tar.gz"
  local INSTALL_TL_DIR=${ARCH_DIR}/install-tl
  TryFetch "${TLNET_URL}/${INSTALL_TL}" ${INSTALL_TL}
  MakeDir ${INSTALL_TL_DIR}
  tar -xf ${INSTALL_TL} --directory ${INSTALL_TL_DIR} --strip-components=1
  rm -rf ${INSTALL_TL}

  ${TOOLS_DIR}/template_expand.py ${START_DIR}/texlive.profile \
    texdir=${ARCH_DIR} scheme=${SCHEME} > ${INSTALL_TL_DIR}/texlive.profile

  LogExecute ${INSTALL_TL_DIR}/install-tl \
    -repository ${TLNET_URL} -profile ${INSTALL_TL_DIR}/texlive.profile
  rm -rf ${INSTALL_TL_DIR}

  if [ "${OS_NAME}" != "Darwin" ]; then
    local tl_executables=$(find ${ARCH_DIR}/share/bin -type f -executable)
  else
    local tl_executables=$(find ${ARCH_DIR}/share/bin -type f -perm +u+x)
  fi

  ChangeDir ${ARCH_DIR}/share
  rm -rf bin
  rm -rf readme-html.dir readme-txt.dir
  rm -rf tlpkg
  rm -rf index.html install-tl install-tl.log release-texlive.txt
  rm -rf LICENSE.CTAN LICENSE.TL README README.usergroups
  find . -name "*.log" -print0 | xargs -0 rm
  for f in $(find . -type l); do
    cp --remove-destination $(readlink -f $f) $f
  done
  cp ${SRC_DIR}/texk/kpathsea/texmf.cnf texmf-dist/web2c/texmf.cnf
  ChangeDir ${ARCH_DIR}

  # TODO(phosek): Undo all source tree changes except for those coming from
  # nacl.patch, this step could be removed once we get kpathsea to work.
  (cd ${SRC_DIR}; git reset --hard)

  # TODO(phosek): The code below could be potentially replaced by
  # PublishByArchForDevEnv, but there is a subtle difference in that we only
  # copy executables valid for the selected scheme (tl_executables) and
  # PublishByArchForDevEnv does not currently have an option to filter out
  # specified files.
  if [ "${OS_NAME}" != "Darwin" ]; then
    local executables=$(find ${DESTDIR}${PREFIX}/bin -type f -executable)
  else
    local executables=$(find ${DESTDIR}${PREFIX}/bin -type f -perm +u+x)
  fi

  for exe in ${executables}; do
    local name=$(basename ${exe})
    name=${name/%.nexe/}
    name=${name/%.pexe/}
    for tl_exe in ${tl_executables}; do
      if [ "${name}" == "$(basename ${tl_exe})" ]; then
        LogExecute cp $(readlink -f ${exe}) ${ARCH_DIR}/${name}
        if $NACLREADELF -V ${ARCH_DIR}/${name} &>/dev/null; then
          pushd ${ARCH_DIR}
          LogExecute cp ${name} ${name}_${NACL_ARCH}${NACL_EXEEXT}
          LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
            ${name}_${NACL_ARCH}${NACL_EXEEXT} -s . -o ${name}.nmf
          LogExecute rm ${name}_${NACL_ARCH}${NACL_EXEEXT}
          LogExecute rm ${name}.nmf
          popd
        fi
      fi
    done
  done

  ChangeDir ${ARCH_DIR}
  LogExecute rm -f ${ARCH_DIR}.zip
  LogExecute zip -r ${ARCH_DIR}.zip .

  # Drop unzipped copy to reduce upload failures on the bots.
  ChangeDir ${PUBLISH_DIR}
  LogExecute rm -rf ${PUBLISH_DIR}/${ARCH_DIR}
}
