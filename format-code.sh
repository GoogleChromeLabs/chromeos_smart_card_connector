#!/usr/bin/env bash

# Copyright 2020 Google Inc. All Rights Reserved.
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

# This script formats the modified C++/JS files via clang-format according to
# the Chromium Style Guides.
# Only files that are known to Git and belong to the diff of the "master" branch
# are considered.
#
# Note: Sorting of C/C++ #include's is currently disabled, since the standard
# algorithm reorders them incorrectly (e.g., it puts includes within our project
# into the same group as C system headers).

set -eu

# Go to the root directory of the repository.
cd $(dirname $(realpath ${0}))

# Find all relevant touched files. Bail out if nothing is found.
# Note: "d" in --diff-filter means excluding deleted files - otherwise
# clang-format fails on these non-existing files.
FILES=$(git diff --name-only --diff-filter=d master -- "*.cc" "*.h" "*.js")
[ "${FILES}" ] || exit 0

# Run clang-format on every found file.
echo "${FILES}" | \
  xargs clang-format -i --style=Chromium --sort-includes=false
