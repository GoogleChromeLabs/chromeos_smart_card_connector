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
 * @fileoverview This file contains a class that can be used to organize a
 * deferred processing of jobs.
 */

goog.provide('GoogleSmartCard.DeferredProcessor');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.log.Logger');
goog.require('goog.structs.Queue');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var suppressUnhandledRejectionError =
    GSC.PromiseHelpers.suppressUnhandledRejectionError;

/**
 * This class can be used to organize a processing of jobs that is deferred
 * until the specified promise is resolved.
 *
 * The jobs (which are essentially simple functions with no input parameters)
 * are executed soon if the awaited promise is already fulfilled, otherwise they
 * are buffered internally and will be executed only after the awaited promise
 * gets fulfilled. The job result is returned through a special promise (see the
 * addJob() method).
 *
 * If the awaited promise is rejected, or if the deferred processor instance is
 * disposed of, then the buffered jobs will be skipped, and their promises will
 * be rejected with an error.
 *
 * The relative order of the jobs is guaranteed to be preserved (i.e. they are
 * processed in the FIFO manner).
 *
 * Note that the class supports nested use cases: the addJob() method is safe to
 * be called from inside another job.
 * @param {!goog.Promise} awaitedPromise
 * @constructor
 * @extends goog.Disposable
 */
GSC.DeferredProcessor = function(awaitedPromise) {
  DeferredProcessor.base(this, 'constructor');

  /**
   * This flag gets true once the awaited promise is resolved, or once the self
   * instance gets disposed (whichever happens first).
   * @type {boolean}
   * @private
   */
  this.isSettled_ = false;

  /**
   * This flag allows to detect whether the execution is nested into a jobs
   * flushing loop upper in the stack.
   * @type {boolean}
   * @private
   */
  this.isCurrentlyFlushingJobs_ = false;

  /**
   * @type {goog.structs.Queue.<Job>}
   * @private
   */
  this.jobsQueue_ = new goog.structs.Queue;

  awaitedPromise.then(
      this.promiseResolvedListener_.bind(this),
      this.promiseRejectedListener_.bind(this));
};

/** @const */
var DeferredProcessor = GSC.DeferredProcessor;

goog.inherits(DeferredProcessor, goog.Disposable);

DeferredProcessor.prototype.logger = GSC.Logging.getScopedLogger(
    'DeferredProcessor');

/**
 * This structure is used to store the jobs in a queue.
 * @param {function()} jobFunction The job function that needs to be executed.
 * @param {!goog.promise.Resolver} promiseResolver The promise (with methods to
 * resolve it) which should be used for returning the job result.
 * @constructor
 * @struct
 */
function Job(jobFunction, promiseResolver) {
  this.jobFunction = jobFunction;
  this.promiseResolver = promiseResolver;
}

/**
 * Adds a new job, whose result will be eventually passed through the returned
 * promise.
 *
 * The job may be executed immediately, if the awaited promise is already
 * fulfilled (though this is not guaranteed to happen immediately). Otherwise,
 * the job will buffered, will be and executed only after the awaited promise
 * gets fulfilled. If the awaited promise gets rejected, or if the deferred
 * processor gets disposed, then the returned promise will be rejected.
 * @param {function()} jobFunction
 * @return {!goog.Promise}
 */
DeferredProcessor.prototype.addJob = function(jobFunction) {
  // Enqueue the job regardless of the state. This allows to deal nicely with
  // the nested cases.
  var promiseResolver = goog.Promise.withResolver();
  suppressUnhandledRejectionError(promiseResolver.promise);
  this.jobsQueue_.enqueue(new Job(jobFunction, promiseResolver));
  if (this.isSettled_)
    this.flushEnqueuedJobs_();
  return promiseResolver.promise;
};

/** @override */
DeferredProcessor.prototype.disposeInternal = function() {
  this.isSettled_ = true;
  this.flushEnqueuedJobs_();

  this.logger.fine('Disposed');

  DeferredProcessor.base(this, 'disposeInternal');
};

/** @private */
DeferredProcessor.prototype.promiseResolvedListener_ = function() {
  if (this.isDisposed())
    return;
  if (this.isSettled_)
    return;
  this.isSettled_ = true;
  this.logger.fine('The awaited promise was resolved');
  this.flushEnqueuedJobs_();
};

/** @private */
DeferredProcessor.prototype.promiseRejectedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.fine('The awaited promise was rejected, disposing...');
  this.dispose();
};

/** @private */
DeferredProcessor.prototype.flushEnqueuedJobs_ = function() {
  GSC.Logging.checkWithLogger(this.logger, this.isSettled_);

  // Use a flag to prevent nested loops of jobs flushing.
  if (this.isCurrentlyFlushingJobs_) {
    // If the flag is already raised, then all jobs will be processed by the
    // flushEnqueuedJobs_() call upper in the stack, so no work has be done
    // here.
    return;
  }
  this.isCurrentlyFlushingJobs_ = true;

  while (!this.jobsQueue_.isEmpty()) {
    var job = this.jobsQueue_.dequeue();
    // Note that it's crucial to check isDisposed() on each loop iteration, as
    // the disposal may happen any time during a job execution.
    if (this.isDisposed()) {
      job.promiseResolver.reject(new Error(
          'Deferred job was skipped: the job processor has been disposed'));
    } else {
      // Run the job inside a try-catch statement in order to treat exceptions
      // during job execution as the job failure. Also this makes sure that the
      // self instance is not left in a broken state.
      /** @preserveTry */
      try {
        job.promiseResolver.resolve(job.jobFunction());
      } catch (exc) {
        job.promiseResolver.reject(exc);
      }
    }
  }

  GSC.Logging.checkWithLogger(this.logger, this.isCurrentlyFlushingJobs_);
  this.isCurrentlyFlushingJobs_ = false;
};

});  // goog.scope
