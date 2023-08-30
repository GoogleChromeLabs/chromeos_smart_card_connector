# Copyright 2022 Google Inc. All Rights Reserved.
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

# This makefile builds C++ unit tests from the Smart Card Connector's code.
#
# The makefile is based on the helper definitions provided by
# "cpp_unit_test_runner".

include ../../../../common/cpp_unit_test_runner/include.mk

SOURCES := \
	../../../src/application_unittest.cc \
	../../../src/testing_smart_card_simulation.cc \

# Explanation:
# -Wno-zero-length-array: Suppress warnings in the third-party reader.h file.
CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/third_party/pcsc-lite/src/src/PCSC \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-std=$(CXX_DIALECT) \
	$(TEST_ADDITIONAL_CXXFLAGS) \

# The libraries are listed in the topological order: a library should only use
# symbols from subsequent ones.
LIBS := \
  smart_card_connector_application \
  google_smart_card_pcsc_lite_server_clients_management \
  google_smart_card_pcsc_lite_server \
  google_smart_card_pcsc_lite_common \
  google_smart_card_ccid \
  google_smart_card_libusb \
  google_smart_card_common \

# Package resource files that the tested code needs at runtime:
$(eval $(call ADD_RESOURCE_RULE, \
	$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file, \
	executable-module-filesystem/pcsc/fake_socket_file))
$(eval $(call ADD_RESOURCE_RULE, \
	$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist, \
	executable-module-filesystem/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist))

# Rules for compiling the test source files and linking them into a resulting
# executable binary.
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))
$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES) $(TEST_RUNNER_SOURCES),\
	$(LIBS) $(TEST_RUNNER_LIBS),$(TEST_RUNNER_DEPS),$(TEST_ADDITIONAL_LDFLAGS)))
