/** @license
 * Copyright 2016 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @fileoverview Main script of the PC/SC-Lite server client permissions user
 * prompt dialog window.
 *
 * Once the dialog is finished with some result (either user pressed the allow
 * button or the deny button, or the dialog was canceled), it uses the methods
 * of the GoogleSmartCard.PopupWindow library for returning the result to the
 * caller window (i.e. to the App's background page).
 */

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptDialog.Main');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PopupWindow.Client');
goog.require('goog.dom');
goog.require('goog.dom.classlist');
goog.require('goog.events');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var APP_LINK_TEMPLATE = 'https://chrome.google.com/webstore/detail/%s';

/** @const */
var UNTRUSTED_CLASS = 'untrusted';

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger(
    'PcscLiteServerClientsManagement.PermissionsChecking.UserPromptDialog.' +
    'Main');

function prepareMessage() {
  var data = GSC.PopupWindow.Client.getData();

  var isClientKnown = data['is_client_known'];
  GSC.Logging.checkWithLogger(logger, goog.isBoolean(isClientKnown));
  goog.asserts.assertBoolean(isClientKnown);

  var clientAppId = data['client_app_id'];
  GSC.Logging.checkWithLogger(logger, goog.isString(clientAppId));
  goog.asserts.assertString(clientAppId);

  var clientAppName = data['client_app_name'];
  if (isClientKnown) {
    GSC.Logging.checkWithLogger(logger, goog.isString(clientAppName));
    goog.asserts.assertString(clientAppName);
  } else {
    GSC.Logging.checkWithLogger(
        logger, !goog.isDef(clientAppName) || goog.isString(clientAppName));
    goog.asserts.assert(
        !goog.isDef(clientAppName) || goog.isString(clientAppName));
  }

  var linkTitle;
  if (isClientKnown) {
    linkTitle = chrome.i18n.getMessage(
        'pcscClientHandlingUserPromptDialogMessageAppNamePart', clientAppName);
  } else {
    linkTitle = chrome.i18n.getMessage(
        'pcscClientHandlingUserPromptDialogMessageAppIdPartForUnknownApp',
        clientAppId);
  }

  var linkHref = goog.string.subs(APP_LINK_TEMPLATE, clientAppId);

  var linkElement = goog.dom.getElement('message-app-link');
  goog.dom.setTextContent(linkElement, linkTitle);
  linkElement["href"] = linkHref;

  if (!isClientKnown) {
    goog.dom.classlist.add(
        goog.dom.getElement('message-prefix'), UNTRUSTED_CLASS);
    goog.dom.classlist.add(
        goog.dom.getElement('message-suffix'), UNTRUSTED_CLASS);
    goog.dom.classlist.remove(goog.dom.getElement('warning-message'), 'hidden');
  }
}

function allowClickListener() {
  GSC.PopupWindow.Client.resolveModalDialog(true);
}

function denyClickListener() {
  GSC.PopupWindow.Client.resolveModalDialog(false);
}

function closeWindowClickListener() {
  chrome.app.window.current().close();
}

prepareMessage();

goog.events.listen(goog.dom.getElement('allow'), 'click', allowClickListener);
goog.events.listen(goog.dom.getElement('deny'), 'click', denyClickListener);

goog.events.listen(
    goog.dom.getElement('close-window'), 'click', closeWindowClickListener);

GSC.PopupWindow.Client.prepareAndShowAsModalDialog();

});  // goog.scope
