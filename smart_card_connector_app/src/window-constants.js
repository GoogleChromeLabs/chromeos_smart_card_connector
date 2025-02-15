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
 * @fileoverview Constants shared between the Connector App's window and
 * background page.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.Constants');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * We open a message port with this name to the background page, to learn about
 * the available devices.
 */
GSC.ConnectorApp.Window.Constants.DEVICE_LIST_MESSAGING_PORT_NAME =
    'available-devices';

/**
 * We open a message port with this name to the background page, to learn about
 * the currently connected client applications.
 */
GSC.ConnectorApp.Window.Constants.CLIENT_APP_LIST_MESSAGING_PORT_NAME =
    'connected-clients';

/**
 * We open a message port with this name to the background page, to learn about
 * the crashes in the application.
 */
GSC.ConnectorApp.Window.Constants.CRASH_EVENT_MESSAGING_PORT_NAME = 'crashed';
});  // goog.scope
