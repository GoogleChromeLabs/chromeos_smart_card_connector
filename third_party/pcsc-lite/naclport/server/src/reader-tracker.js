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
 * @fileoverview This file contains the class used to track readers and their
 * status based on messages received from the hook setup on PC/SC code inside
 * NaCl (check readerfactory_nacl.cc for more information).
 */

goog.provide('GoogleSmartCard.PcscLiteServer.ReaderInfo');
goog.provide('GoogleSmartCard.PcscLiteServer.ReaderStatus');
goog.provide('GoogleSmartCard.PcscLiteServer.ReaderTracker');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessagingCommon');
goog.require('GoogleSmartCard.PcscLiteClient.ReaderTrackerThroughPcscApi');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.Timer');
goog.require('goog.array');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const READER_TRACKER_LOGGER_TITLE = 'ReaderTracker';

const GSC = GoogleSmartCard;

/**
 * Enum for possible values of ReaderInfo status.
 * @enum {string}
 */
GSC.PcscLiteServer.ReaderStatus = {
  INIT: 'init',
  SUCCESS: 'success',
  FAILURE: 'failure'
};

const ReaderStatus = GSC.PcscLiteServer.ReaderStatus;

/**
 * Structure used to store information about the reader.
 * @param {string} name
 * @param {!ReaderStatus} status
 * @param {string=} opt_error
 * @param {boolean=} opt_isCardPresent
 * @constructor
 */
GSC.PcscLiteServer.ReaderInfo = function(
    name, status, opt_error, opt_isCardPresent) {
  /** @type {string} @const */
  this['name'] = this.name = name;
  /** @type {!ReaderStatus} @const */
  this['status'] = this.status = status;
  /** @type {string|undefined} @const */
  this['error'] = this.error = opt_error;
  /** @type {boolean} @const */
  this['isCardPresent'] = this.isCardPresent = !!opt_isCardPresent;
};

const ReaderInfo = GSC.PcscLiteServer.ReaderInfo;

/**
 * This class tracks readers, provides methods to retrieve the list of readers
 * and get updates on change, and listens to messages received from the hook
 * living in the NaCl space.
 * @param {!goog.messaging.AbstractChannel} serverMessageChannel Message channel
 * to the server (normally - to a NaCl module).
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel Message
 * channel that can be used for issuing PC/SC requests.
 * @param {!goog.log.Logger} parentLogger
 * @constructor
 */
GSC.PcscLiteServer.ReaderTracker = function(
    serverMessageChannel, pcscContextMessageChannel, parentLogger) {
  /** @private @const */
  this.logger_ =
      GSC.Logging.getChildLogger(parentLogger, READER_TRACKER_LOGGER_TITLE);

  /**
   * @type {!Array.<function(!Array.<!ReaderInfo>)>}
   * @private @const
   */
  this.updateListeners_ = [];

  /** @private @const */
  this.trackerThroughPcscServerHook_ = new TrackerThroughPcscServerHook(
      this.logger_, serverMessageChannel,
      this.fireOnUpdateListeners_.bind(this));

  /** @private @const */
  this.trackerThroughPcscApi_ =
      new GSC.PcscLiteClient.ReaderTrackerThroughPcscApi(
          this.logger_, pcscContextMessageChannel,
          this.fireOnUpdateListeners_.bind(this));

  goog.log.fine(this.logger_, 'Initialized');
};

const ReaderTracker = GSC.PcscLiteServer.ReaderTracker;

/**
 * Adds a listener that will be called with the list of reader information
 * objects each time it changes.
 *
 * The listener is also called immediately (synchronously from this method) with
 * the list of currently available readers.
 * @param {function(!Array.<!ReaderInfo>)} listener
 */
ReaderTracker.prototype.addOnUpdateListener = function(listener) {
  this.updateListeners_.push(listener);
  goog.log.fine(this.logger_, 'Added an update listener');

  listener(this.getReaders());
};

/**
 * Removes a previously added listener function.
 * @param {function(!Array.<!ReaderInfo>)} listener
 */
