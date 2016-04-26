/* Copyright © 2016 Google Inc.
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
 * Copyright © 2001 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <sys/time.h>

#include <libusb.h>

/**
 * This function was borrowed from libusb sources (file libusb/descriptor.c).
 */
int /*API_EXPORTED*/ libusb_get_string_descriptor_ascii(libusb_device_handle *dev,
    uint8_t desc_index, unsigned char *data, int length)
{
    unsigned char tbuf[255]; /* Some devices choke on size > 255 */
    int r, si, di;
    uint16_t langid;

    /* Asking for the zero'th index is special - it returns a string
     * descriptor that contains all the language IDs supported by the
     * device. Typically there aren't many - often only one. Language
     * IDs are 16 bit numbers, and they start at the third byte in the
     * descriptor. There's also no point in trying to read descriptor 0
     * with this function. See USB 2.0 specification section 9.6.7 for
     * more information.
     */

    if (desc_index == 0)
        return LIBUSB_ERROR_INVALID_PARAM;

    r = libusb_get_string_descriptor(dev, 0, 0, tbuf, sizeof(tbuf));
    if (r < 0)
        return r;

    if (r < 4)
        return LIBUSB_ERROR_IO;

    langid = tbuf[2] | (tbuf[3] << 8);

    r = libusb_get_string_descriptor(dev, desc_index, langid, tbuf,
        sizeof(tbuf));
    if (r < 0)
        return r;

    if (tbuf[1] != LIBUSB_DT_STRING)
        return LIBUSB_ERROR_IO;

    if (tbuf[0] > r)
        return LIBUSB_ERROR_IO;

    for (di = 0, si = 2; si < tbuf[0]; si += 2) {
        if (di >= (length - 1))
            break;

        if ((tbuf[si] & 0x80) || (tbuf[si + 1])) /* non-ASCII */
            data[di++] = '?';
        else
            data[di++] = tbuf[si];
    }

    data[di] = 0;
    return di;
}
