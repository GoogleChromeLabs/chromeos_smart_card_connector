/** @license
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
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteClient.Context');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.Timer');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');
goog.require('goog.structs.Map');

goog.scope(function() {

/** @const */
var READER_TRACKER_LOGGER_TITLE = 'ReaderTracker';

/**
 * TODO(emaxx): Write docs.
 * @const
 */
var READER_STATUS_QUERY_TIMEOUT_MILLISECONDS = 60 * 1000;

/**
 * TODO(emaxx): Write docs.
 * @const
 */
var READER_STATUS_FAILED_QUERY_DELAY_MILLISECONDS = 500;

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var API = GSC.PcscLiteClient.API;

/**
 * Enum for possible values of ReaderInfo status.
 * @enum {string}
 */
GSC.PcscLiteServer.ReaderStatus = {
  INIT: 'init',
  SUCCESS: 'success',
  FAILURE: 'failure'
};

/** @const */
var ReaderStatus = GSC.PcscLiteServer.ReaderStatus;

/**
 * Structure used to store information about the reader.
 * @param {string} name
 * @param {!ReaderStatus} status
 * @param {boolean=} opt_isCardPresent
 * @param {string=} opt_error
 * @constructor
 */
GSC.PcscLiteServer.ReaderInfo = function(
    name, status, opt_error, opt_isCardPresent) {
  /** @type {string} */
  this['name'] = this.name = name;
  /** @type {!ReaderStatus} */
  this['status'] = this.status = status;
  /** @type {string|undefined} */
  this['error'] = this.error = opt_error;
  /** @type {boolean} */
  this['isCardPresent'] = this.isCardPresent = !!opt_isCardPresent;
};

/** @const */
var ReaderInfo = GSC.PcscLiteServer.ReaderInfo;

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
  /** @private */
  this.logger_ = GSC.Logging.getChildLogger(
      parentLogger, READER_TRACKER_LOGGER_TITLE);

  /**
   * @type {!Array.<function(!Array.<!ReaderInfo>)>}
   * @private
   */
  this.updateListeners_ = [];

  /** @private */
  this.trackerThroughPcscServerHook_ = new TrackerThroughPcscServerHook(
      this.logger_,
      serverMessageChannel,
      this.fireOnUpdateListeners_.bind(this));

  /** @private */
  this.trackerThroughPcscApi_ = new TrackerThroughPcscApi(
      this.logger_,
      pcscContextMessageChannel,
      this.fireOnUpdateListeners_.bind(this));

  this.logger_.fine('Initialized');
};

/** @const */
var ReaderTracker = GSC.PcscLiteServer.ReaderTracker;

/**
 * @param {function(!Array.<!ReaderInfo>)} listener
 */
ReaderTracker.prototype.addOnUpdateListener = function(listener) {
  this.logger_.fine('Added an OnUpdateListener');
  this.updateListeners_.push(listener);
};

/**
 * @param {function(!Array.<!ReaderInfo>)} listener
 */
ReaderTracker.prototype.removeOnUpdateListener = function(listener) {
  this.logger_.fine('Removed an OnUpdateListener');
  goog.array.remove(this.updateListeners_, listener);
};

/**
 * @return {!Array.<!ReaderInfo>}
 */
ReaderTracker.prototype.getReaders = function() {
  // Take the information about successfully initialized readers from the PC/SC
  // API.
  var successReaders = this.trackerThroughPcscApi_.getReaders();

  // Take the information about other readers (i.e. that are either under the
  // initialization process or were failed to initialize) from the hook inside
  // the PC/SC server.
  var nonSuccessReaders = goog.array.filter(
      this.trackerThroughPcscServerHook_.getReaders(),
      function(readerInfo) {
        return readerInfo.status != ReaderStatus.SUCCESS;
      });

  return goog.array.concat(nonSuccessReaders, successReaders);
};

/**
 * @private
 */
ReaderTracker.prototype.fireOnUpdateListeners_ = function() {
  var readers = this.getReaders();
  this.logger_.fine('Firing readers updated listeners with data ' +
                    GSC.DebugDump.dump(readers));
  for (let listener of this.updateListeners_) {
    listener(readers);
  }
};

/**
 * TODO(emaxx): Write docs.
 * @param {!goog.log.Logger} logger
 * @param {!goog.messaging.AbstractChannel} serverMessageChannel
 * @param {function()} updateListener
 * @constructor
 */
