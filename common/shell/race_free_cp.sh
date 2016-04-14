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
# Race-free version of "cp" utility.
#
# Arguments:
#    $1: Path to the source file.
#    $2: Path to the destination file.
#    $3: (optional) If specified and non-empty, then creates the parent
#        directories too.
#


set -eu

SCRIPTPATH=$(dirname $(realpath ${0}))

DIRPATH=$(dirname ${2})
if [[ ${3-} ]]; then
  ${SCRIPTPATH}/race_free_mkdir.sh ${DIRPATH} ${3}
fi

RANDOM_FILE="$(mktemp -t google_smart_card.XXXXXX)"

cp -p ${1} ${RANDOM_FILE}

if ! mv -f ${RANDOM_FILE} ${2} 2> /dev/null ; then
  if [ ! -f ${2} ]; then
    echo "race_free_cp.sh failed: couldn't create the destination file (\"${2}\")" && exit 1
  fi
  if ! cmp --silent ${1} ${2} > /dev/null 2> /dev/null ; then
    echo "race_free_cp.sh failed: the destination file (\"${2}\") contents are different from the source file (\"${1}\") contents" && exit 1
  fi
fi
