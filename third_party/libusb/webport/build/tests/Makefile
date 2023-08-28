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

# Makefile for the libusb NaCl port library tests.
#
# The makefile is based on the helper definitions provided by
# "cpp_unit_test_runner".

include ../../../../../common/cpp_unit_test_runner/include.mk

SOURCES_PATH := ../../src

SOURCES := \
	$(SOURCES_PATH)/libusb_js_proxy_unittest.cc \

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/third_party/libusb/src/libusb \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-std=$(CXX_DIALECT) \
	$(TEST_ADDITIONAL_CXXFLAGS) \

LIBS := \
	google_smart_card_libusb \
	google_smart_card_common \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES) $(TEST_RUNNER_SOURCES),\
	$(LIBS) $(TEST_RUNNER_LIBS),$(TEST_RUNNER_DEPS),$(TEST_ADDITIONAL_LDFLAGS)))
