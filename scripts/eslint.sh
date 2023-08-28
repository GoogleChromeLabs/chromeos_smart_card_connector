#!/usr/bin/env bash

# Copyright 2023 Google Inc. All Rights Reserved.
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

# This scripts runs ESLint on all relevant JavaScript files in the repository.

set -eu

# Go to the root directory of the repository.
cd $(dirname $(realpath ${0}))/..

# Find all relevant touched files. Bail out if nothing is found.
FILES=$(scripts/internal/find-files-for-linting.sh "*.js")
[ "${FILES}" ] || exit 0

# Run ESLint on every found file.
# "--prefix" is used to specify the path where NPM packages are installed.
echo "${FILES}" | xargs npm exec --prefix env/ eslint
