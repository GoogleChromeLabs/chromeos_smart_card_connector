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

goog.provide('GoogleSmartCard.ConnectorApp.ChromeApiProvider');

goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ServerRequestHandler');
goog.require('goog.Disposable');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;
const Pcsc = GSC.PcscLiteServerClientsManagement;
const PcscApi = GSC.PcscLiteClient.API;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.ChromeApiProvider');

/**
 * Returns the result code for chrome.smartCardProviderPrivate API that
 * corresponds to the given PCSC error.
 * @param {!PcscApi.ERROR_CODE} errorCode
 * @return {!chrome.smartCardProviderPrivate.ResultCode}
 * @throws {Error} Will throw if unknown error code encountered.
 */
function convertErrorCodeToEnum(errorCode) {
  switch (errorCode) {
    case PcscApi.SCARD_W_REMOVED_CARD:
      return chrome.smartCardProviderPrivate.ResultCode.REMOVED_CARD;
    case PcscApi.SCARD_W_RESET_CARD:
      return chrome.smartCardProviderPrivate.ResultCode.RESET_CARD;
    case PcscApi.SCARD_W_UNPOWERED_CARD:
      return chrome.smartCardProviderPrivate.ResultCode.UNPOWERED_CARD;
    case PcscApi.SCARD_W_UNRESPONSIVE_CARD:
      return chrome.smartCardProviderPrivate.ResultCode.UNRESPONSIVE_CARD;
    case PcscApi.SCARD_W_UNSUPPORTED_CARD:
      return chrome.smartCardProviderPrivate.ResultCode.UNSUPPORTED_CARD;
    case PcscApi.SCARD_E_READER_UNAVAILABLE:
      return chrome.smartCardProviderPrivate.ResultCode.READER_UNAVAILABLE;
    case PcscApi.SCARD_E_SHARING_VIOLATION:
      return chrome.smartCardProviderPrivate.ResultCode.SHARING_VIOLATION;
    case PcscApi.SCARD_E_NOT_TRANSACTED:
      return chrome.smartCardProviderPrivate.ResultCode.NOT_TRANSACTED;
    case PcscApi.SCARD_E_NO_SMARTCARD:
      return chrome.smartCardProviderPrivate.ResultCode.NO_SMARTCARD;
    case PcscApi.SCARD_E_PROTO_MISMATCH:
      return chrome.smartCardProviderPrivate.ResultCode.PROTO_MISMATCH;
    case PcscApi.SCARD_E_SYSTEM_CANCELLED:
      return chrome.smartCardProviderPrivate.ResultCode.SYSTEM_CANCELLED;
    case PcscApi.SCARD_E_NOT_READY:
      return chrome.smartCardProviderPrivate.ResultCode.NOT_READY;
    case PcscApi.SCARD_E_CANCELLED:
      return chrome.smartCardProviderPrivate.ResultCode.CANCELLED;
    case PcscApi.SCARD_E_INSUFFICIENT_BUFFER:
      return chrome.smartCardProviderPrivate.ResultCode.INSUFFICIENT_BUFFER;
    case PcscApi.SCARD_E_INVALID_HANDLE:
      return chrome.smartCardProviderPrivate.ResultCode.INVALID_HANDLE;
    case PcscApi.SCARD_E_INVALID_PARAMETER:
      return chrome.smartCardProviderPrivate.ResultCode.INVALID_PARAMETER;
    case PcscApi.SCARD_E_INVALID_VALUE:
      return chrome.smartCardProviderPrivate.ResultCode.INVALID_VALUE;
    case PcscApi.SCARD_E_NO_MEMORY:
      return chrome.smartCardProviderPrivate.ResultCode.NO_MEMORY;
    case PcscApi.SCARD_E_TIMEOUT:
      return chrome.smartCardProviderPrivate.ResultCode.TIMEOUT;
    case PcscApi.SCARD_E_UNKNOWN_READER:
      return chrome.smartCardProviderPrivate.ResultCode.UNKNOWN_READER;
    case PcscApi.SCARD_E_UNSUPPORTED_FEATURE:
      return chrome.smartCardProviderPrivate.ResultCode.UNSUPPORTED_FEATURE;
    case PcscApi.SCARD_E_NO_READERS_AVAILABLE:
      return chrome.smartCardProviderPrivate.ResultCode.NO_READERS_AVAILABLE;
    case PcscApi.SCARD_E_SERVICE_STOPPED:
      return chrome.smartCardProviderPrivate.ResultCode.SERVICE_STOPPED;
    case PcscApi.SCARD_E_NO_SERVICE:
      return chrome.smartCardProviderPrivate.ResultCode.NO_SERVICE;
    case PcscApi.SCARD_F_COMM_ERROR:
      return chrome.smartCardProviderPrivate.ResultCode.COMM_ERROR;
    case PcscApi.SCARD_F_INTERNAL_ERROR:
      return chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    case PcscApi.SCARD_F_UNKNOWN_ERROR:
      return chrome.smartCardProviderPrivate.ResultCode.UNKNOWN_ERROR;
    case PcscApi.SCARD_E_SERVER_TOO_BUSY:
      return chrome.smartCardProviderPrivate.ResultCode.SERVER_TOO_BUSY;
    case PcscApi.SCARD_E_UNEXPECTED:
      return chrome.smartCardProviderPrivate.ResultCode.UNEXPECTED;
    case PcscApi.SCARD_P_SHUTDOWN:
      return chrome.smartCardProviderPrivate.ResultCode.SHUTDOWN;
    default:
      throw new Error(`Unknown error code ${errorCode} encountered`);
  }
}

