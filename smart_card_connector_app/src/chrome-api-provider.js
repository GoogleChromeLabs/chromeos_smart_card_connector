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
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;
const Pcsc = GSC.PcscLiteServerClientsManagement;
const PcscApi = GSC.PcscLiteClient.API;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.ChromeApiProvider');
/** @type {number} Bitmask of the lower 16 bits. */
const lowerBits = 0x0000FFFF;


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
  // The number of events is stored in the upper 16 bits of the current state
  // value in PCSC API.
  currentState |= readerState.currentCount << 16;
  return PcscApi.createSCardReaderStateIn(readerState.reader, currentState);
}

/**
 * Converts PcscApi.SCARD_READERSTATE_OUT to the
 * chrome.smartCardProviderPrivate.ReaderStateOut type.
 * This function doesn't handle the API.SCARD_STATE_ATRMATCH flag,
 * since there is no corresponding flag in chrome.smartCardProviderPrivate
 * API and we don't expect it to be used on ChromeOS.
 * @param {!PcscApi.SCARD_READERSTATE_OUT} readerState
 * @returns {!chrome.smartCardProviderPrivate.ReaderStateOut}
 */
function convertReaderStateOut(readerState) {
  /** @type {!chrome.smartCardProviderPrivate.ReaderStateFlags} */
  let readerStateFlags = {};
  // Reader state flags are stored in the lower 16 bits of the event state.
  /** @type {number} */
  let eventState = readerState['event_state'] & lowerBits;
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
  // The number of events is stored in the upper 16 bits of the event state.
  /** @type {number} */
  let eventCount = readerState['event_state'] >> 16;
  return {
    reader: readerState['reader_name'],
    eventState: readerStateFlags,
    eventCount: eventCount,
    atr: readerState['atr']
  };
}

/**
 * Converts from a chrome.smartCardProviderPrivate.ShareMode
 * to a PCSC API value.
 * @param {!chrome.smartCardProviderPrivate.ShareMode} shareMode
 * @returns {number}
 * @throws {Error} Throws if unknown share mode is encountered.
 */
function convertShareModeToNumber(shareMode) {
  switch (shareMode) {
    case chrome.smartCardProviderPrivate.ShareMode.EXCLUSIVE:
      return PcscApi.SCARD_SHARE_EXCLUSIVE;
    case chrome.smartCardProviderPrivate.ShareMode.SHARED:
      return PcscApi.SCARD_SHARE_SHARED;
    case chrome.smartCardProviderPrivate.ShareMode.DIRECT:
      return PcscApi.SCARD_SHARE_DIRECT;
    default:
      throw new Error(`Unknown share mode ${shareMode} encountered`);
  }
}

/**
 * Converts from a chrome.smartCardProviderPrivate.Protocols
 * to a PCSC API value.
 * @param {!chrome.smartCardProviderPrivate.Protocols} protocols
 * @returns {number}
 */
function convertProtocolsToNumber(protocols) {
  let protocolsVal = PcscApi.SCARD_PROTOCOL_UNDEFINED;
  if (protocols.t0)
    protocolsVal |= PcscApi.SCARD_PROTOCOL_T0;
  if (protocols.t1)
    protocolsVal |= PcscApi.SCARD_PROTOCOL_T1;
  if (protocols.raw)
    protocolsVal |= PcscApi.SCARD_PROTOCOL_RAW;
  return protocolsVal;
}

/**
 * Converts from a PCSC API value to a chrome.smartCardProviderPrivate.Protocol.
 * @param {number} protocol
 * @returns {!chrome.smartCardProviderPrivate.Protocol}
 * @throws {Error} Throws error if unknown protocol is encountered.
 */
function convertProtocolToEnum(protocol) {
  switch (protocol) {
    case PcscApi.SCARD_PROTOCOL_UNDEFINED:
      return chrome.smartCardProviderPrivate.Protocol.UNDEFINED;
    case PcscApi.SCARD_PROTOCOL_T0:
      return chrome.smartCardProviderPrivate.Protocol.T0;
    case PcscApi.SCARD_PROTOCOL_T1:
      return chrome.smartCardProviderPrivate.Protocol.T1;
    case PcscApi.SCARD_PROTOCOL_RAW:
      return chrome.smartCardProviderPrivate.Protocol.RAW;
    default:
      throw new Error(`Unknown protocol value ${protocol} encountered`);
  }
}

/**
 * Converts from a chrome.smartCardProviderPrivate.Disposition
 * to a PCSC API value.
 * @param {!chrome.smartCardProviderPrivate.Disposition} disposition
 * @returns {number}
 * @throws {Error} Throws error if unknown disposition is encountered.
 */
function convertDispositionToNumber(disposition) {
  switch (disposition) {
    case chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD:
      return PcscApi.SCARD_LEAVE_CARD;
    case chrome.smartCardProviderPrivate.Disposition.RESET_CARD:
      return PcscApi.SCARD_RESET_CARD;
    case chrome.smartCardProviderPrivate.Disposition.UNPOWER_CARD:
      return PcscApi.SCARD_UNPOWER_CARD;
    case chrome.smartCardProviderPrivate.Disposition.EJECT_CARD:
      return PcscApi.SCARD_EJECT_CARD;
    default:
      throw new Error(`Unknown disposition value ${disposition} encountered`);
  }
}

