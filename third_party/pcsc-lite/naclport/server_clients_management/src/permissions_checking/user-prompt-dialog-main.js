/**
 * @license
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
 * of the GoogleSmartCard library for returning the result to the
 * caller window (i.e. to the App's background page).
 *
 * TODO(emaxx): Implement internationalization using the GoogleSmartCard.I18n
 * library instead of the CSS rules (which is ugly and non a11y friendly).
 */

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptDialog.Main');

goog.require('GoogleSmartCard.InPopupMainScript');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Packaging');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.dom.classlist');
goog.require('goog.events');
goog.require('goog.log.Logger');
goog.require('goog.string');

goog.scope(function() {

const UNTRUSTED_CLASS = 'untrusted';

const GSC = GoogleSmartCard;

const urlParams = window.location.search;

const params = urlParams.split('?');

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger(
    'PcscLiteServerClientsManagement.PermissionsChecking.UserPromptDialog.' +
    'Main');

/**
 * @return {string}
 */
function getUrlParam(param) {
  for (let i = 0; i < params.length; i++) {
    if (params[i].indexOf(param) != -1) {
      return params[i].split('=')[1];
    }
  }
  return '';
}

/**
 * @return {!Object}
 */
function getUserData() {
  return {
    'is_client_known': (getUrlParam('is_client_known') === 'true'),
    'client_info_link': getUrlParam('client_info_link'),
    'client_name': getUrlParam('client_name')
  };
}

function prepareMessage() {
  const data = getUserData();

  const isClientKnown = data['is_client_known'];
  GSC.Logging.checkWithLogger(logger, typeof isClientKnown === 'boolean');
  goog.asserts.assertBoolean(isClientKnown);

  const clientInfoLink = data['client_info_link'];
  GSC.Logging.checkWithLogger(logger, typeof clientInfoLink === 'string');
  goog.asserts.assertString(clientInfoLink);

  const clientName = data['client_name'];
  GSC.Logging.checkWithLogger(logger, typeof clientName === 'string');
  goog.asserts.assertString(clientName);

  const linkTitle = chrome.i18n.getMessage(
      'pcscClientHandlingUserPromptDialogMessageClientNamePart', clientName);

  const linkElement = goog.dom.getElement('message-app-link');
  goog.dom.setTextContent(linkElement, linkTitle);
  linkElement['href'] = clientInfoLink;

  if (!isClientKnown) {
    goog.dom.classlist.add(
        goog.dom.getElement('message-prefix'), UNTRUSTED_CLASS);
    goog.dom.classlist.add(
        goog.dom.getElement('message-suffix'), UNTRUSTED_CLASS);
    goog.dom.classlist.remove(goog.dom.getElement('warning-message'), 'hidden');
  }
}

function allowClickListener() {
  GSC.InPopupMainScript.resolveModalDialog(true, getUrlParam('popup_id'));
}

function denyClickListener() {
  GSC.InPopupMainScript.resolveModalDialog(false, getUrlParam('popup_id'));
}

function closeWindowClickListener() {
  window.close();
}

prepareMessage();

goog.events.listen(goog.dom.getElement('allow'), 'click', allowClickListener);
goog.events.listen(goog.dom.getElement('deny'), 'click', denyClickListener);

goog.events.listen(
    goog.dom.getElement('close-window'), 'click', closeWindowClickListener);

GSC.InPopupMainScript.prepareAndShowAsModalDialog();
});  // goog.scope
