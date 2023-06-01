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
    case PcscApi.SCARD_F_INTERNAL_ERROR:
      return chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    // TODO(vkovalova): add other error codes
    default:
      throw new Error(`Unknown error code ${errorCode} encountered`);
  }
}

/**
 * This class subscribes to a Chrome event with a callback, specified in the
 * constructor arguments. Unsubscribes on disposal.
 */
class ChromeEventListener extends goog.Disposable {
  /**
   *
   * @param {!Object} chromeEvent
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
     * @type {!Array<ChromeEventListener>}
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

    var resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    var sCardContext = 0;

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
};
});  // goog.scope
