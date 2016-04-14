#!/usr/bin/env python

# Copyright 2016 Google Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

"""Creates a list of supported smart card reader descriptions in a format
suitable for using in the Chrome App manifest (as the value of the
$.permissions.[?(@.usbDevices)] JSON element - see
<https://developer.chrome.com/apps/app_usb#manifest> for reference).

The input data is expected to be in the format of the
readers/supported_readers.txt file from the original CCID bundle."""

import sys

SECTIONS_TO_PICK = ('supported', 'shouldwork')
SECTION_HEADER_PREFIX = '# section:'
COMMENT_PREFIX = '#'

def main():
    items = []
    ignored_item_count = 0
    current_section = None
    for line in sys.stdin:
        if not line.strip():
            continue
        if line.startswith(SECTION_HEADER_PREFIX):
            current_section = line[len(SECTION_HEADER_PREFIX):].strip()
        if line.startswith(COMMENT_PREFIX):
            continue
        parts = line.split(':')
        vendor_id = int(parts[0], 16)
        product_id = int(parts[1], 16)
        if current_section in SECTIONS_TO_PICK:
            items.append((vendor_id, product_id))
        else:
            ignored_item_count += 1

    print ',\n'.join(
        '        {{"vendorId": {0}, "productId": {1}}}'.format(
            vendor_id, product_id)
        for vendor_id, product_id in sorted(items))

    print >>sys.stderr, ('Extracted {0} supported USB devices and ignored {1} '
                         'items.'.format(len(items), ignored_item_count))

if __name__ == '__main__':
    main()
