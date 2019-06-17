#!/usr/bin/env bash

# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


set -eu


log_message() {
  local message=${1}

  echo -e "\033[33;32m${message}\033[0m"
}

log_error_message() {
  local message=${1}

  echo -e "\033[33;31m${message}\033[0m"
}

initialize_depot_tools() {
  log_message "Installing depot_tools..."
  rm -rf ./depot_tools
  git clone "${DEPOT_TOOLS_REPOSITORY_URL}" depot_tools
  PATH="${SCRIPTPATH}/depot_tools:${PATH}"
  log_message "depot_tools were installed successfully."
}

initialize_nacl_sdk() {
  log_message "Installing Native Client SDK (version ${NACL_SDK_VERSION})..."
  rm -rf nacl_sdk
  cp -r ../third_party/nacl_sdk/nacl_sdk .
  python nacl_sdk/sdk_tools/sdk_update_main.py install pepper_${NACL_SDK_VERSION}
  export NACL_SDK_ROOT="${SCRIPTPATH}/nacl_sdk/pepper_${NACL_SDK_VERSION}"
  log_message "Native Client SDK was installed successfully."
}

initialize_webports() {
  log_message "Installing webports (with building the following libraries: ${WEBPORTS_TARGETS})..."
  rm -rf webports
  mkdir webports
  cd ${SCRIPTPATH}/webports
  gclient config --unmanaged --name=src "${WEBPORTS_REPOSITORY_URL}"
  gclient sync --with_branch_heads
  cd ${SCRIPTPATH}/webports/src
  git checkout -b pepper_${NACL_SDK_VERSION} origin/pepper_${NACL_SDK_VERSION}
  gclient sync
  local failed_targets=
  for target in ${WEBPORTS_TARGETS}; do
    if ! NACL_ARCH=pnacl TOOLCHAIN=pnacl make ${target} ; then
      local failed_targets="${failed_targets} ${target}"
    fi
  done
  cd ${SCRIPTPATH}
  if [[ ${failed_targets} ]]; then
    log_error_message "webports were installed, but the following libraries failed to build:${failed_targets}. You have to fix them manually. Continuing the initialization script..."
  else
    log_message "webports were installed successfully."
  fi
}

create_activate_script() {
  log_message "Creating \"activate\" script..."
  echo "export NACL_SDK_ROOT=${NACL_SDK_ROOT}" > activate
  log_message "\"activate\" script was created successfully. Run \"source $(dirname ${0})/activate\" in order to trigger all necessary environment definitions."
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

. ./constants.sh

initialize_depot_tools
initialize_nacl_sdk
initialize_webports
create_activate_script
