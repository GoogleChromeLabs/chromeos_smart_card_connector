#!/usr/bin/env python
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

"""Downloads from GitHub the latest released version of the client library for
communicating to the Google Smart Card Connector app."""

import json
import os
import sys

try:
  # Python 3+.
  import urllib.request
  urlopen = urllib.request.urlopen
except ImportError:
  # Python 2.
  import urllib2
  urlopen = urllib2.urlopen

GITHUB_REPO_OWNER = "GoogleChrome"
GITHUB_REPO = "chromeos_smart_card_connector"
CLIENT_LIBRARY_ASSET_NAME = "google-smart-card-client-library.js"
OUTPUT_FILE_NAME = "google-smart-card-client-library.js"

GITHUB_LATEST_RELEASE_URL_TEMPLATE = \
    "https://api.github.com/repos/{owner}/{repo}/releases/latest"

def main():
  sys.stderr.write('Accessing GitHub API...\n')
  latest_release_url = GITHUB_LATEST_RELEASE_URL_TEMPLATE.format(
      owner=GITHUB_REPO_OWNER, repo=GITHUB_REPO)
  latest_release_info = json.load(urlopen(latest_release_url))

  client_library_download_url = None
  for asset in latest_release_info.get("assets", []):
    if asset["name"] == CLIENT_LIBRARY_ASSET_NAME:
      client_library_download_url = asset["browser_download_url"]
  if client_library_download_url is None:
    raise RuntimeError("Asset with the client library not found in the latest "
                       "GitHub release")

  sys.stderr.write('Downloading from "{0}"...\n'.format(
      client_library_download_url))
  client_library = urlopen(client_library_download_url).read()

  if os.path.dirname(__file__):
    output_file_path = os.path.join(
        os.path.relpath(os.path.dirname(__file__)), OUTPUT_FILE_NAME)
  else:
    output_file_path = OUTPUT_FILE_NAME
  with open(output_file_path, 'wb') as f:
    f.write(client_library)

  sys.stderr.write(
      'Successfully finished. The library is stored at "{0}".\n'.format(
          output_file_path))

if __name__ == '__main__':
  main()
