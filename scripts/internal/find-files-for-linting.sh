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

# This scripts returns paths to files that are manually edited by this
# repository's maintainers and hence need to undergo linting/reformatting/etc.
#
# Parameters: `find-files-for-linting.sh [glob-pattern]*`.

# Find all touched files tracked by Git. Bail out if nothing is found.
# Note: "d" in --diff-filter means excluding deleted files - they don't need to
# be linted.
git diff --name-only --diff-filter=d main -- "$@"
