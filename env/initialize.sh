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
# Pass "-t <target>" in order to initialize only dependencies needed for
# building with the target ("<target>" can be one of "emscripten", "eslint").


set -eu

force_reinitialization=0
target=


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

initialize_npm() {
  if [ -d ./node_modules -a "${force_reinitialization}" -eq "0" ]; then
    log_message "npm already present, skipping."
    return
  fi
  rm -rf node_modules package.json package-lock.json
  # Initializes NPM (i.e., creates package.json). The "--yes" flag is used to
  # suppress interactive questions (about "package name", "version", etc.).
  npm init --yes
  # Not strictly necessary, but works as a smoke test that NPM works fine (and
  # this package will be installed anyway when installing eslint later).
  npm install js-yaml
  log_message "npm was installed successfully."
}

initialize_eslint() {
  if npm list eslint >/dev/null; then
    log_message "eslint already present, skipping."
    return
  fi
  npm install eslint eslint-plugin-no-floating-promise
  log_message "eslint was installed successfully."
}

initialize_chromedriver() {
  if [ -f ./chromedriver -a "${force_reinitialization}" -eq "0" ]; then
    log_message "chromedriver already present, skipping."
    return
  fi
  log_message "Installing chromedriver..."
  rm -rf ./chromedriver
  # Determine the currently installed version of Chrome, in the
  # major-minor-build format, e.g., 72.0.3626. (The program's output format is
  # "Google Chrome 72.0.3626.0").
  local chrome_version=$(google-chrome --version | cut -f 3 -d ' ' |
    cut -d '.' -f 1-3)
  # Obtain the matching Chromedriver version.
  local chromedriver_version_url="https://googlechromelabs.github.io/chrome-for-testing/LATEST_RELEASE_${chrome_version}"
  local chromedriver_version
  if ! chromedriver_version=$(curl --fail --silent "${chromedriver_version_url}") ; then
    log_error_message "Failed to fetch chromedriver version at ${chromedriver_version_url} ; skipping chromedriver installation..."
    return
  fi
  # Download Chromedriver.
  local chromedriver_url="https://storage.googleapis.com/chrome-for-testing-public/${chromedriver_version}/linux64/chromedriver-linux64.zip"
  if ! wget "${chromedriver_url}" ; then
    log_error_message "Failed to fetch chromedriver at ${chromedriver_url} ; skipping chromedriver installation..."
    return
  fi
  # Unpack a single file with the executable.
  if ! unzip -q -j chromedriver-linux64.zip chromedriver-linux64/chromedriver ; then
    log_error_message "Failed to unpack chromedriver executable; skipping chromedriver installation..."
    return
  fi
  rm -f chromedriver-linux64.zip
  log_message "chromedriver ${chromedriver_version} was installed successfully."
}

create_activate_script() {
  log_message "Creating \"activate\" script..."
  echo > activate
  if [[ "${target}" == "" || "${target}" == "emscripten" ]]; then
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
      target=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

if [[ "${target}" == "" || "${target}" == "emscripten" ]]; then
  initialize_python3_venv
fi

if [[ "${target}" == "" || "${target}" == "emscripten" ]]; then
  initialize_emscripten
fi

if [[ "${target}" == "" || "${target}" == "eslint" ]]; then
  initialize_npm
  # Depends on npm.
  initialize_eslint
fi

if [[ "${target}" == "" || "${target}" == "emscripten" ]]; then
  initialize_chromedriver
fi

create_activate_script
