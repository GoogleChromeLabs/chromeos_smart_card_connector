#!/bin/bash
# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit
set -x

readonly EMSCRIPTEN_TAG='1.34.0'

readonly BUILDTOOLS_DIR=$(cd "$(dirname "${BASH_SOURCE}")" ; pwd)
readonly SRC_DIR=$(dirname "${BUILDTOOLS_DIR}")
readonly OUT_DIR=${SRC_DIR}/out
readonly EMSDK_DIR=${OUT_DIR}/emsdk

if [[ -z ${CHROME_ROOT:-} ]]; then
  echo "Set CHROME_ROOT to your Chrome repo root"
  exit 1
fi

readonly LLVM_DIR=${CHROME_ROOT}/third_party/llvm-build/Release+Asserts/bin

OS_JOBS=$(nproc)

mkdir -p ${EMSDK_DIR}

cd ${OUT_DIR}
# Instructions from
# https://kripken.github.io/emscripten-site/docs/building_from_source/building_fastcomp_manually_from_source.html#building-fastcomp-from-source
mkdir -p fastcomp
cd fastcomp
git clone --depth 1 https://github.com/kripken/emscripten-fastcomp \
    --branch ${EMSCRIPTEN_TAG}
cd emscripten-fastcomp
git clone --depth 1 https://github.com/kripken/emscripten-fastcomp-clang \
    tools/clang --branch ${EMSCRIPTEN_TAG}
mkdir -p build
cd build
# Build using Chrome's clang
CC=${LLVM_DIR}/clang CXX=${LLVM_DIR}/clang++ cmake .. \
  -DCMAKE_INSTALL_PREFIX=${EMSDK_DIR}/fastcomp-install \
  -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86;JSBackend" \
  -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF \
  -DCLANG_INCLUDE_EXAMPLES=OFF -DCLANG_INCLUDE_TESTS=OFF
make -j${OS_JOBS}
make install

# Copy Chrome clang's libstdc++ into the lib dir
cp ${LLVM_DIR}/../lib/libstdc++.so.6 ${EMSDK_DIR}/fastcomp-install/lib

# Instructions from
# https://kripken.github.io/emscripten-site/docs/building_from_source/building_emscripten_from_source_on_linux.html
cd ${EMSDK_DIR}
git clone --depth 1 https://github.com/kripken/emscripten.git \
    --branch ${EMSCRIPTEN_TAG}

# Write .emscripten file. See
# https://kripken.github.io/emscripten-site/docs/tools_reference/emsdk.html#compiler-configuration-file
cat > ${EMSDK_DIR}/.emscripten <<HERE
import os

# Emscripten loads the contents of this file and runs it from tools/shared.py
TOOLS_DIR=os.path.abspath(os.path.dirname(__file__))
EMSCRIPTEN_ROOT=os.path.dirname(TOOLS_DIR)
EMSDK_ROOT=os.path.dirname(EMSCRIPTEN_ROOT)
LLVM_ROOT=os.path.join(EMSDK_ROOT, 'fastcomp-install', 'bin')

SPIDERMONKEY_ENGINE = ''
NODE_JS = 'node'
V8_ENGINE = ''
TEMP_DIR = '/tmp'
COMPILER_ENGINE = NODE_JS
JS_ENGINES = [NODE_JS]
HERE

# Write tarball
cd ${OUT_DIR}
tar czf emsdk-$(date +%Y%m%d).tar.gz emsdk
