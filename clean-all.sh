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
# of the libraries and Apps of this project, and clears the env.
#
# Note: the env/activate file must be source'd into the shell before this script
# execution.


set -eu


DIRS="
	common/cpp/build
	third_party/libusb/naclport/build
	third_party/ccid/naclport/build
	third_party/pcsc-lite/naclport/common/build
	third_party/pcsc-lite/naclport/server/build
	third_party/pcsc-lite/naclport/server_clients_management/build
	third_party/pcsc-lite/naclport/cpp_demo/build
	smart_card_connector_app/build
	example_js_smart_card_client_app/build
	third_party/pcsc-lite/naclport/cpp_client/build
	example_cpp_smart_card_client_app/build
	example_js_standalone_smart_card_client_library"

BUILT_APP_PACKAGES_DIR="built_app_packages"

CONFIGS="Release Debug"


log_message() {
	local message=${1}

	echo -e "\033[33;32m${message}\033[0m"
}

clean_dir_with_all_configs() {
	local dir=${1}

	log_message "Cleaning \"${dir}\"..."
	for config in ${CONFIGS}; do
		CONFIG=${config} make -C ${dir} clean || true
	done
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

for dir in ${DIRS}; do
	clean_dir_with_all_configs ${dir}
done
clean_built_app_packages
clean_env

log_message "Cleaning finished."