/**
 * Converts a chrome.smartCardProviderPrivate.Protocol to a predefined
 * Protocol Control Information structure used in PCSC API.
 * @param {!chrome.smartCardProviderPrivate.Protocol} protocol
 * @returns {!PcscApi.SCARD_IO_REQUEST}
 * @throws {Error} Throws error if unknown protocol is encountered.
 */
function convertProtocolToPci(protocol) {
  switch (protocol) {
    case chrome.smartCardProviderPrivate.Protocol.T0:
      return PcscApi.SCARD_PCI_T0;
    case chrome.smartCardProviderPrivate.Protocol.T1:
      return PcscApi.SCARD_PCI_T1;
    case chrome.smartCardProviderPrivate.Protocol.RAW:
      return PcscApi.SCARD_PCI_RAW;
    default:
      throw new Error(`No predefined PCI structure for protocol ${protocol}`);
  }
}

/**
 * Logs that that the encountered combination of connection state flags
 * is not expected from PCSC-lite.
 * @param {number} state
 */
function logUnexpectedConnectionState(state) {
  goog.log.warning(logger, `Unexpected connection state bitmask: ${state}`)
}

/**
 * Converts from a PCSC API value to a
 * chrome.smartCardProviderPrivate.ConnectionState. We only take the highest bit
 * of the value, since it contains all the needed information, e.g.
 * SCARD_NEGOTIABLE implies that SCARD_POWERED and SCARD_PRESENT are also set.
 * @param {number} pcscState
 * @returns {!chrome.smartCardProviderPrivate.ConnectionState}
 * @throws {Error} Throws error if unknown connection state is encountered.
 */
