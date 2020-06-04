/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// Utilities to allow googletest style tests of apps / extensions.


/**
 * @namespace.
 */
var chrometest = {};

/**
 * @private
 */
chrometest.passed_ = null;
chrometest.currentTest_ = null;
chrometest.currentTestName_ = null;
chrometest.startTime_ = null;
chrometest.finishTest_ = null;
chrometest.tests_ = [];

/**
 * @private
 * @constant
 */
chrometest.ERROR = 'ERROR';
chrometest.WARNING = 'WARNING';
chrometest.INFO = 'INFO';
chrometest.DEBUG = 'DEBUG';


/**
 * Get the decoded query parameters passed to the current page.
 * @return {Object.<string>}.
 */
chrometest.getUrlParameters = function() {
  var items = {};
  if (window.location.search.length < 1) {
    return items;
  }
  var fields = window.location.search.slice(1).split('&');
  for (var i = 0; i < fields.length; i++) {
    var parts = fields[i].split('=');
    items[decodeURIComponent(parts[0])] = decodeURIComponent(parts[1]);
  }
  return items;
};

/**
 * Create a new messaging port to communicate with the testing extension.
 * @return {PortWaiter} A new Port to the testing extension wrapped with
 *     PortWaiter.
 */
chrometest.newTestPort = function() {
  // Pull the id out of: 'ChromeUserAgent/<ID> Chrome/<Ver>'
  var extensionId = navigator.userAgent.split(' ')[0].split('/')[1];
  return new chrometest.PortWaiter(chrome.runtime.connect(extensionId));
};

/**
 * Kill the browser (to end the testing session).
 * @return {Promise} A promise to halt (which will never be resolved because
 *     the browser will be halted by then).
 */
chrometest.haltBrowser = function() {
  var port = chrometest.newTestPort();
  port.postMessage({'name': 'haltBrowser'});
  // Wait for a reply that will never come.
  return port.wait();
};

/**
 * Reset the connection in the testing extension.
 * @returns {Promise.integer} A promise for the number of connections killed.
 */
chrometest.resetExtension = function() {
  var port = chrometest.newTestPort();
  var count = null;
  port.postMessage({'name': 'reset'});
  return port.wait().then(function(msg) {
    ASSERT_EQ('resetReply', msg.name);
    port.disconnect();
    return msg.count;
  });
}

/**
 * Get a list of all loaded extensions.
 *
 * This exposes the result of chrome.management.getAll for use by tests.
 * @returns {Promise.Array.<ExtensionInfo>}.
 */
chrometest.getAllExtensions = function() {
  var port = chrometest.newTestPort();
  port.postMessage({'name': 'getAllExtensions'});
  return port.wait().then(function(msg) {
    ASSERT_EQ('getAllExtensionsResult', msg.name);
    port.disconnect();
    return msg.result;
  });
};

/**
 * Get a mapping of process id to process info for all processes running.
 *
 * This exposes the result of chrome.processes.getProcessInfo for use by tests.
 * @return {Promise.Object.<ProcessInfo>}.
 */
chrometest.getAllProcesses = function() {
  var port = chrometest.newTestPort();
  port.postMessage({'name': 'getAllProcesses'});
  return port.wait().then(function(msg) {
    ASSERT_EQ('getAllProcessesResult', msg.name);
    port.disconnect();
    return msg.result;
  });
};

/**
 * Create a messaging port to communicate with an extension by name.
 *
 * Ordinarily web pages can only communicate with extensions that have
 * explicitly ask for permission in their manifests. However, extensions can
 * communicate with each other without this, but should endeavor to verify that
 * they only communicate with trusted peers. The testing extension should be
 * whitelisted by the extensions under test when in testing mode. This allows
 * the testing extension to offer web pages proxied access to extensions under
 * test without modification.
 * @returns {Promise.PortWaiter} A promise for a PortWaiter to communicate with
 *     the extension on.
 */
