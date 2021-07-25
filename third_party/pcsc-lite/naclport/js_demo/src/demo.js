/** @license
 * Copyright 2016 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @fileoverview PC/SC-Lite demo code - some sequence of PC/SC-Lite commands
 * that perform some basic testing of the core PC/SC-Lite functionality.
 */

goog.provide('GoogleSmartCard.PcscLiteClient.Demo');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('goog.array');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const TIMEOUT_SECONDS = 10;
const SPECIAL_READER_NAME = '\\\\?PnP?\\Notification';

const GSC = GoogleSmartCard;
const Demo = GSC.PcscLiteClient.Demo;
const API = GoogleSmartCard.PcscLiteClient.API;
const dump = GSC.DebugDump.dump;

/**
 * @type {!goog.log.Logger}
 * @const
 */
Demo.logger = GSC.Logging.getScopedLogger('PcscLiteClient.Demo');
const logger = Demo.logger;

/**
 * @param {!API} pcscLiteClientApi
 * @param {function()} onSucceeded
 * @param {function()} onFailed
 */
Demo.run = function(pcscLiteClientApi, onSucceeded, onFailed) {
  startDemo(pcscLiteClientApi, onSucceeded, onFailed);
};

function startDemo(api, onDemoSucceeded, onDemoFailed) {
  establishContext(api, onDemoSucceeded, onDemoFailed);
}

