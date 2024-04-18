/**
 * @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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
 * @fileoverview This file contains helper definitions that can be used for
 * loading and interacting with an Emscripten/WebAssembly module.
 */

goog.provide('GoogleSmartCard.EmscriptenModule');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.DelayedMessageChannel');
goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.html.TrustedResourceUrl');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.net.jsloader');
goog.require('goog.promise.Resolver');
goog.require('goog.string.Const');

goog.scope(function() {

const GSC = GoogleSmartCard;

const LOGGER_SCOPE = 'EmscriptenModule';
const WRAPPER_SUBLOGGER_SCOPE = 'Wrapper';

/**
 * Class that allows to load and run the Emscripten module with the given name
 * and exchange messages with it.
 * @constructor
 * @extends GSC.ExecutableModule
 */
GSC.EmscriptenModule = class extends GSC.ExecutableModule {
  constructor(moduleName) {
    super();
    /** @type {string} @const @private */
    this.moduleName_ = moduleName;
    /** @type {!goog.log.Logger} @const @private */
    this.fromModuleMessagesLogger_ = GSC.Logging.getScopedLogger(LOGGER_SCOPE);
    /** @type {!goog.log.Logger} @const @private */
    this.logger_ = GSC.Logging.getChildLogger(
        this.fromModuleMessagesLogger_, WRAPPER_SUBLOGGER_SCOPE);
    /** @type {!goog.promise.Resolver<void>} @const @private */
    this.loadPromiseResolver_ = goog.Promise.withResolver();
    GSC.PromiseHelpers.suppressUnhandledRejectionError(
        this.loadPromiseResolver_.promise);
    /** @type {!EmscriptenModuleMessageChannel} @const @private */
    this.realMessageChannel_ = new EmscriptenModuleMessageChannel();
    /** @type {!GSC.DelayedMessageChannel} @const @private */
    this.delayedMessageChannel_ = new GSC.DelayedMessageChannel();
    this.delayedMessageChannel_.setUnderlyingChannel(this.realMessageChannel_);
    // The "Module" object exposed by the Emscripten framework.
    /** @type {!Object|null} @private */
    this.emscriptenApiModule_ = null;
    // Object that is an entry point on the C++ side and is used for exchanging
    // messages with it. Untyped, since the class "GoogleSmartCardModule" is
    // defined within the Emscripten module (using Embind) and therefore isn't
    // known to Closure Compiler.
    /** @type {?} @private */
    this.googleSmartCardModule_ = null;
  }

  /** @override */
  getLogger() {
    return this.logger_;
  }

  /** @override */
  startLoading() {
    this.load_().then(
        () => {
          this.loadPromiseResolver_.resolve();
        },
        (e) => {
          goog.log.warning(
              this.logger_, 'Failed to load the Emscripten module: ' + e);
          this.dispose();
          this.loadPromiseResolver_.reject(e);
        });
  }

  /** @override */
  getLoadPromise() {
    return this.loadPromiseResolver_.promise;
  }

  /** @override */
  getMessageChannel() {
    return this.delayedMessageChannel_;
  }

  /** @override */
  disposeInternal() {
    goog.log.fine(this.logger_, 'Disposed');
    if (this.googleSmartCardModule_) {
      // Call `delete()` on the C++ object before dropping the reference on it,
      // in order to make the C++ destructor called. Note: The method is
      // accessed using the square bracket notation, to make sure that Closure
      // Compiler doesn't rename this method call.
      this.googleSmartCardModule_['delete']();
      delete this.googleSmartCardModule_;
    }
    if (this.emscriptenApiModule_) {
      // Terminate threads and worker pools that Emscripten may have reserved.
      // Note that the "pthread_terminateAllThreads" property is created by the
      // code injected from emscripten-module-js-epilog.js.inc and is just an
      // alias to `Module.PThread.terminateAllThreads()`. The alias is needed to
      // work around renamings that happen in the Release mode.
      this.emscriptenApiModule_['pthread_terminateAllThreads']();
      delete this.emscriptenApiModule_;
    }
    this.delayedMessageChannel_.dispose();
    this.realMessageChannel_.dispose();
    super.disposeInternal();
  }

  /**
   * Asynchronously loads and executes the Emscripten module.
   * @return {!Promise<void>}
   * @private
   */
  async load_() {
    // First step: Asynchronously load the JS file containing the runtime
    // support code autogenerated by Emscripten. By convention (see build rules
    // in
    // //common/make/internal/executable_building_emscripten.mk), the file's
    // base name is the module name.
    const jsUrl = goog.html.TrustedResourceUrl.format(
        goog.string.Const.from('/%{moduleName}.js'),
        {'moduleName': this.moduleName_});
    await goog.net.jsloader.safeLoad(jsUrl);

    // Second step: Run the factory function that asynchronously loads the
    // Emscripten module and creates the Emscripten API's Module object. By
    // convention (see the EXPORT_NAME parameter in
    // //common/make/internal/executable_building_emscripten.mk), the function
    // has a specific name based on the module name.
    const factoryFunction =
        goog.global[`loadEmscriptenModule_${this.moduleName_}`];
    GSC.Logging.checkWithLogger(
        this.logger_, factoryFunction,
        'Emscripten factory function not defined');
    this.emscriptenApiModule_ =
        await factoryFunction(this.getEmscriptenApiModuleSettings_());

    // Third step: Create the object that serves as an entry point on the C++
    // side and is used for exchanging messages with it. By convention (see the
    // entry_point_emscripten.cc files), the class is named
    // GoogleSmartCardModule.
    const GoogleSmartCardModule =
        this.emscriptenApiModule_['GoogleSmartCardModule'];
    GSC.Logging.checkWithLogger(
        this.logger_, GoogleSmartCardModule,
        'GoogleSmartCardModule class not defined');
    this.googleSmartCardModule_ = new GoogleSmartCardModule((message) => {
      this.realMessageChannel_.onMessageFromModule(message);
    });

    // Fourth step: Wire up outgoing messages with the module.
    // The method name is again a hardcoded convention (see the
    // entry_point_emscripten.cc files).
    // The method is accessed using the square bracket notation, to make sure
    // that Closure Compiler doesn't rename this method call.
    this.realMessageChannel_.setMessageSender(
        (message) => this.googleSmartCardModule_['postMessage'](message));
    this.delayedMessageChannel_.setReady();
  }

  /**
   * Constructs the settings object for the Emscripten module instance.
   * @return {!Object}
   * @private
   */
  getEmscriptenApiModuleSettings_() {
    // Note: If you add a new key here don't forget to include it into the
    // "INCOMING_MODULE_JS_API" build-time setting in
    // executable_building_emscripten.mk.
    return {
      'onAbort': () => {
        this.onModuleCrashed_();
      },
      'print': (message) => {
        this.onModuleStdoutReceived_(message);
      },
      'printErr': (message) => {
        this.onModuleStderrReceived_(message);
      },
    };
  }

  /**
   * Handles the Emscripten module's crash.
   * @private
   */
  onModuleCrashed_() {
    goog.log.error(this.logger_, 'The Emscripten module crashed');

    // Drop the "module" object reference, because trying to call its destructor
    // in `disposeInternal()` would likely cause new exceptions.
    this.googleSmartCardModule_ = null;

    this.dispose();
  }

  /**
   * Handles an stdout message received from the Emscripten module.
   * @param {string} message
   * @private
   */
  onModuleStdoutReceived_(message) {
    // TODO(#185): Consider downgrading to the "fine" level once structured log
    // messages are received from the module.
    goog.log.info(this.fromModuleMessagesLogger_, message);
  }

  /**
   * Handles an stderr message received from the Emscripten module.
   * @param {string} message
   * @private
   */
  onModuleStderrReceived_(message) {
    // TODO(#185): Consider downgrading to the "fine" level once structured log
    // messages are received from the module.
    goog.log.info(this.fromModuleMessagesLogger_, message);
  }
};

/**
 * Sends and receives messages to/from the Emscripten module.
 *
 * Can be instantiated in advance, to be wired up via `setMessageSender()`
 * and `onMessageFromModule()` once the module is loaded, however `send()`
 * cannot be called until that point.
 * @package
 */
class EmscriptenModuleMessageChannel extends goog.messaging.AbstractChannel {
  constructor() {
    super();
    /** @type {function(!Object)|null} @private */
    this.messageSender_ = null;
  }

  /**
   * @param {function(!Object)} messageSender
   * @package
   */
  setMessageSender(messageSender) {
    GSC.Logging.check(messageSender);
    GSC.Logging.check(!this.messageSender_);
    this.messageSender_ = messageSender;
  }

  /** @override */
  send(serviceName, payload) {
    GSC.Logging.check(goog.isObject(payload));
    goog.asserts.assertObject(payload);
    const typedMessage = new GSC.TypedMessage(serviceName, payload);
    const message = typedMessage.makeMessage();
    if (this.isDisposed())
      return;
    GSC.Logging.check(this.messageSender_);
    this.messageSender_(message);
  }

  /** @override */
  disposeInternal() {
    delete this.messageSender_;
    super.disposeInternal();
  }

  /**
   * @param {?} message
   * @package
   */
  onMessageFromModule(message) {
    if (this.isDisposed()) {
      return;
    }
    const typedMessage = GSC.TypedMessage.parseTypedMessage(message);
    if (!typedMessage) {
      GSC.Logging.fail(
          'Failed to parse message received from Emscripten module: ' +
          GSC.DebugDump.debugDumpSanitized(message));
    }
    this.deliver(typedMessage.type, typedMessage.data);
  }
}
});  // goog.scope
