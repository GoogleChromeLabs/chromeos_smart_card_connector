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


#
# This file contains some helper definitions for building Chrome
# Apps/Extensions.
#
# common.mk must be included before including this file.
#


#
# Special "run" target that starts a Chrome instance with a temporary profile
# and an App loaded from the out directory and running.
#
# For possible customization, refer to the documentation in the common.mk file,
# especially the following variables: CHROME_PATH, CHROME_ENV, CHROME_ARGS.
#
# The "run-nobuild" target is similar to the "run" target, but with the
# exception that it doesnt't perform the rebuilding and doesn't check for the
# changed files (and therefore it works much faster).
#

APP_RUN_USER_DATA_DIR_PATH := ./user-data-dir

APP_RUN_CHROME_ARGS := \
	--enable-nacl \
	--enable-pnacl \
	--no-default-browser-check \
	--no-first-run \
	--user-data-dir=$(APP_RUN_USER_DATA_DIR_PATH) \
	--load-and-launch-app=$(OUT_DIR_PATH) \

.PHONY: run run-nobuild

run: all
	$(CHROME_ENV) $(CHROME_PATH) $(APP_RUN_CHROME_ARGS) $(CHROME_ARGS)

run-nobuild:
	$(CHROME_ENV) $(CHROME_PATH) $(APP_RUN_CHROME_ARGS) $(CHROME_ARGS)

$(eval $(call CLEAN_RULE,$(APP_RUN_USER_DATA_DIR_PATH)))


# A "package" target that creates a .ZIP archive suitable for uploading at
# Chrome WebStore (see <https://developer.chrome.com/webstore/publish>).

.PHONY: package

$(TARGET)__webstore.zip: all
	@rm -f "$(TARGET)__webstore.zip"
	cd "$(OUT_DIR_PATH)" && zip -qr -9 -X "$(CURDIR)/$(TARGET)__webstore.zip" .

package: $(TARGET)__webstore.zip

$(eval $(call CLEAN_RULE,$(TARGET)__webstore.zip))


# A "package_crx" target that creates a packaged App/Extension .CRX file.
#
# The ID of the generated App/Extension packaged in the .CRX file is controlled
# by the private key .P8 file. The P8 file, if missing, is automatically
# generated with some random contents - so, in order to keep ID the same, the P8
# file should be preserved.

.PHONY: package_crx

$(TARGET).p8:
	@rm -f $(TARGET).p8
	openssl genpkey -out $(TARGET).p8 -algorithm RSA -pkeyopt rsa_keygen_bits:2048

$(TARGET).crx: all $(TARGET).p8
	@rm -f $(TARGET).crx
	xvfb-run \
		$(CHROME_ENV) $(CHROME_PATH) \
		--pack-extension="$(abspath $(OUT_DIR_PATH))" \
		--pack-extension-key="$(TARGET).p8"
	@mv $(OUT_DIR_PATH).crx $(TARGET).crx

package_crx: $(TARGET).crx

$(eval $(call CLEAN_RULE,$(TARGET).crx))