/**
 * Converts chrome.smartCardProviderPrivate.ReaderStateIn to the
 * PcscApi.SCARD_READERSTATE_IN type.
 * @param {!chrome.smartCardProviderPrivate.ReaderStateIn} readerState
 * @returns {!PcscApi.SCARD_READERSTATE_IN}
 */
function convertReaderStateIn(readerState) {
  let currentState = 0;
  /** @type {!chrome.smartCardProviderPrivate.ReaderStateFlags} */
  let currentStateFlags = readerState.currentState;
  if (currentStateFlags.unaware)
    currentState |= PcscApi.SCARD_STATE_UNAWARE;
  if (currentStateFlags.ignore)
    currentState |= PcscApi.SCARD_STATE_IGNORE;
  if (currentStateFlags.changed)
    currentState |= PcscApi.SCARD_STATE_CHANGED;
  if (currentStateFlags.unknown)
    currentState |= PcscApi.SCARD_STATE_UNKNOWN;
  if (currentStateFlags.unavailable)
    currentState |= PcscApi.SCARD_STATE_UNAVAILABLE;
  if (currentStateFlags.empty)
    currentState |= PcscApi.SCARD_STATE_EMPTY;
  if (currentStateFlags.present)
    currentState |= PcscApi.SCARD_STATE_PRESENT;
  if (currentStateFlags.exclusive)
    currentState |= PcscApi.SCARD_STATE_EXCLUSIVE;
  if (currentStateFlags.inuse)
    currentState |= PcscApi.SCARD_STATE_INUSE;
  if (currentStateFlags.mute)
    currentState |= PcscApi.SCARD_STATE_MUTE;
  if (currentStateFlags.unpowered)
    currentState |= PcscApi.SCARD_STATE_UNPOWERED;
  return PcscApi.createSCardReaderStateIn(readerState.reader, currentState);
}

/**
 * Converts PcscApi.SCARD_READERSTATE_OUT to the
 * chrome.smartCardProviderPrivate.ReaderStateOut type.
 * This function doesn't handle the API.SCARD_STATE_ATRMATCH flag,
 * since there is no corresponding flag in chrome.smartCardProviderPrivate
 * API and we don't expect it to be used on Chrome OS.
 * @param {!PcscApi.SCARD_READERSTATE_OUT} readerState
 * @returns {!chrome.smartCardProviderPrivate.ReaderStateOut}
 */
