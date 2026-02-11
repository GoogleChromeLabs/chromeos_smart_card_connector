#!/usr/bin/env bash

# Copyright 2026 Google Inc. All Rights Reserved.
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

log_message() {
	local message=${1}

	echo -e "\033[33;32m******* ${message} *******\033[0m"
}

# Initializes NPM (i.e., creates package.json). The "--yes" flag is used to
# suppress interactive questions (about "package name", "version", etc.).
npm init --yes
# Not strictly necessary, but works as a smoke test that NPM works fine (and
# this package will be installed anyway when installing eslint later).
npm install js-yaml
log_message "npm was installed successfully."

# Initialize eslint
if npm list eslint >/dev/null; then
  log_message "eslint already present, skipping."
  return
fi
npm install eslint eslint-plugin-no-floating-promise
log_message "eslint was installed successfully."


scripts/eslint.py $1
log_message "eslint ran successfully."
