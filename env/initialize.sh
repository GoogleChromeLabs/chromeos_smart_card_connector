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
#
# Pass "-t <toolchain>" in order to initialize only dependencies needed for
# building with the toolchain ("<toolchain>" can be one of "emscripten", "pnacl"
# or "coverage").


set -eu

force_reinitialization=0
toolchain=


log_message() {
  local message=${1}

  echo -e "\033[33;32m${message}\033[0m"
}

log_error_message() {
  local message=${1}

  echo -e "\033[33;31m${message}\033[0m"
}

initialize_python3_venv() {
  if [ -d ./python3_venv -a "${force_reinitialization}" -eq "0" ]; then
    log_message "python3_venv already present, skipping."
    return
  fi
  log_message "Creating python3_venv..."
  rm -rf ./python3_venv
  # Skip default Pip installation, since it breaks with mysterious errors.
  python3 -m venv --without-pip ./python3_venv
  # Install Pip.
  source ./python3_venv/bin/activate
  curl https://bootstrap.pypa.io/get-pip.py | python3
  # Install Pip modules we need.
  python3 -m pip install -r ./pip3_requirements.txt
  # Exit venv - we can't have it on by default as NaCl-related steps need
  # Python 2.
  deactivate
  log_message "python3_venv was created successfully."
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

# Creates a complete installation of Python 2 locally, instead of relying on
# system-wide Python 2 installations that become increasingly cumbersome due to
# sunset of Python 2. We need Python 2 as long as we keep using NaCl.
initialize_python2() {
  if [ -d ./python2_venv -a "${force_reinitialization}" -eq "0" ]; then
    log_message "Python 2 already present, skipping."
    return
  fi
  log_message "Installing Python 2..."
  rm -rf ./python2 python2_venv
  mkdir ./python2
  # Download Python 2 sources.
  wget https://www.python.org/ftp/python/2.7.18/Python-2.7.18.tgz
  tar -zxf Python-2.7.18.tgz
  rm -rf Python-2.7.18.tgz
  # Build Python 2.
  cd Python-2.7.18
  ./configure --prefix=$(realpath ../python2/)
  make -j30 -s
  make install -s
  cd ..
  # Clean up Python 2 sources.
  rm -rf ./Python-2.7.18
  # Install Pip 2.
  curl https://bootstrap.pypa.io/pip/2.7/get-pip.py | python2/bin/python
  # Install Virtualenv.
  python2/bin/pip install virtualenv
  # Create virtual environment.
  python2/bin/virtualenv -p python2/bin/python ./python2_venv
  source ./python2_venv/bin/activate
  # Install needed Pip packages.
  pip2 install -r pip2_requirements.txt
  # Exit Python 2 virtual environment: it should only be used in steps that
  # actually need Python 2.
  deactivate
  log_message "Python 2 was installed successfully."
}

initialize_nacl_sdk() {
  export NACL_SDK_ROOT="${SCRIPTPATH}/nacl_sdk/pepper_${NACL_SDK_VERSION}"
  if [ -d ./nacl_sdk -a "${force_reinitialization}" -eq "0" ]; then
    log_message "Native Client SDK already present, skipping."
    return
  fi
  log_message "Installing Native Client SDK (version ${NACL_SDK_VERSION})..."
  # Prepare NaCl SDK installation scripts.
  rm -rf nacl_sdk
  cp -r ../third_party/nacl_sdk/nacl_sdk .
  # Enter the virtual environment - the NaCl SDK scripts still use Python 2.
  source ./python2_venv/bin/activate
  # Set up the complete SDK with the given version.
  python2 nacl_sdk/sdk_tools/sdk_update_main.py install pepper_${NACL_SDK_VERSION}
  # Exit the Python 2 virtual environment, to avoid affecting steps that don't
  # need it.
  deactivate
  log_message "Native Client SDK was installed successfully."
}

initialize_webports() {
  if [ -d ./webports -a "${force_reinitialization}" -eq "0" ]; then
    log_message "webports already present, skipping."
    return
  fi
  log_message "Installing webports (with building the following libraries: ${WEBPORTS_TARGETS})..."
  # Prepare the webports installation scripts.
  rm -rf webports
  cp -r ../third_party/webports/src webports
  # Enter the virtual environment - the webports scripts still use Python 2 (as
  # they are based on NaCl).
  source ./python2_venv/bin/activate
  # Install and build the needed webports libraries.
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
  # Exit the Python 2 virtual environment, to avoid affecting steps that don't
  # need it.
  deactivate
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
  # Note we don't put python3_venv activation here, since legacy NaCl build
  # scripts still use Python 2.
  log_message "Creating \"activate\" script..."
  echo > activate
  if [[ "${toolchain}" == "" || "${toolchain}" == "pnacl" ]]; then
    echo "export NACL_SDK_ROOT=${NACL_SDK_ROOT}" >> activate
  fi
  if [[ "${toolchain}" == "" || "${toolchain}" == "emscripten" ]]; then
    echo "source ${SCRIPTPATH}/emsdk/emsdk_env.sh" >> activate
  fi
  log_message "\"activate\" script was created successfully. Run \"source $(dirname ${0})/activate\" in order to trigger all necessary environment definitions."
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

. ./constants.sh

while getopts ":ft:" opt; do
  case $opt in
    f)
      force_reinitialization=1
      ;;
    t)
      toolchain=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

initialize_python3_venv

if [[ "${toolchain}" == "" || "${toolchain}" == "emscripten" ]]; then
  initialize_emscripten
fi

if [[ "${toolchain}" == "" || "${toolchain}" == "pnacl" ]]; then
  initialize_depot_tools
  initialize_python2
  # Depends on depot_tools and python2.
  initialize_nacl_sdk
  # Depends on nacl_sdk and python2.
  initialize_webports
fi

initialize_chromedriver

create_activate_script