function convertReaderStateOut(readerState) {
  /** @type {!chrome.smartCardProviderPrivate.ReaderStateFlags} */
  let readerStateFlags = {};
  /** @type {number} */
  let eventState = readerState['event_state'];
  if (eventState == PcscApi.SCARD_STATE_UNAWARE)
    readerStateFlags.unaware = true;
  if (eventState & PcscApi.SCARD_STATE_IGNORE)
    readerStateFlags.ignore = true;
  if (eventState & PcscApi.SCARD_STATE_CHANGED)
    readerStateFlags.changed = true;
  if (eventState & PcscApi.SCARD_STATE_UNKNOWN)
    readerStateFlags.unknown = true;
  if (eventState & PcscApi.SCARD_STATE_UNAVAILABLE)
    readerStateFlags.unavailable = true;
  if (eventState & PcscApi.SCARD_STATE_EMPTY)
    readerStateFlags.empty = true;
  if (eventState & PcscApi.SCARD_STATE_PRESENT)
    readerStateFlags.present = true;
  if (eventState & PcscApi.SCARD_STATE_EXCLUSIVE)
    readerStateFlags.exclusive = true;
  if (eventState & PcscApi.SCARD_STATE_INUSE)
    readerStateFlags.inuse = true;
  if (eventState & PcscApi.SCARD_STATE_MUTE)
    readerStateFlags.mute = true;
  if (eventState & PcscApi.SCARD_STATE_UNPOWERED)
    readerStateFlags.unpowered = true;
  return {
    reader: readerState['reader_name'],
    eventState: readerStateFlags,
    atr: readerState['atr']
  };
}

/**
 * This class subscribes to a Chrome event with a callback, specified in the
 * constructor arguments. Unsubscribes on disposal.
 */
class ChromeEventListener extends goog.Disposable {
  /**
   * @param {!ChromeBaseEvent<!Function>} chromeEvent
   * @param {!Function} callback
   */
  constructor(chromeEvent, callback) {
    super();
    this.chromeEvent_ = chromeEvent;
    this.callback_ = callback;
    this.chromeEvent_.addListener(this.callback_);
  }

  /** @override */
  disposeInternal() {
    this.chromeEvent_.removeListener(this.callback_);
    this.callback_ = null;
  }
};

/**
 * This class provides chrome.smartCardProviderPrivate API with PCSC responses.
 *
 * On creation, it subscribes to chrome.smartCardProviderPrivate API events,
 * which correspond to different PCSC requests. When an event is fired,
 * ChromeApiProvider sends a request to Pcsc.ServerRequestHandler and waits for
 * the response. When the response is received, it reports the result back using
 * chrome.smartCardProviderPrivate API.
 */
