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

goog.provide('GoogleSmartCard.PcscLiteClient.API');
goog.provide('GoogleSmartCard.PcscLiteClient.API.ResultOrErrorCode');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCARD_IO_REQUEST');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCARD_READERSTATE_IN');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCARD_READERSTATE_OUT');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardBeginTransactionResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardCancelResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardConnectResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardControlResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardDisconnectResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardEndTransactionResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardEstablishContextResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardGetAttribResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardGetStatusChangeResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardIsValidContextResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardListReaderGroupsResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardListReadersResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardReconnectResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardReleaseContextResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardSetAttribResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardStatusResult');
goog.provide('GoogleSmartCard.PcscLiteClient.API.SCardTransmitResult');

goog.require('GoogleSmartCard.FixedSizeInteger');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;
/** @const */
var Constants = GSC.PcscLiteCommon.Constants;
/** @const */
var castToInt32 = GSC.FixedSizeInteger.castToInt32;

/**
 * JavaScript version of the PC/SC-Lite API (see
 * <a href="http://pcsclite.alioth.debian.org/api/group__API.html">
 * http://pcsclite.alioth.debian.org/api/group__API.html</a>).
 *
 * The major difference from the original PC/SC-Lite API is the asynchronicity
 * of all blocking API function calls. Most functions in this JavaScript version
 * return a promise that is either fulfilled with the operation result once it
 * succeeds, or is rejected if the operation finally fails.
 *
 * This implementation transforms each API call into a message that is sent
 * through the passed message channel. The results are also received through
 * this message channel.
 *
 * Please note that, as well as in the original PC/SC-Lite API, concurrent
 * operations using the same SCARDCONTEXT are not allowed (with the only
 * exception of the {@code API.prototype.SCardCancel} method).
 *
 * @param {!goog.messaging.AbstractChannel} messageChannel Message channel that
 * will be used to send requests and receive responses.
 * @constructor
 * @extends goog.Disposable
 */
GSC.PcscLiteClient.API = function(messageChannel) {
  API.base(this, 'constructor');

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('PcscLiteClient.API');

  /** @private */
  this.messageChannel_ = messageChannel;
  this.messageChannel_.addOnDisposeCallback(
      this.messageChannelDisposedListener_.bind(this));

  /** @private */
  this.requester_ = new GSC.Requester(
      Constants.REQUESTER_TITLE, this.messageChannel_);

  this.logger.fine('Initialized');
};

/** @const */
var API = GSC.PcscLiteClient.API;

goog.inherits(API, goog.Disposable);

goog.exportSymbol('GoogleSmartCard.PcscLiteClient.API', API);


/*
 * Following are constants and type definitions borrowed from the PC/SC-Lite
 * header files (the last update was the 1.8.14 PC/SC-Lite version).
 */

/**
 * Context handle returned by SCardEstablishContext.
 * @typedef {number}
 */
API.SCARDCONTEXT;
/**
 * Card handle returned by SCardConnect.
 * @typedef {number}
 */
API.SCARDHANDLE;

/**
 * Error code.
 * @typedef {number}
 */
API.ERROR_CODE;

/**
 * Maximum ATR size
 * @const
 */
API.MAX_ATR_SIZE = 33;

goog.exportProperty(API, 'MAX_ATR_SIZE', API.MAX_ATR_SIZE);

/*
 * error codes from http://msdn.microsoft.com/en-us/library/aa924526.aspx
 */

/**
 * No error was encountered.
 * @type {number}
 * @const
 */
API.SCARD_S_SUCCESS = castToInt32(0x00000000);

goog.exportProperty(API, 'SCARD_S_SUCCESS', API.SCARD_S_SUCCESS);

/**
 * An internal consistency check failed.
 * @type {number}
 * @const
 */
API.SCARD_F_INTERNAL_ERROR = castToInt32(0x80100001);

goog.exportProperty(API, 'SCARD_F_INTERNAL_ERROR', API.SCARD_F_INTERNAL_ERROR);

/**
 * The action was cancelled by an SCardCancel request.
 * @type {number}
 * @const
 */
API.SCARD_E_CANCELLED = castToInt32(0x80100002);

goog.exportProperty(API, 'SCARD_E_CANCELLED', API.SCARD_E_CANCELLED);

/**
 * The supplied handle was invalid.
 * @type {number}
 * @const
 */
API.SCARD_E_INVALID_HANDLE = castToInt32(0x80100003);

goog.exportProperty(API, 'SCARD_E_INVALID_HANDLE', API.SCARD_E_INVALID_HANDLE);

/**
 * One or more of the supplied parameters could not be properly interpreted.
 * @type {number}
 * @const
 */
API.SCARD_E_INVALID_PARAMETER = castToInt32(0x80100004);

goog.exportProperty(
    API, 'SCARD_E_INVALID_PARAMETER', API.SCARD_E_INVALID_PARAMETER);

/**
 * Registry startup information is missing or invalid.
 * @type {number}
 * @const
 */
API.SCARD_E_INVALID_TARGET = castToInt32(0x80100005);

goog.exportProperty(API, 'SCARD_E_INVALID_TARGET', API.SCARD_E_INVALID_TARGET);

/**
 * Not enough memory available to complete this command.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_MEMORY = castToInt32(0x80100006);

goog.exportProperty(API, 'SCARD_E_NO_MEMORY', API.SCARD_E_NO_MEMORY);

/**
 * An internal consistency timer has expired.
 * @type {number}
 * @const
 */
API.SCARD_F_WAITED_TOO_LONG = castToInt32(0x80100007);

goog.exportProperty(
    API, 'SCARD_F_WAITED_TOO_LONG', API.SCARD_F_WAITED_TOO_LONG);

/**
 * The data buffer to receive returned data is too small for the returned data.
 * @type {number}
 * @const
 */
API.SCARD_E_INSUFFICIENT_BUFFER = castToInt32(0x80100008);

goog.exportProperty(
    API, 'SCARD_E_INSUFFICIENT_BUFFER', API.SCARD_E_INSUFFICIENT_BUFFER);

/**
 * The specified reader name is not recognized.
 * @type {number}
 * @const
 */
API.SCARD_E_UNKNOWN_READER = castToInt32(0x80100009);

goog.exportProperty(API, 'SCARD_E_UNKNOWN_READER', API.SCARD_E_UNKNOWN_READER);

/**
 * The user-specified timeout value has expired.
 * @type {number}
 * @const
 */
API.SCARD_E_TIMEOUT = castToInt32(0x8010000A);

goog.exportProperty(API, 'SCARD_E_TIMEOUT', API.SCARD_E_TIMEOUT);

/**
 * The smart card cannot be accessed because of other connections outstanding.
 * @type {number}
 * @const
 */
API.SCARD_E_SHARING_VIOLATION = castToInt32(0x8010000B);

goog.exportProperty(
    API, 'SCARD_E_SHARING_VIOLATION', API.SCARD_E_SHARING_VIOLATION);

/**
 * The operation requires a Smart Card, but no Smart Card is currently in the
 * device.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_SMARTCARD = castToInt32(0x8010000C);

goog.exportProperty(API, 'SCARD_E_NO_SMARTCARD', API.SCARD_E_NO_SMARTCARD);

/**
 * The specified smart card name is not recognized.
 * @type {number}
 * @const
 */
API.SCARD_E_UNKNOWN_CARD = castToInt32(0x8010000D);

goog.exportProperty(API, 'SCARD_E_UNKNOWN_CARD', API.SCARD_E_UNKNOWN_CARD);

/**
 * The system could not dispose of the media in the requested manner.
 * @type {number}
 * @const
 */
API.SCARD_E_CANT_DISPOSE = castToInt32(0x8010000E);

goog.exportProperty(API, 'SCARD_E_CANT_DISPOSE', API.SCARD_E_CANT_DISPOSE);

/**
 * The requested protocols are incompatible with the protocol currently in use
 * with the smart card.
 * @type {number}
 * @const
 */
API.SCARD_E_PROTO_MISMATCH = castToInt32(0x8010000F);

goog.exportProperty(API, 'SCARD_E_PROTO_MISMATCH', API.SCARD_E_PROTO_MISMATCH);

/**
 * The reader or smart card is not ready to accept commands.
 * @type {number}
 * @const
 */
API.SCARD_E_NOT_READY = castToInt32(0x80100010);

goog.exportProperty(API, 'SCARD_E_NOT_READY', API.SCARD_E_NOT_READY);

/**
 * One or more of the supplied parameters values could not be properly
 * interpreted.
 * @type {number}
 * @const
 */
API.SCARD_E_INVALID_VALUE = castToInt32(0x80100011);

goog.exportProperty(API, 'SCARD_E_INVALID_VALUE', API.SCARD_E_INVALID_VALUE);

/**
 * The action was cancelled by the system, presumably to log off or shut down.
 * @type {number}
 * @const
 */
API.SCARD_E_SYSTEM_CANCELLED = castToInt32(0x80100012);

goog.exportProperty(
    API, 'SCARD_E_SYSTEM_CANCELLED', API.SCARD_E_SYSTEM_CANCELLED);

/**
 * An internal communications error has been detected.
 * @type {number}
 * @const
 */
API.SCARD_F_COMM_ERROR = castToInt32(0x80100013);

goog.exportProperty(API, 'SCARD_F_COMM_ERROR', API.SCARD_F_COMM_ERROR);

/**
 * An internal error has been detected, but the source is unknown.
 * @type {number}
 * @const
 */
API.SCARD_F_UNKNOWN_ERROR = castToInt32(0x80100014);

goog.exportProperty(API, 'SCARD_F_UNKNOWN_ERROR', API.SCARD_F_UNKNOWN_ERROR);

/**
 * An ATR obtained from the registry is not a valid ATR string.
 * @type {number}
 * @const
 */
API.SCARD_E_INVALID_ATR = castToInt32(0x80100015);

goog.exportProperty(API, 'SCARD_E_INVALID_ATR', API.SCARD_E_INVALID_ATR);

/**
 * An attempt was made to end a non-existent transaction.
 * @type {number}
 * @const
 */
API.SCARD_E_NOT_TRANSACTED = castToInt32(0x80100016);

goog.exportProperty(API, 'SCARD_E_NOT_TRANSACTED', API.SCARD_E_NOT_TRANSACTED);

/**
 * The specified reader is not currently available for use.
 * @type {number}
 * @const
 */
API.SCARD_E_READER_UNAVAILABLE = castToInt32(0x80100017);

goog.exportProperty(
    API, 'SCARD_E_READER_UNAVAILABLE', API.SCARD_E_READER_UNAVAILABLE);

/**
 * The operation has been aborted to allow the server application to exit.
 * @type {number}
 * @const
 */
API.SCARD_P_SHUTDOWN = castToInt32(0x80100018);

goog.exportProperty(API, 'SCARD_P_SHUTDOWN', API.SCARD_P_SHUTDOWN);

/**
 * The PCI Receive buffer was too small.
 * @type {number}
 * @const
 */
API.SCARD_E_PCI_TOO_SMALL = castToInt32(0x80100019);

goog.exportProperty(API, 'SCARD_E_PCI_TOO_SMALL', API.SCARD_E_PCI_TOO_SMALL);

/**
 * The reader driver does not meet minimal requirements for support.
 * @type {number}
 * @const
 */
API.SCARD_E_READER_UNSUPPORTED = castToInt32(0x8010001A);

goog.exportProperty(
    API, 'SCARD_E_READER_UNSUPPORTED', API.SCARD_E_READER_UNSUPPORTED);

/**
 * The reader driver did not produce a unique reader name.
 * @type {number}
 * @const
 */
API.SCARD_E_DUPLICATE_READER = castToInt32(0x8010001B);

goog.exportProperty(
    API, 'SCARD_E_DUPLICATE_READER', API.SCARD_E_DUPLICATE_READER);

/**
 * The smart card does not meet minimal requirements for support.
 * @type {number}
 * @const
 */
API.SCARD_E_CARD_UNSUPPORTED = castToInt32(0x8010001C);

goog.exportProperty(
    API, 'SCARD_E_CARD_UNSUPPORTED', API.SCARD_E_CARD_UNSUPPORTED);

/**
 * The Smart card resource manager is not running.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_SERVICE = castToInt32(0x8010001D);

goog.exportProperty(API, 'SCARD_E_NO_SERVICE', API.SCARD_E_NO_SERVICE);

/**
 * The Smart card resource manager has shut down.
 * @type {number}
 * @const
 */
API.SCARD_E_SERVICE_STOPPED = castToInt32(0x8010001E);

goog.exportProperty(
    API, 'SCARD_E_SERVICE_STOPPED', API.SCARD_E_SERVICE_STOPPED);

