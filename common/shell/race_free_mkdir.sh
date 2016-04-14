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

#
# Race-free version of "mkdir" utility.
#
# Arguments:
#    $1: Path to the resulting directory to be created.
#    $2: (optional) If non-empty, then creates the parent directories too.
#


set -eu

create_dir() {
  mkdir ${1} 2> /dev/null || true
  if [ ! -d ${1} ]; then
    echo "race_free_mkdir.sh failed: couldn't create \"${1}\"" && exit 1
  fi
}

create_dir_recursively() {
  if [[ ( ${1} != "" ) && ( ${1} != "/" ) && ( ${1} != "." ) ]]; then
    create_dir_recursively $(dirname ${1})
    create_dir ${1}
  fi
}

if [ ${2-} ]; then
  create_dir_recursively ${1}
else
  create_dir ${1}
fi
