#!/usr/bin/env bash

# Copyright 2017 Google Inc. All Rights Reserved.
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

# This script executes all currently existing tests of the Apps provided by this
# project and their dependencies.
#
# Note: the env must be initialized and the env/activate script must be source'd
# into the shell before executing this script.
#
# For the building prerequisites, refer to the README file.


set -eu


TEST_DIRS="
  common/cpp/build
  common/js/build
  third_party/libusb/naclport/build"

CONFIGS="Release Debug"


log_message() {
  local message=${1}

  echo -e "\033[33;32m${message}\033[0m"
}

run_make_test_with_config() {
  local build_dir=${1}
  local config=${2-}

  CONFIG=${config} make -C ${build_dir} test -j20
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

for dir in ${TEST_DIRS}; do
  for config in ${CONFIGS}; do
    log_message "Testing \"${dir}\" in ${config} mode..."
    run_make_test_with_config ${dir} ${config}
    log_message "Tests of \"${dir}\" in ${config} mode succeeded."
  done
done
