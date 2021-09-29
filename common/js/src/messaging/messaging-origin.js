/**
 * @license
 * Copyright 2021 Google Inc. All Rights Reserved.
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

goog.provide('GoogleSmartCard.MessagingOrigin');

goog.require('GoogleSmartCard.ExtensionId');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.Uri');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @const */
GSC.MessagingOrigin.EXTENSION_PROTOCOL = 'chrome-extension:';

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('MessagingOrigin');

/**
 * Returns an origin that identifies the sender of messages, given the
 * MessageSender object from the Chrome Extensions API. An example of the origin
 * is: "https://example.org". Returns null when the origin cannot be identified.
 * @param {!MessageSender|undefined} messageSender
 * @return {string|null}
 */
GSC.MessagingOrigin.getFromChromeMessageSender = function(messageSender) {
  if (!messageSender)
    return null;
  if (messageSender.id) {
    // The sender is a Chrome extension/app.
    return GSC.MessagingOrigin.getFromExtensionId(messageSender.id);
  }
  if (messageSender.url) {
    // The sender is a web page. We're parsing the origin from the "url"
    // property instead of reading the "origin" property, since in case the
    // sender is in an iframe we want the iframe's URL.
    let url;
    try {
      url = new URL(messageSender.url);
    } catch (exc) {
      // Invalid URLs should never occur in the Chrome's MessageSender fields,
      // but handle this to be on the safe side.
      return null;
    }
    return url.origin;
  }
  // Unknown sender.
  return null;
};

/**
 * Synthesizes an origin that corresponds to the given extension/app ID.
 * @param {string} extensionId
 * @return {string}
 */
GSC.MessagingOrigin.getFromExtensionId = function(extensionId) {
  GSC.Logging.checkWithLogger(
      logger, GSC.ExtensionId.looksLikeExtensionId(extensionId));
  return `${GSC.MessagingOrigin.EXTENSION_PROTOCOL}//${extensionId}`;
};

/**
 * Extracts the extension ID from the origin. Returns null when the origin
 * doesn't refer to an extension/app.
 * @param {string} messagingOrigin
 * @return {string|null}
 */
GSC.MessagingOrigin.extractExtensionId = function(messagingOrigin) {
  let url;
  try {
    url = new URL(messagingOrigin);
  } catch (exc) {
    // Invalid origin.
    return null;
  }
  if (url.protocol !== GSC.MessagingOrigin.EXTENSION_PROTOCOL) {
    // Origin doesn't refer to an extension/app.
    return null;
  }
  return url.hostname;
};
});  // goog.scope
