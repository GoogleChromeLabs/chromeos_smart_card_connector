#!/bin/bash
# Copyright 2020 Google Inc. All Rights Reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Script that can be used by CI server for testing JsCompiler builds.
set -e

# Avoid a "Android SDK api level 30 was requested but it is not installed" error
# that happens when pulling in https://github.com/google/bazel-common
unset ANDROID_HOME

bazel build :all

# TODO: Run other tests needed for open source verification
