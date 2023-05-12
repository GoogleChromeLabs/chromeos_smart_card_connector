goog.provide('GoogleSmartCard.ChromeAPIProvider');

goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ServerRequestHandler');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * @constructor
 * @extends goog.Disposable
 */
GSC.ChromeAPIProvider = function(serverMessageChannel, serverReadinessTracker) {
  this.serverRequester_ =
      GSC.PcscLiteServerClientsManagement.ServerRequestHandler(
          serverMessageChannel, serverReadinessTracker, 'chrome');

  chrome.smartCardProviderPrivate.onEstablishContextRequested.addListener(
      this.webapiEstablishContextListener_.bind(this));

  // TODO: addOnDisposeCallback
};

goog.inherits(GSC.ChromeAPIProvider, goog.Disposable);

/** @override */
ChromeAPIProvider.prototype.disposeInternal = function() {
  // TODO: add impl
};


/**
 * @param {number} requestId
 */
ChromeAPIProvider.prototype.webapiEstablishContextListener_ = function(
    requestId) {
  const callArguments = [2, null, null];
  const remoteCallMessage =
      new GSC.RemoteCallMessage('SCardEstablishContext', callArguments);
  // console.log("sending request", callArguments, serverRequestHandler);
  const promise = this.serverRequester_.handleRequest(remoteCallMessage);
  // console.log("webapi: SCardEstablishContext()");
  promise.then(
      (result) => {
        // console.log(" fulfilled", result);
        var resultCode =
            chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR
        if (result[0] === 0) {
          resultCode = chrome.smartCardProviderPrivate.ResultCode.SUCCESS;
        }
        chrome.smartCardProviderPrivate.reportEstablishContextResult(
            requestId, result[1], resultCode);
      },
      (value) => {
        // console.log("rejected", value);
        chrome.smartCardProviderPrivate.reportEstablishContextResult(
            requestId, 0,
            chrome.smartCardProviderPrivate.ResultCode.INTERNAL_ERROR);
      });
};

});  // goog.scope
