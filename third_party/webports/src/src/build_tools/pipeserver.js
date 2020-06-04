/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * PipeServer manages a set of anonymous pipes.
 */
function PipeServer() {
  // Start id for anonymous pipes.
  this.anonymousPipeId = 1;

  // Status of anonymous pipes.
  this.anonymousPipes = {};
}

/**
 * Pipe error.
 * @type {number}
 */
PipeServer.prototype.EPIPE = 32;

/**
 * Handle an anonymous pipe creation call.
 */
PipeServer.prototype.handleMessageAPipe = function(msg, reply, src) {
  var id = this.anonymousPipeId++;
  this.anonymousPipes[id] = {
    readers: {},
    writers: {},
    readsPending: [],
    writesPending: [],
  };
  var pipe = this.anonymousPipes[id];
  pipe.readers[src.pid] = null;
  pipe.writers[src.pid] = null;
  reply({
    pipe_id: id,
  });
}

/**
 * Handle an anonymous pipe write call.
 */
PipeServer.prototype.handleMessageAPipeWrite = function(
    msg, reply, src) {
  var id = msg.pipe_id;
  var data = msg.data;
  var dataInitialSize = data.byteLength;
  if (!(id in this.anonymousPipes &&
        src.pid in this.anonymousPipes[id].writers)) {
    reply({
      count: this.EPIPE,
    });
    return;
  }
  var pipe = this.anonymousPipes[id];
  while (data.byteLength > 0 && pipe.readsPending.length > 0) {
    var item = pipe.readsPending.shift();
    var part = data.slice(0, item.count);
    item.reply({
      data: part,
    });
    data = data.slice(part.byteLength);
  }
  if (data.byteLength === 0) {
    reply({
      count: dataInitialSize,
    });
  } else {
    if (Object.keys(pipe.readers).length > 0) {
      // TODO(bradnelson): consider limiting slack on buffer here.
      // Using infinite slack for now as this helps git work.
      var replied = false;
      if (1) {
        replied = true;
        reply({
          count: dataInitialSize,
        });
      }
      pipe.writesPending.push({
        dataInitialSize: dataInitialSize,
        data: data,
        reply: reply,
        replied: replied,
        pid: src.pid,
      });
    } else {
      reply({
        count: this.EPIPE,
      });
    }
  }
}

/**
 * Handle an anonymous pipe read call.
 */
PipeServer.prototype.handleMessageAPipeRead = function(
    msg, reply, src) {
  var id = msg.pipe_id;
  var count = msg.count;
  if (count === 0 ||
      !(id in this.anonymousPipes &&
        src.pid in this.anonymousPipes[id].readers)) {
    reply({
      data: new ArrayBuffer(0),
    });
    return;
  }
  var pipe = this.anonymousPipes[id];
  if (pipe.writesPending.length > 0) {
    var item = pipe.writesPending.shift();
    if (item.data.byteLength > count) {
      var part = item.data.slice(0, count);
      reply({
        data: part,
      });
      item.data = item.data.slice(part.byteLength);
      pipe.writesPending.unshift(item);
    } else {
      reply({
        data: item.data,
      });
      if (!item.replied) {
        item.reply({
          count: item.dataInitialSize,
        });
      }
    }
  } else {
    if (Object.keys(pipe.writers).length > 0) {
      pipe.readsPending.push({
        count: count,
        reply: reply,
        pid: src.pid,
      });
    } else {
      reply({
        data: new ArrayBuffer(0),
      });
    }
  }
}

/**
 * Close handle to an anonymous pipe for a process.
 */
PipeServer.prototype.closeAPipe = function(pid, pipeId, writer) {
  if (pipeId in this.anonymousPipes) {
    var pipe = this.anonymousPipes[pipeId];
    if (writer) {
      delete pipe.writers[pid];
    } else {
      delete pipe.readers[pid];
    }
    if (Object.keys(pipe.writers).length === 0 &&
        Object.keys(pipe.readers).length === 0) {
      delete this.anonymousPipes[pipeId];
    } else if (Object.keys(pipe.writers).length === 0) {
      for (var i = 0; i < pipe.readsPending.length; i++) {
        var item = pipe.readsPending[i];
        item.reply({
          data: new ArrayBuffer(0),
        });
      }
      pipe.readsPending = [];
    } else if (Object.keys(pipe.readers).length === 0) {
      for (var i = 0; i < pipe.writesPending.length; i++) {
        var item = pipe.writesPending[i];
        item.reply({
          count: this.EPIPE,
        });
      }
      pipe.writesPending = [];
    }
  }
}

/**
 * Handle an anonymous pipe close call.
 */
PipeServer.prototype.handleMessageAPipeClose = function(
    msg, reply, src) {
  this.closeAPipe(src.pid, msg.pipe_id, msg.writer);
  reply({
    result: 0,
  });
}


/**
 * Add spawned pipe entries.
 * @params {Object} Dictionary of environment variables passed to process.
 */
PipeServer.prototype.addProcessPipes = function(pid, params) {
  var fdCount = 0;
  for (;;fdCount++) {
    var entry = 'NACL_SPAWN_FD_SETUP_' + fdCount;
    if (!(entry in params))
      break;
    // Pull out a a pipe spec of the form:
    // pipe:<fd number>:<pipe-id>:<is-writer>
    var m = params[entry].match(/pipe:([0-9]+):([0-9]+):([0-9]+)/);
    if (m) {
      var fd = parseInt(m[1]);
      var id = parseInt(m[2]);
      var isWriter = parseInt(m[3]);
      if (id in this.anonymousPipes) {
        var pipe = this.anonymousPipes[id];
        if (isWriter) {
          pipe.writers[pid] = null;
        } else {
          pipe.readers[pid] = null;
        }
      }
    }
  }
}


/**
 * Add spawned pipe entries.
 * @params {Object} Dictionary of environment variables passed to process.
 */
PipeServer.prototype.deleteProcess = function(pid) {
  for (var pipeId in this.anonymousPipes) {
    var pipe = this.anonymousPipes[pipeId];
    if (pid in pipe.readers) {
      this.closeAPipe(pid, pipeId, false);
    }
    if (pid in pipe.writers) {
      this.closeAPipe(pid, pipeId, true);
    }
  }
}
