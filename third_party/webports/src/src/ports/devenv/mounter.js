/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// TODO(gdeepti): Clean up global variables.
var mounterBackground = null;
var mounterThumb = null;
var mounterHeader = null;
var mounter = null;
var isVisible = false;
var mounterClient = null;

function intToPixels(int) {
  return int + 'px';
}

function walkDom(callback) {
  var body = document.body;
  var loop = function(element) {
    do {
      var recurse = true;
      if(element.nodeType == 1)
        recurse = callback(element);
      if (recurse && element.hasChildNodes() && !element.hidden)
        loop(element.firstChild);
      element = element.nextSibling;
    }
    while (element);
  };
  loop(body);
}

function grabFocus() {
  walkDom(function(element) {
    if (element == mounterBackground)
      return false;

    if (element.hasAttribute('tabIndex')) {
       element.oldTabIndex = element.getAttribute('tabIndex');
     }
    element.setAttribute('tabIndex', '-1');
    return true;
  })
}

function releaseFocus() {
  walkDom(function(element) {
    if (element == mounterBackground)
      return false;

    if (element.oldTabIndex) {
      element.setAttribute('tabIndex', element.oldTabIndex);
      delete element.oldTabIndex;
    } else {
      element.removeAttribute('tabIndex');
    }
    return true;
  })
  mounterClient.terminal.focus();
}

function sizeBackground() {
  mounterBackground.style.height = intToPixels(window.innerHeight);
  var bgTop = 0;
  if (!isVisible)
    bgTop = 10 - window.innerHeight
  mounterBackground.style.top = intToPixels(bgTop);
}

function changeVisibility(visible) {
  isVisible = visible;

  sizeBackground()
  if (!mounterClient)
    return;

  if (isVisible)
    grabFocus();
  else
    releaseFocus();
}

function backgroundClicked() {
  if (isVisible)
    changeVisibility(false);
}

function thumbClicked(event) {
  if (!isVisible)
    changeVisibility(true);
  event.stopPropagation();
}

function mounterClicked(event) {
  event.stopPropagation();
}

function initMounterclient(mount, chooseFolder,
    mountHandler, unmountHandler, terminal) {
  this.mount = mount;
  this.onChooseFolder = chooseFolder;
  this.onMount = mountHandler;
  this.onUnmount = unmountHandler
  this.terminal = terminal;
}

function addMountControlItem(item, mountControl) {
  mountControl.appendChild(item);
  item.mountControl = mountControl;
}

function populateMountControl(mountControl) {
  mountControl.pathEdit.value = g_mount.mountPoint;
  // TODO(gdeepti): Enable this to accept mountPoint specified by th user.
  // Temporarily disabled to fix sync issues.
  mountControl.pathEdit.disabled = g_mount.mounted;
  mountControl.localPathEdit.value = g_mount.localPath;
  mountControl.localPathEdit.disabled = g_mount.mounted;
  mountControl.selectButton.disabled = g_mount.mounted;
  mountControl.mountButton.disabled =
      (g_mount.mounted ||
      (g_mount.entry == null));
  mountControl.unmountButton.disabled = !g_mount.mounted;
}

function mountPointChanged(event) {
  var mountControl = event.target.mountControl;
  mountControl.mount.mountPoint = event.target.value;
  populateMountControl(mountControl);
}

function chooseFolderClicked(event) {
  var mountControl = event.target.mountControl;
  mounterClient.onChooseFolder(mountControl.mount, function() {
    populateMountControl(mountControl);
  });
}

function mountClicked(event) {
  var mountControl = event.target.mountControl;
  mounterClient.onMount(mountControl.mount, function() {
    populateMountControl(mountControl);
  });
}

function unmountClicked(event) {
  var mountControl = event.target.mountControl;
  mounterClient.onUnmount(mountControl.mount, function() {
    populateMountControl(mountControl);
  });
}

function removeMountClicked(event) {
  var mountControl = event.target.mountControl;
  mounterClient.onRemoveMount(mountControl.mount);
}

function addMountControl(mount, init) {
  var mountControl = document.createElement('div');
  mountControl.classList.add('mountControl');

  var selectButton = document.createElement('button');
  selectButton.textContent = 'Choose Folder';
  addMountControlItem(selectButton, mountControl);
  mountControl.selectButton = selectButton;
  selectButton.onclick = chooseFolderClicked;

  var localPathLabel = document.createElement('span');
  localPathLabel.textContent = 'Local path:';
  mountControl.appendChild(localPathLabel);

  var localPathEdit = document.createElement('input');
  localPathEdit.textContent = '';
  localPathEdit.readOnly = true;
  addMountControlItem(localPathEdit, mountControl);
  mountControl.localPathEdit = localPathEdit;

  var pathEditLabel = document.createElement('span');
  pathEditLabel.textContent = 'Mount point:';
  mountControl.appendChild(pathEditLabel);

  var pathEdit = document.createElement('input');
  pathEdit.value = mount.mountPoint;
  addMountControlItem(pathEdit, mountControl);
  mountControl.pathEdit = pathEdit;
  pathEdit.oninput = mountPointChanged;

  var mountButton = document.createElement('button');
  mountButton.textContent = 'Mount';
  addMountControlItem(mountButton, mountControl);
  mountControl.mountButton = mountButton;
  mountButton.onclick = mountClicked;

  var unmountButton = document.createElement('button');
  unmountButton.textContent = 'Unmount';
  addMountControlItem(unmountButton, mountControl);
  mountControl.unmountButton = unmountButton;
  unmountButton.onclick = unmountClicked;

  mountControl.mount = mount;

  init(function() {
    populateMountControl(mountControl);
  });
  mounter.appendChild(mountControl);

  return mountControl;
}

function initMounter(makeVisible, aMounterClient, init) {
  mounterBackground.onclick = backgroundClicked;
  mounter.onclick = mounterClicked;
  mounterThumb.onclick = thumbClicked;

  mounterClient = aMounterClient;

  addMountControl(g_mount, init);
  changeVisibility(makeVisible);

  window.onresize = function() {
    sizeBackground();
  }
}
