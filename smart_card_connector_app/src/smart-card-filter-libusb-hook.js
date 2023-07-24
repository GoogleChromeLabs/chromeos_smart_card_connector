/**
 * @license
 * Copyright 2023 Google Inc.
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

goog.provide('GoogleSmartCard.SmartCardFilterLibusbHook');

goog.require('GoogleSmartCard.LibusbProxyDataModel');
goog.require('GoogleSmartCard.LibusbProxyHook');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;
const LibusbJsConfigurationDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsConfigurationDescriptor;
const LibusbJsInterfaceDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsInterfaceDescriptor;

/**
 * A hook that hides non-smart-card USB interfaces.
 */
GSC.SmartCardFilterLibusbHook = class extends GSC.LibusbProxyHook {
  /** @override */
  async getConfigurations(deviceId) {
    const originalConfigurations =
        await this.getDelegateOrThrow().getConfigurations(deviceId);
    return originalConfigurations.map(convertConfiguration);
  }
};

/**
 * @param {!LibusbJsConfigurationDescriptor} usbConfiguration
 * @return {!LibusbJsConfigurationDescriptor}
 */
function convertConfiguration(usbConfiguration) {
  const copy = /** @type {!LibusbJsConfigurationDescriptor} */ (
      goog.object.clone(usbConfiguration));
  copy['interfaces'] = copy['interfaces'].filter(isSmartCardInterface);
  return copy;
}

/**
 * @param {!LibusbJsInterfaceDescriptor} usbInterface
 * @return {boolean}
 */
function isSmartCardInterface(usbInterface) {
  // The implementation follows the checks in
  // third_party/ccid/src/src/ccid_usb.c. Note that these checks also allow for
  // vendor-specific class code, which is designed to support the (rare) smart
  // card readers that don't use the proper class code.
  const SMART_CARD_USB_INTERFACE_CLASS = 0x0B;
  const VENDOR_SPECIFIC_USB_INTERFACE_CLASS = 0xFF;
  const CCID_USB_DESCRIPTOR_LENGTH = 54;
  if (usbInterface['interfaceClass'] == SMART_CARD_USB_INTERFACE_CLASS) {
    return true;
  }
  if (usbInterface['interfaceClass'] == VENDOR_SPECIFIC_USB_INTERFACE_CLASS &&
      usbInterface['extraData'] &&
      usbInterface['extraData'].byteLength == CCID_USB_DESCRIPTOR_LENGTH) {
    return true;
  }
  return false;
}
});  // goog.scope
