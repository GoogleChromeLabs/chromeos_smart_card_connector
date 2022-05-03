#!/usr/bin/env python3

# Copyright 2017 Google Inc. All Rights Reserved.
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

"""Builds the bundle of binaries for attaching them to a new Release of the
project at GitHub."""

import os
import shutil
import sys
import tarfile
import tempfile

# Name of the directory where the created binaries will be placed. The directory
# will be created in the same directory where this script is located.
TARGET_DIRECTORY_NAME = 'binaries-for-github-release'

# Base name (not including the extension) of the binary archive files to be
# created in the target directory.
BINARY_ARCHIVE_BASE_FILE_NAME = 'binaries'

# Paths to the files and directories to be copied into the resulting binary
# archives under the same relative path.
BINARY_CONTENTS_RELATIVES_PATHS = [
  'LICENSE',
  'built_app_packages/',
  'example_js_standalone_smart_card_client_library/README.rst',
  'example_js_standalone_smart_card_client_library/js_build/',
]

# Paths to the files and directories to be copied into the resulting binary
# archives, accompanied with the relative target file paths.
PATHS_TO_COPY = [
  ['example_js_standalone_smart_card_client_library/js_build/'
   'app_emscripten_Debug/google-smart-card-client-library.js',
   'google-smart-card-client-library.debug.js'],
  ['example_js_standalone_smart_card_client_library/js_build/'
   'app_emscripten_Release/google-smart-card-client-library.js',
   'google-smart-card-client-library.js'],
]

def main():
  repository_path = (os.path.relpath(os.path.dirname(__file__))
      if os.path.dirname(__file__) else '.')

  target_path = os.path.join(repository_path, TARGET_DIRECTORY_NAME)
  shutil.rmtree(target_path, ignore_errors=True)
  os.mkdir(target_path)

  for source_relative_path, target_relative_path in PATHS_TO_COPY:
    copy_file_or_dir(os.path.join(repository_path, source_relative_path),
                     os.path.join(target_path, target_relative_path))

  temp_path = tempfile.mkdtemp()
  try:
    for relative_path in BINARY_CONTENTS_RELATIVES_PATHS:
      copy_file_or_dir(os.path.join(repository_path, relative_path),
                       os.path.join(temp_path, relative_path))
    binary_archive_base_file_path = os.path.join(target_path,
                                                 BINARY_ARCHIVE_BASE_FILE_NAME)
    make_targz(temp_path, binary_archive_base_file_path)
    make_zip(temp_path, binary_archive_base_file_path)
  finally:
    shutil.rmtree(temp_path)

  sys.stderr.write('Successfully created resulting files at {0}\n'.format(
      target_path))

def mkdir_recursive(directory_path):
  if os.path.isdir(directory_path):
    return
  if os.path.exists(directory_path):
    raise RuntimeError('Cannot create directory "{0}": file already '
                       'exists'.format(directory_path))
  mkdir_recursive(os.path.dirname(directory_path))
  os.mkdir(directory_path)

def copy_file_or_dir(source_path, target_path):
  mkdir_recursive(os.path.dirname(os.path.normpath(target_path)))
  if os.path.isdir(source_path):
    shutil.copytree(source_path, target_path)
  else:
    shutil.copy2(source_path, target_path)

def make_targz(source_directory, target_base_file_path):
  with tarfile.open(target_base_file_path + '.tar.gz', 'w:gz') as tar:
    tar.add(source_directory, arcname='')

def make_zip(source_directory, target_base_file_path):
  shutil.make_archive(target_base_file_path, format='zip',
                      root_dir=source_directory)

if __name__ == '__main__':
  main()
