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

goog.provide('GoogleSmartCard.LibusbProxyDataModel');

goog.scope(function() {

const GSC = GoogleSmartCard;

// The following definitions must match the ones in
// libusb_js_proxy_data_model.h.

/**
 * @typedef {{
 *            deviceId:number,
 *            vendorId:number,
 *            productId:number,
 *            version:(number|undefined),
 *            productName:(string|undefined),
 *            manufacturerName:(string|undefined),
 *            serialNumber:(string|undefined),
 *          }}
 */
GSC.LibusbProxyDataModel.LibusbJsDevice;

const LibusbJsDevice = GSC.LibusbProxyDataModel.LibusbJsDevice;

/**
 * @enum {string}
 */
GSC.LibusbProxyDataModel.LibusbJsDirection = {
  IN: 'in',
  OUT: 'out',
};

const LibusbJsDirection = GSC.LibusbProxyDataModel.LibusbJsDirection;

/**
 * @enum {string}
 */
GSC.LibusbProxyDataModel.LibusbJsEndpointType = {
  BULK: 'bulk',
  CONTROL: 'control',
  INTERRUPT: 'interrupt',
  ISOCHRONOUS: 'isochronous',
};

const LibusbJsEndpointType = GSC.LibusbProxyDataModel.LibusbJsEndpointType;

/**
 * @typedef {{
 *            endpointAddress:number,
 *            direction:!LibusbJsDirection,
 *            type:!LibusbJsEndpointType,
 *            extraData:(!ArrayBuffer|undefined),
 *            maxPacketSize:number
 *          }}
 */
GSC.LibusbProxyDataModel.LibusbJsEndpointDescriptor;

const LibusbJsEndpointDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsEndpointDescriptor;

/**
 * @typedef {{
 *            interfaceNumber:number,
 *            interfaceClass:number,
 *            interfaceSubclass:number,
 *            interfaceProtocol:number,
 *            extraData:(!ArrayBuffer|undefined),
 *            endpoints:!Array<!LibusbJsEndpointDescriptor>
 *          }}
 */
GSC.LibusbProxyDataModel.LibusbJsInterfaceDescriptor;

const LibusbJsInterfaceDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsInterfaceDescriptor;

/**
 * TODO(#429): Investigate remote_wakeup, self_powered, max_power flags.
 * @typedef {{
 *            active:boolean,
 *            configurationValue:number,
 *            extraData:(!ArrayBuffer|undefined),
 *            interfaces:!Array<!LibusbJsInterfaceDescriptor>
 *          }}
 */
GSC.LibusbProxyDataModel.LibusbJsConfigurationDescriptor;

const LibusbJsConfigurationDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsConfigurationDescriptor;
});  // goog.scope