/**
 * An unexpected card error has occurred.
 * @type {number}
 * @const
 */
API.SCARD_E_UNEXPECTED = castToInt32(0x8010001F);

goog.exportProperty(API, 'SCARD_E_UNEXPECTED', API.SCARD_E_UNEXPECTED);

/**
 * This smart card does not support the requested feature.
 * @type {number}
 * @const
 */
API.SCARD_E_UNSUPPORTED_FEATURE = castToInt32(0x8010001F);

goog.exportProperty(
    API, 'SCARD_E_UNSUPPORTED_FEATURE', API.SCARD_E_UNSUPPORTED_FEATURE);

/**
 * No primary provider can be found for the smart card.
 * @type {number}
 * @const
 */
API.SCARD_E_ICC_INSTALLATION = castToInt32(0x80100020);

goog.exportProperty(
    API, 'SCARD_E_ICC_INSTALLATION', API.SCARD_E_ICC_INSTALLATION);

/**
 * The requested order of object creation is not supported.
 * @type {number}
 * @const
 */
API.SCARD_E_ICC_CREATEORDER = castToInt32(0x80100021);

goog.exportProperty(
    API, 'SCARD_E_ICC_CREATEORDER', API.SCARD_E_ICC_CREATEORDER);

/**
 * The identified directory does not exist in the smart card.
 * @type {number}
 * @const
 */
API.SCARD_E_DIR_NOT_FOUND = castToInt32(0x80100023);

goog.exportProperty(API, 'SCARD_E_DIR_NOT_FOUND', API.SCARD_E_DIR_NOT_FOUND);

/**
 * The identified file does not exist in the smart card.
 * @type {number}
 * @const
 */
API.SCARD_E_FILE_NOT_FOUND = castToInt32(0x80100024);

goog.exportProperty(API, 'SCARD_E_FILE_NOT_FOUND', API.SCARD_E_FILE_NOT_FOUND);

/**
 * The supplied path does not represent a smart card directory.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_DIR = castToInt32(0x80100025);

goog.exportProperty(API, 'SCARD_E_NO_DIR', API.SCARD_E_NO_DIR);

/**
 * The supplied path does not represent a smart card file.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_FILE = castToInt32(0x80100026);

goog.exportProperty(API, 'SCARD_E_NO_FILE', API.SCARD_E_NO_FILE);

/**
 * Access is denied to this file.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_ACCESS = castToInt32(0x80100027);

goog.exportProperty(API, 'SCARD_E_NO_ACCESS', API.SCARD_E_NO_ACCESS);

/**
 * The smart card does not have enough memory to store the information.
 * @type {number}
 * @const
 */
API.SCARD_E_WRITE_TOO_MANY = castToInt32(0x80100028);

goog.exportProperty(API, 'SCARD_E_WRITE_TOO_MANY', API.SCARD_E_WRITE_TOO_MANY);

/**
 * There was an error trying to set the smart card file object pointer.
 * @type {number}
 * @const
 */
API.SCARD_E_BAD_SEEK = castToInt32(0x80100029);

goog.exportProperty(API, 'SCARD_E_BAD_SEEK', API.SCARD_E_BAD_SEEK);

/**
 * The supplied PIN is incorrect.
 * @type {number}
 * @const
 */
API.SCARD_E_INVALID_CHV = castToInt32(0x8010002A);

goog.exportProperty(API, 'SCARD_E_INVALID_CHV', API.SCARD_E_INVALID_CHV);

/**
 * An unrecognized error code was returned from a layered component.
 * @type {number}
 * @const
 */
API.SCARD_E_UNKNOWN_RES_MNG = castToInt32(0x8010002B);

goog.exportProperty(
    API, 'SCARD_E_UNKNOWN_RES_MNG', API.SCARD_E_UNKNOWN_RES_MNG);

/**
 * The requested certificate does not exist.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_SUCH_CERTIFICATE = castToInt32(0x8010002C);

goog.exportProperty(
    API, 'SCARD_E_NO_SUCH_CERTIFICATE', API.SCARD_E_NO_SUCH_CERTIFICATE);

/**
 * The requested certificate could not be obtained.
 * @type {number}
 * @const
 */
API.SCARD_E_CERTIFICATE_UNAVAILABLE = castToInt32(0x8010002D);

goog.exportProperty(
    API,
    'SCARD_E_CERTIFICATE_UNAVAILABLE',
    API.SCARD_E_CERTIFICATE_UNAVAILABLE);

/**
 * Cannot find a smart card reader.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_READERS_AVAILABLE = castToInt32(0x8010002E);

goog.exportProperty(
    API, 'SCARD_E_NO_READERS_AVAILABLE', API.SCARD_E_NO_READERS_AVAILABLE);

/**
 * A communications error with the smart card has been detected. Retry the
 * operation.
 * @type {number}
 * @const
 */
API.SCARD_E_COMM_DATA_LOST = castToInt32(0x8010002F);

goog.exportProperty(API, 'SCARD_E_COMM_DATA_LOST', API.SCARD_E_COMM_DATA_LOST);

/**
 * The requested key container does not exist on the smart card.
 * @type {number}
 * @const
 */
API.SCARD_E_NO_KEY_CONTAINER = castToInt32(0x80100030);

goog.exportProperty(
    API, 'SCARD_E_NO_KEY_CONTAINER', API.SCARD_E_NO_KEY_CONTAINER);

/**
 * The Smart Card Resource Manager is too busy to complete this operation.
 * @type {number}
 * @const
 */
API.SCARD_E_SERVER_TOO_BUSY = castToInt32(0x80100031);

goog.exportProperty(
    API, 'SCARD_E_SERVER_TOO_BUSY', API.SCARD_E_SERVER_TOO_BUSY);

/**
 * The reader cannot communicate with the card, due to ATR string configuration
 * conflicts.
 * @type {number}
 * @const
 */
API.SCARD_W_UNSUPPORTED_CARD = castToInt32(0x80100065);

goog.exportProperty(
    API, 'SCARD_W_UNSUPPORTED_CARD', API.SCARD_W_UNSUPPORTED_CARD);

/**
 * The smart card is not responding to a reset.
 * @type {number}
 * @const
 */
API.SCARD_W_UNRESPONSIVE_CARD = castToInt32(0x80100066);

goog.exportProperty(
    API, 'SCARD_W_UNRESPONSIVE_CARD', API.SCARD_W_UNRESPONSIVE_CARD);

/**
 * Power has been removed from the smart card, so that further communication is
 * not possible.
 * @type {number}
 * @const
 */
API.SCARD_W_UNPOWERED_CARD = castToInt32(0x80100067);

goog.exportProperty(API, 'SCARD_W_UNPOWERED_CARD', API.SCARD_W_UNPOWERED_CARD);

/**
 * The smart card has been reset, so any shared state information is invalid.
 * @type {number}
 * @const
 */
API.SCARD_W_RESET_CARD = castToInt32(0x80100068);

goog.exportProperty(API, 'SCARD_W_RESET_CARD', API.SCARD_W_RESET_CARD);

/**
 * The smart card has been removed, so further communication is not possible.
 * @type {number}
 * @const
 */
API.SCARD_W_REMOVED_CARD = castToInt32(0x80100069);

goog.exportProperty(API, 'SCARD_W_REMOVED_CARD', API.SCARD_W_REMOVED_CARD);

/**
 * Access was denied because of a security violation.
 * @type {number}
 * @const
 */
API.SCARD_W_SECURITY_VIOLATION = castToInt32(0x8010006A);

goog.exportProperty(
    API, 'SCARD_W_SECURITY_VIOLATION', API.SCARD_W_SECURITY_VIOLATION);

/**
 * The card cannot be accessed because the wrong PIN was presented.
 * @type {number}
 * @const
 */
API.SCARD_W_WRONG_CHV = castToInt32(0x8010006B);

goog.exportProperty(API, 'SCARD_W_WRONG_CHV', API.SCARD_W_WRONG_CHV);

/**
 * The card cannot be accessed because the maximum number of PIN entry attempts
 * has been reached.
 * @type {number}
 * @const
 */
API.SCARD_W_CHV_BLOCKED = castToInt32(0x8010006C);

goog.exportProperty(API, 'SCARD_W_CHV_BLOCKED', API.SCARD_W_CHV_BLOCKED);

/**
 * The end of the smart card file has been reached.
 * @type {number}
 * @const
 */
API.SCARD_W_EOF = castToInt32(0x8010006D);

goog.exportProperty(API, 'SCARD_W_EOF', API.SCARD_W_EOF);

/**
 * The user pressed "Cancel" on a Smart Card Selection Dialog.
 * @type {number}
 * @const
 */
API.SCARD_W_CANCELLED_BY_USER = castToInt32(0x8010006E);

goog.exportProperty(
    API, 'SCARD_W_CANCELLED_BY_USER', API.SCARD_W_CANCELLED_BY_USER);

/**
 * No PIN was presented to the smart card.
 * @type {number}
 * @const
 */
API.SCARD_W_CARD_NOT_AUTHENTICATED = castToInt32(0x8010006F);

goog.exportProperty(
    API, 'SCARD_W_CARD_NOT_AUTHENTICATED', API.SCARD_W_CARD_NOT_AUTHENTICATED);

/**
 * see SCardFreeMemory()
 * @type {number}
 * @const
 */
API.SCARD_AUTOALLOCATE = castToInt32(0xFFFFFFFF);

goog.exportProperty(API, 'SCARD_AUTOALLOCATE', API.SCARD_AUTOALLOCATE);

/**
 * Scope in user space
 * @type {number}
 * @const
 */
API.SCARD_SCOPE_USER = 0x0000;

goog.exportProperty(API, 'SCARD_SCOPE_USER', API.SCARD_SCOPE_USER);

/**
 * Scope in terminal
 * @type {number}
 * @const
 */
API.SCARD_SCOPE_TERMINAL = 0x0001;

goog.exportProperty(API, 'SCARD_SCOPE_TERMINAL', API.SCARD_SCOPE_TERMINAL);

/**
 * Scope in system
 * @type {number}
 * @const
 */
API.SCARD_SCOPE_SYSTEM = 0x0002;

goog.exportProperty(API, 'SCARD_SCOPE_SYSTEM', API.SCARD_SCOPE_SYSTEM);

/**
 * protocol not set
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_UNDEFINED = 0x0000;

goog.exportProperty(
    API, 'SCARD_PROTOCOL_UNDEFINED', API.SCARD_PROTOCOL_UNDEFINED);

/**
 * backward compat
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_UNSET = API.SCARD_PROTOCOL_UNDEFINED;

goog.exportProperty(API, 'SCARD_PROTOCOL_UNSET', API.SCARD_PROTOCOL_UNSET);

/**
 * T=0 active protocol.
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_T0 = 0x0001;

goog.exportProperty(API, 'SCARD_PROTOCOL_T0', API.SCARD_PROTOCOL_T0);

/**
 * T=1 active protocol.
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_T1 = 0x0002;

goog.exportProperty(API, 'SCARD_PROTOCOL_T1', API.SCARD_PROTOCOL_T1);

/**
 * Raw active protocol.
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_RAW = 0x0004;

goog.exportProperty(API, 'SCARD_PROTOCOL_RAW', API.SCARD_PROTOCOL_RAW);

/**
 * T=15 protocol.
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_T15 = 0x0008;

goog.exportProperty(API, 'SCARD_PROTOCOL_T15', API.SCARD_PROTOCOL_T15);

/**
 * IFD determines prot.
 * @type {number}
 * @const
 */
API.SCARD_PROTOCOL_ANY = API.SCARD_PROTOCOL_T0 | API.SCARD_PROTOCOL_T1;

goog.exportProperty(API, 'SCARD_PROTOCOL_ANY', API.SCARD_PROTOCOL_ANY);

/**
 * Exclusive mode only
 * @type {number}
 * @const
 */
API.SCARD_SHARE_EXCLUSIVE = 0x0001;

goog.exportProperty(API, 'SCARD_SHARE_EXCLUSIVE', API.SCARD_SHARE_EXCLUSIVE);

/**
 * Shared mode only
 * @type {number}
 * @const
 */
API.SCARD_SHARE_SHARED = 0x0002;

goog.exportProperty(API, 'SCARD_SHARE_SHARED', API.SCARD_SHARE_SHARED);

/**
 * Raw mode only
 * @type {number}
 * @const
 */
API.SCARD_SHARE_DIRECT = 0x0003;

goog.exportProperty(API, 'SCARD_SHARE_DIRECT', API.SCARD_SHARE_DIRECT);

/**
 * Do nothing on close
 * @type {number}
 * @const
 */
