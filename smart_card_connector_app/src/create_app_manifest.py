#!/usr/bin/env python

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

"""Builds the Connector app's manifest file."""

import argparse
import string
import sys

# List of section titles that should be picked up for forming the USB devices
# list in the app's manifest file.
CCID_SUPPORTED_READERS_CONFIG_SECTIONS_TO_PICK = ('supported', 'shouldwork')

def load_usb_devices(ccid_supported_readers_file):
  """Parses the specified config containing descriptions of the readers
  supported by CCID.

  Only readers from the sections listed in the
  CCID_SUPPORTED_READERS_CONFIG_SECTIONS_TO_PICK are picked up.

  Args:
    ccid_supported_readers_file: The opened config file containing descriptions
        of the readers supported by CCID.

  Returns list of tuples: (vendor_id, product_id), where vendor_id and
  product_id are integers.
  """

  # The CCID supported readers config file format is the following.
  #
  # There are several sections, each of which starts with a comment block
  # containing the title of the section (e.g. "supported"), followed by a number
  # of lines containing reader descriptions.
  #
  # The reader description line has the following format:
  # "vendor_id:product_id:name", where vendor_id and product_id are hexadecimal
  # integers.
  #
  # Empty lines should be ignored.
  #
  # Comment lines, which start from the hash character, should also be ignored
  # with the only exception of special-formatted comments containing section
  # titles.

  # Prefix marker used for determining section titles.
  SECTION_HEADER_PREFIX = '# section:'
  # Prefix marker used for determining comment lines.
  COMMENT_PREFIX = '#'

  usb_devices = []
  current_section = None
  ignored_usb_device_count = 0
  for line in ccid_supported_readers_file:
    if not line.strip():
      # Ignore empty line
      continue
    if line.startswith(SECTION_HEADER_PREFIX):
      # Parse section title line
      current_section = line[len(SECTION_HEADER_PREFIX):].strip()
      if not current_section:
        raise RuntimeError('Failed to extract section title from the CCID '
                           'supported readers config')
      continue
    if line.startswith(COMMENT_PREFIX):
      # Ignore comment line
      continue
    # Parse reader description line
    parts = line.split(':', 2)
    if len(parts) != 3:
      raise RuntimeError('Failed to parse the reader description from the CCID '
                         'supported readers config: "{0}"'.format(line))
    vendor_id = int(parts[0], base=16)
    product_id = int(parts[1], base=16)
    if current_section is None:
      raise RuntimeError('Unexpected reader definition met in the CCID '
                         'supported readers config before any section title')
    if current_section in CCID_SUPPORTED_READERS_CONFIG_SECTIONS_TO_PICK:
      usb_devices.append((vendor_id, product_id))
    else:
      ignored_usb_device_count += 1

  if not usb_devices:
    raise RuntimeError('No supported USB devices were extracted from the CCID '
                       'supported readers config')
  print >>sys.stderr, ('Extracted {0} supported USB devices from the CCID '
                       'supported readers config, and ignored {1} '
                       'items.'.format(
                           len(usb_devices), ignored_usb_device_count))

  return usb_devices

def build_manifest(manifest_template_file, ccid_supported_readers_file):
  usb_devices = load_usb_devices(ccid_supported_readers_file)
  formatted_usb_devices = ',\n'.join(
      '        {{"vendorId": {0}, "productId": {1}}}'.format(
          vendor_id, product_id)
      for vendor_id, product_id in sorted(usb_devices))

  manifest_template = manifest_template_file.read()
  return string.Template(manifest_template).substitute(
      usb_devices=formatted_usb_devices)

def main():
  args_parser = argparse.ArgumentParser(
      description="Build the Connector app's manifest file.")
  args_parser.add_argument(
      '--manifest-template-path',
      type=argparse.FileType('r'),
      required=True,
      metavar='"path/to/manifest_template_file"',
      dest='manifest_template_file')
  args_parser.add_argument(
      '--ccid-supported-readers-config-path',
      type=argparse.FileType('r'),
      required=True,
      metavar='"path/to/ccid_supported_readers_file"',
      dest='ccid_supported_readers_file')
  args_parser.add_argument(
      '--target-file-path',
      type=argparse.FileType('w'),
      required=True,
      metavar='"path/to/target_file"',
      dest='target_file')
  args = args_parser.parse_args()

  manifest = build_manifest(
      args.manifest_template_file, args.ccid_supported_readers_file)
  args.target_file.write(manifest)

if __name__ == '__main__':
  main()
