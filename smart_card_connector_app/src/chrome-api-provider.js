goog.provide('GoogleSmartCard.ConnectorApp.ChromeApiProvider');

goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ServerRequestHandler');
goog.require('goog.Disposable');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;
const Pcsc = GSC.PcscLiteServerClientsManagement;

/**
 * This class provides chrome.smartCardProviderPrivate API with PCSC responses.
 *
 * On creation, it subscribes to chrome.smartCardProviderPrivate API events,
 * which correspond to different PCSC requests. When an event if fired,
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

    chrome.smartCardProviderPrivate.onEstablishContextRequested.addListener(
        this.webApiEstablishContextListener_.bind(this));

    // TODO: addOnDisposeCallback
  }

  /** @override */
  disposeInternal() {
    // TODO: add impl
  }

  /**
   * @param {number} requestId
   */
  async webApiEstablishContextListener_(requestId) {
    const callArguments = [2, null, null];
    const remoteCallMessage =
        new GSC.RemoteCallMessage('SCardEstablishContext', callArguments);
    // console.log("sending request", callArguments, serverRequestHandler);
    // console.log("webapi: SCardEstablishContext()");
    var resultCode = chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR;
    var sCardContext = 0;
    try {
      const result =
          await this.serverRequester_.handleRequest(remoteCallMessage);

      // TODO: map result codes to enum vals.
      if (result[0] === 0) {
        resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
      }
      sCardContext = result[1];
    } catch (error) {
      // TODO: log error
    }
    chrome.smartCardProviderPrivate.reportEstablishContextResult(
        requestId, sCardContext, resultCode);
  }
};
});  // goog.scope