function establishContext(api, onDemoSucceeded, onDemoFailed) {
  logger.info('Establishing a context...');
  api.SCardEstablishContext(API.SCARD_SCOPE_SYSTEM, null, null).then(
      function(result) {
        result.get(
            onContextEstablished.bind(
                null, api, onDemoSucceeded, onDemoFailed),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onContextEstablished(
    api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Established a new context: ' + dump(sCardContext));
  validateContext(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function validateContext(api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Validating the context...');
  api.SCardIsValidContext(sCardContext).then(function(result) {
    result.get(
        onContextValidated.bind(
            null, api, onDemoSucceeded, onDemoFailed, sCardContext),
        onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onContextValidated(api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Context checked successfully');
  validateInvalidContext(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function validateInvalidContext(
    api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Validating an invalid context...');
  api.SCardIsValidContext(sCardContext + 1).then(function(result) {
    result.get(
        onInvalidContextAccepted.bind(
            null, api, onDemoSucceeded, onDemoFailed, sCardContext),
        onInvalidContextRejected.bind(
            null, api, onDemoSucceeded, onDemoFailed, sCardContext));
  }, onFailure.bind(null, onDemoFailed));
}

function onInvalidContextAccepted(
    api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.warning('failed: the invalid context was accepted');
  onDemoFailed();
}

function onInvalidContextRejected(
    api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('OK, the invalid context was rejected');
  waitForReadersChange(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function waitForReadersChange(
    api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Waiting ' + TIMEOUT_SECONDS + ' seconds for readers change...');
  api.SCardGetStatusChange(
      sCardContext,
      TIMEOUT_SECONDS * 1000,
      [API.createSCardReaderStateIn(
           SPECIAL_READER_NAME, API.SCARD_STATE_UNAWARE, 0xDEADBEEF)]).then(
          function(result) {
            result.get(
                onReadersChanged.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed,
                    sCardContext),
                onReadersChangeWaitingFailed.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed,
                    sCardContext));
          }, onFailure.bind(null, onDemoFailed));
}

function onReadersChanged(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readerStates) {
  if (readerStates.length != 1) {
    logger.warning('failed: returned invalid number of reader states');
    onDemoFailed();
    return;
  }
  if (readerStates[0]['reader_name'] != SPECIAL_READER_NAME) {
    logger.warning('failed: returned wrong reader name');
    onDemoFailed();
    return;
  }
  if (readerStates[0]['user_data'] != 0xDEADBEEF) {
    logger.warning('failed: returned wrong reader name');
    onDemoFailed();
    return;
  }
  if (!(readerStates[0]['event_state'] & API.SCARD_STATE_CHANGED)) {
    logger.warning(
        'failed: returned current state mask without SCARD_STATE_CHANGED bit');
    onDemoFailed();
    return;
  }
  logger.info('Caught readers change: ' + dump(readerStates));
  listReaderGroups(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function onReadersChangeWaitingFailed(
    api, onDemoSucceeded, onDemoFailed, sCardContext, errorCode) {
  if (errorCode != API.SCARD_E_TIMEOUT) {
    onPcscLiteError(api, onDemoFailed, errorCode);
    return;
  }
  logger.info('No readers change events were caught');
  listReaderGroups(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function listReaderGroups(api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Listing reader groups...');
  api.SCardListReaderGroups(sCardContext).then(function(result) {
    result.get(
        onReaderGroupsListed.bind(
            null, api, onDemoSucceeded, onDemoFailed, sCardContext),
        onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onReaderGroupsListed(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readerGroups) {
  logger.info('Listed reader groups: ' +
              goog.iter.join(goog.iter.map(readerGroups, dump), ', '));
  if (!readerGroups) {
    logger.warning('failed: no reader groups found');
    onDemoFailed();
    return;
  }
  listReaders(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function listReaders(api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Listing readers...');
  api.SCardListReaders(sCardContext, null).then(function(result) {
    result.get(
        onReadersListed.bind(
            null, api, onDemoSucceeded, onDemoFailed, sCardContext),
        onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onReadersListed(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readers) {
  logger.info('Listed readers: ' +
              goog.iter.join(goog.iter.map(readers, dump), ', '));
  if (!readers) {
    logger.warning('failed: no readers found');
    onDemoFailed();
    return;
  }
  waitForCardRemoval(
      api, onDemoSucceeded, onDemoFailed, sCardContext, readers[0]);
}

function waitForCardRemoval(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readerName) {
  logger.info('Waiting ' + TIMEOUT_SECONDS + ' seconds for card removal ' +
              'from ' + dump(readerName) + ' reader...');
  api.SCardGetStatusChange(
      sCardContext,
      TIMEOUT_SECONDS * 1000,
      [API.createSCardReaderStateIn(readerName, API.SCARD_STATE_PRESENT)]).then(
          function(result) {
            result.get(
                onCardRemoved.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed,
                    sCardContext,
                    readerName),
                onCardRemovalWaitingFailed.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed,
                    sCardContext,
                    readerName));
          }, onFailure.bind(null, onDemoFailed));
}

function onCardRemoved(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    readerName,
    readerStates) {
  logger.info('Caught card removal: ' + dump(readerStates));
  waitForCardInsertion(
      api, onDemoSucceeded, onDemoFailed, sCardContext, readerName);
}

function onCardRemovalWaitingFailed(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readerName, errorCode) {
  if (errorCode != API.SCARD_E_TIMEOUT) {
    onPcscLiteError(api, onDemoFailed, errorCode);
    return;
  }
  logger.info('No card removal events were caught');
  waitForCardInsertion(
      api, onDemoSucceeded, onDemoFailed, sCardContext, readerName);
}

function waitForCardInsertion(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readerName) {
  logger.info('Waiting ' + TIMEOUT_SECONDS + ' seconds for card insertion ' +
              'into ' + dump(readerName) + ' reader...');
  api.SCardGetStatusChange(
      sCardContext,
      TIMEOUT_SECONDS * 1000,
      [API.createSCardReaderStateIn(readerName, API.SCARD_STATE_EMPTY)]).then(
          function(result) {
            result.get(
                onCardInserted.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed,
                    sCardContext,
                    readerName),
                onPcscLiteError.bind(null, api, onDemoFailed));
          }, onFailure.bind(null, onDemoFailed));
}

function onCardInserted(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    readerName,
    readerStates) {
  logger.info('Caught card insertion: ' + dump(readerStates));
  waitAndCancel(api, onDemoSucceeded, onDemoFailed, sCardContext, readerName);
}

function waitAndCancel(
    api, onDemoSucceeded, onDemoFailed, sCardContext, readerName) {
  logger.info('Started waiting for card removal from ' + dump(readerName) +
              ' reader...');
  api.SCardGetStatusChange(
      sCardContext,
      TIMEOUT_SECONDS * 1000,
      [goog.object.create(
           'reader_name',
           readerName,
           'current_state',
           API.SCARD_STATE_PRESENT)]).then(
          function(result) {
            result.get(
                onCancelingWaitingFinished.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed),
                onCancelingWaitingFailed.bind(
                    null,
                    api,
                    onDemoSucceeded,
                    onDemoFailed,
                    sCardContext,
                    readerName));
          }, onFailure.bind(null, onDemoFailed));
  logger.info('Cancelling the waiting...');
  api.SCardCancel(sCardContext).then(function(result) {
    result.get(
        onCancelSucceeded, onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onCancelingWaitingFailed(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    readerName,
    errorCode) {
  if (errorCode != API.SCARD_E_CANCELLED) {
    onPcscLiteError(api, onDemoFailed, errorCode);
    return;
  }
  connect(api, onDemoSucceeded, onDemoFailed, sCardContext, readerName);
}

function onCancelingWaitingFinished(api, onDemoSucceeded, onDemoFailed) {
  logger.warning(
      'failed: the waiting finished successfully despite the cancellation');
  onDemoFailed();
}

function onCancelSucceeded() {
  logger.info('Successfully initiated cancelling of the waiting');
}

function connect(api, onDemoSucceeded, onDemoFailed, sCardContext, readerName) {
  logger.info('Connecting to the reader ' + dump(readerName) + '...');
  api.SCardConnect(
      sCardContext,
      readerName,
      API.SCARD_SHARE_SHARED,
      API.SCARD_PROTOCOL_ANY).then(function(result) {
        result.get(
            onConnected.bind(
                null, api, onDemoSucceeded, onDemoFailed, sCardContext),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onConnected(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    sCardHandle,
    activeProtocol) {
  logger.info('Successfully connected, handle is: ' + dump(sCardHandle) +
               ', active protocol is: ' +
               getProtocolDebugRepresentation(activeProtocol));
  reconnect(api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle);
}

function reconnect(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle) {
  logger.info('Reconnecting to the card...');
  api.SCardReconnect(
      sCardHandle,
      API.SCARD_SHARE_SHARED,
      API.SCARD_PROTOCOL_ANY,
      API.SCARD_LEAVE_CARD).then(function(result) {
        result.get(
            onReconnected.bind(
                null,
                api,
                onDemoSucceeded,
                onDemoFailed,
                sCardContext,
                sCardHandle),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onReconnected(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle) {
  logger.info('Successfully reconnected');
  getStatus(api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle);
}

function getStatus(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle) {
  logger.info('Obtaining card status...');
  api.SCardStatus(sCardHandle).then(function(result) {
    result.get(
        onStatusGot.bind(
            null,
            api,
            onDemoSucceeded,
            onDemoFailed,
            sCardContext,
            sCardHandle),
        onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onStatusGot(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    sCardHandle,
    readerName,
    state,
    protocol,
    atr) {
  logger.info('Card status obtained: reader name is ' + dump(readerName) +
               ', ' + 'state is ' + dump(state) + ', protocol is ' +
               getProtocolDebugRepresentation(protocol) + ', atr is ' +
               dump(atr));
  getAttrs(
      api,
      onDemoSucceeded,
      onDemoFailed,
      sCardContext,
      sCardHandle,
      protocol,
      0);
}

function getAttrs(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    sCardHandle,
    protocol,
    attrIndex) {
  const attrNames = getPcscLiteAttrNames();
  if (attrIndex == attrNames.length) {
    onAttrsGot(
        api,
        onDemoSucceeded,
        onDemoFailed,
        sCardContext,
        sCardHandle,
        protocol);
    return;
  }
  const attrName = attrNames[attrIndex];
  logger.fine('Requesting the ' + dump(attrName) + ' attribute...');
  api.SCardGetAttrib(
      sCardHandle, API[attrName]).then(function(result) {
        result.get(function(attrValue) {
          logger.info('The ' + dump(attrName) + ' attribute value is: ' +
                     dump(attrValue));
          getAttrs(
              api,
              onDemoSucceeded,
              onDemoFailed,
              sCardContext,
              sCardHandle,
              protocol,
              attrIndex + 1);
        }, function(error) {
          getAttrs(
              api,
              onDemoSucceeded,
              onDemoFailed,
              sCardContext,
              sCardHandle,
              protocol,
              attrIndex + 1);
        });
      }, onFailure.bind(null, onDemoFailed));
}

function onAttrsGot(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  setAttr(
      api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol);
}

function setAttr(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  const ATTR_NAME = 'SCARD_ATTR_DEVICE_FRIENDLY_NAME_A';
  const ATTR_VALUE = [0x54, 0x65, 0x73, 0x74];
  logger.info(
      'Setting the ' + dump(ATTR_NAME) + ' attribute to ' + dump(ATTR_VALUE));
  api.SCardSetAttrib(sCardHandle, API[ATTR_NAME], ATTR_VALUE)
      .then(function(result) {
        result.get(
            onAttrSet.bind(
                null,
                api,
                onDemoSucceeded,
                onDemoFailed,
                sCardContext,
                sCardHandle,
                protocol),
            onAttrSetFailed.bind(
                null,
                api,
                onDemoSucceeded,
                onDemoFailed,
                sCardContext,
                sCardHandle,
                protocol));
      }, onFailure.bind(null, onDemoFailed));
}

function onAttrSet(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  logger.info('The attribute was set successfully');
  beginTransaction(
      api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol);
}

function onAttrSetFailed(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    sCardHandle,
    protocol,
    errorCode) {
  logger.info('The attribute set was unsuccessful (error code: ' +
              dump(errorCode) + ')');
  beginTransaction(
      api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol);
}

function beginTransaction(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  logger.info('Beginning transaction...');
  api.SCardBeginTransaction(sCardHandle).then(function(result) {
    result.get(
        onTransactionBegun.bind(
            null,
            api,
            onDemoSucceeded,
            onDemoFailed,
            sCardContext,
            sCardHandle,
            protocol),
        onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onTransactionBegun(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  logger.info('Transaction begun successfully');
  sendControlCommand(
      api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol);
}

function sendControlCommand(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  logger.info('Sending the CM_IOCTL_GET_FEATURE_REQUEST control command...');
  api.SCardControl(
      sCardHandle,
      API.CM_IOCTL_GET_FEATURE_REQUEST,
      []).then(function(result) {
        result.get(
            onControlCommandSent.bind(
                null,
                api,
                onDemoSucceeded,
                onDemoFailed,
                sCardContext,
                sCardHandle,
                protocol),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onControlCommandSent(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    sCardHandle,
    protocol,
    responseData) {
  logger.info('The CM_IOCTL_GET_FEATURE_REQUEST control command returned: ' +
              dump(responseData));
  sendTransmitCommand(
      api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol);
}

function sendTransmitCommand(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle, protocol) {
  const LIST_DIR_APDU = [0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00, 0x00];
  logger.info(
      'Sending the transmit command with "list dir" APDU ' +
      dump(LIST_DIR_APDU) + '...');
  api.SCardTransmit(
      sCardHandle,
      protocol == API.SCARD_PROTOCOL_T0 ?
          API.SCARD_PCI_T0 :
          API.SCARD_PCI_T1,
      LIST_DIR_APDU).then(function(result) {
        result.get(
            onTransmitCommandSent.bind(
                null,
                api,
                onDemoSucceeded,
                onDemoFailed,
                sCardContext,
                sCardHandle,
                protocol),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onTransmitCommandSent(
    api,
    onDemoSucceeded,
    onDemoFailed,
    sCardContext,
    sCardHandle,
    protocol,
    responseProtocolInformation,
    responseData) {
  logger.info(
      'The transmit command returned: ' + dump(responseData) + ', the ' +
      'response protocol information is: ' + dump(responseProtocolInformation));
  endTransaction(api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle);
}

function endTransaction(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle) {
  logger.info('Ending the transaction...');
  api.SCardEndTransaction(
      sCardHandle,
      API.SCARD_LEAVE_CARD).then(function(result) {
        result.get(
            onTransactionEnded.bind(
                null,
                api,
                onDemoSucceeded,
                onDemoFailed,
                sCardContext,
                sCardHandle),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onTransactionEnded(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle) {
  logger.info('Transaction ended successfully');
  disconnect(api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle);
}

function disconnect(
    api, onDemoSucceeded, onDemoFailed, sCardContext, sCardHandle) {
  logger.info('Disconnecting from the reader...');
  api.SCardDisconnect(
      sCardHandle,
      API.SCARD_LEAVE_CARD).then(function(result) {
        result.get(
            onDisconnected.bind(
                null, api, onDemoSucceeded, onDemoFailed, sCardContext),
            onPcscLiteError.bind(null, api, onDemoFailed));
      }, onFailure.bind(null, onDemoFailed));
}

function onDisconnected(api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Disconnected successfully');
  releaseContext(api, onDemoSucceeded, onDemoFailed, sCardContext);
}

function releaseContext(api, onDemoSucceeded, onDemoFailed, sCardContext) {
  logger.info('Releasing the context...');
  api.SCardReleaseContext(sCardContext).then(function(result) {
    result.get(
        onContextReleased.bind(null, api, onDemoSucceeded, onDemoFailed),
        onPcscLiteError.bind(null, api, onDemoFailed));
  }, onFailure.bind(null, onDemoFailed));
}

function onContextReleased(api, onDemoSucceeded, onDemoFailed) {
  logger.info('Released the context');
  onDemoSucceeded();
}

function onPcscLiteError(api, onDemoFailed, errorCode) {
  logger.warning('failed: PC/SC-Lite error: ' + dump(errorCode));
  api.pcsc_stringify_error(errorCode).then(function(errorText) {
    logger.warning('PC/SC-Lite error text: ' + errorText);
    onDemoFailed();
  }, function() {
    onDemoFailed();
  });
}

function onFailure(onDemoFailed, error) {
  logger.warning('failed: ' + error);
  onDemoFailed();
}

function getPcscLiteAttrNames() {
  return goog.array.filter(
      goog.object.getKeys(API),
      function(key) {
        return key.startsWith('SCARD_ATTR_') && key != 'SCARD_ATTR_VALUE';
      });
}

function getProtocolDebugRepresentation(protocol) {
  if (protocol == API.SCARD_PROTOCOL_T0)
    return 'T0';
  if (protocol == API.SCARD_PROTOCOL_T1)
    return 'T1';
  GSC.Logging.failWithLogger(
      logger, 'Unknown protocol value: ' + dump(protocol));
}

});  // goog.scope