GSC.ConnectorApp.ChromeApiProvider = class extends goog.Disposable {
  /**
   * @param {!goog.messaging.AbstractChannel} serverMessageChannel
   * @param {!Pcsc.ReadinessTracker} serverReadinessTracker
   */
  constructor(serverMessageChannel, serverReadinessTracker) {
    super();
    this.serverRequester_ = new Pcsc.ServerRequestHandler(
        serverMessageChannel, serverReadinessTracker, 'chrome');

    /**
     * @type {!Array<!ChromeEventListener>}
     * @private
     */
    this.chromeEventListeners_ = [];
    this.subscribeToChromeApiEvents_();

    this.serverRequester_.addOnDisposeCallback(() => this.dispose());
  }

  /** @override */
  disposeInternal() {
    this.unsubscribeFromChromeApiEvents_();
    this.serverRequester_.dispose();
    super.disposeInternal();
  }

  /**
   * Subscribes to chrome.smartCardProviderPrivate events.
   * @private
   */
  subscribeToChromeApiEvents_() {
    if (chrome.smartCardProviderPrivate === undefined) {
      goog.log.warning(
          logger,
          'chrome.smartCardProviderPrivate API is not available, ' +
              'could not subscribe to the API events.');
      return;
    }

    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onEstablishContextRequested,
        (requestId) => this.establishContextListener_(requestId)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onReleaseContextRequested,
        (requestId, sCardContext) =>
            this.releaseContextListener_(requestId, sCardContext)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onListReadersRequested,
        (requestId, sCardContext) =>
            this.listReadersListener_(requestId, sCardContext)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onGetStatusChangeRequested,
        (...args) => this.getStatusChangeListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onCancelRequested,
        (requestId, sCardContext) =>
            this.cancelListener_(requestId, sCardContext)));
  }
  /**
   * Unsubscribes from chrome.smartCardProviderPrivate events.
   * @private
   */
  unsubscribeFromChromeApiEvents_() {
    this.chromeEventListeners_.forEach(listener => listener.dispose());
  }

  /**
   * @param {number} requestId
   * @private
   */
  async establishContextListener_(requestId) {
    const callArguments = [PcscApi.SCARD_SCOPE_SYSTEM, null, null];
    const remoteCallMessage =
        new GSC.RemoteCallMessage('SCardEstablishContext', callArguments);

    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    let sCardContext = 0;

    try {
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardEstablishContextResult(responseItems);
      result.get(
          (context) => {
            sCardContext = context;
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onEstablishContextRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportEstablishContextResult(
        requestId, sCardContext, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardContext
   * @private
   */
  async releaseContextListener_(requestId, sCardContext) {
    const callArguments = [sCardContext];
    const remoteCallMessage =
        new GSC.RemoteCallMessage('SCardReleaseContext', callArguments);

    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;

    try {
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardReleaseContextResult(responseItems);
      result.get(
          () => {
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onReleaseContextRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportReleaseContextResult(
        requestId, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardContext
   * @private
   */
  async listReadersListener_(requestId, sCardContext) {
    const callArguments = [sCardContext, null];
    const remoteCallMessage =
        new GSC.RemoteCallMessage('SCardListReaders', callArguments);

    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {!Array<string>} */
    let resultReaders = [];

    try {
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardListReadersResult(responseItems);
      result.get(
          (readers) => {
            resultReaders = readers;
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onListReadersRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportListReadersResult(
        requestId, resultReaders, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardContext
   * @param {!chrome.smartCardProviderPrivate.Timeout} timeout
   * @param {!Array<!chrome.smartCardProviderPrivate.ReaderStateIn>}
   *     readerStates
   * @private
   */
  async getStatusChangeListener_(
      requestId, sCardContext, timeout, readerStates) {
    const timeoutMs = timeout.milliseconds === undefined ? PcscApi.INFINITE :
                                                           timeout.milliseconds;
    const readerStatesPcsc = readerStates.map(convertReaderStateIn);
    const callArguments = [sCardContext, timeoutMs, readerStatesPcsc];
    const remoteCallMessage =
        new GSC.RemoteCallMessage('SCardGetStatusChange', callArguments);

    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {!Array<!chrome.smartCardProviderPrivate.ReaderStateOut>} */
    let newReaderStates = [];

    try {
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardGetStatusChangeResult(responseItems);
      result.get(
          (newReaderStatesPcsc) => {
            newReaderStates = newReaderStatesPcsc.map(convertReaderStateOut);
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onGetStatusChangeRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportGetStatusChangeResult(
        requestId, newReaderStates, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardContext
   * @private
   */
  async cancelListener_(requestId, sCardContext) {
    const callArguments = [sCardContext];
    const remoteCallMessage =
        new GSC.RemoteCallMessage('SCardCancel', callArguments);

    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;

    try {
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardCancelResult(responseItems);
      result.get(
          () => {
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onCancelRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportPlainResult(requestId, resultCode);
  }
};
});  // goog.scope
