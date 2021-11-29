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
goog.provide('GoogleSmartCard.StubLibusbToJsApiAdaptor');

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

GSC.LibusbToJsApiAdaptor = class {
  /** @return {!Promise<!Array<!LibusbJsDevice>>} */
  async listDevices() {}

  /**
   * @param {number} deviceId
   * @return {!Promise<!Array<!LibusbJsConfigurationDescriptor>>}
   */
  async getConfigurations(deviceId) {}

  /**
   * @param {number} deviceId
   * @return {!Promise<number>} Device handle.
   */
  async openDeviceHandle(deviceId) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @return {!Promise<void>}
   */
  async closeDeviceHandle(deviceId, deviceHandle) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {number} interfaceNumber
   * @return {!Promise<void>}
   */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {number} interfaceNumber
   * @return {!Promise<void>}
   */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @return {!Promise<void>}
   */
  async resetDevice(deviceId, deviceHandle) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsControlTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async controlTransfer(deviceId, deviceHandle, parameters) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsGenericTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async bulkTransfer(deviceId, deviceHandle, parameters) {}

  /**
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsGenericTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async interruptTransfer(deviceId, deviceHandle, parameters) {}
};

GSC.StubLibusbToJsApiAdaptor = class extends GSC.LibusbToJsApiAdaptor {
  /** @override */
  async listDevices() {
    return this.callStub_();
  }

  /** @override */
  async getConfigurations(deviceId) {
    return this.callStub_();
  }

  /** @override */
  async openDeviceHandle(deviceId) {
    return this.callStub_();
  }

  /** @override */
  async closeDeviceHandle(deviceId, deviceHandle) {
    return this.callStub_();
  }

  /** @override */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {
    return this.callStub_();
  }

  /** @override */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {
    return this.callStub_();
  }

  /** @override */
  async resetDevice(deviceId, deviceHandle) {
    return this.callStub_();
  }

  /** @override */
  async controlTransfer(deviceId, deviceHandle, parameters) {
    return this.callStub_();
  }

  /** @override */
  async bulkTransfer(deviceId, deviceHandle, parameters) {
    return this.callStub_();
  }

  /** @override */
  async interruptTransfer(deviceId, deviceHandle, parameters) {
    return this.callStub_();
  }

  /** @private */
  callStub_() {
    throw new Error('API unavailable');
  }
};
});  // goog.scope
