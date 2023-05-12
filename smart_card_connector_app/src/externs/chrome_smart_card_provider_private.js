chrome.smartCardProviderPrivate = {};

/** @type {!ChromeBaseEvent<function(!number)>} */
chrome.smartCardProviderPrivate.onEstablishContextRequested;

/**
 * @enum {string}
 */
chrome.smartCardProviderPrivate.ResultCode = {
  SUCCESS: 'SUCCESS',
  INTERNAL_ERROR: 'INTERNAL_ERROR',
};

/**
 * @param {number} requestId
 * @param {number} sCardContext
 * @param {chrome.smartCardProviderPrivate.ResultCode} resultCode
 */
chrome.smartCardProviderPrivate.reportEstablishContextResult = function(
    requestId, sCardContext, resultCode) {};
