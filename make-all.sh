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
# (both .CRX files and .ZIP archives suitable for uploading at Chrome WebStore
# - see <https://developer.chrome.com/webstore/publish>) are put into the
# built_app_packages/ directory.
#
# Note: the env must be initialized and the env/activate script must be source'd
# into the shell before executing this script.
#
# For the building prerequisites, refer to the README file.


set -eu


APP_DIRS="
	smart_card_connector_app/build
	example_js_smart_card_client_app/build
	example_cpp_smart_card_client_app/build"
BUILT_APP_PACKAGES_DIR="built_app_packages"
CONFIGS="Release Debug"
TOOLCHAINS="emscripten pnacl"


log_message() {
	local message=${1}

	echo -e "\033[33;32m******* ${message} *******\033[0m"
}

make_with_toolchain_and_config() {
	local build_dir=${1}
	local targets=${2-}
	local toolchain=${3-}
	local config=${4-}

	if [ "${toolchain}" = pnacl ]; then
		# NaCl build scripts still use Python 2, so enter the virtual environment.
		source env/python2_venv/bin/activate
	fi

	TOOLCHAIN=${toolchain} CONFIG=${config} make -C ${build_dir} ${targets} -j10

	if [ "${toolchain}" = pnacl ]; then
		# Exit the virtual environment to avoid using Python 2 when it's not needed.
		deactivate
	fi
}

prepare_built_app_packages_dir() {
	rm -rf ${BUILT_APP_PACKAGES_DIR}
	mkdir -p ${BUILT_APP_PACKAGES_DIR} \
		${BUILT_APP_PACKAGES_DIR}/Release \
		${BUILT_APP_PACKAGES_DIR}/Debug
}

copy_app_packages() {
	local dir=${1}
	local config=${2}

	cp -p ${dir}/*.zip ${BUILT_APP_PACKAGES_DIR}/${config}/
}

build_app_package() {
	local dir=${1}
	# TODO(#177): Build app packages in Emscripten mode as well.
	local toolchain=pnacl

	for config in ${CONFIGS}; do
		make_with_toolchain_and_config ${dir} package ${toolchain} ${config}
		copy_app_packages ${dir} ${config}
	done
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

for toolchain in ${TOOLCHAINS}; do
	for config in ${CONFIGS}; do
		log_message "Building in mode \"${toolchain}\" \"${config}\"..."
		make_with_toolchain_and_config . all ${toolchain} ${config}
		log_message "Successfully built in mode \"${toolchain}\" \"${config}\"."
	done
done

log_message "Building app packages..."
prepare_built_app_packages_dir
for dir in ${APP_DIRS}; do
	build_app_package ${dir}
done
log_message "Successfully built app packages."
