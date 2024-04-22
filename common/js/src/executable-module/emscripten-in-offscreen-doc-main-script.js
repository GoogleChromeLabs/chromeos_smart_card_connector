/**
 * @license
 * Copyright 2024 Google Inc. All Rights Reserved.
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
 * @fileoverview This file contains code to be directly executed in the
 * Offscreen Document in order to load an Emscripten module.
 *
 * This is to be treated an internal implementation detail of
 * offscreen-doc-emscripten-module.js and shouldn't be called by client code
 * directly. It's expected to be compiled and packaged as the file called
 * "emscripten-offscreen-doc.js" (see "emscripten-offscreen-doc.html").
 */

goog.provide('GoogleSmartCard.EmscriptenInOffscreenDocMain');

goog.require('GoogleSmartCard.EmscriptenModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.OffscreenDocEmscriptenModule');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('goog.asserts');

goog.scope(function() {

const GSC = GoogleSmartCard;

// The URL parameters specify the name of the Emscripten module to be loaded.
const url = new URL(globalThis.location.href);
const moduleName =
    url.searchParams.get(GSC.OffscreenDocEmscriptenModule.URL_PARAM);
GSC.Logging.check(moduleName);
goog.asserts.assert(moduleName);
// A quick sanity check against malicious URL parameters (in case the user was
// tricked into opening our page via chrome-extension://). We don't want to
// allow loading&executing arbitrary URLs.
GSC.Logging.check(/^[0-9a-zA-Z_]+$/.test(moduleName));

const emscriptenModule = new GSC.EmscriptenModule(moduleName);

// Pipe the module's messaging channel to the port to the opener.
const moduleChannel = emscriptenModule.getMessageChannel();
const portChannel = new GSC.PortMessageChannel(chrome.runtime.connect(
    {'name': GSC.OffscreenDocEmscriptenModule.MESSAGING_PORT_NAME}));
portChannel.registerDefaultService((serviceName, payload) => {
  moduleChannel.send(serviceName, payload);
});
moduleChannel.registerDefaultService((serviceName, payload) => {
  portChannel.send(serviceName, payload);
});

// Announce the module's status via a special port to the opener.
const statusChannel = new GSC.PortMessageChannel(chrome.runtime.connect(
    {'name': GSC.OffscreenDocEmscriptenModule.STATUS_PORT_NAME}));
emscriptenModule.getLoadPromise().then(() => {
  statusChannel.send(GSC.OffscreenDocEmscriptenModule.STATUS_LOADED, {});
});
emscriptenModule.addOnDisposeCallback(() => {
  statusChannel.send(GSC.OffscreenDocEmscriptenModule.STATUS_DISPOSED, {});
});

// Start the module startup after we set all listeners.
emscriptenModule.startLoading();
});
