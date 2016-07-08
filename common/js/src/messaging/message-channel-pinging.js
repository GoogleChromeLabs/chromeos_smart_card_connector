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
goog.provide('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.provide('GoogleSmartCard.MessageChannelPinging.PingResponder');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Random');
goog.require('goog.Disposable');
goog.require('goog.Timer');
goog.require('goog.asserts');
goog.require('goog.async.nextTick');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var PINGER_LOGGER_TITLE = 'Pinger';
/** @const */
var PING_RESPONDER_LOGGER_TITLE = 'PingResponder';
/** @const */
var CHANNEL_ID_MESSAGE_KEY = 'channel_id';

/** @const */
var GSC = GoogleSmartCard;

/**
 * This class implements pinging of the specified message channel.
 *
 * Upon construction and then later with some periodicity (see the
 * PINGER_INTERVAL_MILLISECONDS constant), a special "ping" message is sent
 * through the message channel. Also the class subscribes itself for receiving
 * the special "pong" messages through the channel. If no responses are received
 * within some timeout (see the PINGER_TIMEOUT_MILLISECONDS constant) or if the
 * received response is incorrect, then the message channel is immediately
 * disposed.
 *
 * Note that the "pong" message should contain the special channel identifier,
 * which is expected to remain the same during the whole lifetime of the channel
 * to the other end. If the "pong" messages from the other end at some point
 * start containing a different identifier, then the "pong" message is
 * considered incorrect, and the message channel is disposed. This allows to
 * track situations when the other end restarts unexpectedly.
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {!goog.log.Logger} parentLogger
 * @param {function()=} opt_onEstablished Callback to be called when the first
 * correct "pong" response is received.
 * @constructor
 * @extends goog.Disposable
 */
GSC.MessageChannelPinging.Pinger = function(
    messageChannel, parentLogger, opt_onEstablished) {
  Pinger.base(this, 'constructor');

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getChildLogger(parentLogger, PINGER_LOGGER_TITLE);

  /** @private */
  this.messageChannel_ = messageChannel;
  // Register itself for receiving "pong" response messages.
  this.messageChannel_.registerService(
      PingResponder.SERVICE_NAME, this.serviceCallback_.bind(this), true);

  /** @private */
  this.onEstablished_ = goog.isDef(opt_onEstablished) ?
      opt_onEstablished : null;

  /**
   * @type {number|null}
   * @private
   */
  this.previousRemoteEndChannelId_ = null;

  /** @private */
  this.timeoutTimerId_ = null;
  this.scheduleTimeoutTimer_();

  goog.async.nextTick(this.postPingMessage_, this);
};

/** @const */
var Pinger = GSC.MessageChannelPinging.Pinger;

goog.inherits(Pinger, goog.Disposable);

/**
 * This constant represents the timeout in milliseconds after which the channel
 * is considered dead.
 * @const
 */
Pinger.PINGER_TIMEOUT_MILLISECONDS = goog.DEBUG ? 20 * 1000 : 600 * 1000;

/**
 * This constant represents the time in milliseconds between consecutive ping
 * requests.
 *
 * Note that PINGER_INTERVAL_MILLISECONDS < PINGER_TIMEOUT_MILLISECONDS needs to
 * hold because of how pinging is implemented here.
 * @const
 */
Pinger.PINGER_INTERVAL_MILLISECONDS = goog.DEBUG ? 1 * 1000 : 10 * 1000;

/** @const */
Pinger.SERVICE_NAME = 'ping';

/**
 * @return {!Object}
 */
Pinger.createMessageData = function() {
  return {};
};

/** @override */
Pinger.prototype.disposeInternal = function() {
  this.clearTimeoutTimer_();

  this.onEstablished_ = null;

  this.messageChannel_ = null;

  this.logger.fine('Disposed');

  Pinger.base(this, 'disposeInternal');
};

/**
 * @param {string|!Object} messageData
 * @private
 */
