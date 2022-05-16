#!/usr/bin/env bash

# Copyright 2022 Google Inc. All Rights Reserved.
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

CONFIGS="Release Debug"
COVERAGE_PROFILE_DIRS="
  common/cpp/build/tests
  third_party/libusb/webport/build/tests"
EXECUTABLE_MODULE_DIRS="
  smart_card_connector_app/build/executable_module"
UNIT_TEST_EXECUTABLE_DIRS="
  common/cpp/build/tests
  third_party/libusb/webport/build/tests"
IGNORE_FILENAME_REGEX="
  third_party/googletest"

log_message() {
  local message=${1}

  # Create a colored log ("\033[33;32m" switches to the green color, and
  # "\033[0m" switches back).
  echo -e "\033[33;32m******* ${message} *******\033[0m"
}

echo "CPLUS_INCLUDE_PATH=${CPLUS_INCLUDE_PATH}"

for config in ${CONFIGS}; do
  log_message "Building in mode \"${config}\"..."
  TOOLCHAIN=coverage CONFIG=${config} make -j30
  log_message "Running tests in mode \"${config}\"..."
  TOOLCHAIN=coverage CONFIG=${config} make -j30 test
done

log_message "Merging coverage profiles..."
profile_files=
for dir in ${COVERAGE_PROFILE_DIRS}; do
  for config in ${CONFIGS}; do
    profile_files="${profile_files} ${dir}/${config}.profraw"
  done
done
llvm-profdata \
  merge \
  --sparse \
  ${profile_files} \
  --output=env/coverage-artifacts/all.profdata

log_message "Building coverage report..."
executables=
for dir in ${EXECUTABLE_MODULE_DIRS}; do
  for config in ${CONFIGS}; do
    executables="${executables} ${dir}/out-artifacts-coverage/${config}/executable_module"
  done
done
for dir in ${UNIT_TEST_EXECUTABLE_DIRS}; do
  for config in ${CONFIGS}; do
    executables="${executables} ${dir}/out-artifacts-coverage/${config}/cpp_unit_test_runner"
  done
done
# Insert "-object" before each path, except for the first one (that's the
# weirdness of llvm-cov's CLI).
llvm_cov_args=$(echo "${executables}" | tr ' ' ' -object ')
llvm-cov report \
  ${llvm_cov_args} \
  -instr-profile=env/coverage-artifacts/all.profdata \
  -ignore-filename-regex=${IGNORE_FILENAME_REGEX}