chrometest.proxyExtension = function(extensionName) {
  var port = chrometest.newTestPort();
  port.postMessage({'name': 'proxy', 'extension': extensionName});
  return port.wait().then(function(msg) {
    ASSERT_EQ('proxyReply', msg.name, 'expect proxy reply');
    ASSERT_TRUE(
      msg.success, 'should find one extension: ' + extensionName +
      ' found ' + msg.matchCount);
    return port;
  });
};

/**
 * Get an URL that references the test harness.
 * @param {string} path The relative path to a resource hosted by the harness.
 * @return {string} The absolute URL.
 */
chrometest.harnessURL = function(path) {
  var baseURL = location.href.split('/').slice(0, -1).join('/');
  return baseURL + '/' + path;
};

/**
 * Log a message to the test harness.
 * @param {string} level The python logging level of the message.
 * @param {string} message The message to log.
 * @return {Promise} A promise to log it (or rejects with error code).
 */
chrometest.log = function(level, message) {
  // Cap the log line limit.
  var logLimit = 1024;
  var rest = message.substr(logLimit);
  message = message.substr(0, logLimit);
  console.log(level + ': ' + message);
  return chrometest.httpGet(
      '/_command?log=' + encodeURIComponent(message) +
      '&level=' + encodeURIComponent(level)).then(function(result) {
    if (rest.length > 0) {
      // Log the rest if any.
      chrometest.log(level, rest);
    }
  });
};

/**
 * Log an error message.
 * @param {string} message The message to log.
 * @return {Promise} A promise to do it.
 */
chrometest.error = function(message) {
  return chrometest.log(chrometest.ERROR, message);
};

/**
 * Log a warning message.
 * @param {string} message The message to log.
 * @return {Promise} A promise to do it.
 */
chrometest.warning = function(message) {
  return chrometest.log(chrometest.WARNING, message);
};

/**
 * Log an info message.
 * @param {string} message The message to log.
 * @return {Promise} A promise to do it.
 */
chrometest.info = function(message) {
  return chrometest.log(chrometest.INFO, message);
};

/**
 * Log a debug message.
 * @param {string} message The message to log.
 * @return {Promise} A promise to do it.
 */
chrometest.debug = function(message) {
  return chrometest.log(chrometest.DEBUG, message);
};

/**
 * Perform an HTTP GET.
 * @param {string} url The URL to fetch.
 * @return {Promise.string,integer} A promise for the text at the url on
 *     resolve or an integer with the error code on reject.
 */
chrometest.httpGet = function(url) {
  return new Promise(function(resolve, reject) {
    var r = new XMLHttpRequest();
    r.open('GET', url, false);
    r.onload = function() {
      if (r.readyState == 4) {
        if (r.status == 200) {
          resolve(r.responseText);
        } else {
          reject(r.status);
        }
      }
    }
    r.send();
  });
};

/**
 * Sleep for a duration.
 * @param {float} ms Timeout in milliseconds.
 * @return {Promise} A promise to wait.
 */
chrometest.sleep = function(ms) {
  return new Promise(function(resolve, reject) {
    setTimeout(function() {
      resolve();
    }, ms);
  });
};

/**
 * Format a time in milliseconds to XXms or YYs as appropriate.
 * @param {float} ms Time in milliseconds.
 * @return {string} A formatted time.
 */
chrometest.formatDuration = function(ms) {
  if (ms < 1000.0) {
    return ms + 'ms';
  } else {
    return (ms / 1000.0).toFixed(1) + 's';
  }
};

/**
 * Tell the test harness how many test runs to expect.
 * @param {integer} testCount The number of tests to expect.
 * @return {Promise} A promise to do it.
 */
chrometest.reportTestCount_ = function(testCount) {
  console.log('About to run ' + testCount + ' tests.');
  return chrometest.httpGet('/_command?test_count=' + testCount);
};

/**
 * Notify the test harness that a test has begun.
 * @param {string} name The full name of the test.
 * @return {Promise} A promise to do it.
 */
chrometest.beginTest_ = function(name) {
  return chrometest.resetExtension().then(function(count) {
    if (count != 0) {
      throw new Error(
          'Test extension connections from the last test remain active!');
    }
    console.log('[ RUN      ] ' + name);
    chrometest.passed_ = true;
    chrometest.currentTestName_ = name;
    return chrometest.httpGet(
        '/_command?name=' + encodeURIComponent(name) +
        '&start=1');
  }).then(function(result) {
    chrometest.startTime_ = new Date();
  });
};

