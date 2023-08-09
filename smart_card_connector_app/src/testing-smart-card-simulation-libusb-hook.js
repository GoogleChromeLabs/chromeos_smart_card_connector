/**
 * @license
 * Copyright 2023 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

goog.provide('GoogleSmartCard.TestingLibusbSmartCardSimulationHook');

goog.require('GoogleSmartCard.LibusbProxyHook');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('goog.Promise');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Hook that intercepts all Libusb requests sent by the executable module and
 * redirects them to the TestingSmartCardSimulation back in the module.
 */
GSC.TestingLibusbSmartCardSimulationHook = class extends GSC.LibusbProxyHook {
  constructor(executableModuleMessageChannel) {
    super();
    this.executableModuleRequester_ = new GSC.Requester(
        'testing_smart_card_simulation', executableModuleMessageChannel);
  }

  /** @override */
  async listDevices() {
    return await this.callCpp_('listDevices', []);
  }

  /** @override */
  async getConfigurations(deviceId) {
    return await this.callCpp_('getConfigurations', [deviceId]);
  }

  /** @override */
  async openDeviceHandle(deviceId) {
    return await this.callCpp_('openDeviceHandle', [deviceId]);
  }

  /** @override */
  async closeDeviceHandle(deviceId, deviceHandle) {
    return await this.callCpp_('closeDeviceHandle', [deviceId, deviceHandle]);
  }

  /** @override */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {
    return await this.callCpp_(
        'claimInterface', [deviceId, deviceHandle, interfaceNumber]);
  }

  /** @override */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {
    return await this.callCpp_(
        'releaseInterface', [deviceId, deviceHandle, interfaceNumber]);
  }

  /** @override */
  async resetDevice(deviceId, deviceHandle) {
    return await this.callCpp_('resetDevice', [deviceId, deviceHandle]);
  }

  /** @override */
  async controlTransfer(deviceId, deviceHandle, parameters) {
    return await this.callCpp_(
        'controlTransfer', [deviceId, deviceHandle, parameters]);
  }

  /** @override */
  async bulkTransfer(deviceId, deviceHandle, parameters) {
    return await this.callCpp_(
        'bulkTransfer', [deviceId, deviceHandle, parameters]);
  }

  /** @override */
  async interruptTransfer(deviceId, deviceHandle, parameters) {
    return await this.callCpp_(
        'interruptTransfer', [deviceId, deviceHandle, parameters]);
  }

  /**
   * Sends a remote call request to the executable module and returns its
   * response via a promise.
   * @param {string} functionName
   * @param {!Array.<*>} functionArguments
   * @return {!Promise}
   */
  async callCpp_(functionName, functionArguments) {
    const remoteCallMessage =
        new GSC.RemoteCallMessage(functionName, functionArguments);
    const outputs = await this.executableModuleRequester_.postRequest(
        remoteCallMessage.makeRequestPayload());
    GSC.Logging.check(outputs.length == 1);
    return outputs[0];
  }
}
});
