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

# This script removes the temporary files and output files produced by building
# of the libraries and Apps of this project. Optionally, if the "-f" flag is
# specified, it clears the SDKs stored under //env/ as well.
#
# Note: the env/activate file must be source'd into the shell before this script
# execution.


set -eu


BUILT_APP_PACKAGES_DIR="built_app_packages"
CONFIGS="Release Debug"
TOOLCHAINS="emscripten pnacl"


log_message() {
	local message=${1}

	echo -e "\033[33;32m${message}\033[0m"
}

clean_dir_with_toolchain_and_config() {
	local toolchain=${1}
	local config=${2}

	log_message "Cleaning \"${toolchain}\" \"${config}\"..."
	TOOLCHAIN=${toolchain} CONFIG=${config} make clean -j10 || true
}

clean_built_app_packages() {
	log_message "Cleaning built App packages..."
	rm -rf ${BUILT_APP_PACKAGES_DIR}
}

clean_env() {
	log_message "Cleaning env..."
	env/clean.sh || true
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

force_env_clean=0
while getopts ":f" opt; do
  case $opt in
    f)
      force_env_clean=1
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

for toolchain in ${TOOLCHAINS}; do
	for config in ${CONFIGS}; do
		clean_dir_with_toolchain_and_config ${toolchain} ${config}
	done
done
clean_built_app_packages
if [ "${force_env_clean}" -eq "1" ]; then
	clean_env
fi

log_message "Cleaning finished."