API.SCARD_LEAVE_CARD = 0x0000;

goog.exportProperty(API, 'SCARD_LEAVE_CARD', API.SCARD_LEAVE_CARD);

/**
 * Reset on close
 * @type {number}
 * @const
 */
API.SCARD_RESET_CARD = 0x0001;

goog.exportProperty(API, 'SCARD_RESET_CARD', API.SCARD_RESET_CARD);

/**
 * Power down on close
 * @type {number}
 * @const
 */
API.SCARD_UNPOWER_CARD = 0x0002;

goog.exportProperty(API, 'SCARD_UNPOWER_CARD', API.SCARD_UNPOWER_CARD);

/**
 * Eject on close
 * @type {number}
 * @const
 */
API.SCARD_EJECT_CARD = 0x0003;

goog.exportProperty(API, 'SCARD_EJECT_CARD', API.SCARD_EJECT_CARD);

/**
 * Unknown state
 * @type {number}
 * @const
 */
API.SCARD_UNKNOWN = 0x0001;

goog.exportProperty(API, 'SCARD_UNKNOWN', API.SCARD_UNKNOWN);

/**
 * Card is absent
 * @type {number}
 * @const
 */
API.SCARD_ABSENT = 0x0002;

goog.exportProperty(API, 'SCARD_ABSENT', API.SCARD_ABSENT);

/**
 * Card is present
 * @type {number}
 * @const
 */
API.SCARD_PRESENT = 0x0004;

goog.exportProperty(API, 'SCARD_PRESENT', API.SCARD_PRESENT);

/**
 * Card not powered
 * @type {number}
 * @const
 */
API.SCARD_SWALLOWED = 0x0008;

goog.exportProperty(API, 'SCARD_SWALLOWED', API.SCARD_SWALLOWED);

/**
 * Card is powered
 * @type {number}
 * @const
 */
API.SCARD_POWERED = 0x0010;

goog.exportProperty(API, 'SCARD_POWERED', API.SCARD_POWERED);

/**
 * Ready for PTS
 * @type {number}
 * @const
 */
API.SCARD_NEGOTIABLE = 0x0020;

goog.exportProperty(API, 'SCARD_NEGOTIABLE', API.SCARD_NEGOTIABLE);

/**
 * PTS has been set
 * @type {number}
 * @const
 */
API.SCARD_SPECIFIC = 0x0040;

goog.exportProperty(API, 'SCARD_SPECIFIC', API.SCARD_SPECIFIC);

/**
 * App wants status
 * @type {number}
 * @const
 */
API.SCARD_STATE_UNAWARE = 0x0000;

goog.exportProperty(API, 'SCARD_STATE_UNAWARE', API.SCARD_STATE_UNAWARE);

/**
 * Ignore this reader
 * @type {number}
 * @const
 */
API.SCARD_STATE_IGNORE = 0x0001;

goog.exportProperty(API, 'SCARD_STATE_IGNORE', API.SCARD_STATE_IGNORE);

/**
 * State has changed
 * @type {number}
 * @const
 */
API.SCARD_STATE_CHANGED = 0x0002;

goog.exportProperty(API, 'SCARD_STATE_CHANGED', API.SCARD_STATE_CHANGED);

/**
 * Reader unknown
v * @const
 */
API.SCARD_STATE_UNKNOWN = 0x0004;

goog.exportProperty(API, 'SCARD_STATE_UNKNOWN', API.SCARD_STATE_UNKNOWN);

/**
 * Status unavailable
 * @type {number}
 * @const
 */
API.SCARD_STATE_UNAVAILABLE = 0x0008;

goog.exportProperty(
    API, 'SCARD_STATE_UNAVAILABLE', API.SCARD_STATE_UNAVAILABLE);

/**
 * Card removed
 * @type {number}
 * @const
 */
API.SCARD_STATE_EMPTY = 0x0010;

goog.exportProperty(API, 'SCARD_STATE_EMPTY', API.SCARD_STATE_EMPTY);

/**
 * Card inserted
 * @type {number}
 * @const
 */
API.SCARD_STATE_PRESENT = 0x0020;

goog.exportProperty(API, 'SCARD_STATE_PRESENT', API.SCARD_STATE_PRESENT);

/**
 * ATR matches card
 * @type {number}
 * @const
 */
API.SCARD_STATE_ATRMATCH = 0x0040;

goog.exportProperty(API, 'SCARD_STATE_ATRMATCH', API.SCARD_STATE_ATRMATCH);

/**
 * Exclusive Mode
 * @type {number}
 * @const
 */
API.SCARD_STATE_EXCLUSIVE = 0x0080;

goog.exportProperty(API, 'SCARD_STATE_EXCLUSIVE', API.SCARD_STATE_EXCLUSIVE);

/**
 * Shared Mode
 * @type {number}
 * @const
 */
API.SCARD_STATE_INUSE = 0x0100;

goog.exportProperty(API, 'SCARD_STATE_INUSE', API.SCARD_STATE_INUSE);

/**
 * Unresponsive card
 * @type {number}
 * @const
 */
API.SCARD_STATE_MUTE = 0x0200;

goog.exportProperty(API, 'SCARD_STATE_MUTE', API.SCARD_STATE_MUTE);

/**
 * Unpowered card
 * @type {number}
 * @const
 */
API.SCARD_STATE_UNPOWERED = 0x0400;

goog.exportProperty(API, 'SCARD_STATE_UNPOWERED', API.SCARD_STATE_UNPOWERED);

/**
 * Infinite timeout
 * @type {number}
 * @const
 */
API.INFINITE = 0xFFFFFFFF;

goog.exportProperty(API, 'INFINITE', API.INFINITE);

/**
 * Maximum readers context (a slot is count as a reader)
 * @const
 */
API.PCSCLITE_MAX_READERS_CONTEXTS = 16;

goog.exportProperty(
    API, 'PCSCLITE_MAX_READERS_CONTEXTS', API.PCSCLITE_MAX_READERS_CONTEXTS);

/** @const */
API.MAX_READERNAME = 128;

goog.exportProperty(API, 'MAX_READERNAME', API.MAX_READERNAME);

/**
 * Maximum ATR size
 * @const
 */
API.SCARD_ATR_LENGTH = API.MAX_ATR_SIZE;

goog.exportProperty(API, 'SCARD_ATR_LENGTH', API.SCARD_ATR_LENGTH);

/*
 * The message and buffer sizes must be multiples of 16.
 * The max message size must be at least large enough
 * to accomodate the transmit_struct
 */

/**
 * Maximum Tx/Rx Buffer for short APDU
 * @const
 */
API.MAX_BUFFER_SIZE = 264;

goog.exportProperty(API, 'MAX_BUFFER_SIZE', API.MAX_BUFFER_SIZE);

/**
 * enhanced (64K + APDU + Lc + Le + SW) Tx/Rx Buffer
 * @const
 */
API.MAX_BUFFER_SIZE_EXTENDED = 4 + 3 + (1 << 16) + 3 + 2;

goog.exportProperty(
    API, 'MAX_BUFFER_SIZE_EXTENDED', API.MAX_BUFFER_SIZE_EXTENDED);

/*
* Tags for requesting card and reader attributes
*/

/**
 * @param {number} class_
 * @param {number} tag
 * @return {number}
 */
API.SCARD_ATTR_VALUE = function(class_, tag) {
  return (class_ << 16) | tag;
};

goog.exportProperty(API, 'SCARD_ATTR_VALUE', API.SCARD_ATTR_VALUE);

/**
 * Vendor information definitions
 * @const
 */
API.SCARD_CLASS_VENDOR_INFO = 1;

goog.exportProperty(
    API, 'SCARD_CLASS_VENDOR_INFO', API.SCARD_CLASS_VENDOR_INFO);

/**
 * Communication definitions
 * @const
 */
API.SCARD_CLASS_COMMUNICATIONS = 2;

goog.exportProperty(
    API, 'SCARD_CLASS_COMMUNICATIONS', API.SCARD_CLASS_COMMUNICATIONS);

/**
 * Protocol definitions
 * @const
 */
API.SCARD_CLASS_PROTOCOL = 3;

goog.exportProperty(API, 'SCARD_CLASS_PROTOCOL', API.SCARD_CLASS_PROTOCOL);

/**
 * Power Management definitions
 * @const
 */
API.SCARD_CLASS_POWER_MGMT = 4;

goog.exportProperty(API, 'SCARD_CLASS_POWER_MGMT', API.SCARD_CLASS_POWER_MGMT);

/**
 * Security Assurance definitions
 * @const
 */
API.SCARD_CLASS_SECURITY = 5;

goog.exportProperty(API, 'SCARD_CLASS_SECURITY', API.SCARD_CLASS_SECURITY);

/**
 * Mechanical characteristic definitions
 * @const
 */
API.SCARD_CLASS_MECHANICAL = 6;

goog.exportProperty(API, 'SCARD_CLASS_MECHANICAL', API.SCARD_CLASS_MECHANICAL);

/**
 * Vendor specific definitions
 * @const
 */
API.SCARD_CLASS_VENDOR_DEFINED = 7;

goog.exportProperty(
    API, 'SCARD_CLASS_VENDOR_DEFINED', API.SCARD_CLASS_VENDOR_DEFINED);

/**
 * Interface Device Protocol options
 * @const
 */
API.SCARD_CLASS_IFD_PROTOCOL = 8;

goog.exportProperty(
    API, 'SCARD_CLASS_IFD_PROTOCOL', API.SCARD_CLASS_IFD_PROTOCOL);

/**
 * ICC State specific definitions
 * @const
 */
API.SCARD_CLASS_ICC_STATE = 9;

goog.exportProperty(API, 'SCARD_CLASS_ICC_STATE', API.SCARD_CLASS_ICC_STATE);

/**
 * System-specific definitions
 * @const
 */
API.SCARD_CLASS_SYSTEM = 0x7fff;

goog.exportProperty(API, 'SCARD_CLASS_SYSTEM', API.SCARD_CLASS_SYSTEM);

/**
 * Vendor name.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_VENDOR_NAME = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_INFO, 0x0100);

goog.exportProperty(API, 'SCARD_ATTR_VENDOR_NAME', API.SCARD_ATTR_VENDOR_NAME);

/**
 * Vendor-supplied interface device type (model designation of reader).
 * @type {number}
 * @const
 */
API.SCARD_ATTR_VENDOR_IFD_TYPE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_INFO, 0x0101);

goog.exportProperty(
    API, 'SCARD_ATTR_VENDOR_IFD_TYPE', API.SCARD_ATTR_VENDOR_IFD_TYPE);

/**
 * Vendor-supplied interface device version (DWORD in the form 0xMMmmbbbb where
 * MM = major version, mm = minor version, and bbbb = build number).
 * @type {number}
 * @const
 */
API.SCARD_ATTR_VENDOR_IFD_VERSION = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_INFO, 0x0102);

goog.exportProperty(
    API, 'SCARD_ATTR_VENDOR_IFD_VERSION', API.SCARD_ATTR_VENDOR_IFD_VERSION);

