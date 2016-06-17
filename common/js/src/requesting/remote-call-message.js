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
 * the remote call messages.
 *
 * The remote call message is basically a request payload (see the
 * requester-message.js file) that contains two fields: string function name and
 * an array of the function arguments.
 */

goog.provide('GoogleSmartCard.RemoteCallMessage');

goog.require('GoogleSmartCard.DebugDump');
goog.require('goog.iter');
goog.require('goog.object');
goog.require('goog.string');

goog.scope(function() {

/** @const */
var FUNCTION_NAME_MESSAGE_KEY = 'function_name';
/** @const */
var ARGUMENTS_MESSAGE_KEY = 'arguments';

/** @const */
var GSC = GoogleSmartCard;

/**
 * The structure that can be used to store the fields of the remote call
 * message.
 * @param {string} functionName
 * @param {!Array.<*>} functionArguments
 * @constructor
 */
GSC.RemoteCallMessage = function(functionName, functionArguments) {
  /** @type {string} */
  this.functionName = functionName;
  /** @type {!Array.<*>} */
  this.functionArguments = functionArguments;
};

/** @const */
var RemoteCallMessage = GSC.RemoteCallMessage;

/**
 * Parses the specified request payload into the remote call message fields
 * (function name and function arguments).
 *
 * Returns null if the parsing failed.
 * @param {!Object} requestPayload
 * @return {RemoteCallMessage}
 */
RemoteCallMessage.parseRequestPayload = function(requestPayload) {
  if (goog.object.getCount(requestPayload) != 2 ||
      !goog.object.containsKey(requestPayload, FUNCTION_NAME_MESSAGE_KEY) ||
      !goog.isString(requestPayload[FUNCTION_NAME_MESSAGE_KEY]) ||
      !goog.object.containsKey(requestPayload, ARGUMENTS_MESSAGE_KEY) ||
      !goog.isArray(requestPayload[ARGUMENTS_MESSAGE_KEY])) {
    return null;
  }
  return new RemoteCallMessage(
      requestPayload[FUNCTION_NAME_MESSAGE_KEY],
      requestPayload[ARGUMENTS_MESSAGE_KEY]);
};

/**
 * Constructs the object containing the fields of the remote call message.
 * @return {!Object}
 */
RemoteCallMessage.prototype.makeRequestPayload = function() {
  return goog.object.create(
      FUNCTION_NAME_MESSAGE_KEY,
      this.functionName,
      ARGUMENTS_MESSAGE_KEY,
      this.functionArguments);
};

/**
 * Generates a debug textual representation of the remote call message
 * structure.
 *
 * This function is safe to be used in Release builds, because all potentially
 * privacy-sensitive data is stripped away from the resulting text.
 * @return {string}
 */
RemoteCallMessage.prototype.getDebugRepresentation = function() {
  return goog.string.subs(
      '%s(%s)',
      this.functionName,
      goog.iter.join(goog.iter.map(
          this.functionArguments, GSC.DebugDump.debugDump), ', '));
};

});  // goog.scope
