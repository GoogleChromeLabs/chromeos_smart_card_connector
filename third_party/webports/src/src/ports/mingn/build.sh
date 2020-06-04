# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  # Nothing to build.
  return
}

InstallStep() {
  MakeDir ${INSTALL_DIR}${PREFIX}
  ChangeDir ${INSTALL_DIR}${PREFIX}

  # Create an archive which contains include files and shared objects.

  # Copy files (for now just headers) from $NACL_SDK_ROOT to the package.
  local dirs="
toolchain/${OS_SUBDIR}_x86_glibc/x86_64-nacl/include
"
  for d in ${dirs}; do
    local o=$(echo ${d} | sed "s/toolchain\/${OS_SUBDIR}_x86_glibc\///")
    echo "Copying libs from: ${d} -> ${o}"
    MakeDir ${o}
    if [ -d ${NACL_SDK_ROOT}/${d} ]; then
      cp -R ${NACL_SDK_ROOT}/${d} $(dirname ${o})
    else
      MakeDir ${o}
    fi
  done

  # Create libmingn.so ldscripts.
  MakeDir lib
  cat <<EOF > lib/libmingn.so
GROUP(-lcli_main -lnacl_spawn -lppapi_simple -lnacl_io -lppapi -lstdc++ -lm)
EXTERN(PSUserMainGet)
EOF

  # Remove unnecessary files to reduce the size of the archive.
  LogExecute rm -fr x86_64-nacl/lib*/{gconv,libgfortran*}
  # These headers gome from the 'gcc' package
  LogExecute rm -fr x86_64-nacl/include/c++/
  # These scripts come from the 'binutils' package
  LogExecute rm -fr x86_64-nacl/lib/ldscripts/

  # Resolve all symlinks as nacl_io does not support symlinks.
  for i in $(find . -type l); do
    if [ ! -d ${i} ]; then
      cp ${i} ${i}.tmp
      rm ${i}
      mv ${i}.tmp ${i}
    fi
  done

  # Remove shared objects which are symlinked after we resolve them.
  find . -name '*.so.*.*' -exec rm -f {} \;

  MakeDir lib
  # Copy core libraries
  LogExecute cp -r ${NACL_SDK_LIB}/*.a lib/
  LogExecute cp -r ${NACL_SDK_LIB}/*.o lib/

  # Copy SDK libs
  LogExecute cp -r ${NACL_SDK_LIBDIR}/*.a lib/
  LogExecute rm -fr lib/libgtest.*a
  LogExecute rm -fr lib/libgmock.*a

  # Copy in SDK includes.
  LogExecute cp -r ${NACL_SDK_ROOT}/include ./
  LogExecute rm -fr include/gtest
  LogExecute rm -fr include/gmock
}
