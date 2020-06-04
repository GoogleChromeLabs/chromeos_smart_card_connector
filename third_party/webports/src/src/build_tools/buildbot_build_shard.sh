#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

######################################################################
# Notes on directory layout:
# makefile location (base_dir):  naclports/src
# toolchain injection point:     specified externally via NACL_SDK_ROOT.
######################################################################

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
source ${SCRIPT_DIR}/buildbot_common.sh

set -o nounset
set -o errexit

readonly BNAME_REGEX="(nightly-|naclports-)?(.+)-(.+)-(.+)"
if [[ ${BUILDBOT_BUILDERNAME} =~ ${BNAME_REGEX} ]]; then
  readonly PREFIX=${BASH_REMATCH[1]}
  if [ "${PREFIX}" = "naclports-" ]; then
    readonly TRYBOT=1
  else
    readonly TRYBOT=0
  fi
fi

readonly PARTCMD="${PYTHON} build_tools/partition.py"
readonly SHARD_CMD="${PARTCMD} -t ${SHARD} -n ${SHARDS}"
echo "Calculating targets for shard $((${SHARD} + 1)) of ${SHARDS}"
PACKAGE_LIST=$(${SHARD_CMD})
if [ -z "${PACKAGE_LIST}" ]; then
  echo "sharding command failed: ${SHARD_CMD}"
  exit 1
fi

echo "Shard contains following packages: ${PACKAGE_LIST}"
if [ "${TRYBOT}" = "1" ]; then
  EFFECTED_FILES=$(git diff --cached --name-only origin/master)
  EFFECTED_PACKAGES=$(${PYTHON} build_tools/find_effected_packages.py \
      $EFFECTED_FILES)
  echo "Patch effects the following packages: ${EFFECTED_PACKAGES}"
  if [[ $EFFECTED_PACKAGES != "all" ]]; then
    # Run find_effected_packages again with --deps to include the dependecies
    # of the effected packages

    PACKAGE_LIST=$(${PYTHON} build_tools/find_effected_packages.py --deps \
        $EFFECTED_FILES <<< ${PACKAGE_LIST})
    echo "Building package subset: ${PACKAGE_LIST}"
  fi
fi

for PKG in ${PACKAGE_LIST}; do
  InstallPackageMultiArch ${PKG}
done

echo "@@@BUILD_STEP Summary@@@"
if [[ ${RESULT} != 0 ]] ; then
  echo "@@@STEP_FAILURE@@@"
  echo -e "${MESSAGES}"
fi

exit ${RESULT}
