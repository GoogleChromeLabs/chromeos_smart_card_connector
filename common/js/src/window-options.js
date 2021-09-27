/**
 * @license
 * Copyright 2021 Google Inc. All Rights Reserved.
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

 goog.provide('GoogleSmartCard.WindowOptions');
 
 goog.scope(function() {
 
 const GSC = GoogleSmartCard;
 
 /**
 * Class that generalizes the window otions to be used by both app and extension modes.
 * @param {boolean} alwaysOnTop
 * @param {string} frame
 * @param {boolean} hidden
 * @param {string} id
 * @param {number} width
 * @param {number} height
 * @param {boolean} resizeable
 * @param {boolean} visibleOnAllWorkspaces
 * @constructor
 */
 GSC.WindowOptions = function(alwaysOnTop = false, frame = 'chrome', hidden = false, id = 'windowID', width = 200, height = 200, resizeable = true, visibleOnAllWorkspaces = false) {
    this.alwaysOnTop = alwaysOnTop;
    this.frame = frame;
    this.hidden = hidden;
    this.id = id;
    this.width = width;
    this.height = height;
    this.resizeable = resizeable;
    this.visibleOnAllWorkspaces = visibleOnAllWorkspaces;
 };
 
 });  // goog.scope
 