/**
 * Vendor-supplied interface device serial number.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_VENDOR_IFD_SERIAL_NO = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_INFO, 0x0103);

goog.exportProperty(
    API,
    'SCARD_ATTR_VENDOR_IFD_SERIAL_NO',
    API.SCARD_ATTR_VENDOR_IFD_SERIAL_NO);

/**
 * DWORD encoded as castToInt32(0xDDDDCCCC), where DDDD = data channel type and
 * CCCC = channel number
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CHANNEL_ID = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_COMMUNICATIONS, 0x0110);

goog.exportProperty(API, 'SCARD_ATTR_CHANNEL_ID', API.SCARD_ATTR_CHANNEL_ID);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ASYNC_PROTOCOL_TYPES = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0120);

goog.exportProperty(
    API,
    'SCARD_ATTR_ASYNC_PROTOCOL_TYPES',
    API.SCARD_ATTR_ASYNC_PROTOCOL_TYPES);

/**
 * Default clock rate, in kHz.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEFAULT_CLK = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0121);

goog.exportProperty(API, 'SCARD_ATTR_DEFAULT_CLK', API.SCARD_ATTR_DEFAULT_CLK);

/**
 * Maximum clock rate, in kHz.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_MAX_CLK = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0122);

goog.exportProperty(API, 'SCARD_ATTR_MAX_CLK', API.SCARD_ATTR_MAX_CLK);

/**
 * Default data rate, in bps.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEFAULT_DATA_RATE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0123);

goog.exportProperty(
    API, 'SCARD_ATTR_DEFAULT_DATA_RATE', API.SCARD_ATTR_DEFAULT_DATA_RATE);

/**
 * Maximum data rate, in bps.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_MAX_DATA_RATE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0124);

goog.exportProperty(
    API, 'SCARD_ATTR_MAX_DATA_RATE', API.SCARD_ATTR_MAX_DATA_RATE);

/**
 * Maximum bytes for information file size device.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_MAX_IFSD = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0125);

goog.exportProperty(API, 'SCARD_ATTR_MAX_IFSD', API.SCARD_ATTR_MAX_IFSD);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_SYNC_PROTOCOL_TYPES = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_PROTOCOL, 0x0126);

goog.exportProperty(
    API, 'SCARD_ATTR_SYNC_PROTOCOL_TYPES', API.SCARD_ATTR_SYNC_PROTOCOL_TYPES);

/**
 * Zero if device does not support power down while smart card is inserted.
 * Nonzero otherwise.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_POWER_MGMT_SUPPORT = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_POWER_MGMT, 0x0131);

goog.exportProperty(
    API, 'SCARD_ATTR_POWER_MGMT_SUPPORT', API.SCARD_ATTR_POWER_MGMT_SUPPORT);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SECURITY, 0x0140);

goog.exportProperty(
    API,
    'SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE',
    API.SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_USER_AUTH_INPUT_DEVICE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SECURITY, 0x0142);

goog.exportProperty(
    API,
    'SCARD_ATTR_USER_AUTH_INPUT_DEVICE',
    API.SCARD_ATTR_USER_AUTH_INPUT_DEVICE);

/**
 * DWORD indicating which mechanical characteristics are supported. If zero, no
 * special characteristics are supported. Note that multiple bits can be set
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CHARACTERISTICS = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_MECHANICAL, 0x0150);

goog.exportProperty(
    API, 'SCARD_ATTR_CHARACTERISTICS', API.SCARD_ATTR_CHARACTERISTICS);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_PROTOCOL_TYPE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0201);

goog.exportProperty(
    API,
    'SCARD_ATTR_CURRENT_PROTOCOL_TYPE',
    API.SCARD_ATTR_CURRENT_PROTOCOL_TYPE);

/**
 * Current clock rate, in kHz.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_CLK = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0202);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_CLK', API.SCARD_ATTR_CURRENT_CLK);

/**
 * Clock conversion factor.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_F = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0203);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_F', API.SCARD_ATTR_CURRENT_F);

/**
 * Bit rate conversion factor.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_D = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0204);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_D', API.SCARD_ATTR_CURRENT_D);

/**
 * Current guard time.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_N = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0205);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_N', API.SCARD_ATTR_CURRENT_N);

/**
 * Current work waiting time.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_W = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0206);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_W', API.SCARD_ATTR_CURRENT_W);

/**
 * Current byte size for information field size card.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_IFSC = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0207);

goog.exportProperty(
    API, 'SCARD_ATTR_CURRENT_IFSC', API.SCARD_ATTR_CURRENT_IFSC);

/**
 * Current byte size for information field size device.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_IFSD = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0208);

goog.exportProperty(
    API, 'SCARD_ATTR_CURRENT_IFSD', API.SCARD_ATTR_CURRENT_IFSD);

/**
 * Current block waiting time.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_BWT = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x0209);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_BWT', API.SCARD_ATTR_CURRENT_BWT);

/**
 * Current character waiting time.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_CWT = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x020a);

goog.exportProperty(API, 'SCARD_ATTR_CURRENT_CWT', API.SCARD_ATTR_CURRENT_CWT);

/**
 * Current error block control encoding.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_EBC_ENCODING = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x020b);

goog.exportProperty(
    API,
    'SCARD_ATTR_CURRENT_EBC_ENCODING',
    API.SCARD_ATTR_CURRENT_EBC_ENCODING);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_EXTENDED_BWT = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_IFD_PROTOCOL, 0x020c);

goog.exportProperty(
    API, 'SCARD_ATTR_EXTENDED_BWT', API.SCARD_ATTR_EXTENDED_BWT);

/**
 * Single byte indicating smart card presence
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ICC_PRESENCE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_ICC_STATE, 0x0300);

goog.exportProperty(
    API, 'SCARD_ATTR_ICC_PRESENCE', API.SCARD_ATTR_ICC_PRESENCE);

/**
 * Single byte. Zero if smart card electrical contact is not active; nonzero if
 * contact is active.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ICC_INTERFACE_STATUS = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_ICC_STATE, 0x0301);

goog.exportProperty(
    API,
    'SCARD_ATTR_ICC_INTERFACE_STATUS',
    API.SCARD_ATTR_ICC_INTERFACE_STATUS);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_CURRENT_IO_STATE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_ICC_STATE, 0x0302);

goog.exportProperty(
    API, 'SCARD_ATTR_CURRENT_IO_STATE', API.SCARD_ATTR_CURRENT_IO_STATE);

/**
 * Answer to reset (ATR) string.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ATR_STRING = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_ICC_STATE, 0x0303);

goog.exportProperty(API, 'SCARD_ATTR_ATR_STRING', API.SCARD_ATTR_ATR_STRING);

/**
 * Single byte indicating smart card type
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ICC_TYPE_PER_ATR = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_ICC_STATE, 0x0304);

goog.exportProperty(
    API, 'SCARD_ATTR_ICC_TYPE_PER_ATR', API.SCARD_ATTR_ICC_TYPE_PER_ATR);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ESC_RESET = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_DEFINED, 0xA000);

goog.exportProperty(API, 'SCARD_ATTR_ESC_RESET', API.SCARD_ATTR_ESC_RESET);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ESC_CANCEL = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_DEFINED, 0xA003);

goog.exportProperty(API, 'SCARD_ATTR_ESC_CANCEL', API.SCARD_ATTR_ESC_CANCEL);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_ESC_AUTHREQUEST = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_DEFINED, 0xA005);

goog.exportProperty(
    API, 'SCARD_ATTR_ESC_AUTHREQUEST', API.SCARD_ATTR_ESC_AUTHREQUEST);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_MAXINPUT = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_VENDOR_DEFINED, 0xA007);

goog.exportProperty(API, 'SCARD_ATTR_MAXINPUT', API.SCARD_ATTR_MAXINPUT);

/**
 * Instance of this vendor's reader attached to the computer. The first instance
 * will be device unit 0, the next will be unit 1 (if it is the same brand of
 * reader) and so on. Two different brands of readers will both have zero for
 * this value.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_UNIT = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0001);

goog.exportProperty(API, 'SCARD_ATTR_DEVICE_UNIT', API.SCARD_ATTR_DEVICE_UNIT);

/**
 * Reserved for future use.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_IN_USE = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0002);

goog.exportProperty(
    API, 'SCARD_ATTR_DEVICE_IN_USE', API.SCARD_ATTR_DEVICE_IN_USE);

/**
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_FRIENDLY_NAME_A = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0003);

goog.exportProperty(
    API,
    'SCARD_ATTR_DEVICE_FRIENDLY_NAME_A',
    API.SCARD_ATTR_DEVICE_FRIENDLY_NAME_A);

/**
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_SYSTEM_NAME_A = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0004);

goog.exportProperty(
    API,
    'SCARD_ATTR_DEVICE_SYSTEM_NAME_A',
    API.SCARD_ATTR_DEVICE_SYSTEM_NAME_A);

/**
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_FRIENDLY_NAME_W = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0005);

goog.exportProperty(
    API,
    'SCARD_ATTR_DEVICE_FRIENDLY_NAME_W',
    API.SCARD_ATTR_DEVICE_FRIENDLY_NAME_W);

/**
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_SYSTEM_NAME_W = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0006);

goog.exportProperty(
    API,
    'SCARD_ATTR_DEVICE_SYSTEM_NAME_W',
    API.SCARD_ATTR_DEVICE_SYSTEM_NAME_W);

/**
 * FIXME
 * @type {number}
 * @const
 */
API.SCARD_ATTR_SUPRESS_T1_IFS_REQUEST = API.SCARD_ATTR_VALUE(
    API.SCARD_CLASS_SYSTEM, 0x0007);

goog.exportProperty(
    API,
    'SCARD_ATTR_SUPRESS_T1_IFS_REQUEST',
    API.SCARD_ATTR_SUPRESS_T1_IFS_REQUEST);

/**
 * Reader's display name.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_FRIENDLY_NAME =
    API.SCARD_ATTR_DEVICE_FRIENDLY_NAME_W;

goog.exportProperty(
    API,
    'SCARD_ATTR_DEVICE_FRIENDLY_NAME',
    API.SCARD_ATTR_DEVICE_FRIENDLY_NAME);

/**
 * Reader's system name.
 * @type {number}
 * @const
 */
API.SCARD_ATTR_DEVICE_SYSTEM_NAME =
    API.SCARD_ATTR_DEVICE_SYSTEM_NAME_W;

goog.exportProperty(
    API, 'SCARD_ATTR_DEVICE_SYSTEM_NAME', API.SCARD_ATTR_DEVICE_SYSTEM_NAME);

/**
 * Provide source compatibility on different platforms
 * @param {number} code
 * @return {number}
 */
API.SCARD_CTL_CODE = function(code) {
  return castToInt32(0x42000000) + code;
};

goog.exportProperty(API, 'SCARD_CTL_CODE', API.SCARD_CTL_CODE);

/*
 * PC/SC part 10 v2.02.07 March 2010 reader tags
 */

/**
 * @type {number}
 * @const
 */
API.CM_IOCTL_GET_FEATURE_REQUEST = API.SCARD_CTL_CODE(3400);

goog.exportProperty(
    API, 'CM_IOCTL_GET_FEATURE_REQUEST', API.CM_IOCTL_GET_FEATURE_REQUEST);

/** @const */
API.FEATURE_VERIFY_PIN_START = 0x01;

goog.exportProperty(
    API, 'FEATURE_VERIFY_PIN_START', API.FEATURE_VERIFY_PIN_START);

/** @const */
API.FEATURE_VERIFY_PIN_FINISH = 0x02;

goog.exportProperty(
    API, 'FEATURE_VERIFY_PIN_FINISH', API.FEATURE_VERIFY_PIN_FINISH);

/** @const */
API.FEATURE_MODIFY_PIN_START = 0x03;

goog.exportProperty(
    API, 'FEATURE_MODIFY_PIN_START', API.FEATURE_MODIFY_PIN_START);

/** @const */
API.FEATURE_MODIFY_PIN_FINISH = 0x04;

goog.exportProperty(
    API, 'FEATURE_MODIFY_PIN_FINISH', API.FEATURE_MODIFY_PIN_FINISH);

/** @const */
API.FEATURE_GET_KEY_PRESSED = 0x05;

goog.exportProperty(
    API, 'FEATURE_GET_KEY_PRESSED', API.FEATURE_GET_KEY_PRESSED);

/**
 * Verify PIN
 * @const
 */
API.FEATURE_VERIFY_PIN_DIRECT = 0x06;

goog.exportProperty(
    API, 'FEATURE_VERIFY_PIN_DIRECT', API.FEATURE_VERIFY_PIN_DIRECT);

/**
 * Modify PIN
 * @const
 */
API.FEATURE_MODIFY_PIN_DIRECT = 0x07;

goog.exportProperty(
    API, 'FEATURE_MODIFY_PIN_DIRECT', API.FEATURE_MODIFY_PIN_DIRECT);

/** @const */
API.FEATURE_MCT_READER_DIRECT = 0x08;

goog.exportProperty(
    API, 'FEATURE_MCT_READER_DIRECT', API.FEATURE_MCT_READER_DIRECT);

/** @const */
API.FEATURE_MCT_UNIVERSAL = 0x09;

goog.exportProperty(API, 'FEATURE_MCT_UNIVERSAL', API.FEATURE_MCT_UNIVERSAL);

/**
 * retrieve properties of the IFD regarding PIN handling
 * @const
 */
API.FEATURE_IFD_PIN_PROPERTIES = 0x0A;

goog.exportProperty(
    API, 'FEATURE_IFD_PIN_PROPERTIES', API.FEATURE_IFD_PIN_PROPERTIES);

/** @const */
API.FEATURE_ABORT = 0x0B;

goog.exportProperty(API, 'FEATURE_ABORT', API.FEATURE_ABORT);

/** @const */
API.FEATURE_SET_SPE_MESSAGE = 0x0C;

goog.exportProperty(
    API, 'FEATURE_SET_SPE_MESSAGE', API.FEATURE_SET_SPE_MESSAGE);

/** @const */
API.FEATURE_VERIFY_PIN_DIRECT_APP_ID = 0x0D;

goog.exportProperty(
    API,
    'FEATURE_VERIFY_PIN_DIRECT_APP_ID',
    API.FEATURE_VERIFY_PIN_DIRECT_APP_ID);

