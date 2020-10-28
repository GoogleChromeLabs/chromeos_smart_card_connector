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
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA


#
# Makefile for the libusb NaCl port library tests.
#
# The tests are executed through the separate tests runner, and therefore its
# makefile has to be used - with our specific options plugged into it through a
# bunch of make variables.
#


ADDITIONAL_TEST_LIBS_PREFIX := \
	google_smart_card_libusb \
	google_smart_card_common \

ADDITIONAL_TEST_LIBS_SUFFIX := \
	nacl_io \


ADDITIONAL_TEST_DEPS = \
	nacl_io \
	google_smart_card_libusb:.. \


TEST_SOURCES_PATH := ../../src

TEST_SOURCES := \
	$(TEST_SOURCES_PATH)/libusb_over_chrome_usb_unittest.cc \

ADDITIONAL_TEST_CPPFLAGS = \
	-I$(TEST_SOURCES_PATH) \
	-I$(NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH)/google_smart_card_libusb/libusb-1.0 \
	-I$(ROOT_PATH)/common/cpp/src \


include ../../../../../common/tests_runner/build.mk
