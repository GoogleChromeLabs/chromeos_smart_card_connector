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
 * @fileoverview This file contains helper definitions for parsing and creating
 * the requester messages: request messages and response messages.
 *
 * The request message is basically a typed message (see the
 * ../messaging/typed-message.js file) that has a special type, uniquely
 * representing the requester, and the data containing an integer request
 * identifier and an arbitrary object payload.
 *
 * The response message is a typed message whose type is also referring to the
 * requester (that was used to send the request message) and whose data contains
 * the request identifier and either the response payload (arbitrary object) or
 * an error message text.
 */

goog.provide('GoogleSmartCard.RequesterMessage');
goog.provide('GoogleSmartCard.RequesterMessage.RequestMessageData');
goog.provide('GoogleSmartCard.RequesterMessage.ResponseMessageData');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var REQUEST_MESSAGE_TYPE_SUFFIX = '::request';
/** @const */
var RESPONSE_MESSAGE_TYPE_SUFFIX = '::response';
/** @const */
var REQUEST_ID_MESSAGE_KEY = 'request_id';
/** @const */
var PAYLOAD_MESSAGE_KEY = 'payload';
/** @const */
var ERROR_MESSAGE_KEY = 'error';

/** @const */
var RequesterMessage = GSC.RequesterMessage;

/**
 * Returns the message type for the requests for the requester with the
 * specified name.
 * @param {string} name
 * @return {string}
 */
RequesterMessage.getRequestMessageType = function(name) {
  return name + REQUEST_MESSAGE_TYPE_SUFFIX;
};

/**
 * Returns the message type for the requests responses for the requester with
 * the specified name.
 * @param {string} name
 * @return {string}
 */
RequesterMessage.getResponseMessageType = function(name) {
  return name + RESPONSE_MESSAGE_TYPE_SUFFIX;
};

/**
 * The structure that can be used to store the fields of the request message.
 * @param {number} requestId
 * @param {!Object} payload
 * @constructor
 */
RequesterMessage.RequestMessageData = function(requestId, payload) {
  /** @type {number} */
  this.requestId = requestId;
  /** @type {!Object} */
  this.payload = payload;
};

/** @const */
var RequestMessageData = RequesterMessage.RequestMessageData;

/**
 * Parses the specified message data into the request message fields.
 *
 * Returns null if the parsing failed.
 * @param {!Object} messageData
 * @return {RequestMessageData?}
 */
RequestMessageData.parseMessageData = function(messageData) {
  if (goog.object.getCount(messageData) != 2 ||
      !goog.object.containsKey(messageData, REQUEST_ID_MESSAGE_KEY) ||
      typeof messageData[REQUEST_ID_MESSAGE_KEY] !== 'number' ||
      !goog.object.containsKey(messageData, PAYLOAD_MESSAGE_KEY) ||
      !goog.isObject(messageData[PAYLOAD_MESSAGE_KEY])) {
    return null;
  }
  return new RequestMessageData(
      messageData[REQUEST_ID_MESSAGE_KEY], messageData[PAYLOAD_MESSAGE_KEY]);
};

/**
 * Constructs the object containing the fields of the request message data.
 * @return {!Object}
 */
RequestMessageData.prototype.makeMessageData = function() {
  return goog.object.create(
      REQUEST_ID_MESSAGE_KEY,
      this.requestId,
      PAYLOAD_MESSAGE_KEY,
      this.payload);
};

/**
 * The structure that can be used to store the fields of the response message.
 * @param {number} requestId
 * @param {*=} opt_payload
 * @param {string=} opt_errorMessage
 * @constructor
 */
RequesterMessage.ResponseMessageData = function(
    requestId, opt_payload, opt_errorMessage) {
  /** @type {number} */
  this.requestId = requestId;
  /** @type {*} */
  this.payload = opt_payload;
  /** @type {string|undefined} */
  this.errorMessage = opt_errorMessage;

  GSC.Logging.checkWithLogger(
      this.logger, opt_payload === undefined || opt_errorMessage === undefined);
};

/** @const */
var ResponseMessageData = RequesterMessage.ResponseMessageData;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ResponseMessageData.prototype.logger = GSC.Logging.getScopedLogger(
    'RequesterMessage.ResponseMessageData');

/**
 * @return {boolean}
 */
ResponseMessageData.prototype.isSuccessful = function() {
  return this.errorMessage === undefined;
};

/**
 * @return {*}
 */
ResponseMessageData.prototype.getPayload = function() {
  GSC.Logging.checkWithLogger(this.logger, this.isSuccessful());
  return this.payload;
};

/**
 * @return {string}
 */
ResponseMessageData.prototype.getErrorMessage = function() {
  GSC.Logging.checkWithLogger(this.logger, !this.isSuccessful());
  GSC.Logging.checkWithLogger(
      this.logger, typeof this.errorMessage === 'string');
  goog.asserts.assertString(this.errorMessage);
  return this.errorMessage;
};

/**
 * Parses the specified message data into the response message fields.
 *
 * Returns null if the parsing failed.
 * @param {!Object} messageData
 * @return {ResponseMessageData?}
 */
ResponseMessageData.parseMessageData = function(messageData) {
  if (goog.object.getCount(messageData) != 2 ||
      !goog.object.containsKey(messageData, REQUEST_ID_MESSAGE_KEY) ||
      typeof messageData[REQUEST_ID_MESSAGE_KEY] !== 'number') {
    return null;
  }
  var requestId = messageData[REQUEST_ID_MESSAGE_KEY];
  if (goog.object.containsKey(messageData, PAYLOAD_MESSAGE_KEY)) {
    return new ResponseMessageData(requestId, messageData[PAYLOAD_MESSAGE_KEY]);
  }
  if (goog.object.containsKey(messageData, ERROR_MESSAGE_KEY) &&
      typeof messageData[ERROR_MESSAGE_KEY] === 'string') {
    return new ResponseMessageData(
        requestId, undefined, messageData[ERROR_MESSAGE_KEY]);
  }
  return null;
};

/**
 * Constructs the object containing the fields of the response message data.
 * @return {!Object}
 */
ResponseMessageData.prototype.makeMessageData = function() {
  var args = [REQUEST_ID_MESSAGE_KEY, this.requestId];
  if (this.isSuccessful()) {
    args.push(PAYLOAD_MESSAGE_KEY);
    args.push(this.getPayload());
  } else {
    args.push(ERROR_MESSAGE_KEY);
    args.push(this.getErrorMessage());
  }
  return goog.object.create(args);
};

});  // goog.scope