ReaderTracker.prototype.removeOnUpdateListener = function(listener) {
  if (goog.array.remove(this.updateListeners_, listener)) {
    goog.log.fine(this.logger_, 'Removed an update listener');
  } else {
    goog.log.warning(
        this.logger_,
        'Failed to remove an update listener: the passed ' +
            'function was not found');
  }
};

/**
 * Returns the current list of reader information objects.
 *
 * It is guaranteed that the successfully initialized readers will always
 * precede the failed readers. Besides that, the order of items is unspecified,
 * but expected to be relatively stable across multiple calls.
 * @return {!Array.<!ReaderInfo>}
 */
ReaderTracker.prototype.getReaders = function() {
  // Take the information about successfully initialized readers from the PC/SC
  // API.
  const successReaders = goog.array.map(
      this.trackerThroughPcscApi_.getReaders(), function(reader) {
        return new ReaderInfo(
            reader.name, ReaderStatus.SUCCESS,
            /*error=*/ undefined, reader.isCardPresent);
      });

  // Take the information about other readers (i.e. that are either under the
  // initialization process or were failed to initialize) from the hook inside
  // the PC/SC server.
  const nonSuccessReaders = goog.array.filter(
      this.trackerThroughPcscServerHook_.getReaders(), function(readerInfo) {
        return readerInfo.status != ReaderStatus.SUCCESS;
      });

  return goog.array.concat(successReaders, nonSuccessReaders);
};

/**
 * @private
 */
ReaderTracker.prototype.fireOnUpdateListeners_ = function() {
  const readers = this.getReaders();
  goog.log.fine(
      this.logger_,
      'Firing readers updated listeners with data ' +
          GSC.DebugDump.dump(readers));
  for (let listener of this.updateListeners_) {
    listener(readers);
  }
};

/**
 * This class tracks the readers, basing on the information from the hook in the
 * PC/SC-Lite server (see readerfactory_nacl.cc for more information).
 * @param {!goog.log.Logger} logger
 * @param {!goog.messaging.AbstractChannel} serverMessageChannel
 * @param {function()} updateListener
 * @constructor
 */
function TrackerThroughPcscServerHook(
    logger, serverMessageChannel, updateListener) {
  /** @private @const */
  this.logger_ = logger;

  /** @private @const */
  this.updateListener_ = updateListener;

  /**
   * Map from the port number, as reported by the PC/SC-Lite server, to the
   * reader information.
   *
   * The values can be null, which corresponds to readers that should be hidden
   * from the result.
   * @type {!Map.<number, ReaderInfo?>}
   * @private @const
   */
  this.portToReaderInfoMap_ = new Map;

  serverMessageChannel.registerService(
      'reader_init_add', this.readerInitAddListener_.bind(this), true);
  serverMessageChannel.registerService(
      'reader_finish_add', this.readerFinishAddListener_.bind(this), true);
  serverMessageChannel.registerService(
      'reader_remove', this.readerRemoveListener_.bind(this), true);
}

/**
 * @return {!Array.<!ReaderInfo>}
 */
TrackerThroughPcscServerHook.prototype.getReaders = function() {
  // Return the readers sorted by their port (the exact key used for the
  // ordering doesn't matter, it's just a simplest way to guarantee the stable
  // order across multiple calls).
  const ports = Array.from(this.portToReaderInfoMap_.keys());
  goog.array.sort(ports);
  const mappedReaderInfos = goog.array.map(
      ports, this.portToReaderInfoMap_.get, this.portToReaderInfoMap_);
  // Remove null entries, as they correspond to readers that should be hidden.
  return /** @type {!Array.<!ReaderInfo>} */ (
      mappedReaderInfos.filter(function(item) {
        return item !== null;
      }));
};

/**
 * @param {!Object|string} message
 * @private
 */
