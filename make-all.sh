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

# This script performs the building of the Apps provided by this project,
# including the building of all of their dependencies. The built Apps packages
# (the .ZIP archives suitable for uploading at Chrome WebStore - see
# <https://developer.chrome.com/webstore/publish>) are put into the
# built_app_packages/ directory.
#
# Note: the env must be initialized and the env/activate script must be source'd
# into the shell before executing this script.
#
# For the building prerequisites, refer to the README file.


set -eu


CONFIGS="Release Debug"


log_message() {
	local message=${1}

	echo -e "\033[33;32m******* ${message} *******\033[0m"
}

make_specific_configuration() {
	local build_dir=${1}
	local targets=${2-}
	local toolchain=${3-}
	local packaging=${4-}
	local config=${5-}

	log_message "Building ${targets} in mode \"${toolchain} ${packaging} ${config}\"..."

	TOOLCHAIN=${toolchain} CONFIG=${config} PACKAGING=${packaging} \
		make -C ${build_dir} ${targets} -j10

	log_message "Successfully built ${targets} in mode \"${toolchain} ${packaging} ${config}\"."
}

make_all_configs() {
	local build_dir=${1}
	local targets=${2-}
	local toolchain=${3-}
	local packaging=${4-}

	local config
	for config in ${CONFIGS}; do
		make_specific_configuration ${build_dir} ${targets} ${toolchain} ${packaging} ${config}
	done
}

TARGET=${1-all}
SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

make_all_configs . ${TARGET} emscripten app
make_all_configs . ${TARGET} emscripten extension
