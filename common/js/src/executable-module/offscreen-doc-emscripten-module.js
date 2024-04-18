/**
 * @license
 * Copyright 2024 Google Inc. All Rights Reserved.
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

/**
 * @fileoverview This file provides a wrapper around the GSC.EmscriptenModule
 * class that hosts the Emscripten module in an Offscreen Document.
 *
 * This is useful for loading multi-threaded modules with background code in
 * manifest v3 extensions, since Workers aren't currently allowed by Chrome in
 * Service Workers. The GSC.OffscreenDocEmscriptenModule class is a facade that
 * lets the Service Worker use the convenient GSC.ExecutableModule interface,
 * meanwhile behind the scenes the module lives in an Offscreen Document.
 *
 * Note: it's expected that the emscripten-in-offscreen-doc-main-script.js is
 * compiled into a separate JavaScript file "emscripten-offscreen-doc.js", and
 * the "emscripten-offscreen-doc.html" file is packaged as well.
 */

goog.provide('GoogleSmartCard.OffscreenDocEmscriptenModule');

goog.require('GoogleSmartCard.DelayedMessageChannel');
goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.PortMessageChannelWaiter');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const GSC = GoogleSmartCard;

const OFFSCREEN_DOC_URL = '/emscripten-offscreen-doc.html';
// See
// <https://developer.chrome.com/docs/extensions/reference/api/offscreen#type-Reason>:
const OFFSCREEN_DOC_REASONS = ['WORKERS'];
const OFFSCREEN_DOC_JUSTIFICATION = 'running a WebAssembly module';
const URL_PARAM = 'google_smart_card_emscripten_module_name';
// The name of the port that the Offscreen Document should use for exchanging
// the module's messages with us.
const MESSAGING_PORT_NAME = 'offscreen_doc_emscripten_messages';
// The name of the port that the Offscreen Document should use for signaling the
// status.
const STATUS_PORT_NAME = 'offscreen_doc_emscripten_status';
// Status messages to be sent over the `STATUS_PORT_NAME` port.
const STATUS_LOADED = 'loaded';
const STATUS_DISPOSED = 'disposed';

GSC.OffscreenDocEmscriptenModule = class extends GSC.ExecutableModule {
  /**
   * @param {string} moduleName
   */
  constructor(moduleName) {
    super();
    /** @type {string} @const @private */
    this.moduleName_ = moduleName;
    /** @type {!goog.log.Logger} @const @private */
    this.logger_ = GSC.Logging.getScopedLogger('OffscreenDocEmscriptenModule');
    /** @type {boolean} @private */
    this.offscreenDocActive_ = false;
    /** @type {!GSC.DelayedMessageChannel} @const @private */
    this.messageChannel_ = new GSC.DelayedMessageChannel();
    /** @type {!GSC.PortMessageChannel|null} @private */
    this.statusChannel_ = null;
    /** @type {!goog.promise.Resolver<void>} @const @private */
    this.loadPromiseResolver_ = goog.Promise.withResolver();
    GSC.PromiseHelpers.suppressUnhandledRejectionError(
        this.loadPromiseResolver_.promise);
  }

  /** @override */
  getLogger() {
    return this.logger_;
  }

  /** @override */
  startLoading() {
    // Start the asynchronous load operation.
    this.load_();
  }

  /** @override */
  getLoadPromise() {
    return this.loadPromiseResolver_.promise;
  }

  /** @override */
  getMessageChannel() {
    return this.messageChannel_;
  }

  /** @override */
  disposeInternal() {
    if (this.offscreenDocActive_)
      chrome.offscreen.closeDocument();
    this.messageChannel_.dispose();
    if (this.statusChannel_)
      this.statusChannel_.dispose();
    this.loadPromiseResolver_.reject();
    super.disposeInternal();
  }

  /**
   * @returns {!Promise<void>}
   * @private
   */
  async load_() {
    // Listen for the incoming message channel from the Offscreen Document.
    const statusChannelWaiter =
        new GSC.PortMessageChannelWaiter(STATUS_PORT_NAME);

    // Prepare parameters for the Offscreen Document. The module name is passed
    // via URL query parameters.
    const url = new URL(OFFSCREEN_DOC_URL, globalThis.location.href);
    url.searchParams.append(URL_PARAM, this.moduleName_);
    const docParams = {
      'url': url.toString(),
      'reasons': OFFSCREEN_DOC_REASONS,
      'justification': OFFSCREEN_DOC_JUSTIFICATION,
    };

    // Load the Offscreen Document.
    await chrome.offscreen.createDocument(docParams);
    if (this.isDisposed()) {
      // If we got disposed of while waiting for the
      // `chrome.offscreen.createDocument()` completion, the logic in
      // `disposeInternal()` didn't have a chance to clean up the document.
      chrome.offscreen.closeDocument();
      throw new Error('Disposed');
    }
    this.offscreenDocActive_ = true;

    // Wait until the Offscreen Document opens the status message channel.
    const statusChannel = await statusChannelWaiter.getPromise();
    if (this.isDisposed()) {
      statusChannel.dispose();
      throw new Error('Disposed');
    }
    this.statusChannel_ = statusChannel;

    // Wait until the Offscreen Document signals us about the module's
    // loaded/disposed status (whichever comes first).
    await Promise.all([this.waitLoadedMessage_(), this.waitDisposedMessage_()]);

    // Connect pipes for exchanging messages with the executable module in the
    // Offscreen Document.
    const messageChannelWaiter =
        new GSC.PortMessageChannelWaiter(MESSAGING_PORT_NAME);
    this.messageChannel_.setUnderlyingChannel(
        await messageChannelWaiter.getPromise());
    this.messageChannel_.setReady();
  }

  /**
   * Returns a promise that gets resolved on receiving the "loaded" status
   * message from the Offscreen Document.
   * @return {!Promise<void>}
   * @private
   */
  async waitLoadedMessage_() {
    await this.waitMessage_(STATUS_LOADED);
  }

  /**
   * Returns a promise that gets rejected on receiving the "disposed" status
   * message from the Offscreen Document. Also disposes of `this`.
   * @return {!Promise<void>}
   * @private
   */
  async waitDisposedMessage_() {
    await this.waitMessage_(STATUS_DISPOSED);
    this.dispose();
    throw new Error('Disposed');
  }

  /**
   * @param {string} awaitedMessageType
   * @return {!Promise<void>}
   * @private
   */
  waitMessage_(awaitedMessageType) {
    return new Promise((resolve) => {
      this.statusChannel_.registerService(awaitedMessageType, () => {
        resolve();
      });
    });
  }
};

GSC.OffscreenDocEmscriptenModule.URL_PARAM = URL_PARAM;
GSC.OffscreenDocEmscriptenModule.MESSAGING_PORT_NAME = MESSAGING_PORT_NAME;
GSC.OffscreenDocEmscriptenModule.STATUS_PORT_NAME = STATUS_PORT_NAME;
GSC.OffscreenDocEmscriptenModule.STATUS_LOADED = STATUS_LOADED;
GSC.OffscreenDocEmscriptenModule.STATUS_DISPOSED = STATUS_DISPOSED;
});  // goog.scope