function TrackerThroughPcscServerHook(
    logger, serverMessageChannel, updateListener) {
  /** @private */
  this.logger_ = logger;

  /** @private */
  this.updateListener_ = updateListener;

  /**
   * @type {!goog.structs.Map.<number, !ReaderInfo>}
   * @private
   */
  this.portToReaderInfoMap_ = new goog.structs.Map;

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
  var ports = this.portToReaderInfoMap_.getKeys();
  goog.array.sort(ports);
  return goog.array.map(
      ports, this.portToReaderInfoMap_.get, this.portToReaderInfoMap_);
};

/**
 * @param {!Object|string} message
 * @private
 */
TrackerThroughPcscServerHook.prototype.readerInitAddListener_ = function(
    message) {
  /** @type {string} */
  var name = GSC.MessagingCommon.extractKey(message, 'readerName');
  /** @type {number} */
  var port = GSC.MessagingCommon.extractKey(message, 'port');
  /** @type {string} */
  var device = GSC.MessagingCommon.extractKey(message, 'device');

  this.logger_.info('A new reader "' + name + '" (port ' + port + ', device "' +
                    device + '") is being initialized...');

  GSC.Logging.checkWithLogger(
      this.logger_,
      !this.portToReaderInfoMap_.containsKey(port),
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
  var name = GSC.MessagingCommon.extractKey(message, 'readerName');
  /** @type {number} */
  var port = GSC.MessagingCommon.extractKey(message, 'port');
  /** @type {string} */
  var device = GSC.MessagingCommon.extractKey(message, 'device');
  /** @type {number} */
  var returnCode = GSC.MessagingCommon.extractKey(message, 'returnCode');

  var returnCodeHex = GSC.DebugDump.dump(returnCode);

  var logMessage =
      'The reader "' + name + '" (port ' + port + ', device "' + device +
      '") was initialized with return code ' + returnCodeHex;
  if (returnCode === 0)
    this.logger_.info(logMessage);
  else
    this.logger_.warning(logMessage);

  if (returnCode === 0) {
    var value = new ReaderInfo(name, ReaderStatus.SUCCESS);
  } else {
    var value = new ReaderInfo(name, ReaderStatus.FAILURE, returnCodeHex);
  }

  GSC.Logging.checkWithLogger(
      this.logger_,
      this.portToReaderInfoMap_.containsKey(port),
      'Finishing initializing reader without present reader!');
  this.portToReaderInfoMap_.set(port, value);
  this.updateListener_();
};

/**
 * @param {!Object|string} message
 * @private
 */
TrackerThroughPcscServerHook.prototype.readerRemoveListener_ = function(
    message) {
  /** @type {string} */
  var name = GSC.MessagingCommon.extractKey(message, 'readerName');
  /** @type {number} */
  var port = GSC.MessagingCommon.extractKey(message, 'port');

  this.logger_.info('Removing the reader "' + name + '" (port ' + port + ')');

  GSC.Logging.checkWithLogger(
      this.logger_,
      this.portToReaderInfoMap_.containsKey(port),
      'Tried removing non-existing reader!');
  this.portToReaderInfoMap_.remove(port);
  this.updateListener_();
};

/**
 * This class tracks the readers, basing on the information from the PC/SC API
 * (i.e. on the public API, which is also provided to the external
 * applications).
 *
 * First, this allows to obtain exactly the same information that is exposed to
 * the external applications.
 *
 * Second, this makes it possible to reuse the existing PC/SC code, without the
 * need to modify it for obtaining the information about readers.
 * @param {!goog.log.Logger} logger
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel
 * @param {function()} updateListener
 * @constructor
 */
function TrackerThroughPcscApi(
    logger, pcscContextMessageChannel, updateListener) {
  /** @private */
  this.logger_ = logger;

  /** @private */
  this.updateListener_ = updateListener;

  /**
   * @type {!Array.<!ReaderInfo>}
   * @private
   */
  this.result_ = [];

  this.startStatusTracking_(pcscContextMessageChannel);
}

/**
 * @return {!Array.<!ReaderInfo>}
 */
TrackerThroughPcscApi.prototype.getReaders = function() {
  return this.result_;
};

/**
 * Starts the tracking which will work infinitely unless some error occurs.
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel
 * @private
 */
TrackerThroughPcscApi.prototype.startStatusTracking_ = function(
    pcscContextMessageChannel) {
  this.logger_.fine('Started tracking through PC/SC API');

  this.makeApiPromise_(pcscContextMessageChannel).then(function(api) {
    return this.makeStatusTrackingPromise_(api);
  }, null, this).thenCatch(function(error) {
    this.logger_.warning(
        'Stopped tracking through PC/SC API: ' + error.message);
    this.updateResult_([]);
  }, this);
};

/**
 * Starts the tracking, given the PC/SC client API instance, and returns the
 * promise that corresponds to the tracking.
 *
 * The returned promise will be never fulfilled, and may only become rejected -
 * if the tracking fails due to some error.
 * @param {!API} api
 * @private
 */
TrackerThroughPcscApi.prototype.makeStatusTrackingPromise_ = function(api) {
  return this.makeSCardContextPromise_(api).then(function(sCardContext) {
    return this.makeStatusTrackingLoopPromise_(api, sCardContext);
  }, null, this);
};

/**
 * Performs the infinite loop of the tracking, and returns the promise that
 * corresponds to this infinite loop.
 *
 * The returned promise will be never fulfilled, and may only become rejected -
 * if the tracking fails due to some error.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @private
 */
TrackerThroughPcscApi.prototype.makeStatusTrackingLoopPromise_ = function(
    api, sCardContext) {
  return this.makeReaderNamesPromise_(
      api, sCardContext).then(function(readerNames) {
    return this.makeReaderStatesPromise_(api, sCardContext, readerNames);
  }, null, this).then(function(readerStates) {
    // If an intermittent error was returned from the previous request, then
    // just sleep for some time and repeat the tracking loop body; otherwise -
    // block on waiting for a notification from PC/SC that something is changed.
    if (goog.isNull(readerStates)) {
      return goog.Timer.promise(READER_STATUS_FAILED_QUERY_DELAY_MILLISECONDS);
    } else {
      this.updateResultFromReaderStates_(readerStates);
      return this.makeReaderStatesChangePromise_(
          api, sCardContext, readerStates);
    }
  }, null, this).then(function() {
    return this.makeStatusTrackingLoopPromise_(api, sCardContext);
  }, null, this);
};

/**
 * TODO(emaxx): Write docs.
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel
 * @return {!goog.Promise.<!API>}
 * @private
 */
TrackerThroughPcscApi.prototype.makeApiPromise_ = function(
    pcscContextMessageChannel) {
  /** @type {!goog.promise.Resolver.<!API>} */
  var promiseResolver = goog.Promise.withResolver();

  var context = new GSC.PcscLiteClient.Context('internal reader tracker', null);
  context.addOnInitializedCallback(
      promiseResolver.resolve.bind(promiseResolver));
  context.addOnDisposeCallback(function() {
    promiseResolver.reject(new Error(
        'Failed to obtain the client API instance: The PC/SC client context ' +
        'was disposed'));
  });
  context.initialize(pcscContextMessageChannel);

  return promiseResolver.promise;
};

/**
 * TODO(emaxx): Write docs.
 * @param {!API} api
 * @return {!goog.Promise.<!API.SCARDCONTEXT>}
 * @private
 */
TrackerThroughPcscApi.prototype.makeSCardContextPromise_ = function(api) {
  /** @type {!goog.promise.Resolver.<!API.SCARDCONTEXT>} */
  var promiseResolver = goog.Promise.withResolver();

  api.SCardEstablishContext(API.SCARD_SCOPE_SYSTEM, null, null).then(
      function(result) {
        result.get(
            promiseResolver.resolve.bind(promiseResolver),
            function(errorCode) {
              promiseResolver.reject(new Error(
                  'Failed to establish a PC/SC context with error code ' +
                  GSC.DebugDump.dump(errorCode)));
            });
      },
      function(error) {
        promiseResolver.reject(new Error(
            'Failed to establish a PC/SC context: ' + error));
      });

  return promiseResolver.promise;
};

/**
 * TODO(emaxx): Write docs.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @return {!goog.Promise.<!Array.<string>>}
 * @private
 */
TrackerThroughPcscApi.prototype.makeReaderNamesPromise_ = function(
    api, sCardContext) {
  /** @type {!goog.promise.Resolver.<!Array.<string>>} */
  var promiseResolver = goog.Promise.withResolver();

  api.SCardListReaders(sCardContext, null).then(function(result) {
    result.get(
        promiseResolver.resolve.bind(promiseResolver),
        function(errorCode) {
          if (errorCode == API.SCARD_E_NO_READERS_AVAILABLE) {
            promiseResolver.resolve([]);
          } else {
            promiseResolver.reject(new Error(
                'Failed to get the list of readers from PC/SC with error ' +
                'code ' + GSC.DebugDump.dump(errorCode)));
          }
        });
  }, function(error) {
    promiseResolver.reject(new Error(
        'Failed to get the list of readers from PC/SC: ' + error));
  });

  return promiseResolver.promise;
};

/**
 * TODO(emaxx): Write docs.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @param {!Array.<string>} readerNames
 * @return {!goog.Promise.<!Array.<!API.SCARD_READERSTATE_OUT>|null>} Either the
 * states of the readers, or the null value in case of intermittent error.
 * @private
 */
TrackerThroughPcscApi.prototype.makeReaderStatesPromise_ = function(
    api, sCardContext, readerNames) {
  /** @type {!goog.promise.Resolver.<!Array.<!API.SCARD_READERSTATE_OUT>|null>} */
  var promiseResolver = goog.Promise.withResolver();

  var readerStatesIn = goog.array.map(readerNames, function(readerName) {
    return new API.SCARD_READERSTATE_IN(readerName, API.SCARD_STATE_UNAWARE);
  });

  api.SCardGetStatusChange(sCardContext, 0, readerStatesIn).then(
      function(result) {
        result.get(
            promiseResolver.resolve.bind(promiseResolver),
            function(errorCode) {
              if (errorCode == API.SCARD_E_UNKNOWN_READER) {
                this.logger_.warning(
                    'Failed to get the statuses of the readers  from PC/SC ' +
                    'with error code ' + GSC.DebugDump.dump(errorCode));
                promiseResolver.resolve(null);
              } else {
                promiseResolver.reject(new Error(
                    'Failed to get the reader statuses from PC/SC with error ' +
                    'code ' + GSC.DebugDump.dump(errorCode)));
              }
            },
            this);
      }, function(error) {
        promiseResolver.reject(new Error(
            'Failed to get the reader statuses from PC/SC: ' + error));
      });

  return promiseResolver.promise;
};

/**
 * TODO(emaxx): Write docs.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @param {!Array.<!API.SCARD_READERSTATE_OUT>} previousReaderStatesOut
 * @return {!goog.Promise}
 * @private
 */
TrackerThroughPcscApi.prototype.makeReaderStatesChangePromise_ = function(
    api, sCardContext, previousReaderStatesOut) {
  /** @type {!goog.promise.Resolver} */
  var promiseResolver = goog.Promise.withResolver();

  var readerStatesIn = goog.array.map(
      previousReaderStatesOut,
      function(readerStateOut) {
        // Clear the upper 16 bits that correspond to the number of events that
        // happened during previous call (see the SCardGetStatusChange docs for
        // the details).
        var currentState = readerStateOut['event_state'] & 0xFFFF;
        return new API.SCARD_READERSTATE_IN(
            readerStateOut['reader_name'], currentState);
      });
  // Add a magic entry that tells PC/SC to watch for the reader adding/removing
  // events too (see the SCardGetStatusChange docs for the details).
  readerStatesIn.push(new API.SCARD_READERSTATE_IN(
      '\\\\?PnP?\\Notification', API.SCARD_STATE_UNAWARE));

  api.SCardGetStatusChange(
      sCardContext,
      READER_STATUS_QUERY_TIMEOUT_MILLISECONDS,
      readerStatesIn).then(function(result) {
    result.get(
        promiseResolver.resolve.bind(promiseResolver),
        function(errorCode) {
          if (errorCode == API.SCARD_E_TIMEOUT) {
            this.logger_.fine(
                'Timeout while waiting for reader statuses change from PC/SC');
            promiseResolver.resolve();
          } else if (errorCode == API.SCARD_E_UNKNOWN_READER) {
            this.logger_.warning(
                'Failed to wait for the reader statuses change from PC/SC ' +
                'with error code ' + GSC.DebugDump.dump(errorCode));
            promiseResolver.resolve();
          } else {
            promiseResolver.reject(new Error(
                'Failed to wait for the reader statuses change from PC/SC ' +
                'with error code ' + GSC.DebugDump.dump(errorCode)));
          }
        },
        this);
  }, function(error) {
    promiseResolver.reject(new Error(
        'Failed to get the reader statuses from PC/SC: ' + error));
  }, this);

  return promiseResolver.promise;
};

/**
 * TODO(emaxx): Write docs.
 * @param {!Array.<!API.SCARD_READERSTATE_OUT>} readerStates
 * @private
 */
TrackerThroughPcscApi.prototype.updateResultFromReaderStates_ = function(
    readerStates) {
  this.updateResult_(goog.array.map(readerStates, function(readerState) {
    var isCardPresent =
        (readerState['event_state'] & API.SCARD_STATE_PRESENT) != 0;
    return new ReaderInfo(
        readerState['reader_name'],
        ReaderStatus.SUCCESS,
        undefined,
        isCardPresent);
  }));
};

/**
 * TODO(emaxx): Write docs.
 * @param {!Array.<!ReaderInfo>} result
 * @private
 */
TrackerThroughPcscApi.prototype.updateResult_ = function(result) {
  var isSame = goog.array.equals(
      result, this.result_, goog.object.equals.bind(goog.object));
  if (!isSame) {
    this.result_ = result;
    this.updateListener_();
  }
};

});  // goog.scope
