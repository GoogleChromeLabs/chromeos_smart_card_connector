# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

export LIB_OSG=libosg.a
export LIB_OSGUTIL=libosgUtil.a
export LIB_OPENTHREADS=libOpenThreads.a

ConfigureStep() {
  return 0
}

BuildStep() {
  SetupCrossEnvironment
  CFLAGS+=" ${CPPFLAGS}"
  CXXFLAGS+=" ${CPPFLAGS}"
  DefaultBuildStep
}

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}
  Remove ${DESTDIR_INCLUDE}/osg
  Remove ${DESTDIR_INCLUDE}/osgUtil
  Remove ${DESTDIR_INCLUDE}/OpenThreads
  LogExecute cp -R include/osg ${DESTDIR_INCLUDE}/osg
  LogExecute cp -R include/osgUtil ${DESTDIR_INCLUDE}/osgUtil
  LogExecute cp -R include/OpenThreads ${DESTDIR_INCLUDE}/OpenThreads
  Remove ${DESTDIR_LIB}/libosg.a
  Remove ${DESTDIR_LIB}/libosgUtil.a
  Remove ${DESTDIR_LIB}/libOpenThreads.a
  install -m 644 ${LIB_OSG} ${DESTDIR_LIB}/${LIB_OSG}
  install -m 644 ${LIB_OSGUTIL} ${DESTDIR_LIB}/${LIB_OSGUTIL}
  install -m 644 ${LIB_OPENTHREADS} ${DESTDIR_LIB}/${LIB_OPENTHREADS}
}