function convertConnectionStateToEnum(pcscState) {
  // Upper 16 bits contains the number of events for the reader and don't have
  // state flags.
  state = pcscState & lowerBits;
  if (state & PcscApi.SCARD_SPECIFIC) {
    if (state !==
        (PcscApi.SCARD_SPECIFIC | PcscApi.SCARD_POWERED |
         PcscApi.SCARD_PRESENT))
      logUnexpectedConnectionState(state);
    return chrome.smartCardProviderPrivate.ConnectionState.SPECIFIC;
  }
  if (state & PcscApi.SCARD_NEGOTIABLE) {
    if (state !==
        (PcscApi.SCARD_NEGOTIABLE | PcscApi.SCARD_POWERED |
         PcscApi.SCARD_PRESENT))
      logUnexpectedConnectionState(state);
    return chrome.smartCardProviderPrivate.ConnectionState.NEGOTIABLE;
  }
  if (state & PcscApi.SCARD_POWERED) {
    if (state !== (PcscApi.SCARD_POWERED | PcscApi.SCARD_PRESENT))
      logUnexpectedConnectionState(state);
    return chrome.smartCardProviderPrivate.ConnectionState.POWERED;
  }
  if (state & PcscApi.SCARD_SWALLOWED) {
    if (state !== (PcscApi.SCARD_SWALLOWED | PcscApi.SCARD_PRESENT))
      logUnexpectedConnectionState(state);
    return chrome.smartCardProviderPrivate.ConnectionState.SWALLOWED;
  }
  if (state & PcscApi.SCARD_PRESENT) {
    if (state !== PcscApi.SCARD_PRESENT)
      logUnexpectedConnectionState(state);
    return chrome.smartCardProviderPrivate.ConnectionState.PRESENT;
  }
  if (state & PcscApi.SCARD_ABSENT) {
    if (state !== PcscApi.SCARD_ABSENT)
      logUnexpectedConnectionState(state);
    return chrome.smartCardProviderPrivate.ConnectionState.ABSENT;
  }
  throw new Error(`Unknown connection state value ${state} encountered`);
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
   * @param {!goog.Promise} serverReadinessTracker
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
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onConnectRequested,
        (...args) => this.connectListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onDisconnectRequested,
        (...args) => this.disconnectListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onTransmitRequested,
        (...args) => this.transmitListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onControlRequested,
        (...args) => this.controlListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onGetAttribRequested,
        (...args) => this.getAttribListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onSetAttribRequested,
        (...args) => this.setAttribListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onStatusRequested,
        (...args) => this.statusListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onBeginTransactionRequested,
        (...args) => this.beginTransactionListener_(...args)));
    this.chromeEventListeners_.push(new ChromeEventListener(
        chrome.smartCardProviderPrivate.onEndTransactionRequested,
        (...args) => this.endTransactionListener_(...args)));
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

  /**
   * @param {number} requestId
   * @param {number} sCardContext
   * @param {string} reader
   * @param {!chrome.smartCardProviderPrivate.ShareMode} shareMode
   * @param {!chrome.smartCardProviderPrivate.Protocols} preferredProtocols
   * @private
   */
  async connectListener_(
      requestId, sCardContext, reader, shareMode, preferredProtocols) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {number} */
    let sCardHandle = 0;
    /** @type {!chrome.smartCardProviderPrivate.Protocol} */
    let activeProtocol = chrome.smartCardProviderPrivate.Protocol.UNDEFINED;

    try {
      const callArguments = [
        sCardContext, reader, convertShareModeToNumber(shareMode),
        convertProtocolsToNumber(preferredProtocols)
      ];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardConnect', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardConnectResult(responseItems);
      result.get(
          (handle, protocol) => {
            sCardHandle = handle;
            activeProtocol = convertProtocolToEnum(protocol);
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onConnectRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportConnectResult(
        requestId, sCardHandle, activeProtocol, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @param {!chrome.smartCardProviderPrivate.Disposition} disposition
   * @private
   */
  async disconnectListener_(requestId, sCardHandle, disposition) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;

    try {
      const callArguments =
          [sCardHandle, convertDispositionToNumber(disposition)];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardDisconnect', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardDisconnectResult(responseItems);
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
          'Failed to process onDisconnectRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportPlainResult(requestId, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @param {!chrome.smartCardProviderPrivate.Protocol} protocol
   * @param {!ArrayBuffer} data
   * @private
   */
  async transmitListener_(requestId, sCardHandle, protocol, data) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {!ArrayBuffer} */
    let resultData = new ArrayBuffer(0);

    try {
      const callArguments =
          [sCardHandle, convertProtocolToPci(protocol), data, null];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardTransmit', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardTransmitResult(responseItems);
      result.get(
          (pci, data) => {
            resultData = data;
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onTransmitRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportDataResult(
        requestId, resultData, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @param {number} controlCode
   * @param {!ArrayBuffer} data
   * @private
   */
  async controlListener_(requestId, sCardHandle, controlCode, data) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {!ArrayBuffer} */
    let resultData = new ArrayBuffer(0);

    try {
      const callArguments = [sCardHandle, controlCode, data];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardControl', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardControlResult(responseItems);
      result.get(
          (data) => {
            resultData = data;
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onControlRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportDataResult(
        requestId, resultData, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @param {number} attribId
   * @private
   */
  async getAttribListener_(requestId, sCardHandle, attribId) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {!ArrayBuffer} */
    let resultData = new ArrayBuffer(0);

    try {
      const callArguments = [sCardHandle, attribId];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardGetAttrib', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardGetAttribResult(responseItems);
      result.get(
          (data) => {
            resultData = data;
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onGetAttribRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportDataResult(
        requestId, resultData, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @param {number} attribId
   * @param {!ArrayBuffer} data
   * @private
   */
  async setAttribListener_(requestId, sCardHandle, attribId, data) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;

    try {
      const callArguments = [sCardHandle, attribId, data];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardSetAttrib', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardSetAttribResult(responseItems);
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
          'Failed to process onSetAttribRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportPlainResult(requestId, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @private
   */
  async statusListener_(requestId, sCardHandle) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    /** @type {string} */
    let resultReaderName = '';
    /** @type {!chrome.smartCardProviderPrivate.ConnectionState} */
    let resultState = chrome.smartCardProviderPrivate.ConnectionState.ABSENT;
    /** @type {!chrome.smartCardProviderPrivate.Protocol} */
    let resultProtocol = chrome.smartCardProviderPrivate.Protocol.UNDEFINED;
    /** @type {!ArrayBuffer} */
    let resultAtr = new ArrayBuffer(0);

    try {
      const callArguments = [sCardHandle];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardStatus', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardStatusResult(responseItems);
      result.get(
          (readerName, state, protocol, atr) => {
            resultReaderName = readerName;
            resultState = convertConnectionStateToEnum(state);
            resultProtocol = convertProtocolToEnum(protocol);
            resultAtr = atr;
            resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
          },
          (errorCode) => {
            resultCode = convertErrorCodeToEnum(errorCode);
          });
    } catch (error) {
      goog.log.log(
          logger,
          this.isDisposed() ? goog.log.Level.FINE : goog.log.Level.WARNING,
          'Failed to process onStatusRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportStatusResult(
        requestId, resultReaderName, resultState, resultProtocol, resultAtr,
        resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @private
   */
  async beginTransactionListener_(requestId, sCardHandle) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;

    try {
      const callArguments = [sCardHandle];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardBeginTransaction', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardBeginTransactionResult(responseItems);
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
          'Failed to process onBeginTransactionRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportPlainResult(requestId, resultCode);
  }

  /**
   * @param {number} requestId
   * @param {number} sCardHandle
   * @param {!chrome.smartCardProviderPrivate.Disposition} disposition
   * @private
   */
  async endTransactionListener_(requestId, sCardHandle, disposition) {
    let resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;

    try {
      const callArguments =
          [sCardHandle, convertDispositionToNumber(disposition)];
      const remoteCallMessage =
          new GSC.RemoteCallMessage('SCardEndTransaction', callArguments);
      const responseItems =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      const result = new PcscApi.SCardEndTransactionResult(responseItems);
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
          'Failed to process onEndTransactionRequested event ' +
              `with request id ${requestId}: ${error}`);
    }

    chrome.smartCardProviderPrivate.reportPlainResult(requestId, resultCode);
  }
};
});  // goog.scope