/** @const */
API.FEATURE_MODIFY_PIN_DIRECT_APP_ID = 0x0E;

goog.exportProperty(
    API,
    'FEATURE_MODIFY_PIN_DIRECT_APP_ID',
    API.FEATURE_MODIFY_PIN_DIRECT_APP_ID);

/** @const */
API.FEATURE_WRITE_DISPLAY = 0x0F;

goog.exportProperty(API, 'FEATURE_WRITE_DISPLAY', API.FEATURE_WRITE_DISPLAY);

/** @const */
API.FEATURE_GET_KEY = 0x10;

goog.exportProperty(API, 'FEATURE_GET_KEY', API.FEATURE_GET_KEY);

/** @const */
API.FEATURE_IFD_DISPLAY_PROPERTIES = 0x11;

goog.exportProperty(
    API, 'FEATURE_IFD_DISPLAY_PROPERTIES', API.FEATURE_IFD_DISPLAY_PROPERTIES);

/** @const */
API.FEATURE_GET_TLV_PROPERTIES = 0x12;

goog.exportProperty(
    API, 'FEATURE_GET_TLV_PROPERTIES', API.FEATURE_GET_TLV_PROPERTIES);

/** @const */
API.FEATURE_CCID_ESC_COMMAND = 0x13;

goog.exportProperty(
    API, 'FEATURE_CCID_ESC_COMMAND', API.FEATURE_CCID_ESC_COMMAND);

/** @const */
API.FEATURE_EXECUTE_PACE = 0x20;

goog.exportProperty(API, 'FEATURE_EXECUTE_PACE', API.FEATURE_EXECUTE_PACE);

/* properties returned by FEATURE_GET_TLV_PROPERTIES */

/** @const */
API.PCSCv2_PART10_PROPERTY_wLcdLayout = 1;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_wLcdLayout',
    API.PCSCv2_PART10_PROPERTY_wLcdLayout);

/** @const */
API.PCSCv2_PART10_PROPERTY_bEntryValidationCondition = 2;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_bEntryValidationCondition',
    API.PCSCv2_PART10_PROPERTY_bEntryValidationCondition);

/** @const */
API.PCSCv2_PART10_PROPERTY_bTimeOut2 = 3;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_bTimeOut2',
    API.PCSCv2_PART10_PROPERTY_bTimeOut2);

/** @const */
API.PCSCv2_PART10_PROPERTY_wLcdMaxCharacters = 4;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_wLcdMaxCharacters',
    API.PCSCv2_PART10_PROPERTY_wLcdMaxCharacters);

/** @const */
API.PCSCv2_PART10_PROPERTY_wLcdMaxLines = 5;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_wLcdMaxLines',
    API.PCSCv2_PART10_PROPERTY_wLcdMaxLines);

/** @const */
API.PCSCv2_PART10_PROPERTY_bMinPINSize = 6;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_bMinPINSize',
    API.PCSCv2_PART10_PROPERTY_bMinPINSize);

/** @const */
API.PCSCv2_PART10_PROPERTY_bMaxPINSize = 7;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_bMaxPINSize',
    API.PCSCv2_PART10_PROPERTY_bMaxPINSize);

/** @const */
API.PCSCv2_PART10_PROPERTY_sFirmwareID = 8;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_sFirmwareID',
    API.PCSCv2_PART10_PROPERTY_sFirmwareID);

/** @const */
API.PCSCv2_PART10_PROPERTY_bPPDUSupport = 9;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_bPPDUSupport',
    API.PCSCv2_PART10_PROPERTY_bPPDUSupport);

/** @const */
API.PCSCv2_PART10_PROPERTY_dwMaxAPDUDataSize = 10;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_dwMaxAPDUDataSize',
    API.PCSCv2_PART10_PROPERTY_dwMaxAPDUDataSize);

/** @const */
API.PCSCv2_PART10_PROPERTY_wIdVendor = 11;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_wIdVendor',
    API.PCSCv2_PART10_PROPERTY_wIdVendor);

/** @const */
API.PCSCv2_PART10_PROPERTY_wIdProduct = 12;

goog.exportProperty(
    API,
    'PCSCv2_PART10_PROPERTY_wIdProduct',
    API.PCSCv2_PART10_PROPERTY_wIdProduct);

/**
 * Protocol Control Information (PCI).
 *
 * Please note that all accesses to the object properties should be done using
 * the [] operator and the quoted string names - otherwise the compiler can
 * break these operations in the advanced compilation mode.
 *
 * The only instance properties are:
 * - protocol
 *
 * @param {number} protocol
 * @constructor
 * @dict
 */
API.SCARD_IO_REQUEST = function(protocol) {
  this['protocol'] = protocol;
};

goog.exportProperty(API, 'SCARD_IO_REQUEST', API.SCARD_IO_REQUEST);

/** @const */
API.SCARD_PCI_T0 = new API.SCARD_IO_REQUEST(API.SCARD_PROTOCOL_T0);

goog.exportProperty(API, 'SCARD_PCI_T0', API.SCARD_PCI_T0);

/** @const */
API.SCARD_PCI_T1 = new API.SCARD_IO_REQUEST(API.SCARD_PROTOCOL_T1);

goog.exportProperty(API, 'SCARD_PCI_T1', API.SCARD_PCI_T1);

/** @const */
API.SCARD_PCI_RAW = new API.SCARD_IO_REQUEST(API.SCARD_PROTOCOL_RAW);

goog.exportProperty(API, 'SCARD_PCI_RAW', API.SCARD_PCI_RAW);

/**
 * Version of the SCARD_READERSTATE PC/SC-Lite API structure that should be used
 * when calling the SCardGetStatusChange function.
 *
 * Please note that all accesses to the object properties should be done using
 * the [] operator and the quoted string names - otherwise the compiler can
 * break these operations in the advanced compilation mode.
 *
 * The only instance properties are:
 * - reader_name
 * - current_state
 * - user_data (optional)
 *
 * @param {string} readerName
 * @param {number} currentState
 * @param {number=} opt_userData
 * @constructor
 * @dict
 */
API.SCARD_READERSTATE_IN = function(readerName, currentState, opt_userData) {
  this['reader_name'] = readerName;
  this['current_state'] = currentState;
  if (goog.isDef(opt_userData))
    this['user_data'] = opt_userData;
};

goog.exportProperty(API, 'SCARD_READERSTATE_IN', API.SCARD_READERSTATE_IN);

/**
 * @param {string} readerName
 * @param {number} currentState
 * @param {number=} opt_userData
 * @return {!API.SCARD_READERSTATE_IN}
 */
API.createSCardReaderStateIn = function(
    readerName, currentState, opt_userData) {
  return new API.SCARD_READERSTATE_IN(readerName, currentState, opt_userData);
};

/**
 * Version of the SCARD_READERSTATE PC/SC-Lite API structure that will be used
 * in the result values returned by the SCardGetStatusChange function.
 *
 * Please note that all accesses to the object properties should be done using
 * the [] operator and the quoted string names - otherwise the compiler can
 * break these operations in the advanced compilation mode.
 *
 * The only instance properties are:
 * - reader_name
 * - current_state
 * - event_state
 * - atr
 * - user_data (optional)
 *
 * @param {string} readerName
 * @param {number} currentState
 * @param {number} eventState
 * @param {!Array.<number>} atr
 * @param {number=} opt_userData
 * @constructor
 * @dict
 */
API.SCARD_READERSTATE_OUT = function(
    readerName, currentState, eventState, atr, opt_userData) {
  this['reader_name'] = readerName;
  this['current_state'] = currentState;
  this['event_state'] = eventState;
  this['atr'] = atr;
  if (goog.isDef(opt_userData))
    this['user_data'] = opt_userData;
};

goog.exportProperty(API, 'SCARD_READERSTATE_OUT', API.SCARD_READERSTATE_OUT);

/**
 * @param {string} readerName
 * @param {number} currentState
 * @param {number} eventState
 * @param {!Array.<number>} atr
 * @param {number=} opt_userData
 * @return {!API.SCARD_READERSTATE_OUT}
 */
API.createSCardReaderStateOut = function(
    readerName, currentState, eventState, atr, opt_userData) {
  return new API.SCARD_READERSTATE_OUT(
      readerName, currentState, eventState, atr, opt_userData);
};


/**
 * @param {!Array} responseItems
 * @constructor
 */
API.ResultOrErrorCode = function(responseItems) {
  GSC.Logging.checkWithLogger(this.logger, responseItems.length >= 1);
  /**
   * An array of all values returned from the function call (including the error
   * code).
   * @type {!Array}
   */
  this.responseItems = responseItems;
  /**
   * Error code returned by the function call.
   * @type {API.ERROR_CODE}
   **/
  this.errorCode = responseItems[0];
  /**
   * An array of result values returned from the function call (NOT including
   * the error code).
   *
   * Undefined if there are no result values (i.e. if the function returned only
   * the error code).
   * @type {!Array|undefined}
   */
  this.resultItems = undefined;
  if (this.isSuccessful())
    this.resultItems = goog.array.slice(responseItems, 1);
};

goog.exportProperty(API, 'ResultOrErrorCode', API.ResultOrErrorCode);

/**
 * @type {!goog.log.Logger}
 * @const
 */
API.ResultOrErrorCode.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteClient.API.ResultOrErrorCode');

/**
 * @param {number} successfulResultItemCount
 * @param {function(...)|undefined} opt_onSucceeded
 * @param {function(API.ERROR_CODE)|undefined} opt_onFailed
 */
API.ResultOrErrorCode.prototype.getBase = function(
    successfulResultItemCount, opt_onSucceeded, opt_onFailed) {
  if (this.isSuccessful()) {
    GSC.Logging.checkWithLogger(
        this.logger, this.resultItems.length == successfulResultItemCount);
    if (opt_onSucceeded)
      opt_onSucceeded.apply(undefined, this.resultItems);
  } else {
    if (opt_onFailed)
      opt_onFailed.apply(undefined, [this.errorCode]);
  }
};

goog.exportProperty(
    API.ResultOrErrorCode.prototype,
    'getBase',
    API.ResultOrErrorCode.prototype.getBase);

/**
 * @return {boolean}
 */
API.ResultOrErrorCode.prototype.isSuccessful = function() {
  return this.errorCode == API.SCARD_S_SUCCESS;
};

goog.exportProperty(
    API.ResultOrErrorCode.prototype,
    'isSuccessful',
    API.ResultOrErrorCode.prototype.isSuccessful);

/**
 * @return {!Array}
 */
API.ResultOrErrorCode.prototype.getResult = function() {
  GSC.Logging.checkWithLogger(this.logger, this.isSuccessful());
  GSC.Logging.checkWithLogger(this.logger, goog.isDef(this.resultItems));
  goog.asserts.assert(this.resultItems);
  return this.resultItems;
};

goog.exportProperty(
    API.ResultOrErrorCode.prototype,
    'getResult',
    API.ResultOrErrorCode.prototype.getResult);


/**
 * Gets a stringified error response.
 *
 * @param {API.ERROR_CODE} errorCode
 *
 * @return {!goog.Promise.<string>}
 */
API.prototype.pcsc_stringify_error = function(errorCode) {
  var logger = this.logger;
  return this.postRequest_(
      'pcsc_stringify_error',
      [errorCode],
      function(responseItems) {
        GSC.Logging.checkWithLogger(logger, responseItems.length == 1);
        GSC.Logging.checkWithLogger(logger, goog.isString(responseItems[0]));
        return responseItems[0];
      });
};

goog.exportProperty(
    API.prototype,
    'pcsc_stringify_error',
    API.prototype.pcsc_stringify_error);

/**
 * Creates an Application Context to the PC/SC Resource Manager.
 *
 * This must be the first WinSCard function called in a PC/SC application.
 * Each thread of an application shall use its own SCARDCONTEXT.
 *
 * Values returned asynchronously upon successful execution:
 * - {@link API.SCARDCONTEXT} sCardContext
 *
 * Error codes that may be returned:
 * - API.SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_PARAMETER Pointer to context is null
 * - SCARD_E_INVALID_VALUE Invalid scope type passed
 * - SCARD_E_NO_MEMORY There is no free slot to store pointer to context
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 * - SCARD_F_INTERNAL_ERROR An internal consistency check failed
 *
 * @param {number} scope Scope of the establishment.
 * This can either be a local or remote connection.
 * - SCARD_SCOPE_USER - Not used.
 * - SCARD_SCOPE_TERMINAL - Not used.
 * - SCARD_SCOPE_GLOBAL - Not used.
 * - SCARD_SCOPE_SYSTEM - Services on the local machine.
 * @param {null=} opt_reserved_1 Reserved for future use.
 * @param {null=} opt_reserved_2 Reserved for future use.
 *
 * @return {!goog.Promise.<!API.SCardEstablishContextResult>}
 */
