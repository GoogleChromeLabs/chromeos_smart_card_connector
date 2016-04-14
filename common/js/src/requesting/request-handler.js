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
 * @fileoverview This file contains helper definitions for implementing the
 * request handlers (see the request-receiver.js file).
 */

goog.provide('GoogleSmartCard.RequestHandler');

goog.require('goog.Promise');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * This interface represents an abstract request handler.
 * @interface
 */
GSC.RequestHandler = function() {};

/** @const */
var RequestHandler = GSC.RequestHandler;

/**
 * Method that will be called when a new request is received and needs to be
 * handled.
 *
 * The handler should return the result (either successful or not) through a
 * promise.
 * @param {!Object} payload
 * @return {!goog.Promise}
 */
RequestHandler.prototype.handleRequest = function(payload) {};

});  // goog.scope
