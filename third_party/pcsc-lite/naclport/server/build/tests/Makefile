# Copyright 2022 Google Inc.
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

include ../../../../../../common/cpp_unit_test_runner/include.mk

SOURCES := \
  ../../src/google_smart_card_pcsc_lite_server/global_unittest.cc \

CXXFLAGS := \
  -I$(ROOT_PATH)/common/cpp/src \
  -pedantic \
  -Wall \
  -Werror \
  -Wextra \
  -std=$(CXX_DIALECT) \
  $(TEST_ADDITIONAL_CXXFLAGS) \

# The libraries are listed in the topological order: a library should only use
# symbols from subsequent ones.
LIBS := \
  google_smart_card_pcsc_lite_server \
  google_smart_card_ccid \
  google_smart_card_libusb \
  google_smart_card_common \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES) $(TEST_RUNNER_SOURCES),\
  $(LIBS) $(TEST_RUNNER_LIBS),$(TEST_RUNNER_DEPS),$(TEST_ADDITIONAL_LDFLAGS)))