API.prototype.SCardEstablishContext = function(
    scope, opt_reserved_1, opt_reserved_2) {
  if (!goog.isDef(opt_reserved_1))
    opt_reserved_1 = null;
  if (!goog.isDef(opt_reserved_2))
    opt_reserved_2 = null;
  return this.postRequest_(
      'SCardEstablishContext',
      [scope, opt_reserved_1, opt_reserved_2],
      function(responseItems) {
        return new API.SCardEstablishContextResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype,
    'SCardEstablishContext',
    API.prototype.SCardEstablishContext);

/**
 * Type representing the result of the SCardEstablishContext function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardEstablishContextResult = function(responseItems) {
  API.SCardEstablishContextResult.base(this, 'constructor', responseItems);
};

goog.inherits(API.SCardEstablishContextResult, API.ResultOrErrorCode);

goog.exportProperty(
    API, 'SCardEstablishContextResult', API.SCardEstablishContextResult);

/**
 * @param {function(API.SCARDCONTEXT)} opt_onSucceeded callback:
 * function(sCardContext)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardEstablishContextResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardEstablishContextResult.prototype,
    'get',
    API.SCardEstablishContextResult.prototype.get);

/**
 * Destroys a communication context to the PC/SC Resource Manager. This must be
 * the last function called in a PC/SC application.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_INVALID_HANDLE Invalid sCardContext handle
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to be closed.
 *
 * @return {!goog.Promise.<!API.SCardReleaseContextResult>}
 */
API.prototype.SCardReleaseContext = function(sCardContext) {
  return this.postRequest_(
      'SCardReleaseContext',
      [sCardContext],
      function(responseItems) {
        return new API.SCardReleaseContextResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype,
    'SCardReleaseContext',
    API.prototype.SCardReleaseContext);

/**
 * Type representing the result of the SCardReleaseContext function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardReleaseContextResult = function(responseItems) {
  API.SCardReleaseContextResult.base(this, 'constructor', responseItems);
};

goog.inherits(API.SCardReleaseContextResult, API.ResultOrErrorCode);

goog.exportProperty(
    API, 'SCardReleaseContextResult', API.SCardReleaseContextResult);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardReleaseContextResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardReleaseContextResult.prototype,
    'get',
    API.SCardReleaseContextResult.prototype.get);

/**
 * Establishes a connection to the reader specified in reader.
 *
 * Values returned asynchronously upon successful execution:
 * - {API.SCARDHANDLE} sCardHandle Handle to this connection.
 * - {number} activeProtocol Established protocol to this connection.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid pointer to the handle
 * - SCARD_E_INVALID_PARAMETER Pointer to handle or active protocol is NULL
 * - SCARD_E_INVALID_VALUE Invalid sharing mode, requested protocol, or reader
 *   name
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NO_SMARTCARD No smart card present
 * - SCARD_E_NOT_READY Could not allocate the desired port
 * - SCARD_E_PROTO_MISMATCH Requested protocol is unknown
 * - SCARD_E_READER_UNAVAILABLE Could not power up the reader or card
 * - SCARD_E_SHARING_VIOLATION Someone else has exclusive rights
 * - SCARD_E_UNKNOWN_READER szReader is NULL
 * - SCARD_E_UNSUPPORTED_FEATURE Protocol not supported
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 * - SCARD_F_INTERNAL_ERROR An internal consistency check failed
 * - SCARD_W_UNPOWERED_CARD Card is not powered
 * - SCARD_W_UNRESPONSIVE_CARD Card is mute
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to the PC/SC
 * Resource Manager.
 * @param {string} reader Reader name to connect to.
 * @param {number} shareMode Mode of connection type: exclusive or shared.
 * - SCARD_SHARE_SHARED - This application will allow others to share the
 *   reader.
 * - SCARD_SHARE_EXCLUSIVE - This application will NOT allow others to share the
 *   reader.
 * - SCARD_SHARE_DIRECT - Direct control of the reader, even without a
 *   card.  SCARD_SHARE_DIRECT can be used before using SCardControl() to
 *   send control commands to the reader even if a card is not present in the
 *   reader. Contrary to Windows winscard behavior, the reader is accessed in
 *   shared mode and not exclusive mode.
 * @param {number} preferredProtocols Desired protocol use.
 * - 0 - valid only if dwShareMode is SCARD_SHARE_DIRECT
 * - SCARD_PROTOCOL_T0 - Use the T=0 protocol.
 * - SCARD_PROTOCOL_T1 - Use the T=1 protocol.
 * - SCARD_PROTOCOL_RAW - Use with memory type cards.
 * preferredProtocols is a bit mask of acceptable protocols for the connection.
 * You can use (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1) if you do not have a
 * preferred protocol.
 *
 * @return {!goog.Promise.<!API.SCardConnectResult>}
 */
API.prototype.SCardConnect = function(
    sCardContext, reader, shareMode, preferredProtocols) {
  return this.postRequest_(
      'SCardConnect',
      [sCardContext, reader, shareMode, preferredProtocols],
      function(responseItems) {
        return new API.SCardConnectResult(responseItems);
      });
};

goog.exportProperty(API.prototype, 'SCardConnect', API.prototype.SCardConnect);

/**
 * Type representing the result of the SCardConnect function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardConnectResult = function(responseItems) {
  API.SCardConnectResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardConnectResult', API.SCardConnectResult);

goog.inherits(API.SCardConnectResult, API.ResultOrErrorCode);

/**
 * @param {function(API.SCARDHANDLE, number)} opt_onSucceeded callback:
 * function(sCardHandle, activeProtocol)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardConnectResult.prototype.get = function(opt_onSucceeded, opt_onFailed) {
  return this.getBase(2, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardConnectResult.prototype,
    'get',
    API.SCardConnectResult.prototype.get);

/**
 * Reestablishes a connection to a reader that was
 * previously connected to using SCardConnect.
 *
 * In a multi application environment it is possible for an application to
 * reset the card in shared mode. When this occurs any other application trying
 * to access certain commands will be returned the value
 * SCARD_W_RESET_CARD. When this occurs SCardReconnect() must be called in
 * order to acknowledge that the card was reset and allow it to change its
 * state accordingly.
 *
 * Values returned asynchronously upon successful execution:
 * - {number} activeProtocol Established protocol to this connection.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_PARAMETER Pointer to context is null.
 * - SCARD_E_INVALID_VALUE Invalid sharing mode, requested protocol, or reader
 *   name
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NO_SMARTCARD No smart card present
 * - SCARD_E_NOT_READY Could not allocate the desired port
 * - SCARD_E_PROTO_MISMATCH Requested protocol is unknown
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_E_SHARING_VIOLATION Someone else has exclusive rights
 * - SCARD_E_UNSUPPORTED_FEATURE Protocol not supported
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 * - SCARD_F_INTERNAL_ERROR An internal consistency check failed
 * - SCARD_W_REMOVED_CARD The smart card has been removed
 * - SCARD_W_UNRESPONSIVE_CARD Card is mute
 *
 * @param {API.SCARDHANDLE} sCardHandle Handle to a previous call to connect.
 * @param {number} shareMode Mode of connection type: exclusive/shared.
 * - SCARD_SHARE_SHARED - This application will allow others to share
 *   the reader.
 * - SCARD_SHARE_EXCLUSIVE - This application will NOT allow others to
 *   share the reader.
 * @param {number} preferredProtocols Desired protocol use.
 * - SCARD_PROTOCOL_T0 - Use the T=0 protocol.
 * - SCARD_PROTOCOL_T1 - Use the T=1 protocol.
 * - SCARD_PROTOCOL_RAW - Use with memory type cards.
 * preferredProtocols is a bit mask of acceptable protocols for
 * the connection. You can use (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)
 * if you do not have a preferred protocol.
 * @param {number} initialization Desired action taken on the card/reader.
 * - SCARD_LEAVE_CARD - Do nothing.
 * - SCARD_RESET_CARD - Reset the card (warm reset).
 * - SCARD_UNPOWER_CARD - Power down the card (cold reset).
 * - SCARD_EJECT_CARD - Eject the card.
 *
 * @return {!goog.Promise.<!API.SCardReconnectResult>}
 */
API.prototype.SCardReconnect = function(
    sCardHandle, shareMode, preferredProtocols, initialization) {
  return this.postRequest_(
      'SCardReconnect',
      [sCardHandle, shareMode, preferredProtocols, initialization],
      function(responseItems) {
        return new API.SCardReconnectResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardReconnect', API.prototype.SCardReconnect);

/**
 * Type representing the result of the SCardReconnect function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardReconnectResult = function(responseItems) {
  API.SCardReconnectResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardReconnectResult', API.SCardReconnectResult);

goog.inherits(API.SCardReconnectResult, API.ResultOrErrorCode);

/**
 * @param {function(number)} opt_onSucceeded callback:
 * function(activeProtocol)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardReconnectResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardReconnectResult.prototype,
    'get',
    API.SCardReconnectResult.prototype.get);

/**
 * Terminates a connection made through SCardConnect.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_VALUE Invalid disposition
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NO_SMARTCARD No smart card present
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 * @param {number} disposition Reader function to execute.
 * - SCARD_LEAVE_CARD - Do nothing.
 * - SCARD_RESET_CARD - Reset the card (warm reset).
 * - SCARD_UNPOWER_CARD - Power down the card (cold reset).
 * - SCARD_EJECT_CARD - Eject the card.
 *
 * @return {!goog.Promise.<!API.SCardDisconnectResult>}
 */
API.prototype.SCardDisconnect = function(sCardHandle, disposition) {
  return this.postRequest_(
      'SCardDisconnect', [sCardHandle, disposition], function(responseItems) {
        return new API.SCardDisconnectResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardDisconnect', API.prototype.SCardDisconnect);

/**
 * Type representing the result of the SCardDisconnect function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardDisconnectResult = function(responseItems) {
  API.SCardDisconnectResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardDisconnectResult', API.SCardDisconnectResult);

goog.inherits(API.SCardDisconnectResult, API.ResultOrErrorCode);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardDisconnectResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardDisconnectResult.prototype,
    'get',
    API.SCardDisconnectResult.prototype.get);

/**
 * Establishes a temporary exclusive access mode for
 * doing a serie of commands in a transaction.
 *
 * You might want to use this when you are selecting a few files and then
 * writing a large file so you can make sure that another application will
 * not change the current file. If another application has a lock on this
 * reader or this application is in SCARD_SHARE_EXCLUSIVE there will be no
 * action taken.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_E_SHARING_VIOLATION Someone else has exclusive rights
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 *
 * @return {!goog.Promise.<!API.SCardBeginTransactionResult>}
 */
API.prototype.SCardBeginTransaction = function(sCardHandle) {
  return this.postRequest_(
      'SCardBeginTransaction', [sCardHandle], function(responseItems) {
        return new API.SCardBeginTransactionResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype,
    'SCardBeginTransaction',
    API.prototype.SCardBeginTransaction);

/**
 * Type representing the result of the SCardBeginTransaction function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardBeginTransactionResult = function(responseItems) {
  API.SCardBeginTransactionResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardBeginTransactionResult', API.SCardBeginTransactionResult);

goog.inherits(API.SCardBeginTransactionResult, API.ResultOrErrorCode);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardBeginTransactionResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardBeginTransactionResult.prototype,
    'get',
    API.SCardBeginTransactionResult.prototype.get);

/**
 * Ends a previously begun transaction.
 *
 * The calling application must be the owner of the previously begun
 * transaction or an error will occur.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_VALUE Invalid value for disposition
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_E_SHARING_VIOLATION Someone else has exclusive rights
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 * @param {number} disposition Action to be taken on the reader.
 * - SCARD_LEAVE_CARD - Do nothing.
 * - SCARD_RESET_CARD - Reset the card.
 * - SCARD_UNPOWER_CARD - Power down the card.
 * - SCARD_EJECT_CARD - Eject the card.
 *
 * @return {!goog.Promise.<!API.SCardEndTransactionResult>}
 */
API.prototype.SCardEndTransaction = function(sCardHandle, disposition) {
  return this.postRequest_(
      'SCardEndTransaction',
      [sCardHandle, disposition],
      function(responseItems) {
        return new API.SCardEndTransactionResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardEndTransaction', API.prototype.SCardEndTransaction);

/**
 * Type representing the result of the SCardEndTransaction function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardEndTransactionResult = function(responseItems) {
  API.SCardEndTransactionResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardEndTransactionResult', API.SCardEndTransactionResult);

goog.inherits(API.SCardEndTransactionResult, API.ResultOrErrorCode);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardEndTransactionResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardEndTransactionResult.prototype,
    'get',
    API.SCardEndTransactionResult.prototype.get);

/**
 * Returns the current status of the reader connected to
 * by sCardHandle.
 *
 * The returned state is a DWORD possibly OR'd with the following values:
 * - SCARD_ABSENT - There is no card in the reader.
 * - SCARD_PRESENT - There is a card in the reader, but it has not
 *   been moved into position for use.
 * - SCARD_SWALLOWED - There is a card in the reader in position for
 *   use.  The card is not powered.
 * - SCARD_POWERED - Power is being provided to the card, but the
 *   reader driver is unaware of the mode of the card.
 * - SCARD_NEGOTIABLE - The card has been reset and is awaiting PTS
 *   negotiation.
 * - SCARD_SPECIFIC - The card has been reset and specific
 *   communication protocols have been established.
 *
 * The current state also contains the number of events in the upper 16 bits
 * (state & castToInt32(0xFFFF0000)). This number of events is incremented for each card
 * insertion or removal in the specified reader. This can be used to detect a
 * card removal/insertion between two calls to SCardStatus().
 *
 * Values returned asynchronously upon successful execution:
 * - {string} readerName Friendly name of the reader
 * - {number} state Current state.
 * - {number} protocol Current protocol
 * - {!Array.<number>} atr ATR
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INSUFFICIENT_BUFFER Not enough allocated memory for pointer to
 *   reader name or ATR
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_PARAMETER Pointer to reader name length of ATR length is
 *   NULL
 * - SCARD_E_NO_MEMORY Memory allocation failed
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 * - SCARD_F_INTERNAL_ERROR An internal consistency check failed
 * - SCARD_W_REMOVED_CARD The smart card has been removed
 * - SCARD_W_RESET_CARD The smart card has been reset
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 *
 * @return {!goog.Promise.<!API.SCardStatusResult>}
 */
API.prototype.SCardStatus = function(sCardHandle) {
  return this.postRequest_(
      'SCardStatus', [sCardHandle], function(responseItems) {
        return new API.SCardStatusResult(responseItems);
      });
};

goog.exportProperty(API.prototype, 'SCardStatus', API.prototype.SCardStatus);

/**
 * Type representing the result of the SCardStatus function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardStatusResult = function(responseItems) {
  API.SCardStatusResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardStatusResult', API.SCardStatusResult);

goog.inherits(API.SCardStatusResult, API.ResultOrErrorCode);

/**
 * @param {function(string, number, number, !Array.<number>)} opt_onSucceeded
 * callback: function(readerName, state, protocol, atr)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardStatusResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(4, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardStatusResult.prototype,
    'get',
    API.SCardStatusResult.prototype.get);

/**
 * Blocks execution until the current availability
 * of the cards in a specific set of readers changes.
 *
 * This function receives a structure or list of structures containing
 * reader names. It then blocks waiting for a change in state to occur
 * for a maximum blocking time of timeout or forever if INFINITE is used.
 *
 * The new event state will be returned asynchronously. A status change
 * might be a card insertion or removal event, a change in ATR, etc.
 *
 * The returned event state also contains a number of events in the upper 16
 * bits (eventState & castToInt32(0xFFFF0000)). This number of events is incremented
 * for each card insertion or removal in the specified reader. This can
 * be used to detect a card removal/insertion between two calls to
 * SCardGetStatusChange().
 *
 * To wait for a reader event (reader added or removed) you may use the special
 * reader name "\\?PnP?\Notification". If a reader event occurs the state of
 * this reader will change and the bit SCARD_STATE_CHANGED will be set.
 *
 * Value of currentState and eventState:
 * - SCARD_STATE_UNAWARE The application is unaware of the current
 *   state, and would like to know. The use of this value results in an
 *   immediate return from state transition monitoring services. This is
 *   represented by all bits set to zero.
 * - SCARD_STATE_IGNORE This reader should be ignored
 * - SCARD_STATE_CHANGED There is a difference between the state
 *   believed by the application, and the state known by the resource
 *   manager.  When this bit is set, the application may assume a
 *   significant state change has occurred on this reader.
 * - SCARD_STATE_UNKNOWN The given reader name is not recognized by the
 *   resource manager. If this bit is set, then SCARD_STATE_CHANGED and
 *   SCARD_STATE_IGNORE will also be set
 * - SCARD_STATE_UNAVAILABLE The actual state of this reader is not
 *   available. If this bit is set, then all the following bits are clear.
 * - SCARD_STATE_EMPTY There is no card in the reader. If this bit
 *   is set, all the following bits will be clear
 * - SCARD_STATE_PRESENT There is a card in the reader
 * - SCARD_STATE_EXCLUSIVE The card in the reader is allocated for
 *   exclusive use by another application. If this bit is set,
 *   SCARD_STATE_PRESENT will also be set.
 * - SCARD_STATE_INUSE The card in the reader is in use by one or more
 *   other applications, but may be connected to in shared mode. If this
 *   bit is set, SCARD_STATE_PRESENT will also be set.
 * - SCARD_STATE_MUTE There is an unresponsive card in the reader.
 *
 * Values returned asynchronously upon successful execution:
 * - {!Array.<!API.SCARD_READERSTATE_OUT>} readerStates Structures of readers
 * with new states.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_NO_SERVICE Server is not running
 * - SCARD_E_INVALID_PARAMETER readerStates is NULL and number of readers is > 0
 * - SCARD_E_INVALID_VALUE Invalid States, reader name, etc
 * - SCARD_E_INVALID_HANDLE Invalid hContext handle
 * - SCARD_E_READER_UNAVAILABLE The reader is unavailable
 * - SCARD_E_TIMEOUT The user-specified timeout value has expired
 * - SCARD_E_CANCELLED The call has been cancelled by a call to SCardCancel()
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to the PC/SC
 * Resource Manager.
 * @param {number} timeout Maximum waiting time (in milliseconds) for status
 * change, INFINITE for infinite.
 * @param {!Array.<!API.SCARD_READERSTATE_IN>} readerStates Structures of
 * readers with current states.
 *
 * @return {!goog.Promise.<!API.SCardGetStatusChangeResult>}
 */
API.prototype.SCardGetStatusChange = function(
    sCardContext, timeout, readerStates) {
  return this.postRequest_(
      'SCardGetStatusChange',
      [sCardContext, timeout, readerStates],
      function(responseItems) {
        return new API.SCardGetStatusChangeResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardGetStatusChange', API.prototype.SCardGetStatusChange);

/**
 * Type representing the result of the SCardGetStatusChange function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardGetStatusChangeResult = function(responseItems) {
  API.SCardGetStatusChangeResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardGetStatusChangeResult', API.SCardGetStatusChangeResult);

goog.inherits(API.SCardGetStatusChangeResult, API.ResultOrErrorCode);

/**
 * @param {function(!Array.<!API.SCARD_READERSTATE_OUT>)} opt_onSucceeded
 * callback: function(readerStates)
 * @param {function(!API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardGetStatusChangeResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardGetStatusChangeResult.prototype,
    'get',
    API.SCardGetStatusChangeResult.prototype.get);

/**
 * Sends a command directly to the IFD Handler (reader driver) to be processed
 * by the reader.
 *
 * This is useful for creating client side reader drivers for functions like
 * PIN pads, biometrics, or other extensions to the normal smart card reader
 * that are not normally handled by PC/SC.
 *
 * Values returned asynchronously upon successful execution:
 * - {!Array.<number>} responseData Response from the reader.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INSUFFICIENT_BUFFER Lengths of send or receive buffers are too big
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_PARAMETER Send buffer is NULL or pointer to send buffer
 *   length is NULL and the IFDHandler is version 2.0 (without controlCode)
 * - SCARD_E_INVALID_VALUE Invalid value was presented
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NOT_TRANSACTED Data exchange not successful
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_E_UNSUPPORTED_FEATURE Driver does not support
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 * - SCARD_W_REMOVED_CARD The card has been removed from the reader
 * - SCARD_W_RESET_CARD The card has been reset by another application
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 * @param {number} controlCode Control code for the operation. See
 * http://anonscm.debian.org/viewvc/pcsclite/trunk/Drivers/ccid/SCARDCONTOL.txt?view=markup
 * for a list of supported commands by some drivers.
 * @param {!Array.<number>} dataToSend Command to send to the reader.
 *
 * @return {!goog.Promise.<!API.SCardControlResult>}
 */
API.prototype.SCardControl = function(sCardHandle, controlCode, dataToSend) {
  return this.postRequest_(
      'SCardControl',
      [sCardHandle, controlCode, dataToSend],
      function(responseItems) {
        return new API.SCardControlResult(responseItems);
      });
};

goog.exportProperty(API.prototype, 'SCardControl', API.prototype.SCardControl);

/**
 * Type representing the result of the SCardControl function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardControlResult = function(responseItems) {
  API.SCardControlResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardControlResult', API.SCardControlResult);

goog.inherits(API.SCardControlResult, API.ResultOrErrorCode);

/**
 * @param {function(!Array.<number>)} opt_onSucceeded callback:
 * function(responseData)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardControlResult.prototype.get = function(opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardControlResult.prototype,
    'get',
    API.SCardControlResult.prototype.get);

/**
 * Get an attribute from the IFD Handler (reader driver).
 *
 * Not all the attrId values listed above may be implemented in the IFD
 * Handler you are using. And some attrId values not listed here may be
 * implemented.
 * - SCARD_ATTR_ASYNC_PROTOCOL_TYPES
 * - SCARD_ATTR_ATR_STRING
 * - SCARD_ATTR_CHANNEL_ID
 * - SCARD_ATTR_CHARACTERISTICS
 * - SCARD_ATTR_CURRENT_BWT
 * - SCARD_ATTR_CURRENT_CLK
 * - SCARD_ATTR_CURRENT_CWT
 * - SCARD_ATTR_CURRENT_D
 * - SCARD_ATTR_CURRENT_EBC_ENCODING
 * - SCARD_ATTR_CURRENT_F
 * - SCARD_ATTR_CURRENT_IFSC
 * - SCARD_ATTR_CURRENT_IFSD
 * - SCARD_ATTR_CURRENT_IO_STATE
 * - SCARD_ATTR_CURRENT_N
 * - SCARD_ATTR_CURRENT_PROTOCOL_TYPE
 * - SCARD_ATTR_CURRENT_W
 * - SCARD_ATTR_DEFAULT_CLK
 * - SCARD_ATTR_DEFAULT_DATA_RATE
 * - SCARD_ATTR_DEVICE_FRIENDLY_NAME\n
 *   Implemented by pcsc-lite if the IFD Handler (driver) returns
 *   IFD_ERROR_TAG.  pcsc-lite then returns the same reader name as
 *   returned by SCardListReaders().
 * - SCARD_ATTR_DEVICE_IN_USE
 * - SCARD_ATTR_DEVICE_SYSTEM_NAME
 * - SCARD_ATTR_DEVICE_UNIT
 * - SCARD_ATTR_ESC_AUTHREQUEST
 * - SCARD_ATTR_ESC_CANCEL
 * - SCARD_ATTR_ESC_RESET
 * - SCARD_ATTR_EXTENDED_BWT
 * - SCARD_ATTR_ICC_INTERFACE_STATUS
 * - SCARD_ATTR_ICC_PRESENCE
 * - SCARD_ATTR_ICC_TYPE_PER_ATR
 * - SCARD_ATTR_MAX_CLK
 * - SCARD_ATTR_MAX_DATA_RATE
 * - SCARD_ATTR_MAX_IFSD
 * - SCARD_ATTR_MAXINPUT
 * - SCARD_ATTR_POWER_MGMT_SUPPORT
 * - SCARD_ATTR_SUPRESS_T1_IFS_REQUEST
 * - SCARD_ATTR_SYNC_PROTOCOL_TYPES
 * - SCARD_ATTR_USER_AUTH_INPUT_DEVICE
 * - SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE
 * - SCARD_ATTR_VENDOR_IFD_SERIAL_NO
 * - SCARD_ATTR_VENDOR_IFD_TYPE
 * - SCARD_ATTR_VENDOR_IFD_VERSION
 * - SCARD_ATTR_VENDOR_NAME
 *
 * Values returned asynchronously upon successful execution:
 * - {!Array.<number>} attr The received attribute.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_UNSUPPORTED_FEATURE the dwAttrId attribute is not supported by the
 *   driver
 * - SCARD_E_NOT_TRANSACTED the driver returned an error
 * - SCARD_E_INSUFFICIENT_BUFFER Attribute length is too big
 * - SCARD_E_INSUFFICIENT_BUFFER Reader buffer not large enough
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_PARAMETER A parameter is NULL and should not
 * - SCARD_E_NO_MEMORY Memory allocation failed
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NOT_TRANSACTED Data exchange not successful
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 * @param {number} attrId Identifier for the attribute to get.
 *
 * @return {!goog.Promise.<!API.SCardGetAttribResult>}
 */
API.prototype.SCardGetAttrib = function(sCardHandle, attrId) {
  return this.postRequest_(
      'SCardGetAttrib',
      [sCardHandle, attrId],
      function(responseItems) {
        return new API.SCardGetAttribResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardGetAttrib', API.prototype.SCardGetAttrib);

/**
 * Type representing the result of the SCardGetAttrib function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardGetAttribResult = function(responseItems) {
  API.SCardGetAttribResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardGetAttribResult', API.SCardGetAttribResult);

goog.inherits(API.SCardGetAttribResult, API.ResultOrErrorCode);

/**
 * @param {function(!Array.<number>)} opt_onSucceeded callback: function(attr)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardGetAttribResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardGetAttribResult.prototype,
    'get',
    API.SCardGetAttribResult.prototype.get);

/**
 * Set an attribute of the IFD Handler.
 *
 * The list of attributes you can set is dependent on the IFD Handler you are
 * using.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INSUFFICIENT_BUFFER Pointer to attribute length is too big
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_PARAMETER A parameter is NULL and should not
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NOT_TRANSACTED Data exchange not successful
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 * @param {number} attrId Identifier for the attribute to set.
 * @param {!Array.<number>} attr Buffer with the attribute.
 *
 * @return {!goog.Promise.<!API.SCardSetAttribResult>}
 */
API.prototype.SCardSetAttrib = function(sCardHandle, attrId, attr) {
  return this.postRequest_(
      'SCardSetAttrib',
      [sCardHandle, attrId, attr],
      function(responseItems) {
        return new API.SCardSetAttribResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardSetAttrib', API.prototype.SCardSetAttrib);

/**
 * Type representing the result of the SCardSetAttrib function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardSetAttribResult = function(responseItems) {
  API.SCardSetAttribResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardSetAttribResult', API.SCardSetAttribResult);

goog.inherits(API.SCardSetAttribResult, API.ResultOrErrorCode);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardSetAttribResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardSetAttribResult.prototype,
    'get',
    API.SCardSetAttribResult.prototype.get);

/**
 * Sends an APDU to the smart card contained in the reader connected to by
 * SCardConnect.
 *
 * The card responds from the APDU and the response is returned asynchronously.
 *
 * Values returned asynchronously upon successful execution:
 * - {!API.SCARD_IO_REQUEST} responseProtocolInformation Response protocol
 *   information.
 * - {!Array.<number>} responseData Response from the card.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INSUFFICIENT_BUFFER The receive buffer length was not large enough
 *   for the card response.
 * - SCARD_E_INVALID_HANDLE Invalid sCardHandle handle
 * - SCARD_E_INVALID_PARAMETER Send buffer or receive buffer or pointer to
 *   length of receive buffer or pointer to send protocol information is null
 * - SCARD_E_INVALID_VALUE Invalid Protocol, reader name, etc
 * - SCARD_E_NO_SERVICE The server is not running
 * - SCARD_E_NOT_TRANSACTED APDU exchange not successful
 * - SCARD_E_PROTO_MISMATCH Connect protocol is different than desired
 * - SCARD_E_READER_UNAVAILABLE The reader has been removed
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 * - SCARD_W_RESET_CARD The card has been reset by another application
 * - SCARD_W_REMOVED_CARD The card has been removed from the reader
 *
 * @param {API.SCARDHANDLE} sCardHandle Connection made from SCardConnect.
 * @param {!API.SCARD_IO_REQUEST} sendProtocolInformation Structure of Protocol
 * Control Information.
 * - SCARD_PCI_T0 - Predefined T=0 PCI structure.
 * - SCARD_PCI_T1 - Predefined T=1 PCI structure.
 * - SCARD_PCI_RAW - Predefined RAW PCI structure.
 * @param {!Array.<number>} dataToSend APDU to send to the card.
 * @param {!API.SCARD_IO_REQUEST=} opt_receiveProtocolInformation Structure of
 * protocol information.
 *
 * @return {!goog.Promise.<!API.SCardTransmitResult>}
 */
API.prototype.SCardTransmit = function(
    sCardHandle,
    sendProtocolInformation,
    dataToSend,
    opt_receiveProtocolInformation) {
  return this.postRequest_(
      'SCardTransmit',
      [sCardHandle,
       sendProtocolInformation,
       dataToSend,
       opt_receiveProtocolInformation],
      function(responseItems) {
        return new API.SCardTransmitResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardTransmit', API.prototype.SCardTransmit);

/**
 * Type representing the result of the SCardTransmit function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardTransmitResult = function(responseItems) {
  API.SCardTransmitResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardTransmitResult', API.SCardTransmitResult);

goog.inherits(API.SCardTransmitResult, API.ResultOrErrorCode);

/**
 * @param {function(!API.SCARD_IO_REQUEST, !Array.<number>)} opt_onSucceeded
 * callback: function(responseProtocolInformation, responseData)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardTransmitResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(2, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardTransmitResult.prototype,
    'get',
    API.SCardTransmitResult.prototype.get);

/**
 * Returns a list of currently available readers on the system.
 *
 * Encoding:
 * The reader names and group names are encoded using UTF-8.
 *
 * Values returned asynchronously upon successful execution:
 * - {!Array.<string>} readers List of readers.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INSUFFICIENT_BUFFER Reader buffer not large enough
 * - SCARD_E_INVALID_HANDLE Invalid Scope Handle
 * - SCARD_E_INVALID_PARAMETER Pointer to readers is NULL
 * - SCARD_E_NO_MEMORY Memory allocation failed
 * - SCARD_E_NO_READERS_AVAILABLE No readers available
 * - SCARD_E_NO_SERVICE The server is not running
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to the PC/SC
 * Resource Manager.
 * @param {null} groups List of groups to list readers (not used).
 *
 * @return {!goog.Promise.<!API.SCardListReadersResult>}
 */
API.prototype.SCardListReaders = function(sCardContext, groups) {
  return this.postRequest_(
      'SCardListReaders', [sCardContext, groups], function(responseItems) {
        return new API.SCardListReadersResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardListReaders', API.prototype.SCardListReaders);

/**
 * Type representing the result of the SCardListReaders function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardListReadersResult = function(responseItems) {
  API.SCardListReadersResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(API, 'SCardListReadersResult', API.SCardListReadersResult);

goog.inherits(API.SCardListReadersResult, API.ResultOrErrorCode);

/**
 * @param {function(!Array.<string>)} opt_onSucceeded callback:
 * function(readers)
 * @param {function(API.ERROR_CODE)} opt_onFailed
 */
API.SCardListReadersResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardListReadersResult.prototype,
    'get',
    API.SCardListReadersResult.prototype.get);

/**
 * Returns a list of currently available reader groups on the system.
 *
 * Values returned asynchronously upon successful execution:
 * - {!Array.<string>} groups List of reader groups.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INSUFFICIENT_BUFFER Reader buffer not large enough
 * - SCARD_E_INVALID_HANDLE Invalid Scope Handle
 * - SCARD_E_INVALID_PARAMETER Receive buffer is null when autoallocation is
 *   requested
 * - SCARD_E_NO_MEMORY Memory allocation failed
 * - SCARD_E_NO_SERVICE The server is not running
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to the PC/SC
 * Resource Manager.
 *
 * @return {!goog.Promise.<!API.SCardListReaderGroupsResult>}
 */
API.prototype.SCardListReaderGroups = function(sCardContext) {
  return this.postRequest_(
      'SCardListReaderGroups', [sCardContext], function(responseItems) {
        return new API.SCardListReaderGroupsResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype,
    'SCardListReaderGroups',
    API.prototype.SCardListReaderGroups);

/**
 * Type representing the result of the SCardListReaderGroups function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardListReaderGroupsResult = function(responseItems) {
  API.SCardListReaderGroupsResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardListReaderGroupsResult', API.SCardListReaderGroupsResult);

goog.inherits(API.SCardListReaderGroupsResult, API.ResultOrErrorCode);

/**
 * @param {function(!Array.<string>)} opt_onSucceeded callback: function(groups)
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardListReaderGroupsResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(1, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardListReaderGroupsResult.prototype,
    'get',
    API.SCardListReaderGroupsResult.prototype.get);

/**
 * Cancels all pending blocking requests on the SCardGetStatusChange function.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid sCardContext handle
 * - SCARD_E_NO_SERVICE Server is not running
 * - SCARD_F_COMM_ERROR An internal communications error has been detected
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to the PC/SC
 * Resource Manager.
 *
 * @return {!goog.Promise.<!API.SCardCancelResult>}
 */
API.prototype.SCardCancel = function(sCardContext) {
  return this.postRequest_(
      'SCardCancel', [sCardContext], function(responseItems) {
        return new API.SCardCancelResult(responseItems);
      });
};

goog.exportProperty(API.prototype, 'SCardCancel', API.prototype.SCardCancel);

/**
 * Type representing the result of the SCardCancel function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardCancelResult = function(responseItems) {
  API.SCardCancelResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardCancelResult', API.SCardCancelResult);

goog.inherits(API.SCardCancelResult, API.ResultOrErrorCode);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardCancelResult.prototype.get = function(opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardCancelResult.prototype,
    'get',
    API.SCardCancelResult.prototype.get);

/**
 * Check if a SCARDCONTEXT is valid.
 *
 * Call this function to determine whether a smart card context handle is still
 * valid. After a smart card context handle has been returned by
 * SCardEstablishContext, it may become invalid if the resource manager service
 * has been shut down.
 *
 * Error codes that may be returned:
 * - SCARD_S_SUCCESS Successful
 * - SCARD_E_INVALID_HANDLE Invalid sCardContext handle
 *
 * @param {API.SCARDCONTEXT} sCardContext Connection context to the PC/SC
 * Resource Manager.
 *
 * @return {!goog.Promise.<!API.SCardIsValidContextResult>}
 */
API.prototype.SCardIsValidContext = function(sCardContext) {
  return this.postRequest_(
      'SCardIsValidContext', [sCardContext], function(responseItems) {
        return new API.SCardIsValidContextResult(responseItems);
      });
};

goog.exportProperty(
    API.prototype, 'SCardIsValidContext', API.prototype.SCardIsValidContext);

/**
 * Type representing the result of the SCardIsValidContext function call.
 * @param {!Array} responseItems
 * @extends {API.ResultOrErrorCode}
 * @constructor
 */
API.SCardIsValidContextResult = function(responseItems) {
  API.SCardIsValidContextResult.base(this, 'constructor', responseItems);
};

goog.exportProperty(
    API, 'SCardIsValidContextResult', API.SCardIsValidContextResult);

goog.inherits(API.SCardIsValidContextResult, API.ResultOrErrorCode);

/**
 * @param {function()} opt_onSucceeded callback: function()
 * @param {function(API.ERROR_CODE)} opt_onFailed callback: function(errorCode)
 */
API.SCardIsValidContextResult.prototype.get = function(
    opt_onSucceeded, opt_onFailed) {
  return this.getBase(0, opt_onSucceeded, opt_onFailed);
};

goog.exportProperty(
    API.SCardIsValidContextResult.prototype,
    'get',
    API.SCardIsValidContextResult.prototype.get);


/** @override */
API.prototype.disposeInternal = function() {
  this.requester_.dispose();
  this.requester_ = null;

  this.messageChannel_ = null;

  this.logger.fine('Disposed');

  API.base(this, 'disposeInternal');
};

/** @private */
API.prototype.messageChannelDisposedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.info('Message channel was disposed, disposing...');
  this.dispose();
};

/**
 * @param {string} functionName
 * @param {!Array} functionArguments
 * @param {function(!Array):*} successfulResultTransformer
 * @return {!goog.Promise}
 * @private
 */
API.prototype.postRequest_ = function(
    functionName, functionArguments, successfulResultTransformer) {
  if (this.isDisposed()) {
    return goog.Promise.reject(new Error(
        'The API instance is already disposed'));
  }
  var remoteCallMessage = new GSC.RemoteCallMessage(
      functionName, functionArguments);
  var payload = remoteCallMessage.makeRequestPayload();
  var promise = this.requester_.postRequest(payload);
  return promise.then(successfulResultTransformer);
};

});  // goog.scope