Pinger.prototype.serviceCallback_ = function(messageData) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(messageData));
  goog.asserts.assertObject(messageData);

  if (this.isDisposed())
    return;

  if (!goog.object.containsKey(messageData, CHANNEL_ID_MESSAGE_KEY)) {
    this.logger.warning(
        'Received pong message has wrong format: no "' +
        CHANNEL_ID_MESSAGE_KEY + '" field is present. Disposing...');
    this.disposeChannelAndSelf_();
    return;
  }
  var channelId = messageData[CHANNEL_ID_MESSAGE_KEY];
  if (!goog.isNumber(channelId)) {
    this.logger.warning('Received pong message has wrong format: channel id ' +
                        'is not a number. Disposing...');
    this.disposeChannelAndSelf_();
    return;
  }

  if (goog.isNull(this.previousRemoteEndChannelId_)) {
    this.logger.fine(
        'Received the first pong response (remote channel id is ' + channelId +
        '). The message channel is considered established');
    this.previousRemoteEndChannelId_ = channelId;
    if (this.onEstablished_) {
      this.onEstablished_();
      this.onEstablished_ = null;
    }
  } else if (this.previousRemoteEndChannelId_ == channelId) {
    this.logger.finest('Received a pong response with the correct channel ' +
                       'id, so the remote end considered alive');
    this.clearTimeoutTimer_();
    this.scheduleTimeoutTimer_();
  } else {
    this.logger.warning(
        'Received a pong response with a channel id different from the ' +
        'expected one (expected ' + this.previousRemoteEndChannelId_ +
        ', received ' + channelId + '). Disposing...');
    this.disposeChannelAndSelf_();
  }
};

/** @private */
Pinger.prototype.disposeChannelAndSelf_ = function() {
  this.logger.fine('Disposing the message channel and self');
  this.messageChannel_.dispose();
  this.dispose();
};

/** @private */
Pinger.prototype.postPingMessage_ = function() {
  if (this.isDisposed())
    return;
  this.logger.finest('Sending a ping request...');

  this.messageChannel_.send(Pinger.SERVICE_NAME, Pinger.createMessageData());

  goog.Timer.callOnce(
      this.postPingMessage_, Pinger.PINGER_INTERVAL_MILLISECONDS, this);
};

/** @private */
Pinger.prototype.scheduleTimeoutTimer_ = function() {
  GSC.Logging.checkWithLogger(this.logger, goog.isNull(this.timeoutTimerId_));
  this.timeoutTimerId_ = goog.Timer.callOnce(
      this.timeoutCallback_.bind(this),
      Pinger.PINGER_TIMEOUT_MILLISECONDS,
      this);
};

/** @private */
Pinger.prototype.clearTimeoutTimer_ = function() {
  if (!goog.isNull(this.timeoutTimerId_)) {
    goog.Timer.clear(this.timeoutTimerId_);
    this.timeoutTimerId_ = null;
  }
};

/** @private */
Pinger.prototype.timeoutCallback_ = function() {
  if (this.isDisposed())
    return;
  this.logger.warning('No pong response received in time, the remote end is ' +
                      'dead. Disposing...');
  this.disposeChannelAndSelf_();
};

/**
 * This class provides responding with the "pong" messages to the "ping"
 * messages received through the specified message channel.
 *
 * Upon construction, the channel identifier is randomly generated. This
 * identifier will be specified along with the all sent "pong" messages.
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {!goog.log.Logger} parentLogger
 * @constructor
 */
GSC.MessageChannelPinging.PingResponder = function(
    messageChannel, parentLogger) {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getChildLogger(
      parentLogger, PING_RESPONDER_LOGGER_TITLE);

  /** @private */
  this.messageChannel_ = messageChannel;
  // Register itself for receiving the "ping" messages.
  this.messageChannel_.registerService(
      Pinger.SERVICE_NAME, this.serviceCallback_.bind(this), true);

  this.channelId_ = PingResponder.generateChannelId();

  this.logger.fine(
      'Initialized (generated channel id is ' + this.channelId_ + ')');
};

/** @const */
var PingResponder = GSC.MessageChannelPinging.PingResponder;

/** @const */
PingResponder.SERVICE_NAME = 'pong';

/**
 * @return {number}
 */
PingResponder.generateChannelId = function() {
  return GSC.Random.randomIntegerNumber();
};

/**
 * @param {number} channelId
 * @return {!Object}
 */
PingResponder.createMessageData = function(channelId) {
  return goog.object.create(CHANNEL_ID_MESSAGE_KEY, channelId);
};

/** @private */
PingResponder.prototype.serviceCallback_ = function() {
  this.logger.finest('Received a ping request, sending pong response...');
  this.messageChannel_.send(
      PingResponder.SERVICE_NAME,
      PingResponder.createMessageData(this.channelId_));
};

});  // goog.scope
