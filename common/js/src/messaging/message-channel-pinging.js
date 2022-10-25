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
 * @fileoverview This file contains definitions for performing the pinging of
 * the message channel.
 *
 * Pinging means that one end of the channel sends a special "ping" message and
 * then waits for the special "pong" response from the other end of the message
 * channel.
 *
 * Pinging allows to track the cases when the other endpoint dies or becomes
 * unresponsive or is suddenly restarted. This may be important, even in the
 * case when the Chrome long-lived message connections are used, which can track
 * most of such situations themselves (see
 * <https://developer.chrome.com/extensions/messaging#connect>).
 */

goog.provide('GoogleSmartCard.MessageChannelPinging');
goog.provide('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.provide('GoogleSmartCard.MessageChannelPinging.Pinger');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Random');
goog.require('goog.Disposable');
goog.require('goog.Timer');
goog.require('goog.asserts');
goog.require('goog.async.nextTick');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');

goog.scope(function() {

const PINGER_LOGGER_TITLE = 'Pinger';
const PING_RESPONDER_LOGGER_TITLE = 'PingResponder';
const CHANNEL_ID_MESSAGE_KEY = 'channel_id';
const PINGER_SERVICE_NAME = 'ping';
const PING_RESPONDER_SERVICE_NAME = 'pong';
const PING_RESPONDER_CHANNEL_ID = generateChannelId();

/**
 * This constant represents the timeout in milliseconds after which the channel
 * is considered dead.
 */
const PINGER_TIMEOUT_MILLISECONDS = goog.DEBUG ? 20 * 1000 : 600 * 1000;

/**
 * This constant represents the time in milliseconds between consecutive ping
 * requests.
 */
const PINGER_INTERVAL_MILLISECONDS = goog.DEBUG ? 1 * 1000 : 10 * 1000;

const GSC = GoogleSmartCard;

// The implementation in this file relies on the interval to be strictly smaller
// than the timeout.
goog.asserts.assert(PINGER_INTERVAL_MILLISECONDS < PINGER_TIMEOUT_MILLISECONDS);

// Used for overriding `PINGER_TIMEOUT_MILLISECONDS` and
// `PINGER_INTERVAL_MILLISECONDS` constants in tests.
let timeoutOverrideForTesting = null;
let intervalOverrideForTesting = null;

/**
 * @return {number}
 */
function generateChannelId() {
  return GSC.Random.randomIntegerNumber();
}

/**
 * @return {!Object}
 */
function createPingerMessageData() {
  return {};
}

/**
 * @param {number} channelId
 * @return {!Object}
 */
function createPingResponderMessageData(channelId) {
  return goog.object.create(CHANNEL_ID_MESSAGE_KEY, channelId);
}

/**
 * This class implements pinging of the specified message channel.
 *
 * Upon construction and then later with some periodicity (see the
 * `PINGER_INTERVAL_MILLISECONDS` constant), a special "ping" message is sent
 * through the message channel. Also the class subscribes itself for receiving
 * the special "pong" messages through the channel. If no responses are received
 * within some timeout (see the `PINGER_TIMEOUT_MILLISECONDS` constant) or if
 * the received response is incorrect, then the message channel is immediately
 * disposed.
 *
 * Note that the "pong" message should contain the special channel identifier,
 * which is expected to remain the same during the whole lifetime of the channel
 * to the other end. If the "pong" messages from the other end at some point
 * start containing a different identifier, then the "pong" message is
 * considered incorrect, and the message channel is disposed. This allows to
 * track situations when the other end restarts unexpectedly.
 */
GSC.MessageChannelPinging.Pinger = class extends goog.Disposable {
/**
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {!goog.log.Logger} parentLogger
 * @param {function()=} opt_onEstablished Callback to be called when the first
 * correct "pong" response is received.
 */
constructor(messageChannel, parentLogger, opt_onEstablished) {
  super();

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getChildLogger(parentLogger, PINGER_LOGGER_TITLE);

  /** @private */
  this.messageChannel_ = messageChannel;
  // Register itself for receiving "pong" response messages.
  this.messageChannel_.registerService(
      PING_RESPONDER_SERVICE_NAME, this.serviceCallback_.bind(this), true);

  /** @private */
  this.onEstablished_ =
      opt_onEstablished !== undefined ? opt_onEstablished : null;

  /**
   * @type {number|null}
   * @private
   */
  this.previousRemoteEndChannelId_ = null;

  /** @private */
  this.timeoutTimerId_ = null;
  this.scheduleTimeoutTimer_();

  goog.async.nextTick(this.postPingMessageAndScheduleNext_, this);
}

/**
 * Posts a ping message through the message channel.
 */
postPingMessage() {
  if (this.isDisposed())
    return;
  goog.log.log(this.logger, goog.log.Level.FINEST, 'Sending a ping request...');

  this.messageChannel_.send(PINGER_SERVICE_NAME, createPingerMessageData());
}

/** @override */
disposeInternal() {
  this.clearTimeoutTimer_();

  this.onEstablished_ = null;

  this.messageChannel_ = null;

  goog.log.fine(this.logger, 'Disposed');

  super.disposeInternal();
}

/**
 * @param {string|!Object} messageData
 * @private
 */
serviceCallback_(messageData) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(messageData));
  goog.asserts.assertObject(messageData);

  if (this.isDisposed())
    return;

  if (!goog.object.containsKey(messageData, CHANNEL_ID_MESSAGE_KEY)) {
    goog.log.warning(
        this.logger,
        'Received pong message has wrong format: no "' +
            CHANNEL_ID_MESSAGE_KEY + '" field is present. Disposing...');
    this.disposeChannelAndSelf_();
    return;
  }
  const channelId = messageData[CHANNEL_ID_MESSAGE_KEY];
  if (typeof channelId !== 'number') {
    goog.log.warning(
        this.logger,
        'Received pong message has wrong format: channel id ' +
            'is not a number. Disposing...');
    this.disposeChannelAndSelf_();
    return;
  }

  if (this.previousRemoteEndChannelId_ === null) {
    goog.log.fine(
        this.logger,
        'Received the first pong response (remote channel id is ' + channelId +
            '). The message channel is considered established');
    this.previousRemoteEndChannelId_ = channelId;
    if (this.onEstablished_) {
      this.onEstablished_();
      this.onEstablished_ = null;
    }
  } else if (this.previousRemoteEndChannelId_ == channelId) {
    goog.log.log(
        this.logger, goog.log.Level.FINEST,
        'Received a pong response with the correct channel ' +
            'id, so the remote end considered alive');
    this.clearTimeoutTimer_();
    this.scheduleTimeoutTimer_();
  } else {
    goog.log.warning(
        this.logger,
        'Received a pong response with a channel id different from the ' +
            'expected one (expected ' + this.previousRemoteEndChannelId_ +
            ', received ' + channelId + '). Disposing...');
    this.disposeChannelAndSelf_();
  }
}

/** @private */
disposeChannelAndSelf_() {
  goog.log.fine(this.logger, 'Disposing the message channel and self');
  this.messageChannel_.dispose();
  this.dispose();
}

/** @private */
postPingMessageAndScheduleNext_() {
  this.postPingMessage();
  this.schedulePostingPingMessage_();
}

/** @private */
schedulePostingPingMessage_() {
  if (this.isDisposed())
    return;
  let interval = PINGER_INTERVAL_MILLISECONDS;
  if (intervalOverrideForTesting)
    interval = intervalOverrideForTesting;
  goog.Timer.callOnce(this.postPingMessageAndScheduleNext_, interval, this);
}

/** @private */
scheduleTimeoutTimer_() {
  GSC.Logging.checkWithLogger(this.logger, this.timeoutTimerId_ === null);
  let timeout = PINGER_TIMEOUT_MILLISECONDS;
  if (timeoutOverrideForTesting)
    timeout = timeoutOverrideForTesting;
  this.timeoutTimerId_ =
      goog.Timer.callOnce(this.timeoutCallback_.bind(this), timeout, this);
}

/** @private */
clearTimeoutTimer_() {
  if (this.timeoutTimerId_ !== null) {
    goog.Timer.clear(this.timeoutTimerId_);
    this.timeoutTimerId_ = null;
  }
}

/** @private */
timeoutCallback_() {
  if (this.isDisposed())
    return;
  goog.log.warning(
      this.logger,
      'No pong response received in time, the remote end is ' +
          'dead. Disposing...');
  this.disposeChannelAndSelf_();
}
};

// Expose static properties:

GSC.MessageChannelPinging.Pinger.createMessageData = createPingerMessageData;
GSC.MessageChannelPinging.Pinger.SERVICE_NAME = PINGER_SERVICE_NAME;

/**
 * @param {number|null} timeoutMilliseconds
 */
GSC.MessageChannelPinging.Pinger.overrideTimeoutForTesting = function(timeoutMilliseconds) {
  timeoutOverrideForTesting = timeoutMilliseconds;
};

/**
 * @param {number|null} intervalMilliseconds
 */
GSC.MessageChannelPinging.Pinger.overrideIntervalForTesting = function(intervalMilliseconds) {
  intervalOverrideForTesting = intervalMilliseconds;
};

/**
 * This class provides responding with the "pong" messages to the "ping"
 * messages received through the specified message channel.
 *
 * Upon construction, the channel identifier is randomly generated. This
 * identifier will be specified along with the all sent "pong" messages.
 */
GSC.MessageChannelPinging.PingResponder = class extends goog.Disposable {
/**
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {!goog.log.Logger} parentLogger
 * @param {function()=} opt_onPingReceived
 */
constructor(messageChannel, parentLogger, opt_onPingReceived) {
  super();

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger =
      GSC.Logging.getChildLogger(parentLogger, PING_RESPONDER_LOGGER_TITLE);

  /** @private */
  this.messageChannel_ = messageChannel;
  // Register itself for receiving the "ping" messages.
  this.messageChannel_.registerService(
      PINGER_SERVICE_NAME, this.serviceCallback_.bind(this), true);

  /** @private */
  this.onPingReceivedListener_ = opt_onPingReceived;

  goog.log.fine(
      this.logger,
      'Initialized (generated channel id is ' + PING_RESPONDER_CHANNEL_ID + ')');
}

/** @private */
serviceCallback_() {
  if (this.isDisposed())
    return;

  goog.log.log(
      this.logger, goog.log.Level.FINEST,
      'Received a ping request, sending pong response...');

  this.messageChannel_.send(
      PING_RESPONDER_SERVICE_NAME,
      createPingResponderMessageData(PING_RESPONDER_CHANNEL_ID));

  if (this.onPingReceivedListener_)
    this.onPingReceivedListener_();
}

/** @override */
disposeInternal() {
  this.messageChannel_ = null;

  this.onPingReceivedListener_ = null;

  goog.log.fine(this.logger, 'Disposed');

  super.disposeInternal();
}
};

// Expose static properties.
GSC.MessageChannelPinging.PingResponder.SERVICE_NAME = PING_RESPONDER_SERVICE_NAME;
GSC.MessageChannelPinging.PingResponder.createMessageData = createPingResponderMessageData;
GSC.MessageChannelPinging.PingResponder.generateChannelId = generateChannelId;
});  // goog.scope