/**
 * Notify the test harness that a test has ended.
 * @return {Promise} A promise to do it.
 */
chrometest.endTest_ = function() {
  return chrometest.resetExtension().then(function(count) {
    EXPECT_EQ(0, count,
              'all connection to the test extension should be closed');
    var endTime = new Date();
    var duration = endTime.getTime() - chrometest.startTime_.getTime();
    duration = chrometest.formatDuration(duration);
    var name = chrometest.currentTestName_;
    if (chrometest.passed_) {
      var resultMsg = '      OK';
      var result = 1;
    } else {
      var resultMsg = ' FAILED ';
      var result = 0;
    }
    console.log('[ ' + resultMsg + ' ] ' + name + ' (' + duration + ')');
    chrometest.startTime_ = null;
    chrometest.currentTest_ = null;
    chrometest.currentTestName_ = null;
    return chrometest.httpGet(
        '/_command?name=' + encodeURIComponent(name) + '&' +
        'duration=' + encodeURIComponent(duration) + '&' +
        'result=' + result);
  });
};

/**
 * Mark current test as failed.
 */
chrometest.fail = function() {
  chrometest.passed_ = false;
};

/**
 * Format an error object as a string.
 * Error objects use their stack trace.
 * @param {?} error A thrown value.
 */
chrometest.formatError = function(error) {
  if (error === undefined || error.stack === undefined) {
    return '' + error;
  } else {
    return error.stack;
  }
};


/**
 * Assert that something must be true to continue the current test.
 *
 * This halts the current test by throwing an exception.
 * Unfortunately, this has the danger that it may not actually halt the test.
 * Ideally, any exception handling in the test itself should be done very
 * carefully to ensure it passes along 'assert' exceptions.
 * If the code under test eats the exception, at least the test will be marked
 * as failed. If the exception causes the code under test to wait indefinitely,
 * the timeout in the testing harness will eventually bring everything down.
 *
 * Halts the current test if the condition is not true.
 * @param {boolean} condition A condition to check.
 * @param {string} description A description of the context in which the
 *                             condition is being checked (to help
 *                             label / find it).
 */
chrometest.assert = function(condition, description) {
  if (!condition) {
    chrometest.fail();
    if (description === undefined) {
      description = 'no description';
    }
    var error = new Error('ASSERT FAILED! - ' + description);
    chrometest.error(chrometest.formatError(error)).then(function() {
      throw 'assert';
    });
  }
};

/**
 * Declare that something must be true for the current test to pass.
 *
 * Does not halt the current test if the condition is false, but does emit
 * information on the failure location and mark the test as failed.
 * @param {boolean} condition A condition to check.
 * @param {string} description A description of the context in which the
 *                             condition is being checked (to help
 *                             label / find it).
 */
chrometest.expect = function(condition, description) {
  if (!condition) {
    chrometest.fail();
    if (description === undefined) {
      description = 'no description';
    }
    var error = new Error('EXPECT FAILED! - ' + description);
    chrometest.error(chrometest.formatError(error));
  }
};

/**
 * Run a list of tests.
 * param {Array.<Test>} testList The list of tests to run.
 * @return {Promise} A promise to do it.
 */
chrometest.runTests_ = function(testList) {
  var p = Promise.resolve();
  testList.forEach(function(test) {
    p = p.then(function() {
      return test.call();
    });
  });
  return p;
};

/**
 * Check if a string matches a wildcard string.
 * @param string filter A wildcard string (* - any string, ? - one char).
 * @param string s A string to match.
 */
chrometest.wildcardMatch = function(filter, s) {
  filter = filter.replace(/[.]/g, '[.]');
  filter = filter.replace(/\*/g, '.*');
  filter = filter.replace(/\?/g, '.');
  filter = '^' + filter + '$';
  var re = new RegExp(filter);
  return re.test(s);
};

