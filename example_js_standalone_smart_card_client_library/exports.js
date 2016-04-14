/** @license
 * Copyright 2016 Google Inc. All Rights Reserved.
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
 * @fileoverview Exports for the essential symbols.
 *
 * These symbols will be available under their original names even when the
 * compilation level is set to advanced.
 */

goog.exportSymbol('goog.log.getLogger', goog.log.getLogger);
goog.exportSymbol('goog.log.addHandler', goog.log.addHandler);
goog.exportSymbol('goog.log.removeHandler', goog.log.removeHandler);
goog.exportSymbol('goog.log.log', goog.log.log);
goog.exportSymbol('goog.log.error', goog.log.error);
goog.exportSymbol('goog.log.warning', goog.log.warning);
goog.exportSymbol('goog.log.info', goog.log.info);
goog.exportSymbol('goog.log.fine', goog.log.fine);

goog.exportSymbol('goog.log.LogRecord', goog.log.LogRecord);
goog.exportProperty(
    goog.log.LogRecord.prototype, 'reset', goog.log.LogRecord.prototype.reset);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'getLoggerName',
    goog.log.LogRecord.prototype.getLoggerName);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'getException',
    goog.log.LogRecord.prototype.getException);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'setException',
    goog.log.LogRecord.prototype.setException);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'setLoggerName',
    goog.log.LogRecord.prototype.setLoggerName);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'getLevel',
    goog.log.LogRecord.prototype.getLevel);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'setLevel',
    goog.log.LogRecord.prototype.setLevel);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'getMessage',
    goog.log.LogRecord.prototype.getMessage);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'setMessage',
    goog.log.LogRecord.prototype.setMessage);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'getMillis',
    goog.log.LogRecord.prototype.getMillis);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'setMillis',
    goog.log.LogRecord.prototype.setMillis);
goog.exportProperty(
    goog.log.LogRecord.prototype,
    'getSequenceNumber',
    goog.log.LogRecord.prototype.getSequenceNumber);

goog.exportSymbol('goog.log.Level', goog.log.Level);
goog.exportProperty(goog.log.Level, 'OFF', goog.log.Level.OFF);
goog.exportProperty(goog.log.Level, 'SHOUT', goog.log.Level.SHOUT);
goog.exportProperty(goog.log.Level, 'SEVERE', goog.log.Level.SEVERE);
goog.exportProperty(goog.log.Level, 'WARNING', goog.log.Level.WARNING);
goog.exportProperty(goog.log.Level, 'INFO', goog.log.Level.INFO);
goog.exportProperty(goog.log.Level, 'CONFIG', goog.log.Level.CONFIG);
goog.exportProperty(goog.log.Level, 'FINE', goog.log.Level.FINE);
goog.exportProperty(goog.log.Level, 'FINER', goog.log.Level.FINER);
goog.exportProperty(goog.log.Level, 'FINEST', goog.log.Level.FINEST);
goog.exportProperty(goog.log.Level, 'ALL', goog.log.Level.ALL);
goog.exportProperty(
    goog.log.Level, 'PREDEFINED_LEVELS', goog.log.Level.PREDEFINED_LEVELS);
goog.exportProperty(
    goog.log.Level, 'getPredefinedLevel', goog.log.Level.getPredefinedLevel);
goog.exportProperty(
    goog.log.Level,
    'getPredefinedLevelByValue',
    goog.log.Level.getPredefinedLevelByValue);
goog.exportProperty(
    goog.log.Level, 'getPredefinedLevel', goog.log.Level.getPredefinedLevel);
goog.exportProperty(
    goog.log.Level, 'getPredefinedLevel', goog.log.Level.getPredefinedLevel);
goog.exportProperty(
    goog.log.Level.prototype, 'toString', goog.log.Level.prototype.toString);

goog.exportSymbol('goog.log.Logger', goog.log.Logger);
goog.exportProperty(
    goog.log.Logger, 'ROOT_LOGGER_NAME', goog.log.Logger.ROOT_LOGGER_NAME);
goog.exportProperty(goog.log.Logger, 'Level', goog.log.Logger.Level);
goog.exportProperty(goog.log.Logger, 'getLogger', goog.log.Logger.getLogger);
goog.exportProperty(
    goog.log.Logger, 'logToProfilers', goog.log.Logger.logToProfilers);
goog.exportProperty(
    goog.log.Logger.prototype, 'getName', goog.log.Logger.prototype.getName);
goog.exportProperty(
    goog.log.Logger.prototype,
    'addHandler',
    goog.log.Logger.prototype.addHandler);
goog.exportProperty(
    goog.log.Logger.prototype,
    'removeHandler',
    goog.log.Logger.prototype.removeHandler);
goog.exportProperty(
    goog.log.Logger.prototype,
    'getParent',
    goog.log.Logger.prototype.getParent);
