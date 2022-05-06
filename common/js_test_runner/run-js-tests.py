#!/usr/bin/env python3

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
import math
import os.path
import pathlib
import pyvirtualdisplay
import sys
from selenium import webdriver
from selenium.webdriver.support import ui as webdriver_ui

def create_virtual_display(show_ui):
  return pyvirtualdisplay.Display(visible=show_ui)

def create_driver(chromedriver_path):
  service = webdriver.chrome.service.Service(executable_path=chromedriver_path)
  options = webdriver.chrome.options.Options()
  capabilities = \
      webdriver.common.desired_capabilities.DesiredCapabilities.CHROME.copy()
  # Collect all JS logs, not only warnings/errors which is the default.
  capabilities['goog:loggingPrefs'] = {'browser': 'ALL'}
  return webdriver.Chrome(service=service, options=options,
                          desired_capabilities=capabilities)

def load_test_page(driver, test_html_page_path):
  # An absolute path should be used to construct a file:// URL.
  abs_path = os.path.abspath(test_html_page_path)
  driver.get("file://%s" % abs_path)

def wait_for_test_completion(driver, timeout_seconds):
  webdriver_ui.WebDriverWait(
      driver, timeout_seconds if timeout_seconds else math.inf).until(
          lambda driver: is_js_test_finished(driver) or
                         is_page_load_failed(driver))
  if is_page_load_failed(driver):
    raise RuntimeError(f'Failed to load the page: {get_page_text(driver)}')

def is_page_load_failed(driver):
  # Note that `driver.current_url` wouldn't work here, as it returns the
  # attempted page's URL, and never Chrome internal URLs.
  page_url = driver.execute_script('return document.location.href')
  return page_url.startswith('chrome-error://')

def get_page_text(driver):
  return driver.find_element(
      by=webdriver.common.by.By.TAG_NAME, value='body').text

def is_js_test_finished(driver):
  # Inspect the Jsunit (Closure Library's test framework that we use to write JS
  # tests) state to check whether it finished the test runner.
  return driver.execute_script(
      'return window.G_testRunner && window.G_testRunner.isFinished()')

def is_js_test_successful(driver):
  # Inspect the Jsunit state to check whether the JavaScript tests passed.
  return driver.execute_script('return window.G_testRunner.isSuccess()')

def get_js_test_report(driver):
  return driver.execute_script('return window.G_testRunner.getReport()')

def get_js_logs(driver):
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
  parser.add_argument('--timeout', type=int, default=60,
                      help='timeout, in seconds, for the tests to run; pass 0 '
                           'for infinite timeout')
  parser.add_argument('--print-js-logs', action='store_true',
                      help='use to get all JavaScript logs dumped')
  parser.add_argument('--show-ui', action='store_true',
                      help="make the launched Chrome visible (and don't use a "
                           "hidden virtual display)")
  return parser.parse_args()

def main():
  args = parse_command_line_args()
  sys.stderr.write('Initializing environment...\n')
  with create_virtual_display(args.show_ui):
    with create_driver(args.chromedriver_path) as driver:
      sys.stderr.write('Starting tests...\n')
      load_test_page(driver, args.test_html_page_path)
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
