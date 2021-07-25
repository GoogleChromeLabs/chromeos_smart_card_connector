/** @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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
 * status based on the information exposed via the PC/SC API.
 */

goog.provide('GoogleSmartCard.PcscLiteClient.ReaderTrackerThroughPcscApi');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteClient.Context');
goog.require('goog.array');
goog.require('goog.async.nextTick');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');
goog.require('goog.Promise');
goog.require('goog.Timer');

goog.scope(function() {

/**
 * The timeout for waiting for reader states change event (i.e. it's passed to
 * the SCardGetStatusChange PC/SC function).
 *
 * Note that this timeout shouldn't be set to a very big value (and definitely
 * not to the infitite value), because sometimes the implementation in this file
 * may miss some changes - in which case the correct information would be
 * received only after this timeout passes (see the TrackerThroughPcscApi
 * class for the details).
 */
const READER_STATUS_QUERY_TIMEOUT_MILLISECONDS = 60 * 1000;

/**
 * The delay that is used when updating of the reader states failed with an
 * intermittent error.
 */
const READER_STATUS_FAILED_QUERY_DELAY_MILLISECONDS = 500;

const GSC = GoogleSmartCard;

const API = GSC.PcscLiteClient.API;

/**
 * Starts tracking readers, which will work until an error occurs. Tracking is
 * based on the information provided via the PC/SC API.
 *
 * First, this allows to obtain exactly the same information that is exposed to
 * the external applications.
 *
 * Second, this makes it possible to reuse the existing PC/SC code, without the
 * need to modify it for obtaining the information about readers.
 *
 * TODO(issue #125): This implementation is imperfect, as it can sometimes miss
 * some changes (that's because PC/SC API provides no race-free way to wait for
 * reader adding/removing). When this happens, the missed change will be updated
 * when the READER_STATUS_QUERY_TIMEOUT_MILLISECONDS timeout exceeds.
 *
 * @param {!goog.log.Logger} logger
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel
 * @param {function()} updateListener
 * @constructor
 */
GSC.PcscLiteClient.ReaderTrackerThroughPcscApi = function(
    logger, pcscContextMessageChannel, updateListener) {
  /** @private */
  this.logger_ = logger;

  /** @private */
  this.updateListener_ = updateListener;

  /**
   * @type {!Array.<{name: string, isCardPresent: boolean}>}
   * @private
   */
  this.result_ = [];

  // Start the status tracking asynchronously, so that the actual operations
  // begin after the ReaderTracker constructor ends - which makes the log
  // messages clearer.
  goog.async.nextTick(this.startStatusTracking_.bind(
      this, pcscContextMessageChannel));
};

const ReaderTrackerThroughPcscApi =
    GSC.PcscLiteClient.ReaderTrackerThroughPcscApi;

/**
 * @return {!Array.<{name: string, isCardPresent: boolean}>}
 */
ReaderTrackerThroughPcscApi.prototype.getReaders = function() {
  return this.result_;
};

/**
 * Starts the tracking which will work infinitely unless some error occurs.
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.startStatusTracking_ = function(
    pcscContextMessageChannel) {
  this.logger_.fine('Started tracking through PC/SC API');

  const promise = this.makeApiPromise_(
      pcscContextMessageChannel).then(function(api) {
    this.startStatusTrackingWithApi_(api);
  }, null, this);
  this.addPromiseErrorHandler_(promise);
};

/**
 * Makes a promise of PC/SC client API instance.
 * @param {!goog.messaging.AbstractChannel} pcscContextMessageChannel
 * @return {!goog.Promise.<!API>}
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.makeApiPromise_ = function(
    pcscContextMessageChannel) {
  /** @type {!goog.promise.Resolver.<!API>} */
  const promiseResolver = goog.Promise.withResolver();

  const context = new GSC.PcscLiteClient.Context(
      'internal reader tracker', null);
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
 * Makes a promise of the established PC/SC context.
 * @param {!API} api
 * @return {!goog.Promise.<!API.SCARDCONTEXT>}
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.makeSCardContextPromise_ = function(api) {
  /** @type {!goog.promise.Resolver.<!API.SCARDCONTEXT>} */
  const promiseResolver = goog.Promise.withResolver();

  api.SCardEstablishContext(
      API.SCARD_SCOPE_SYSTEM,
      /*opt_reserved_1=*/null,
      /*opt_reserved_2=*/null).then(
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
 * Starts the tracking, given the PC/SC client API instance.
 * @param {!API} api
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.startStatusTrackingWithApi_ = function(
    api) {
  const promise = this.makeSCardContextPromise_(api).then(
      function(sCardContext) {
        this.runStatusTrackingLoop_(api, sCardContext);
      }, null, this);
  this.addPromiseErrorHandler_(promise);
};

/**
 * Attaches a rejection handler to the passed promise.
 * @param {!goog.Promise} promise
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.addPromiseErrorHandler_ = function(
    promise) {
  promise.thenCatch(function(error) {
    this.logger_.warning(
        'Stopped tracking through PC/SC API: ' +
        (/** @type {{message:string}} */ (error)).message);
    this.updateResult_([]);
  }, this);
};

