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

# This scripts initializes the build environment, by downloading and setting up
# all necessary tools and libraries.
#
# By default, existing directories and files are left intact (except the
# "./activate" file), so it's safe to run this script multiple times. Add the
# "-f" flag in order to enforce overwriting the existing state.


set -eu

force_reinitialization=0


log_message() {
  local message=${1}

  echo -e "\033[33;32m${message}\033[0m"
}

log_error_message() {
  local message=${1}

  echo -e "\033[33;31m${message}\033[0m"
}

initialize_emscripten() {
  if [ -d ./emsdk -a "${force_reinitialization}" -eq "0" ]; then
    log_message "Emscripten already present, skipping."
    return
  fi
  log_message "Installing Emscripten..."
  rm -rf ./emsdk
  git clone "${EMSCRIPTEN_SDK_REPOSITORY_URL}" emsdk
  ./emsdk/emsdk install "${EMSCRIPTEN_VERSION}"
  ./emsdk/emsdk activate "${EMSCRIPTEN_VERSION}"
  log_message "Emscripten was installed successfully."
}

initialize_depot_tools() {
  if [ -d ./depot_tools -a "${force_reinitialization}" -eq "0" ]; then
    log_message "depot_tools already present, skipping."
    return
  fi
  log_message "Installing depot_tools..."
  rm -rf ./depot_tools
  git clone "${DEPOT_TOOLS_REPOSITORY_URL}" depot_tools
  PATH="${SCRIPTPATH}/depot_tools:${PATH}"
  log_message "depot_tools were installed successfully."
}

initialize_nacl_sdk() {
  export NACL_SDK_ROOT="${SCRIPTPATH}/nacl_sdk/pepper_${NACL_SDK_VERSION}"
  if [ -d ./nacl_sdk -a "${force_reinitialization}" -eq "0" ]; then
    log_message "Native Client SDK already present, skipping."
    return
  fi
  log_message "Installing Native Client SDK (version ${NACL_SDK_VERSION})..."
  rm -rf nacl_sdk
  cp -r ../third_party/nacl_sdk/nacl_sdk .
  python2 nacl_sdk/sdk_tools/sdk_update_main.py install pepper_${NACL_SDK_VERSION}
  log_message "Native Client SDK was installed successfully."
}

initialize_webports() {
  if [ -d ./webports -a "${force_reinitialization}" -eq "0" ]; then
    log_message "webports already present, skipping."
    return
  fi
  log_message "Installing webports (with building the following libraries: ${WEBPORTS_TARGETS})..."
  rm -rf webports
  cp -r ../third_party/webports/src webports
  cd webports
  tar -zxf git.tar.gz
  gclient runhooks
  cd src
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

initialize_chromedriver() {
  if [ -f ./chromedriver -a "${force_reinitialization}" -eq "0" ]; then
    log_message "chromedriver already present, skipping."
    return
  fi
  log_message "Installing chromedriver..."
  rm -rf ./chromedriver
  # Determine the currently installed version of Chrome. Leave only the
  # major-minor-build triple, e.g., 72.0.3626.
  local chrome_version=$(google-chrome --version | cut -f 3 -d ' ' | cut -d '.' -f 1-3)
  # Obtain the matching Chromedriver version.
  local chromedriver_version=$(curl http://chromedriver.storage.googleapis.com/LATEST_RELEASE_${chrome_version})
  # Download Chromedriver.
  wget https://chromedriver.storage.googleapis.com/${chromedriver_version}/chromedriver_linux64.zip
  unzip -q chromedriver_linux64.zip
  rm -rf chromedriver_linux64.zip
  log_message "chromedriver was installed successfully."
}

create_activate_script() {
  log_message "Creating \"activate\" script..."
  echo > activate
  echo "export NACL_SDK_ROOT=${NACL_SDK_ROOT}" >> activate
  echo "source ${SCRIPTPATH}/emsdk/emsdk_env.sh" >> activate
  log_message "\"activate\" script was created successfully. Run \"source $(dirname ${0})/activate\" in order to trigger all necessary environment definitions."
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

. ./constants.sh

while getopts ":f" opt; do
  case $opt in
    f)
      force_reinitialization=1
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

initialize_depot_tools

initialize_emscripten

# Depends on depot_tools.
initialize_nacl_sdk
# Depends on nacl_sdk.
initialize_webports

initialize_chromedriver

create_activate_script
