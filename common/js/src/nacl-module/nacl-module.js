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
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

const LOGGER_TITLE = 'Wrapper';

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
 * @param {string} naclModulePath URL of the NaCl module manifest (.NMF file).
 * @param {!NaclModule.Type} type Type of the NaCl module (@code
 * {NaclModule.Type}).
 * @param {boolean=} logModulePath Whether the logger scope should include the
 * NaCl module path.
 * @constructor
 * @extends GSC.ExecutableModule
 */
GSC.NaclModule = function(naclModulePath, type, logModulePath = false) {
  NaclModule.base(this, 'constructor');

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
   * @type {!GSC.NaclModule.Type}
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
   * Message channel that can be used to exchange messages with the NaCl module.
   * @type {!GSC.NaclModuleMessageChannel}
   * @private
   */
  this.messageChannel_ =
      new GSC.NaclModuleMessageChannel(this.element_, this.logger_);

  /** @type {!GSC.NaclModuleLogMessagesReceiver} */
  this.logMessagesReceiver = new GSC.NaclModuleLogMessagesReceiver(
      this.messageChannel_, messagesReceiverLogger);

  this.addStatusEventListeners_();
};

const NaclModule = GSC.NaclModule;

goog.inherits(NaclModule, GSC.ExecutableModule);

/**
 * Type of the NaCl module to be loaded.
 * @enum {number}
 */
NaclModule.Type = {
  NACL: 0,
  PNACL: 1
};

/** @override */
NaclModule.prototype.getLogger = function() {
  return this.logger_;
};

/** @override */
NaclModule.prototype.startLoading = function() {
  this.logger_.info('Loading NaCl module...');
  GSC.Logging.checkWithLogger(this.logger_, !this.element_.parentNode);
  GSC.Logging.checkWithLogger(this.logger_, document.body);
  document.body.appendChild(this.element_);
  this.forceElementLoading_();
};

/** @override */
NaclModule.prototype.getLoadPromise = function() {
  return this.loadPromiseResolver_.promise;
};

/** @override */
NaclModule.prototype.getMessageChannel = function() {
  return this.messageChannel_;
};

/** @override */
NaclModule.prototype.disposeInternal = function() {
  delete this.logMessagesReceiver;

  this.messageChannel_.dispose();
  delete this.messageChannel_;

  goog.dom.removeNode(this.element_);
  goog.events.removeAll(this.element_);
  delete this.element_;

  this.loadPromiseResolver_.reject(new Error('Disposed'));

  this.logger_.fine('Disposed');

  NaclModule.base(this, 'disposeInternal');
};

/**
 * @return {!Element}
 * @private
 */
NaclModule.prototype.createElement_ = function() {
  const mimeType = this.getMimeType_();
  this.logger_.fine('Preparing NaCl embed (MIME type: "' + mimeType + '")...');
  return goog.dom.createDom(
      'embed',
      {'type': mimeType, 'width': 0, 'height': 0, 'src': this.naclModulePath});
};

/**
 * @return {string}
 * @private
 */
NaclModule.prototype.getMimeType_ = function() {
  switch (this.type) {
    case NaclModule.Type.NACL:
      return 'application/x-nacl';
    case NaclModule.Type.PNACL:
      return 'application/x-pnacl';
    default:
      GSC.Logging.failWithLogger(
          this.logger_, 'Unexpected NaclModule type: ' + this.type);
      return '';
  }
};

/**
 * @param {string} type
 * @param {function(?)} listener
 * @private
 */
NaclModule.prototype.addEventListener_ = function(type, listener) {
  this.element_.addEventListener(type, listener, true);
};

/** @private */
NaclModule.prototype.addStatusEventListeners_ = function() {
  this.addEventListener_('load', this.loadEventListener_.bind(this));
  this.addEventListener_('abort', this.abortEventListener_.bind(this));
  this.addEventListener_('error', this.errorEventListener_.bind(this));
  this.addEventListener_('crash', this.crashEventListener_.bind(this));
};

/** @private */
NaclModule.prototype.loadEventListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger_.info('Successfully loaded NaCl module');
  this.loadPromiseResolver_.resolve();
};

/** @private */
NaclModule.prototype.abortEventListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger_.severe(
      'NaCl module load was aborted with the following ' +
      'message: ' + this.element_['lastError']);
  this.dispose();
};

/** @private */
NaclModule.prototype.errorEventListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger_.severe(
      'Failed to load NaCl module with the following ' +
      'message: ' + this.element_['lastError']);
  this.dispose();
};

/** @private */
NaclModule.prototype.crashEventListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger_.severe('The NaCl module has crashed');
  this.dispose();
};

/** @private */
NaclModule.prototype.forceElementLoading_ = function() {
  // Request the offsetTop property to force a relayout. As of June 29, 2014,
  // this is needed if the module is being loaded in a background page (see
  // crbug.com/350445).
  this.element_.offsetTop = this.element_.offsetTop;
};
});  // goog.scope