/**
 * Performs the infinite loop of the tracking.
 *
 * The loop "body" consists of obtaining the list of reader names, then
 * obtaining their statuses, updating the result with the obtained data, and
 * finally waiting for the state change.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.runStatusTrackingLoop_ = function(
    api, sCardContext) {
  const promise = this.makeReaderNamesPromise_(
      api, sCardContext).then(function(readerNames) {
    return this.makeReaderStatesPromise_(api, sCardContext, readerNames);
  }, null, this).then(function(readerStates) {
    // If an intermittent error was returned from the previous request, then
    // just sleep for some time and repeat the tracking loop body; otherwise -
    // block on waiting for a notification from PC/SC that something is changed.
    if (readerStates === null)
      return goog.Timer.promise(READER_STATUS_FAILED_QUERY_DELAY_MILLISECONDS);
    this.updateResultFromReaderStates_(readerStates);
    return this.makeReaderStatesChangePromise_(
        api, sCardContext, readerStates);
  }, null, this).then(function() {
    this.runStatusTrackingLoop_(api, sCardContext);
  }, null, this);
  this.addPromiseErrorHandler_(promise);
};

/**
 * Makes a promise of reader names that are currently reported by PC/SC.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @return {!goog.Promise.<!Array.<string>>}
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.makeReaderNamesPromise_ = function(
    api, sCardContext) {
  /** @type {!goog.promise.Resolver.<!Array.<string>>} */
  const promiseResolver = goog.Promise.withResolver();

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
 * Makes a promise of reader states that are currently reported by PC/SC.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @param {!Array.<string>} readerNames
 * @return {!goog.Promise.<!Array.<!API.SCARD_READERSTATE_OUT>|null>} Either the
 * states of the readers, or the null value in case of intermittent error.
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.makeReaderStatesPromise_ = function(
    api, sCardContext, readerNames) {
  /** @type {!goog.promise.Resolver.<!Array.<!API.SCARD_READERSTATE_OUT>|null>} */
  const promiseResolver = goog.Promise.withResolver();

  const readerStatesIn = goog.array.map(readerNames, function(readerName) {
    return new API.SCARD_READERSTATE_IN(readerName, API.SCARD_STATE_UNAWARE);
  });

  api.SCardGetStatusChange(sCardContext, 0, readerStatesIn).then(
      function(result) {
        result.get(
            promiseResolver.resolve.bind(promiseResolver),
            function(errorCode) {
              if (errorCode == API.SCARD_E_UNKNOWN_READER) {
                this.logger_.warning(
                    'Getting the statuses of the readers from PC/SC finished ' +
                    'unsuccessfully due to removal of the tracked reader. A ' +
                    'retry will be attempted after some delay');
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
 * Updates the result reported by this instance, given the reader states
 * obtained from a call to the SCardGetStatusChange function.
 * @param {!Array.<!API.SCARD_READERSTATE_OUT>} readerStates
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.updateResultFromReaderStates_ = function(
    readerStates) {
  this.updateResult_(goog.array.map(readerStates, function(readerState) {
    return {
      name: readerState['reader_name'],
      isCardPresent: (readerState['event_state'] & API.SCARD_STATE_PRESENT) != 0
    };
  }));
};

/**
 * Makes a promise that will be resolved once some change with the specified
 * state is detected, or when the timeout passes.
 *
 * This also watches for the reader adding/removing events, but this,
 * unfortunately, cannot be done in a race-free manner: the PC/SC
 * SCardGetStatusChange function would notify only about those reader
 * adding/removing events that occurred during its call. So the reader
 * adding/removing may happen.
 * @param {!API} api
 * @param {!API.SCARDCONTEXT} sCardContext
 * @param {!Array.<!API.SCARD_READERSTATE_OUT>} previousReaderStatesOut
 * @return {!goog.Promise}
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.makeReaderStatesChangePromise_ = function(
    api, sCardContext, previousReaderStatesOut) {
  /** @type {!goog.promise.Resolver} */
  const promiseResolver = goog.Promise.withResolver();

  const readerStatesIn = goog.array.map(
      previousReaderStatesOut,
      function(readerStateOut) {
        // Clear the upper 16 bits that correspond to the number of events that
        // happened during previous call (see the SCardGetStatusChange docs for
        // the details).
        const currentState = readerStateOut['event_state'] & 0xFFFF;
        return new API.SCARD_READERSTATE_IN(
            readerStateOut['reader_name'], currentState);
      });
  // Add a magic entry that tells PC/SC to watch for the reader adding/removing
  // events too (see the SCardGetStatusChange docs for the details).
  readerStatesIn.push(new API.SCARD_READERSTATE_IN(
      '\\\\?PnP?\\Notification', API.SCARD_STATE_UNAWARE));

  this.logger_.fine(
      'Waiting for the reader statuses change from PC/SC with the following ' +
      'data: ' + GSC.DebugDump.dump(readerStatesIn) + '...');

  api.SCardGetStatusChange(
      sCardContext,
      READER_STATUS_QUERY_TIMEOUT_MILLISECONDS,
      readerStatesIn).then(function(result) {
    result.get(function(readerStatesOut) {
      this.logger_.fine('Received a reader statuses change event from PC/SC: ' +
                        GSC.DebugDump.dump(readerStatesOut));
      promiseResolver.resolve();
    }, function(errorCode) {
      if (errorCode == API.SCARD_E_TIMEOUT) {
        this.logger_.fine('No reader statuses changes were reported by PC/SC ' +
                          'within the timeout');
        promiseResolver.resolve();
      } else if (errorCode == API.SCARD_E_UNKNOWN_READER) {
        this.logger_.warning(
            'Waiting for the reader statuses changes from PC/SC finished ' +
            'unsuccessfully due to removal of the tracked reader');
        promiseResolver.resolve();
      } else {
        promiseResolver.reject(new Error(
            'Failed to wait for the reader statuses change from PC/SC with ' +
            'error code ' + GSC.DebugDump.dump(errorCode)));
      }
    }, this);
  }, function(error) {
    promiseResolver.reject(new Error(
        'Failed to get the reader statuses from PC/SC: ' + error));
  }, this);

  return promiseResolver.promise;
};

/**
 * Updates the result reported by this instance, and fires the update listener
 * when necessary.
 * @param {!Array.<{name: string, isCardPresent: boolean}>} result
 * @private
 */
ReaderTrackerThroughPcscApi.prototype.updateResult_ = function(result) {
  const isSame = goog.array.equals(
      result, this.result_, goog.object.equals.bind(goog.object));
  if (isSame)
    return;

  const dumpedResults = goog.array.map(result, function(readerInfo) {
    return '"' + readerInfo.name + '"' +
           (readerInfo.isCardPresent ? ' (with inserted card)' : '');
  });
  this.logger_.info(
      'Information about readers returned by PC/SC: ' +
      (dumpedResults.length ?
           goog.iter.join(dumpedResults, ', ') : 'no readers'));

  this.result_ = result;
  this.updateListener_();
};

});  // goog.scope
