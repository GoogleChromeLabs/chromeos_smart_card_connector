#!/bin/sh
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export TEST_BUILDBOT=1
export NACLPORTS_NO_UPLOAD=1
export BUILDBOT_BUILDERNAME=naclports-linux-clang-0
# BUILDBOT_REVISION is used in the foldername when
# publishing data to Google Cloud Storage.  Set to a unique
# value for testing purposes.
export BUILDBOT_REVISION=9111111

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
${SCRIPT_DIR}/buildbot_selector.sh
