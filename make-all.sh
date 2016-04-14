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


LIBRARY_DIRS="
	common/cpp/build
	third_party/libusb/naclport/build
	third_party/ccid/naclport/build
	third_party/pcsc-lite/naclport/common/build
	third_party/pcsc-lite/naclport/server/build
	third_party/pcsc-lite/naclport/server_clients_management/build
	third_party/pcsc-lite/naclport/cpp_demo/build
	third_party/pcsc-lite/naclport/cpp_client/build
	example_js_standalone_smart_card_client_library"

APP_DIRS="
	smart_card_connector_app/build
	example_js_smart_card_client_app/build
	example_cpp_smart_card_client_app/build"

BUILT_APP_PACKAGES_DIR="built_app_packages"

CONFIGS="Release Debug"


log_message() {
	local message=${1}

	echo -e "\033[33;32m${message}\033[0m"
}

make_with_config() {
	local build_dir=${1}
	local targets=${2-}
	local config=${3-}

	local command="make -C ${build_dir} ${targets} -j20"
	if [[ ${config} ]]; then
		CONFIG=${config} ${command}
	else
		${command}
	fi
}

build_library() {
	local dir=${1}

	log_message "Building library \"${dir}\"..."
	for config in ${CONFIGS}; do
		make_with_config ${dir} all ${config}
	done
	make_with_config ${dir} all
	log_message "Library \"${dir}\" was built successfully."
}

prepare_built_app_packages_dir() {
	rm -rf ${BUILT_APP_PACKAGES_DIR}
	mkdir -p ${BUILT_APP_PACKAGES_DIR} ${BUILT_APP_PACKAGES_DIR}/Release ${BUILT_APP_PACKAGES_DIR}/Debug
}

copy_app_packages() {
	local dir=${1}
	local config=${2}

	cp -p ${dir}/*.crx ${BUILT_APP_PACKAGES_DIR}/${config}/
	cp -p ${dir}/*.zip ${BUILT_APP_PACKAGES_DIR}/${config}/
}

build_app() {
	local dir=${1}

	log_message "Building App \"${dir}\"..."
	for config in ${CONFIGS}; do
		make_with_config ${dir} package ${config}
		copy_app_packages ${dir} ${config}
	done
	make_with_config ${dir} package
	log_message "App \"${dir}\" was built successfully."
}


SCRIPTPATH=$(dirname $(realpath ${0}))
cd ${SCRIPTPATH}

for dir in ${LIBRARY_DIRS}; do
	build_library ${dir}
done
prepare_built_app_packages_dir
for dir in ${APP_DIRS}; do
	build_app ${dir}
done
