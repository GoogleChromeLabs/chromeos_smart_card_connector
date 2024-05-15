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

goog.provide('GoogleSmartCard.PopupConstants');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Opener passes the ID to the popup under this key.
 * @const
 */
GSC.PopupConstants.POPUP_ID_KEY = 'popup_id';

/**
 * The popup opens a messaging port with the "<prefix><popupId>" to report its
 * result.
 * @const
 */
GSC.PopupConstants.PORT_NAME_PREFIX = 'popup_';

/**
 * The popup sends its result as a message with this type.
 * @const
 */
GSC.PopupConstants.RESULT_MESSAGE_TYPE = 'result';

/**
 * On success, the popup sends the result as a {<key>: <value>} message.
 * @const
 */
GSC.PopupConstants.RESULT_VALUE_KEY = 'value';

/**
 * On error, the popup sends the {<key>: <error>} message.
 * @const
 */
GSC.PopupConstants.RESULT_ERROR_KEY = 'error';
});
