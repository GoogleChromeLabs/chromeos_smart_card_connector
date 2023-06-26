/**
 * @license
 * Copyright 2016 Google Inc. All Rights Reserved.
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
 * @fileoverview This file contains helper definitions that can be used for
 * loading and interacting with a NaCl module.
 */

goog.provide('GoogleSmartCard.NaclModule');

goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModuleLogMessagesReceiver');
goog.require('GoogleSmartCard.NaclModuleMessageChannel');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.log');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

const LOGGER_TITLE = 'Wrapper';

/**
 * Type of the NaCl module to be loaded.
 * @enum {number}
 */
const Type = {
  NACL: 0,
  PNACL: 1
};

/**
 * NaCl/PNaCl module element wrapper.
 *
 * When the object is constructed, the corresponding DOM element is created, but
 * not added to the page DOM tree yet. This allows setting up all required event
 * handlers before the actual module load happens - triggered by calling the
 * load method.
 *
 * Additionally, the class creates a wrapper around the message channel to the
 * NaCl module (see the nacl-module-messaging-channel.js file).
 */
GSC.NaclModule = class extends GSC.ExecutableModule {
  /**
   * @param {string} naclModulePath URL of the NaCl module manifest (.NMF file).
   * @param {!Type} type Type of the NaCl module.
   * @param {boolean=} logModulePath Whether the logger scope should include the
   * NaCl module path.
   */
  constructor(naclModulePath, type, logModulePath = false) {
    super();

    const loggerScope =
        'NaclModule' + (logModulePath ? `<"${naclModulePath}">` : '');
    const messagesReceiverLogger = GSC.Logging.getScopedLogger(loggerScope);
    /**
     * @type {!goog.log.Logger}
     * @const @private
     */
    this.logger_ =
        GSC.Logging.getChildLogger(messagesReceiverLogger, LOGGER_TITLE);

    /**
     * @type {string}
     * @const
     */
    this.naclModulePath = naclModulePath;

    /**
     * @type {!Type}
     * @const
     */
    this.type = type;

    /** @private @const */
    this.loadPromiseResolver_ = goog.Promise.withResolver();
    GSC.PromiseHelpers.suppressUnhandledRejectionError(
        this.loadPromiseResolver_.promise);

    /**
     * @type {!Element}
     * @private
     */
    this.element_ = this.createElement_();

    /**
     * Message channel that can be used to exchange messages with the NaCl
     * module.
     * @type {!GSC.NaclModuleMessageChannel}
     * @private
     */
    this.messageChannel_ =
        new GSC.NaclModuleMessageChannel(this.element_, this.logger_);

    /** @type {!GSC.NaclModuleLogMessagesReceiver} */
    this.logMessagesReceiver = new GSC.NaclModuleLogMessagesReceiver(
        this.messageChannel_, messagesReceiverLogger);

    this.addStatusEventListeners_();
  }

  /** @override */
  getLogger() {
    return this.logger_;
  }

  /** @override */
  startLoading() {
    goog.log.info(this.logger_, 'Loading NaCl module...');
    GSC.Logging.checkWithLogger(this.logger_, !this.element_.parentNode);
    GSC.Logging.checkWithLogger(this.logger_, document.body);
    document.body.appendChild(this.element_);
    this.forceElementLoading_();
    if (!this.isElementProperlyCreated_()) {
      goog.log.error(
          this.logger_,
          'NaCl embed construction failed: Native Client is likely disabled');
      this.dispose();
    }
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
    delete this.logMessagesReceiver;

    this.messageChannel_.dispose();
    delete this.messageChannel_;

    goog.dom.removeNode(this.element_);
    goog.events.removeAll(this.element_);
    delete this.element_;

    this.loadPromiseResolver_.reject(new Error('Disposed'));

    goog.log.fine(this.logger_, 'Disposed');

    super.disposeInternal();
  }

  /**
   * @return {!Element}
   * @private
   */
  createElement_() {
    const mimeType = this.getMimeType_();
    goog.log.fine(
        this.logger_,
        'Preparing NaCl embed (MIME type: "' + mimeType + '")...');
    return goog.dom.createDom('embed', {
      'type': mimeType,
      'width': 0,
      'height': 0,
      'src': this.naclModulePath
    });
  }

  /**
   * @return {string}
   * @private
   */
  getMimeType_() {
    switch (this.type) {
      case Type.NACL:
        return 'application/x-nacl';
      case Type.PNACL:
        return 'application/x-pnacl';
      default:
        GSC.Logging.failWithLogger(
            this.logger_, 'Unexpected NaclModule type: ' + this.type);
        return '';
    }
  }

  /**
   * @param {string} type
   * @param {function(?)} listener
   * @private
   */
  addEventListener_(type, listener) {
    this.element_.addEventListener(type, listener, true);
  }

  /** @private */
  addStatusEventListeners_() {
    this.addEventListener_('load', this.loadEventListener_.bind(this));
    this.addEventListener_('abort', this.abortEventListener_.bind(this));
    this.addEventListener_('error', this.errorEventListener_.bind(this));
    this.addEventListener_('crash', this.crashEventListener_.bind(this));
  }

  /** @private */
  loadEventListener_() {
    if (this.isDisposed())
      return;
    goog.log.info(this.logger_, 'Successfully loaded NaCl module');
    this.loadPromiseResolver_.resolve();
  }

  /** @private */
  abortEventListener_() {
    if (this.isDisposed())
      return;
    goog.log.error(
        this.logger_,
        'NaCl module load was aborted with the following ' +
            'message: ' + this.element_['lastError']);
    this.dispose();
  }

  /** @private */
  errorEventListener_() {
    if (this.isDisposed())
      return;
    goog.log.error(
        this.logger_,
        'Failed to load NaCl module with the following ' +
            'message: ' + this.element_['lastError']);
    this.dispose();
  }

  /** @private */
  crashEventListener_() {
    if (this.isDisposed())
      return;
    goog.log.error(this.logger_, 'The NaCl module has crashed');
    this.dispose();
  }

  /** @private */
  forceElementLoading_() {
    // Request the offsetTop property to force a relayout. As of June 29, 2014,
    // this is needed if the module is being loaded in a background page (see
    // crbug.com/350445).
    // Assign the result to a random property, so that Closure Compiler doesn't
    // optimize the "useless" expression away.
    this.element_.style.top = this.element_.offsetTop;
  }

  /**
   * @private
   * @return {boolean}
   */
  isElementProperlyCreated_() {
    if (!this.element_) {
      return false;
    }
    // If the browser doesn't have NaCl plugin installed, it'll silently fall
    // back to creating the <embed> without any functionality. Detect this case
    // by checking for an arbitrary NaCl-specific property.
    return this.element_.hasOwnProperty('postMessage');
  }
};

// Expose static attributes.
GSC.NaclModule.Type = Type;
});  // goog.scope
