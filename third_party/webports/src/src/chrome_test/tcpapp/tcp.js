/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// This is the PPAPI code for TCP FIN (disconnected).
var ERROR_CODE_FIN = -100;

// Map sockets to onConnectExternal ports, to remember where we route incoming
// TCP messages.
var portMap = {};

/*
 * The following functions convert between ArrayBuffer and Array, as TCP
 * requires ArrayBuffers while postMessage requires Arrays.
 */

// Convert an ArrayBuffer to an Array.
function ab2Array(buf) {
  return Array.apply(null, new Uint8Array(buf));
}

// Convert an Array to an ArrayBuffer.
function array2Ab(arr) {
  return new Uint8Array(arr).buffer;
}

// Route incoming TCP messages.
chrome.sockets.tcp.onReceive.addListener(function(info) {
  if (portMap[info.socketId] === undefined) {
    chrome.sockets.tcp.close(info.socketId, function() {});
    return;
  }
  portMap[info.socketId].postMessage({
    name: 'tcp_message',
    socket: info.socketId,
    data: ab2Array(info.data)
  });
});

// Handle TCP disconnects.
chrome.sockets.tcp.onReceiveError.addListener(function(info) {
  if (info.resultCode !== ERROR_CODE_FIN) {
    console.log('unhandled error: ' + info.resultCode);
  }
  if (portMap[info.socketId] === undefined) {
    return;
  }
  portMap[info.socketId].postMessage({
    name: 'tcp_disconnect',
    socket: info.socketId
  });
});

chrome.runtime.onConnectExternal.addListener(function(port) {
  var tcp = chrome.sockets.tcp;

  port.onMessage.addListener(function(msg) {
    function resolve(data) {
      var response = {
        name: msg.name + '_reply',
        data: data || null
      }
      port.postMessage(response);
    }
    function reject(err) {
      var response = {
        name: msg.name + '_error',
        error: err || null
      }
      port.postMessage(response);
    }

    switch (msg.name) {
      case 'tcp_connect':
        if (msg.addr === undefined || msg.port === undefined) {
          reject('"addr" and "port" fields expected');
          break;
        }
        tcp.create({}, function(createInfo) {
          var socketId = createInfo.socketId;
          tcp.connect(socketId, msg.addr, parseInt(msg.port), function(result) {
            if (result < 0) {
              tcp.close(socketId, function() {
                reject(result);
              });
            } else {
              portMap[socketId] = port;
              resolve(socketId);
            }
          });
        });
        break;
      case 'tcp_send':
        if (msg.socket === undefined || msg.data === undefined) {
          reject('"socket" and "data" fields expected');
          break;
        }
        var data = array2Ab(msg.data);
        tcp.send(msg.socket, data, function(info) {
          if (info.resultCode < 0) {
            reject('received error code ' + info.resultCode);
          } else {
            resolve(info.bytesSent);
          }
        });
        break;
      case 'tcp_close':
        if (msg.socket === undefined || portMap[msg.socket] === undefined) {
          reject('socket not found');
          break;
        }
        delete portMap[msg.socket];
        tcp.close(msg.socket, function() {
          resolve();
        });
        break;
      default:
        port.postMessage({name: 'unknown_error', error: 'unknown message'});
    }
  });

  port.onDisconnect.addListener(function() {
    // O(N) time, but there's something wrong if there are enough TCP
    // connections that the performance of this loop matters.
    for (var socket in portMap) {
      if (portMap[socket] === port) {
        delete portMap[socket];
        tcp.close(parseInt(socket), function() {});
      }
    }
  });
});
