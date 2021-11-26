/**
 * @license
 * Copyright 2021 Google Inc.
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

goog.provide('GoogleSmartCard.LibusbToJsApiAdaptor');

goog.require('GoogleSmartCard.LibusbProxyDataModel');

goog.scope(function() {

const GSC = GoogleSmartCard;
const LibusbJsConfigurationDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsConfigurationDescriptor;
const LibusbJsControlTransferParameters =
    GSC.LibusbProxyDataModel.LibusbJsControlTransferParameters;
const LibusbJsDevice = GSC.LibusbProxyDataModel.LibusbJsDevice;
const LibusbJsGenericTransferParameters =
    GSC.LibusbProxyDataModel.LibusbJsGenericTransferParameters;
const LibusbJsTransferResult = GSC.LibusbProxyDataModel.LibusbJsTransferResult;

/** @abstract */
GSC.LibusbToJsApiAdaptor = class {
  /**
   * @abstract
   * @return {!Promise<!Array<!LibusbJsDevice>>}
   */
  async listDevices() {}

  /**
   * @abstract
   * @param {number} deviceId
   * @return {!Promise<!Array<!LibusbJsConfigurationDescriptor>>}
   */
  async getConfigurations(deviceId) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @return {!Promise<number>} Device handle.
   */
  async openDeviceHandle(deviceId) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @return {!Promise<void>}
   */
  async closeDeviceHandle(deviceId, deviceHandle) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {number} interfaceNumber
   * @return {!Promise<void>}
   */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {number} interfaceNumber
   * @return {!Promise<void>}
   */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @return {!Promise<void>}
   */
  async resetDevice(deviceId, deviceHandle) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsControlTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async controlTransfer(deviceId, deviceHandle, parameters) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsGenericTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async bulkTransfer(deviceId, deviceHandle, parameters) {}

  /**
   * @abstract
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsGenericTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async interruptTransfer(deviceId, deviceHandle, parameters) {}
};
});  // goog.scope