/**
 * Check if a string matches a googletest style filter.
 * A filter consists of zero or more ':' separated positive wildcard
 * strings, followed optionally by a '-' and zero or more ':' separated
 * negative wildcard strings.
 * @param string filter A googletest style filter string.
 * @param string s A string to match.
 */
chrometest.filterMatch = function(filter, s) {
  var parts = filter.split('-');
  if (parts.length == 1) {
    var positive = parts[0].split(':');
    var negative = [];
  } else if (parts.length == 2) {
    var positive = parts[0].split(':');
    var negative = parts[1].split(':');
  } else {
    // Treat ill-formated filters as non-matches.
    return false;
  }
  if (positive.length == 1 && positive[0] == '') {
    positive = ['*'];
  }
  if (negative.length == 1 && negative[0] == '') {
    negative = [];
  }
  for (var i = 0; i < positive.length; i++) {
    if (!chrometest.wildcardMatch(positive[i], s)) {
      return false;
    }
  }
  for (var i = 0; i < negative.length; i++) {
    if (chrometest.wildcardMatch(negative[i], s)) {
      return false;
    }
  }
  return true;
};

/**
 * Filter tests based on harness filter.
 * @returns {Promose} A promise to do it.
 */
chrometest.filterTests_ = function() {
  return chrometest.httpGet('/_command?filter=1').then(function(filter) {
    var keep = [];
    var tests = chrometest.tests_;
    for (var i = 0; i < tests.length; i++) {
      if (chrometest.filterMatch(filter, tests[i].name)) {
        keep.push(tests[i]);
      }
    }
    chrometest.tests_ = keep;
  }).catch(function(responseCode) {
    throw new Error(
        'Requesting filter from test harness failed! (code: ' +
        responseCode + ')');
  });
};

/**
 * Report the test count and run all register tests and halt the browser.
 * @return {Promise} A promise to do it.
 */
chrometest.runAllTests_ = function() {
  return Promise.resolve().then(function() {
    return chrometest.filterTests_();
  }).then(function() {
    // Sleep 100ms before starting the tests as extensions may not load
    // simultaneously.
    return chrometest.sleep(100);
  }).then(function() {
    return chrometest.reportTestCount_(chrometest.tests_.length);
  }).then(function() {
    return chrometest.runTests_(chrometest.tests_);
  }).catch(function(error) {
    chrometest.fail();
    return chrometest.error(chrometest.formatError(error));
  }).then(function() {
    return chrometest.haltBrowser();
  });
};

/**
 * Load a javascript module.
 * @param {string} filename Filename to load.
 * @return {Promise} A promise to load the module.
 */
chrometest.load = function(filename) {
  return new Promise(function(resolve, reject) {
    // Register a window wide handler just in case (things leak thru).
    window.onerror = function(
        errorMsg, url, lineNumber, columnNumber, error) {
      chrometest.fail();
      chrometest.error(
          errorMsg + ' in ' + url + ' at ' +
          lineNumber + ':' + columnNumber + '\n' +
          chrometest.formatError(error)).then(function() {
        reject(chrometest.haltBrowser());
      });
    };

    var script = document.createElement('script');
    script.src = filename;
    script.onerror = function(e) {
      chrometest.error(
         'Error loading ' + e.target.src + '\n').then(function() {
        reject(chrometest.haltBrowser());
      });
    };
    script.onload = function() {
      resolve();
    };
    document.body.appendChild(script);
  });
};

/**
 * Load a list of javascript files into script tags.
 * @param {Array.<string>} sources A list of javascript files to load tests
 *                                 from.
 */
chrometest.run = function(sources) {
  var p = Promise.resolve();
  sources.forEach(function(filename) {
    p = p.then(function() {
      return chrometest.load(filename);
    });
  });
  return p.then(function() {
    return chrometest.runAllTests_();
  });
};


/**
 * An class that monitors an object that behaves like a messaging Port or an
 * event listener, allowing Promise yielding waits.
 * Descendants or wrappers will want to perform port type specific setup and
 * tear down.
 * @constructor
 * @param {function()} tearDown A function called to detach any handles
 *     associated with the port.
 * @param {Object} port An object that implements postMessage.
 */
