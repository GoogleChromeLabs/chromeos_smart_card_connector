/**
 * @license
 * Copyright 2017 Google Inc. All Rights Reserved.
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
 * @fileoverview Entry point of the Smart Card Client App background script (see
 * <https://developer.chrome.com/apps/event_pages>).
 */

/**
 * Client title for the connection to the server App.
 *
 * Currently this is only used for the debug logs produced by the server App.
 */
const CLIENT_TITLE = 'example_js_client_app';

/**
 * Context for talking to the Smart Card Connector app for making PC/SC API
 * requests.
 * @type {GoogleSmartCard.PcscLiteClient.Context}
 */
let apiContext = null;

/**
 * Object that allows to make PC/SC API requests to the Smart Card Connector
 * app.
 * @type {GoogleSmartCard.PcscLiteClient.API}
 */
let api = null;

/**
 * PC/SC-Lite SCard context.
 * @type {int}
 */
let sCardContext = null;

function initialize() {
  console.log('Establishing connection to the Connector app...');
  apiContext = new GoogleSmartCard.PcscLiteClient.Context(CLIENT_TITLE);
  apiContext.addOnInitializedCallback(onInitializationSucceeded);
  apiContext.addOnDisposeCallback(contextDisposedListener);
  apiContext.initialize();
}

function onInitializationSucceeded(constructedApi) {
  console.log('Successfully connected to the Connector app');
  api = constructedApi;
  establishContext();
}

function establishContext() {
  console.log('Establishing PC/SC-Lite context...');
  api.SCardEstablishContext(
         GoogleSmartCard.PcscLiteClient.API.SCARD_SCOPE_SYSTEM, null, null)
      .then(function(result) {
        result.get(onContextEstablished, onPcscLiteError);
      }, onRequestFailed);
}

/** @param {int} establishedSCardContext PC/SC-Lite SCard context. */
function onContextEstablished(establishedSCardContext) {
  console.log('Established PC/SC-Lite context ' + establishedSCardContext);
  sCardContext = establishedSCardContext;
  listReaders();
}

function listReaders() {
  console.log('Obtaining list of PC/SC-lite readers...');
  api.SCardListReaders(sCardContext, null).then(function(result) {
    result.get(onReadersListed, onPcscLiteError);
  }, onRequestFailed);
}

/** @param {!Array.<string>} readers List of reader names. */
function onReadersListed(readers) {
  console.log('List of PC/SC-Lite readers: ' + readers);
  if (readers.length)
    getReaderState(readers[0]);
}

/** @param {string} reader Name of the reader */
function getReaderState(reader) {
  console.log('Obtaining reader state');
  api.SCardGetStatusChange(
         sCardContext, GoogleSmartCard.PcscLiteClient.API.INFINITE, [{
           reader_name: reader,
           current_state: GoogleSmartCard.PcscLiteClient.API.SCARD_STATE_UNAWARE
         }])
      .then(function(result) {
        result.get(onReaderStateGot, onPcscLiteError);
      }, onRequestFailed);
}

/**
 * @param {!Array.<!GoogleSmartCard.PcscLiteClient.API.SCARD_READERSTATE_OUT>}
 *     readerStates
 */
function onReaderStateGot(readerStates) {
  console.log(
      'Waiting for reader state change. Current state: ' +
      JSON.stringify(readerStates));
  api.SCardGetStatusChange(
         sCardContext, GoogleSmartCard.PcscLiteClient.API.INFINITE, [{
           reader_name: readerStates[0].reader_name,
           current_state: readerStates[0].event_state
         }])
      .then(function(result) {
        result.get(onReaderStateGot, onPcscLiteError);
      }, onRequestFailed);
}

/**
 * @param {!Array.<!GoogleSmartCard.PcscLiteClient.API.SCARD_READERSTATE_OUT>}
 *     readerStates
 */
function onReaderStateChanged(readerStates) {
  console.log(
      'Got notification about reader state change: ' +
      JSON.stringify(readerStates));
}

function contextDisposedListener() {
  console.warn('Connection to the server app was shut down');
  sCardContext = null;
  api = null;
  apiContext = null;
}

/** @param {int} errorCode PC/SC-Lite error code. */
function onPcscLiteError(errorCode) {
  console.warn('PC/SC-Lite request failed with error code ' + errorCode);
}

/** @param {*} error The exception that happened during the request. */
function onRequestFailed(error) {
  console.warn(
      'Failed to perform request to the Smart Card Connector app: ' + error);
}

initialize();
