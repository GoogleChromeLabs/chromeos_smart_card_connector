#!/usr/bin/env python3

# Copyright 2022 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Runs JavaScript tests via Selenium.

This script's main purpose is to automatically check the result of executing
tests written in JavaScript: the Jsunit test framework (from Closure Library)
we're using doesn't provide a native way to report the results back to the
command line, neither does it support running under Node.JS.

This script uses Selenium and Chromedriver in order to launch the page with the
JS tests under Chrome. The script inspects the Jsunit state on the page to wait
until the tests complete and to obtain the results.

Additionally, the script uses pyvirtualdisplay in order to hide the launched
browser and to allow the script work in UI-less scenarios (for Continuous
Integration).
"""

import argparse
import contextlib
import functools
import os.path
import pyvirtualdisplay
from selenium import webdriver
from selenium.webdriver.support import ui as webdriver_ui
import sys
import threading

@contextlib.contextmanager
def host_on_web_server_if_needed(serve_via_web_server, test_html_page_path):
  if not serve_via_web_server:
    # No web server is requested. Hence use the file:// URL.
    abs_path = os.path.abspath(test_html_page_path)
    url = "file://%s" % abs_path
    try:
      yield url
    finally:
      return
  # Start a web server that hosts the contents of the specified file's parent
  # directory. The URL will look like "localhost:<port>/<base_file_name>".
  import http.server
  port = 0  # means random
  address = ('localhost', port)
  Handler = functools.partial(http.server.SimpleHTTPRequestHandler,
                              directory=os.path.dirname(test_html_page_path))
  server = http.server.HTTPServer(address, Handler)
  server.timeout = 1  # in seconds
  sys.stderr.write('Started web server {}:{}.\n'.format(*server.server_address))
  url = "http://{}:{}/{}".format(*server.server_address,
                                 os.path.basename(test_html_page_path))
  shutdown_event = threading.Event()
  def server_thread():
    while not shutdown_event.is_set():
      server.handle_request()  # sleeps at most `server.timeout`
    server.server_close()
  server_thread = threading.Thread(target=server_thread)
  server_thread.start()
  try:
    yield url
  finally:
    # We use an event instead of doing serve_forever()+shutdown(), because
    # shutdown() can deadlock if called before serve_forever() gets started on
    # the server thread.
    shutdown_event.set()
    server_thread.join()

def create_virtual_display(show_ui):
  """Returns a virtual display context manager."""
  return pyvirtualdisplay.Display(visible=show_ui)

def create_driver(chromedriver_path):
  """Launches Chrome (Chromedriver) and returns the Selenium driver object."""
  service = webdriver.chrome.service.Service(executable_path=chromedriver_path)
  options = webdriver.chrome.options.Options()
  # Collect all JS logs, not only warnings/errors which is the default.
  options.set_capability('goog:loggingPrefs', {'browser': 'ALL'})
  return webdriver.Chrome(service=service, options=options)

def load_test_page(driver, url):
  """Navigates the Chromedriver to the given page."""
  driver.get(url)

def wait_for_test_completion(driver, timeout_seconds):
  """Waits until the Jsunit tests finish in the page opened in Chromedriver."""
  webdriver_ui.WebDriverWait(driver, timeout_seconds).until(
      lambda driver: is_js_test_finished(driver) or is_page_load_failed(driver))
  if is_page_load_failed(driver):
    raise RuntimeError(f'Failed to load the page: {get_page_text(driver)}')

def is_page_load_failed(driver):
  """Returns whether the page loading failed in Chromedriver (e.g., 404)."""
  # Note that `driver.current_url` wouldn't work here, as it returns the
  # attempted page's URL, and never Chrome internal URLs.
  page_url = driver.execute_script('return document.location.href')
  return page_url.startswith('chrome-error://')

def get_page_text(driver):
  """Returns the page's full text - useful for debugging purposes."""
  return driver.find_element(
      by=webdriver.common.by.By.TAG_NAME, value='body').text

def is_js_test_finished(driver):
  """Returns whether the Jsunit tests finished in the page in Chromedriver."""
  # Inspect the Jsunit state to check it initialized and finished.
  return driver.execute_script(
      'return window.G_testRunner && window.G_testRunner.isFinished()')

def is_js_test_successful(driver):
  """Returns whether the Jsunit tests passed in the page in Chromedriver."""
  # Inspect the Jsunit state.
  return driver.execute_script('return window.G_testRunner.isSuccess()')

def get_js_test_report(driver):
  """Returns the Jsunit human-readable summary from the page in Chromedriver."""
  # Inspect the Jsunit state.
  return driver.execute_script('return window.G_testRunner.getReport()')

def get_js_logs(driver):
  """Returns all JavaScript logs from the page in Chromedriver."""
  return '\n'.join(f'{log["level"]}: {log["message"]}'
                   for log in driver.get_log('browser'))

def parse_command_line_args():
  parser = argparse.ArgumentParser(
      description='Run JavaScript tests via Selenium and report the result to '
                  'the command line.')
  parser.add_argument('test_html_page_path', type=str,
                      help='path to the HTML page that bundles the tests')
  parser.add_argument('--chromedriver-path', type=str, required=True,
                      help='path to chromedriver to be used by Selenium')
  parser.add_argument('--serve-via-web-server', action='store_true',
                      help='host the whole directory via localhost webserver '
                           'and navigate to it instead of a file:// URL')
  parser.add_argument('--timeout', type=int, default=60,
                      help='timeout, in seconds, for the tests to run')
  parser.add_argument('--print-js-logs', action='store_true',
                      help='use to get all JavaScript logs dumped')
  parser.add_argument('--show-ui', action='store_true',
                      help="make the launched Chrome visible (and don't use a "
                           "hidden virtual display)")
  return parser.parse_args()

def main():
  args = parse_command_line_args()
  sys.stderr.write('Initializing environment...\n')
  with host_on_web_server_if_needed(args.serve_via_web_server,
                                    args.test_html_page_path) as url:
    with create_virtual_display(args.show_ui):
      with create_driver(args.chromedriver_path) as driver:
        sys.stderr.write('Running {}...\n'.format(url))
        load_test_page(driver, url)
        sys.stderr.write('Waiting for the test completion...\n')
        wait_for_test_completion(driver, args.timeout)
        print(get_js_test_report(driver))
        if args.print_js_logs:
          print(get_js_logs(driver))
        if not is_js_test_successful(driver):
          return 1
  return 0

if __name__ == '__main__':
  sys.exit(main())