chrometest.PortlikeWaiter = function(tearDown, port) {
  var self = this;
  self.port_ = port;
  self.messages_ = [];
  self.waiter_ = null;
  self.tearDown_ = tearDown;
};

/**
 * Enqueue a message to any waiter.
 * @param {Promise.Object} A Promise for a message.
 */
chrometest.PortlikeWaiter.prototype.enqueue = function(msg) {
  var self = this;
  if (self.waiter_ !== null) {
    self.waiter_(msg);
  } else {
    self.messages_.push(msg);
  }
};

/**
 * Wait a message.
 * @return {Promise.Object} A promise for a message.
 */
chrometest.PortlikeWaiter.prototype.wait = function() {
  var self = this;
  return new Promise(function(resolve) {
    if (self.messages_.length > 0) {
      var msg = self.messages_.shift();
      resolve(msg);
    } else {
      if (self.waiter_ !== null) {
        throw new Error('Multiple waiters on a PortlikeWaiter!');
      }
      self.waiter_ = function(msg) {
        self.waiter_ = null;
        resolve(msg);
      };
    }
  });
};

/**
 * Post a message to the port object associated with this waiter.
 */
chrometest.PortlikeWaiter.prototype.postMessage = function() {
  this.port_.postMessage.apply(this.port_, arguments);
};

/**
 * Detach the port object wrapper by this waiter for use.
 * @return {Object} The port like object managed by this object.
 */
chrometest.PortlikeWaiter.prototype.detach = function() {
  var self = this;
  var port = self.port_;
  self.port_ = null;
  self.messages_ = null;
  self.waiter_ = null;
  if (self.tearDown_) {
    var tearDown = self.tearDown_;
    self.tearDown_ = null;
    tearDown();
  }
  return port;
};


/**
 * An object that monitors a messaging port and doles out promises.
 * Takes ownership of the port. Calls to postMessage and disconnect on the port
 * should be down to the waiter instead.
 * Detach can be used to release the underlying Port.
 * @constructor
 * @param {Port} port The port to monitor.
 */
chrometest.PortWaiter = function(port) {
  var self = this;
  function handleMessage(msg) {
    self.enqueue(Promise.resolve(msg));
  }
  function handleDisconnect() {
    self.enqueue(Promise.reject());
    self.detach();
  }
  chrometest.PortlikeWaiter.call(self, function() {
    port.onMessage.removeListener(handleMessage);
    port.onDisconnect.removeListener(handleDisconnect);
  }, port);
  self.port_.onMessage.addListener(handleMessage);
  self.port_.onDisconnect.addListener(handleDisconnect);
};
chrometest.PortWaiter.prototype = new chrometest.PortlikeWaiter();

/**
 * Disconnect the Port object wrapped by this waiter.
 * @param {Object} msg Message to send.
 */
chrometest.PortWaiter.prototype.disconnect = function() {
  var self = this;
  var port = self.detach();
  if (port !== null) {
    port.disconnect();
  }
};


/**
 * A test case.
 * @constructor
 */
chrometest.Test = function() {
};

/**
 * The default setUp method for a test case (does nothing).
 * @return {Promise/void} Optionally return a promise to set up.
 */
chrometest.Test.prototype.setUp = function() {
};

/**
 * The default tearDown method for a test case (does nothing).
 * @return {Promise/void} Optionally return a promise to tear down.
 */
chrometest.Test.prototype.tearDown = function() {
};


// Below this point functions are declare at global scope and use a naming
// convention that matches googletest. This done for several reasons:
//   - A global name makes use through multiple tests convenient.
//   - Using an existing naming convention makes use and intent clear.
//   - ALL CAPS highlights the testing constructs visually.


// TEST Types
// ----------

/**
 * Register a test case using a fixture class.
 * @param {string} fixtureClass The test fixture class object.
 * @param {string} testName The name of the test.
 * @param {function()} testFunc Called to run the test, may return
 *                              a Promise.
 * @param {string} opt_caseName Optional name for the case, otherwise the
 *     fixtureClass class name is used.
 */
