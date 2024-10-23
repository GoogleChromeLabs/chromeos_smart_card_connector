#!/bin/bash

# fail on first error
set -e

BUILD_DIR="build"
rm -rf "$BUILD_DIR"
meson setup "$BUILD_DIR"
cd "$BUILD_DIR"
meson compile
meson install

