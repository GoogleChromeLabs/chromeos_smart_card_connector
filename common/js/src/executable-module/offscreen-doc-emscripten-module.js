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

goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PortMessageChannel');
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
    /** @type {!OffscreenDocMessageChannel} @const @private */
    this.messageChannel_ = new OffscreenDocMessageChannel();
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
    // Listen for incoming message channels from the Offscreen Document once
    // it's loaded.
    chrome.runtime.onConnect.addListener((port) => this.onConnect_(port));

    // Load the Offscreen Document. The loading arguments are passed via URL
    // query parameters.
    const url = new URL(OFFSCREEN_DOC_URL, globalThis.location.href);
    url.searchParams.append(URL_PARAM, this.moduleName_);
    chrome.offscreen.createDocument(
        {
          'url': url.toString(),
          'reasons': OFFSCREEN_DOC_REASONS,
          'justification': OFFSCREEN_DOC_JUSTIFICATION,
        },
        () => this.onOffscreenDocCreated_());
    // Once the document is loaded, it'll immediately start loading the
    // Emscripten module and report the status to us via m
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

  /** @private */
  onOffscreenDocCreated_() {
    if (chrome && chrome.runtime && chrome.runtime.lastError) {
      const error =
          goog.object.get(chrome.runtime.lastError, 'message', 'Unknown error');
      goog.log.warning(
          this.logger_, `Failed to load the offscreen document: ${error}`);
    } else {
      this.offscreenDocActive_ = true;
    }

    if (this.offscreenDocActive_ && this.isDisposed()) {
      // Edge case: we got destroyed while waiting for the
      // `chrome.offscreen.createDocument()` completion; make sure to not leave
      // the doc in that case as well.
      chrome.offscreen.closeDocument();
    } else if (!this.offscreenDocActive_) {
      this.dispose();
    }
  }

  /**
   * @param {!Port} port
   * @private
   */
  onConnect_(port) {
    if (this.isDisposed())
      return;
    if (port.name === MESSAGING_PORT_NAME) {
      // This connection is for exchanging messages with the executable module
      // in the Offscreen Document.
      this.messageChannel_.resolve(new GSC.PortMessageChannel(port));
    } else if (port.name === STATUS_PORT_NAME) {
      // This connection is for signaling the module's status.
      this.statusChannel_ = new GSC.PortMessageChannel(port);
      this.statusChannel_.registerService(
          STATUS_LOADED, () => this.onLoadedInOffscreen_());
      this.statusChannel_.registerService(
          STATUS_DISPOSED, () => this.onDisposedInOffscreen_());
    } else {
      // Ignore - this is a connection originating from some other component.
    }
  }

  /** @private */
  onLoadedInOffscreen_() {
    this.loadPromiseResolver_.resolve();
  }

  /** @private */
  onDisposedInOffscreen_() {
    this.dispose();
  }
};

GSC.OffscreenDocEmscriptenModule.URL_PARAM = URL_PARAM;
GSC.OffscreenDocEmscriptenModule.MESSAGING_PORT_NAME = MESSAGING_PORT_NAME;
GSC.OffscreenDocEmscriptenModule.STATUS_PORT_NAME = STATUS_PORT_NAME;
GSC.OffscreenDocEmscriptenModule.STATUS_LOADED = STATUS_LOADED;
GSC.OffscreenDocEmscriptenModule.STATUS_DISPOSED = STATUS_DISPOSED;

/**
 * Initially accumulates all sent messages, and once resolved to a channel,
 * starts acting as a proxy to it.
 */
class OffscreenDocMessageChannel extends goog.messaging.AbstractChannel {
  constructor() {
    super();
    /** @type {!goog.messaging.AbstractChannel|null} @private */
    this.underlyingChannel_ = null;
    /** @type {!Array<!{serviceName: string, payload: (string|!Object)}>} @private */
    this.pendingOutgoingMessages_ = [];
  }

  /** @override */
  send(serviceName, payload) {
    if (this.isDisposed())
      return;
    if (!this.underlyingChannel_ || this.pendingOutgoingMessages_.length > 0) {
      // Enqueue the message until the proxied channel is set and all previously
      // enqueued messages are sent.
      this.pendingOutgoingMessages_.push({serviceName, payload});
      return;
    }
    this.underlyingChannel_.send(serviceName, payload);
  }

  /**
   * Makes ourselves to behave as a proxy to the given channel. Previously
   * enqueued messages are sent immediately.
   * @param {!goog.messaging.AbstractChannel} offscreenDocChannel
   */
  resolve(offscreenDocChannel) {
    GSC.Logging.check(!this.underlyingChannel_);

    this.underlyingChannel_ = offscreenDocChannel;

    // Let ourselves receive all (unhandled) messages that are received on
    // `offscreenDocChannel`.
    offscreenDocChannel.registerDefaultService((serviceName, payload) => {
      this.deliver(serviceName, payload);
    });

    // Send all previously enqueued messages. Note that, in theory, new items
    // might be added to the array while we're iterating over it, which should
    // be fine as the for-of loop will visit all of them.
    for (const message of this.pendingOutgoingMessages_)
      offscreenDocChannel.send(message.serviceName, message.payload);
    this.pendingOutgoingMessages_ = [];
  }

  /** @override */
  disposeInternal() {
    this.pendingOutgoingMessages_ = [];
    if (this.underlyingChannel_) {
      this.underlyingChannel_.dispose();
      this.underlyingChannel_ = null;
    }
    super.disposeInternal();
  }
}
});  // goog.scope