function TEST_F(fixtureClass, testName, testFunc, opt_caseName) {
  if (opt_caseName == undefined) {
    opt_caseName = fixtureClass.name;
  }
  var fullName = opt_caseName + '.' + testName;
  chrometest.tests_.push({
    'name': fullName,
    'call': function() {
      return Promise.resolve().then(function() {
        return chrometest.beginTest_(fullName);
      }).then(function() {
        chrometest.currentTest_ = new fixtureClass();
        return Promise.resolve().then(function() {
          return chrometest.currentTest_.setUp();
        }).then(function() {
          return Promise.resolve().then(function() {
            return testFunc.call(chrometest.currentTest_);
          }).catch(function(error) {
            chrometest.fail();
            return chrometest.error(chrometest.formatError(error));
          });
        }).then(function() {
          return chrometest.currentTest_.tearDown();
        }).catch(function(error) {
          chrometest.fail();
          return chrometest.error(chrometest.formatError(error));
        });
      }).then(function() {
        return chrometest.endTest_();
      });
    },
  });
}

/**
 * Register a single test.
 * @param {string} testCase A test case name in lieu of a fixture.
 * @param {string} testName The name of the test.
 * @param {function()} testFunc Called to run the test, may return
 *                              a Promise.
 */
function TEST(testCase, testName, testFunc) {
  TEST_F(chrometest.Test, testName, testFunc, testCase);
}


// ASSERT VARIANTS
// ---------------

function ASSERT_EQ(expected, actual, context) {
  expected = JSON.stringify(expected);
  actual = JSON.stringify(actual);
  chrometest.assert(expected == actual, 'Expected ' + expected + ' but got ' +
                    JSON.stringify(actual) + ' when ' +
                    JSON.stringify(context));
}

function ASSERT_NE(expected, actual, context) {
  expected = JSON.stringify(expected);
  actual = JSON.stringify(actual);
  chrometest.assert(expected != actual, 'Did not expect ' + expected +
                    ' but got ' + actual + ' when ' + context);
}

function ASSERT_TRUE(value, context) {
  ASSERT_EQ(true, value, context);
}

function ASSERT_FALSE(value, context) {
  ASSERT_EQ(false, value, context);
}

function ASSERT_LT(a, b, context) {
  chrometest.assert(a < b, 'Expected ' + a + ' < ' + b + ' when ' + context);
}

function ASSERT_GT(a, b, context) {
  chrometest.assert(a > b, 'Expected ' + a + ' > ' + b + ' when ' + context);
}

function ASSERT_LE(a, b, context) {
  chrometest.assert(a <= b, 'Expected ' + a + ' <= ' + b + ' when ' + context);
}

function ASSERT_GE(a, b, context) {
  chrometest.assert(a >= b, 'Expected ' + a + ' >= ' + b + ' when ' + context);
}


// EXPECT VARIANTS
// ---------------

function EXPECT_EQ(expected, actual, context) {
  expected = JSON.stringify(expected);
  actual = JSON.stringify(actual);
  chrometest.expect(expected == actual, 'Expected ' + expected + ' but got ' +
                    JSON.stringify(actual) + ' when ' + context);
}

function EXPECT_NE(expected, actual, context) {
  expected = JSON.stringify(expected);
  actual = JSON.stringify(actual);
  chrometest.expect(expected != actual, 'Did not expect ' + expected +
                    ' but got ' + actual + ' when ' + context);
}

function EXPECT_TRUE(value, context) {
  EXPECT_EQ(true, value, context);
}

function EXPECT_FALSE(value, context) {
  EXPECT_EQ(false, value, context);
}

function EXPECT_LT(a, b, context) {
  chrometest.expect(a < b, 'Expected ' + a + ' < ' + b + ' when ' + context);
}

function EXPECT_GT(a, b, context) {
  chrometest.expect(a > b, 'Expected ' + a + ' > ' + b + ' when ' + context);
}

function EXPECT_LE(a, b, context) {
  chrometest.expect(a <= b, 'Expected ' + a + ' <= ' + b + ' when ' + context);
}

function EXPECT_GE(a, b, context) {
  chrometest.expect(a >= b, 'Expected ' + a + ' >= ' + b + ' when ' + context);
}
