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
 * the typed messages.
 *
 * The "typed" message is the message of specific structure, which consists of
 * the string type and of the data (which can be an arbitrary object).
 */

goog.provide('GoogleSmartCard.TypedMessage');

goog.require('goog.object');

goog.scope(function() {

/** @const */
var TYPE_MESSAGE_KEY = 'type';
/** @const */
var DATA_MESSAGE_KEY = 'data';

/** @const */
var GSC = GoogleSmartCard;

/**
 * The structure that can be used to store the fields of the typed message.
 * @param {string} type
 * @param {!Object} data
 * @constructor
 */
GSC.TypedMessage = function(type, data) {
  /** @type {string} */
  this.type = type;
  /** @type {!Object} */
  this.data = data;
};

/** @const */
var TypedMessage = GSC.TypedMessage;

/**
 * Parses the specified message into the typed message fields (type and data).
 *
 * Returns null if the parsing failed.
 * @param {*} message
 * @return {TypedMessage?}
 */
TypedMessage.parseTypedMessage = function(message) {
  if (!goog.isObject(message) ||
      goog.object.getCount(message) != 2 ||
      !goog.object.containsKey(message, TYPE_MESSAGE_KEY) ||
      !goog.isString(message[TYPE_MESSAGE_KEY]) ||
      !goog.object.containsKey(message, DATA_MESSAGE_KEY) ||
      !goog.isObject(message[DATA_MESSAGE_KEY])) {
    return null;
  }
  return new TypedMessage(message[TYPE_MESSAGE_KEY], message[DATA_MESSAGE_KEY]);
};

/**
 * Constructs the object containing the fields of the typed message.
 * @return {!Object}
 */
TypedMessage.prototype.makeMessage = function() {
  return goog.object.create(
      TYPE_MESSAGE_KEY, this.type, DATA_MESSAGE_KEY, this.data);
};

});  // goog.scope