goog.exportProperty(
    goog.log.Logger.prototype,
    'getChildren',
    goog.log.Logger.prototype.getChildren);
goog.exportProperty(
    goog.log.Logger.prototype, 'setLevel', goog.log.Logger.prototype.setLevel);
goog.exportProperty(
    goog.log.Logger.prototype, 'getLevel', goog.log.Logger.prototype.getLevel);
goog.exportProperty(
    goog.log.Logger.prototype,
    'getEffectiveLevel',
    goog.log.Logger.prototype.getEffectiveLevel);
goog.exportProperty(
    goog.log.Logger.prototype,
    'isLoggable',
    goog.log.Logger.prototype.isLoggable);
goog.exportProperty(
    goog.log.Logger.prototype, 'log', goog.log.Logger.prototype.log);
goog.exportProperty(
    goog.log.Logger.prototype,
    'getLogRecord',
    goog.log.Logger.prototype.getLogRecord);
goog.exportProperty(
    goog.log.Logger.prototype, 'shout', goog.log.Logger.prototype.shout);
goog.exportProperty(
    goog.log.Logger.prototype, 'severe', goog.log.Logger.prototype.severe);
goog.exportProperty(
    goog.log.Logger.prototype, 'warning', goog.log.Logger.prototype.warning);
goog.exportProperty(
    goog.log.Logger.prototype, 'info', goog.log.Logger.prototype.info);
goog.exportProperty(
    goog.log.Logger.prototype, 'config', goog.log.Logger.prototype.config);
goog.exportProperty(
    goog.log.Logger.prototype, 'fine', goog.log.Logger.prototype.fine);
goog.exportProperty(
    goog.log.Logger.prototype, 'finer', goog.log.Logger.prototype.finer);
goog.exportProperty(
    goog.log.Logger.prototype, 'finest', goog.log.Logger.prototype.finest);
goog.exportProperty(
    goog.log.Logger.prototype,
    'logRecord',
    goog.log.Logger.prototype.logRecord);

goog.exportSymbol('goog.Promise', goog.Promise);
goog.exportProperty(goog.Promise, 'resolve', goog.Promise.resolve);
goog.exportProperty(goog.Promise, 'reject', goog.Promise.reject);
goog.exportProperty(goog.Promise, 'race', goog.Promise.race);
goog.exportProperty(goog.Promise, 'all', goog.Promise.all);
goog.exportProperty(goog.Promise, 'allSettled', goog.Promise.allSettled);
goog.exportProperty(
    goog.Promise, 'firstFulfilled', goog.Promise.firstFulfilled);
goog.exportProperty(goog.Promise, 'withResolver', goog.Promise.withResolver);
goog.exportProperty(
    goog.Promise,
    'setUnhandledRejectionHandler',
    goog.Promise.setUnhandledRejectionHandler);
goog.exportProperty(
    goog.Promise, 'CancellationError', goog.Promise.CancellationError);
goog.exportProperty(
    goog.Promise.prototype, 'then', goog.Promise.prototype.then);
goog.exportProperty(
    goog.Promise.prototype, 'thenAlways', goog.Promise.prototype.thenAlways);
goog.exportProperty(
    goog.Promise.prototype, 'thenCatch', goog.Promise.prototype.thenCatch);
goog.exportProperty(
    goog.Promise.prototype, 'cancel', goog.Promise.prototype.cancel);
goog.exportProperty(
    goog.Promise.prototype, 'thenAlways', goog.Promise.prototype.thenAlways);

goog.exportSymbol('goog.Disposable', goog.Disposable);
goog.exportProperty(
    goog.Disposable.prototype,
    'isDisposed',
    goog.Disposable.prototype.isDisposed);
goog.exportProperty(
    goog.Disposable.prototype, 'dispose', goog.Disposable.prototype.dispose);
goog.exportProperty(
    goog.Disposable.prototype,
    'addOnDisposeCallback',
    goog.Disposable.prototype.addOnDisposeCallback);

goog.exportSymbol(
    'goog.messaging.AbstractChannel', goog.messaging.AbstractChannel);
goog.exportProperty(
    goog.messaging.AbstractChannel.prototype,
    'connect',
    goog.messaging.AbstractChannel.prototype.connect);
goog.exportProperty(
    goog.messaging.AbstractChannel.prototype,
    'isConnected',
    goog.messaging.AbstractChannel.prototype.isConnected);
goog.exportProperty(
    goog.messaging.AbstractChannel.prototype,
    'registerService',
    goog.messaging.AbstractChannel.prototype.registerService);
goog.exportProperty(
    goog.messaging.AbstractChannel.prototype,
    'registerDefaultService',
    goog.messaging.AbstractChannel.prototype.registerDefaultService);
goog.exportProperty(
    goog.messaging.AbstractChannel.prototype,
    'send',
    goog.messaging.AbstractChannel.prototype.send);