TrackerThroughPcscServerHook.prototype.readerInitAddListener_ = function(
    message) {
  /** @type {string} */
  const name = GSC.MessagingCommon.extractKey(message, 'readerName');
  /** @type {number} */
  const port = GSC.MessagingCommon.extractKey(message, 'port');
  /** @type {string} */
  const device = GSC.MessagingCommon.extractKey(message, 'device');

  goog.log.info(
      this.logger_,
      'A new reader "' + name + '" (port ' + port + ', device "' + device +
          '") is being initialized...');

  GSC.Logging.checkWithLogger(
      this.logger_, !this.portToReaderInfoMap_.has(port),
      'Initializing reader which is already present!');
  this.portToReaderInfoMap_.set(port, new ReaderInfo(name, ReaderStatus.INIT));
  this.updateListener_();
};

/**
 * @param {!Object|string} message
 * @private
 */
TrackerThroughPcscServerHook.prototype.readerFinishAddListener_ = function(
    message) {
  /** @type {string} */
  const name = GSC.MessagingCommon.extractKey(message, 'readerName');
  /** @type {number} */
  const port = GSC.MessagingCommon.extractKey(message, 'port');
  /** @type {string} */
  const device = GSC.MessagingCommon.extractKey(message, 'device');
  /** @type {number} */
  const returnCode = GSC.MessagingCommon.extractKey(message, 'returnCode');

  const returnCodeHex = GSC.DebugDump.dump(returnCode);

  /** @type {ReaderInfo?} */
  let readerInfo = null;
  const readerTitleForLog =
      '"' + name + '" (port ' + port + ', device "' + device + '")';
  if (returnCode === 0) {
    goog.log.info(
        this.logger_,
        'The reader ' + readerTitleForLog + ' was successfully ' +
            'initialized');
    readerInfo = new ReaderInfo(name, ReaderStatus.SUCCESS);
  } else if (this.shouldHideFailedReader_(device)) {
    goog.log.info(
        this.logger_,
        'Silent error while initializing the reader ' + readerTitleForLog +
            ': error code ' + returnCodeHex +
            '. This reader will be hidden from UI.');
  } else {
    goog.log.warning(
        this.logger_,
        'Failure while initializing the reader ' + readerTitleForLog +
            ': error code ' + returnCodeHex);
    readerInfo = new ReaderInfo(name, ReaderStatus.FAILURE, returnCodeHex);
  }

  GSC.Logging.checkWithLogger(
      this.logger_, this.portToReaderInfoMap_.has(port),
      'Finishing initializing reader without present reader!');

  // Note that the inserted value may be null, which means that this reader
  // should be hidden from the result.
  this.portToReaderInfoMap_.set(port, readerInfo);

  this.updateListener_();
};

/**
 * @param {!Object|string} message
 * @private
 */
TrackerThroughPcscServerHook.prototype.readerRemoveListener_ = function(
    message) {
  /** @type {string} */
  const name = GSC.MessagingCommon.extractKey(message, 'readerName');
  /** @type {number} */
  const port = GSC.MessagingCommon.extractKey(message, 'port');

  goog.log.info(
      this.logger_,
      'The reader "' + name + '" (port ' + port + ') was removed');

  GSC.Logging.checkWithLogger(
      this.logger_, this.portToReaderInfoMap_.has(port),
      'Tried removing non-existing reader!');
  this.portToReaderInfoMap_.delete(port);
  this.updateListener_();
};

/**
 * Checks whether the specified failed reader should be hidden from the reported
 * list of readers.
 * @param {string} device The device name, as reported from the PC/SC-Lite
 * server.
 * @return {boolean}
 * @private
 */
TrackerThroughPcscServerHook.prototype.shouldHideFailedReader_ = function(
    device) {
  // Hide failed readers which are named with non-zero interface number. They
  // usually correspond to non smart card reader interfaces of composite USB
  // devices - while, typically, the CCID driver guesses the right interface to
  // talk to from the reader whose name contains interface equal to zero.
  // Therefore it makes sense to just hide from UI these items as they don't
  // actually signal about any error.
  return /:[1-9][0-9]*$/.test(device);
};
});  // goog.scope
