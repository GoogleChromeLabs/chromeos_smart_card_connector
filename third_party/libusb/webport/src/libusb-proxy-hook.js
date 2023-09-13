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

goog.provide('GoogleSmartCard.LibusbProxyHook');

goog.require('GoogleSmartCard.LibusbToChromeUsbAdaptor');

goog.scope(function() {

const GSC = GoogleSmartCard;

GSC.LibusbProxyHook = class extends GSC.LibusbToJsApiAdaptor {
  constructor() {
    super();
    /** @type {!GSC.LibusbToJsApiAdaptor|null} */
    this.delegate_ = null;
  }

  /**
   * Sets the delegate, which can be used by the hook to invoke the original
   * request handling implementation.
   * @param {!GSC.LibusbToJsApiAdaptor} delegate
   */
  setDelegate(delegate) {
    this.delegate_ = delegate;
  }

  /**
   * @return {!GSC.LibusbToJsApiAdaptor|null}
   */
  getDelegate() {
    return this.delegate_;
  }

  /**
   * @return {!GSC.LibusbToJsApiAdaptor}
   */
  getDelegateOrThrow() {
    if (!this.delegate_)
      throw new Error('Delegate must be set for libusb proxy hook');
    return this.delegate_;
  }

  /** @override */
  async listDevices() {
    return await this.getDelegateOrThrow().listDevices();
  }

  /** @override */
  async getConfigurations(deviceId) {
    return await this.getDelegateOrThrow().getConfigurations(deviceId);
  }

  /** @override */
  async openDeviceHandle(deviceId) {
    return await this.getDelegateOrThrow().openDeviceHandle(deviceId);
  }

  /** @override */
  async closeDeviceHandle(deviceId, deviceHandle) {
    return await this.getDelegateOrThrow().closeDeviceHandle(
        deviceId, deviceHandle);
  }

  /** @override */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {
    return await this.getDelegateOrThrow().claimInterface(
        deviceId, deviceHandle, interfaceNumber);
  }

  /** @override */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {
    return await this.getDelegateOrThrow().releaseInterface(
        deviceId, deviceHandle, interfaceNumber);
  }

  /** @override */
  async resetDevice(deviceId, deviceHandle) {
    return await this.getDelegateOrThrow().resetDevice(deviceId, deviceHandle);
  }

  /** @override */
  async controlTransfer(deviceId, deviceHandle, parameters) {
    return await this.getDelegateOrThrow().controlTransfer(
        deviceId, deviceHandle, parameters);
  }

  /** @override */
  async bulkTransfer(deviceId, deviceHandle, parameters) {
    return await this.getDelegateOrThrow().bulkTransfer(
        deviceId, deviceHandle, parameters);
  }

  /** @override */
  async interruptTransfer(deviceId, deviceHandle, parameters) {
    return await this.getDelegateOrThrow().interruptTransfer(
        deviceId, deviceHandle, parameters);
  }
};
});  // goog.scope
