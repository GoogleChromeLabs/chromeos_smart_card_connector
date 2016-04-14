/* Copyright © 2016 Google Inc.
 * Copyright © 2012-2013 Nathan Hjelm <hjelmn@cs.unm.edu>
 * Copyright © 2007-2008 Daniel Drake <dsd@gentoo.org>
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

#include "libusb.h"

#include "config.h"

/**
 * This function was borrowed from libusb sources (file libusb/core.c).
 */
DEFAULT_VISIBILITY const char * LIBUSB_CALL libusb_error_name(int error_code)
{
    switch (error_code) {
    case LIBUSB_ERROR_IO:
        return "LIBUSB_ERROR_IO";
    case LIBUSB_ERROR_INVALID_PARAM:
        return "LIBUSB_ERROR_INVALID_PARAM";
    case LIBUSB_ERROR_ACCESS:
        return "LIBUSB_ERROR_ACCESS";
    case LIBUSB_ERROR_NO_DEVICE:
        return "LIBUSB_ERROR_NO_DEVICE";
    case LIBUSB_ERROR_NOT_FOUND:
        return "LIBUSB_ERROR_NOT_FOUND";
    case LIBUSB_ERROR_BUSY:
        return "LIBUSB_ERROR_BUSY";
    case LIBUSB_ERROR_TIMEOUT:
        return "LIBUSB_ERROR_TIMEOUT";
    case LIBUSB_ERROR_OVERFLOW:
        return "LIBUSB_ERROR_OVERFLOW";
    case LIBUSB_ERROR_PIPE:
        return "LIBUSB_ERROR_PIPE";
    case LIBUSB_ERROR_INTERRUPTED:
        return "LIBUSB_ERROR_INTERRUPTED";
    case LIBUSB_ERROR_NO_MEM:
        return "LIBUSB_ERROR_NO_MEM";
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return "LIBUSB_ERROR_NOT_SUPPORTED";
    case LIBUSB_ERROR_OTHER:
        return "LIBUSB_ERROR_OTHER";

    case LIBUSB_TRANSFER_ERROR:
        return "LIBUSB_TRANSFER_ERROR";
    case LIBUSB_TRANSFER_TIMED_OUT:
        return "LIBUSB_TRANSFER_TIMED_OUT";
    case LIBUSB_TRANSFER_CANCELLED:
        return "LIBUSB_TRANSFER_CANCELLED";
    case LIBUSB_TRANSFER_STALL:
        return "LIBUSB_TRANSFER_STALL";
    case LIBUSB_TRANSFER_NO_DEVICE:
        return "LIBUSB_TRANSFER_NO_DEVICE";
    case LIBUSB_TRANSFER_OVERFLOW:
        return "LIBUSB_TRANSFER_OVERFLOW";

    case 0:
        return "LIBUSB_SUCCESS / LIBUSB_TRANSFER_COMPLETED";
    default:
        return "**UNKNOWN**";
    }
}
